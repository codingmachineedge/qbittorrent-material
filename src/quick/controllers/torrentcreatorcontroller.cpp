/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026  qBittorrent-Material contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "torrentcreatorcontroller.h"

#include <QMetaObject>
#include <QRegularExpression>
#include <QStringList>
#include <QThread>
#include <QThreadPool>

#include "base/bittorrent/addtorrentparams.h"
#include "base/bittorrent/session.h"
#include "base/bittorrent/sharelimits.h"
#include "base/bittorrent/torrent.h"
#include "base/bittorrent/torrentcontentlayout.h"
#include "base/bittorrent/torrentcreator.h"
#include "base/bittorrent/torrentdescriptor.h"
#include "base/logging.h"
#include "base/utils/fs/path.h"
#include "base/utils/misc.h"

namespace
{
#ifdef QBT_USES_LIBTORRENT2
    /// Map the combo index (0=V2, 1=Hybrid, 2=V1) to the engine enum.
    BitTorrent::TorrentFormat toTorrentFormat(const int comboIndex)
    {
        switch (comboIndex)
        {
        case 0:
            return BitTorrent::TorrentFormat::V2;
        case 2:
            return BitTorrent::TorrentFormat::V1;
        case 1:
        default:
            return BitTorrent::TorrentFormat::Hybrid;
        }
    }
#endif
}

TorrentCreatorController *TorrentCreatorController::create(QQmlEngine *, QJSEngine *)
{
    qCDebug(lcUi) << "Creating TorrentCreatorController";
    return new TorrentCreatorController;
}

TorrentCreatorController::TorrentCreatorController(QObject *parent)
    : QObject(parent)
    , m_threadPool {new QThreadPool(this)}
    , m_pieceSizes {buildPieceSizes()}
{
    // The torrent creator is CPU-bound; run one job at a time.
    m_threadPool->setMaxThreadCount(1);
    qCDebug(lcUi) << "TorrentCreatorController ready; libtorrent2 =" << isLibtorrent2();
}

TorrentCreatorController::~TorrentCreatorController()
{
    if (m_activeCreator)
        m_activeCreator->requestInterruption();
    m_threadPool->clear();
    m_threadPool->waitForDone();
    qCDebug(lcUi) << "TorrentCreatorController destroyed";
}

bool TorrentCreatorController::isLibtorrent2() const
{
#ifdef QBT_USES_LIBTORRENT2
    return true;
#else
    return false;
#endif
}

QVariantList TorrentCreatorController::buildPieceSizes()
{
    QVariantList sizes;
    sizes.append(QVariantMap {{QStringLiteral("text"), tr("Auto")}, {QStringLiteral("value"), 0}});
    // 16 KiB (1024 << 4) up to 128 MiB (1024 << 17), matching the legacy dialog.
    for (int i = 4; i <= 17; ++i)
    {
        const int size = 1024 << i;
        sizes.append(QVariantMap {
            {QStringLiteral("text"), Utils::Misc::friendlyUnit(size, false, 0)},
            {QStringLiteral("value"), size}
        });
    }
    return sizes;
}

void TorrentCreatorController::calculateTotalPieces(const QString &sourcePath, const int pieceSize
        , const bool ignoreDotfiles, const int torrentFormat
        , const bool isAlignmentOptimized, const int paddedFileSizeLimit)
{
    if (sourcePath.isEmpty())
    {
        qCWarning(lcUi) << "calculateTotalPieces called with empty source path";
        return;
    }

    qCDebug(lcUi) << "Calculating total pieces for" << sourcePath << "pieceSize=" << pieceSize;
    setTotalPiecesText(tr("Calculating…"));

    const Path input {sourcePath};
    QThread *thread = QThread::create(
        [this, input, pieceSize, ignoreDotfiles, torrentFormat, isAlignmentOptimized, paddedFileSizeLimit]()
        {
            int count = 0;
#ifdef QBT_USES_LIBTORRENT2
            Q_UNUSED(isAlignmentOptimized)
            Q_UNUSED(paddedFileSizeLimit)
            count = BitTorrent::TorrentCreator::calculateTotalPieces(
                    input, pieceSize, ignoreDotfiles, toTorrentFormat(torrentFormat));
#else
            Q_UNUSED(torrentFormat)
            count = BitTorrent::TorrentCreator::calculateTotalPieces(
                    input, pieceSize, ignoreDotfiles, isAlignmentOptimized, paddedFileSizeLimit);
#endif
            QMetaObject::invokeMethod(this, [this, count]()
            {
                qCDebug(lcUi) << "Total pieces calculated:" << count;
                setTotalPiecesText(QString::number(count));
            }, Qt::QueuedConnection);
        });
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    thread->start();
}

void TorrentCreatorController::createTorrent(const QVariantMap &params)
{
    if (m_creating)
    {
        qCWarning(lcUi) << "createTorrent ignored: a job is already running";
        return;
    }

    const QString sourcePath = params.value(QStringLiteral("sourcePath")).toString();
    const QString destPath = params.value(QStringLiteral("torrentFilePath")).toString();
    if (sourcePath.isEmpty() || destPath.isEmpty())
    {
        qCWarning(lcUi) << "createTorrent aborted: missing source or destination path";
        emit creationFailed(tr("Reason: Path to file/folder is not readable."));
        return;
    }

    m_startSeeding = params.value(QStringLiteral("startSeeding"), false).toBool();
    m_ignoreShareLimits = params.value(QStringLiteral("ignoreShareLimits"), false).toBool();

    // Trackers: collapse runs of 3+ newlines to a single blank-line tier break.
    QStringList trackers;
    const QString trackersText = params.value(QStringLiteral("trackers")).toString().trimmed();
    if (!trackersText.isEmpty())
    {
        trackers = QString(trackersText)
                .replace(QRegularExpression(QStringLiteral("\n\n[\n]+")), QStringLiteral("\n\n"))
                .split(u'\n');
    }
    const QStringList urlSeeds = params.value(QStringLiteral("urlSeeds")).toString()
            .split(u'\n', Qt::SkipEmptyParts);

    BitTorrent::TorrentCreatorParams creatorParams;
    creatorParams.ignoreDotfiles = params.value(QStringLiteral("ignoreDotfiles"), true).toBool();
    creatorParams.isPrivate = params.value(QStringLiteral("isPrivate"), false).toBool();
#ifdef QBT_USES_LIBTORRENT2
    creatorParams.torrentFormat = toTorrentFormat(params.value(QStringLiteral("torrentFormat"), 1).toInt());
#else
    creatorParams.isAlignmentOptimized = params.value(QStringLiteral("isAlignmentOptimized"), true).toBool();
    creatorParams.paddedFileSizeLimit = params.value(QStringLiteral("paddedFileSizeLimit"), -1).toInt();
#endif
    creatorParams.pieceSize = params.value(QStringLiteral("pieceSize"), 0).toInt();
    creatorParams.sourcePath = Path {sourcePath};
    creatorParams.torrentFilePath = Path {destPath};
    creatorParams.comment = params.value(QStringLiteral("comment")).toString();
    creatorParams.source = params.value(QStringLiteral("source")).toString();
    creatorParams.trackers = trackers;
    creatorParams.urlSeeds = urlSeeds;

    qCInfo(lcUi) << "Creating torrent from" << sourcePath << "->" << destPath
                 << "pieceSize=" << creatorParams.pieceSize
                 << "private=" << creatorParams.isPrivate;

    auto *creator = new BitTorrent::TorrentCreator(creatorParams);
    m_activeCreator = creator;

    connect(creator, &BitTorrent::TorrentCreator::progressUpdated
            , this, &TorrentCreatorController::setProgress, Qt::QueuedConnection);
    connect(creator, &BitTorrent::TorrentCreator::creationSuccess
            , this, &TorrentCreatorController::onCreationSuccess, Qt::QueuedConnection);
    connect(creator, &BitTorrent::TorrentCreator::creationFailure
            , this, &TorrentCreatorController::onCreationFailure, Qt::QueuedConnection);

    setProgress(0);
    setCreating(true);
    m_threadPool->start(creator);
}

void TorrentCreatorController::cancelCreation()
{
    if (m_activeCreator)
    {
        qCInfo(lcUi) << "Requesting interruption of torrent creation";
        m_activeCreator->requestInterruption();
    }
}

void TorrentCreatorController::clearTotalPieces()
{
    setTotalPiecesText(QString());
}

void TorrentCreatorController::onCreationSuccess(const BitTorrent::TorrentCreatorResult &result)
{
    qCInfo(lcUi) << "Torrent created:" << result.torrentFilePath.toString();
    m_activeCreator = nullptr;
    setCreating(false);
    setProgress(100);

    if (m_startSeeding)
    {
        const auto loadResult = BitTorrent::TorrentDescriptor::loadFromFile(result.torrentFilePath);
        if (loadResult)
        {
            BitTorrent::AddTorrentParams addParams;
            // Set every value explicitly so a changed global default cannot make
            // the "start seeding" add fail unexpectedly (mirrors the legacy dialog).
            addParams.addStopped = false;
            addParams.contentLayout = BitTorrent::TorrentContentLayout::Original;
            addParams.savePath = result.savePath;
            addParams.skipChecking = true;
            addParams.stopCondition = BitTorrent::Torrent::StopCondition::None;
            addParams.useAutoTMM = false;
            addParams.useDownloadPath = false;

            if (m_ignoreShareLimits)
            {
                addParams.shareLimits.ratioLimit = BitTorrent::NO_RATIO_LIMIT;
                addParams.shareLimits.seedingTimeLimit = BitTorrent::NO_SEEDING_TIME_LIMIT;
                addParams.shareLimits.inactiveSeedingTimeLimit = BitTorrent::NO_SEEDING_TIME_LIMIT;
            }

            qCInfo(lcUi) << "Starting to seed the newly created torrent";
            BitTorrent::Session::instance()->addTorrent(loadResult.value(), addParams);
        }
        else
        {
            qCWarning(lcUi) << "Failed to load created torrent for seeding:" << loadResult.error();
            emit addTorrentFailed(loadResult.error());
        }
    }

    emit creationSucceeded(result.torrentFilePath.toString(), result.savePath.toString());
}

void TorrentCreatorController::onCreationFailure(const QString &message)
{
    qCWarning(lcUi) << "Torrent creation failed:" << message;
    m_activeCreator = nullptr;
    setCreating(false);
    emit creationFailed(message);
}

void TorrentCreatorController::setCreating(const bool creating)
{
    if (m_creating == creating)
        return;
    m_creating = creating;
    emit creatingChanged();
}

void TorrentCreatorController::setProgress(const int progress)
{
    if (m_progress == progress)
        return;
    m_progress = progress;
    emit progressChanged();
}

void TorrentCreatorController::setTotalPiecesText(const QString &text)
{
    if (m_totalPiecesText == text)
        return;
    m_totalPiecesText = text;
    emit totalPiecesChanged();
}
