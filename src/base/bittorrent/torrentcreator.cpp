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

#include "torrentcreator.h"

#include <functional>

#include <libtorrent/bencode.hpp>
#include <libtorrent/create_torrent.hpp>
#include <libtorrent/entry.hpp>
#include <libtorrent/file_storage.hpp>
#include <libtorrent/torrent_info.hpp>

#include <QDateTime>
#include <QFile>
#include <QHash>
#include <QString>

#include "base/global.h"
#include "base/logging.h"
#include "base/utils/fs.h"
#include "base/utils/io.h"
#include "base/utils/misc.h"
#include "base/version.h"

namespace
{
    // do not include files and folders whose names start with a dot
    bool fileFilter(const std::string &f)
    {
        return !Path(f).filename().startsWith(u'.');
    }

#ifdef QBT_USES_LIBTORRENT2
    lt::create_flags_t toNativeTorrentFormatFlag(const BitTorrent::TorrentFormat torrentFormat)
    {
        switch (torrentFormat)
        {
        case BitTorrent::TorrentFormat::V1:
            return lt::create_torrent::v1_only;
        case BitTorrent::TorrentFormat::V2:
            return lt::create_torrent::v2_only;
        case BitTorrent::TorrentFormat::Hybrid:
        default:
            return {};
        }
    }
#endif
}

using namespace BitTorrent;

TorrentCreator::TorrentCreator(const TorrentCreatorParams &params, QObject *parent)
    : QObject(parent)
    , m_params {params}
{
    setAutoDelete(false);
}

const TorrentCreatorParams &TorrentCreator::params() const
{
    return m_params;
}

void TorrentCreator::sendProgressSignal(const int currentPieceIdx, const int totalPieces)
{
    emit progressUpdated((currentPieceIdx * 100) / totalPieces);
}

void TorrentCreator::checkInterruptionRequested() const
{
    if (isInterruptionRequested())
        throw RuntimeError(tr("Operation aborted"));
}

void TorrentCreator::requestInterruption()
{
    qCDebug(lcEngine) << "Torrent creation interruption requested";
    m_interruptionRequested = true;
}

bool TorrentCreator::isInterruptionRequested() const
{
    return m_interruptionRequested.load(std::memory_order_relaxed);
}

void TorrentCreator::run()
{
    emit started();
    qCInfo(lcEngine) << "Torrent creation started for" << m_params.sourcePath.data();

    try
    {
        const Path parentPath = m_params.sourcePath.parentPath();

        // Adding files to the torrent
        lt::file_storage fs;
        if (isInterruptionRequested())
            return;

        lt::create_flags_t flags = {};
#ifdef QBT_USES_LIBTORRENT2
        flags |= toNativeTorrentFormatFlag(m_params.torrentFormat);
#else
        if (m_params.isAlignmentOptimized)
            flags |= lt::create_torrent::optimize_alignment;
#endif

        lt::add_files(fs, m_params.sourcePath.toString().toStdString()
                , (m_params.ignoreDotfiles ? std::function(fileFilter) : std::function<bool (const std::string &)>()), flags);
        if (isInterruptionRequested())
            return;

#ifdef QBT_USES_LIBTORRENT2
        lt::create_torrent newTorrent {fs, m_params.pieceSize, flags};
#else
        lt::create_torrent newTorrent {fs, m_params.pieceSize
                , (m_params.isAlignmentOptimized ? m_params.paddedFileSizeLimit : -1), flags};
#endif

        // Add url seeds
        for (const QString &seed : asConst(m_params.urlSeeds))
            newTorrent.add_url_seed(seed.trimmed().toStdString());

        int tier = 0;
        bool newline = false;
        for (const QString &tracker : asConst(m_params.trackers))
        {
            if (tracker.isEmpty())
            {
                if (newline)
                    ++tier;
                newline = false;
                continue;
            }
            newTorrent.add_tracker(tracker.trimmed().toStdString(), tier);
            newline = true;
        }

        if (isInterruptionRequested())
            return;

        // calculate the hash for all pieces
        lt::set_piece_hashes(newTorrent, parentPath.toString().toStdString()
                , [this, &newTorrent](const lt::piece_index_t n)
        {
            checkInterruptionRequested();
            sendProgressSignal(static_cast<LTUnderlyingType<lt::piece_index_t>>(n), newTorrent.num_pieces());
        });

        // Set qBittorrent as creator and add user comment to
        // torrent_info structure
        newTorrent.set_creator(QStringLiteral("qBittorrent " QBT_VERSION).toUtf8().constData());
        newTorrent.set_comment(m_params.comment.toUtf8().constData());
        // Is private ?
        newTorrent.set_priv(m_params.isPrivate);

        if (!m_params.source.isEmpty())
            newTorrent.set_root_cert(m_params.source.toStdString());

        if (isInterruptionRequested())
            return;

        lt::entry entry = newTorrent.generate();

        // add source field
        if (!m_params.source.isEmpty())
            entry[u"info"_s.toStdString()][u"source"_s.toStdString()] = m_params.source.toStdString();

        if (isInterruptionRequested())
            return;

        // create the torrent and save it to a file
        QByteArray torrentContent;
        lt::bencode(std::back_inserter(torrentContent), entry);

        const nonstd::expected<void, QString> result = Utils::IO::saveToFile(m_params.torrentFilePath, torrentContent);
        if (!result)
        {
            throw RuntimeError(tr("Create new torrent file failed. Reason: %1.")
                    .arg(result.error()));
        }

        if (isInterruptionRequested())
            return;

        qCInfo(lcEngine) << "Torrent created successfully:" << m_params.torrentFilePath.data();
        emit creationSuccess({m_params.torrentFilePath, parentPath, newTorrent.piece_length()});
    }
    catch (const RuntimeError &err)
    {
        qCWarning(lcEngine) << "Torrent creation failed:" << err.message();
        emit creationFailure(err.message());
    }
    catch (const std::exception &err)
    {
        qCWarning(lcEngine) << "Torrent creation failed:" << err.what();
        emit creationFailure(QString::fromLocal8Bit(err.what()));
    }
}

#ifdef QBT_USES_LIBTORRENT2
int TorrentCreator::calculateTotalPieces(const Path &inputPath, const int pieceSize, const bool ignoreDotfiles, const TorrentFormat torrentFormat)
#else
int TorrentCreator::calculateTotalPieces(const Path &inputPath, const int pieceSize, const bool ignoreDotfiles, const bool isAlignmentOptimized, const int paddedFileSizeLimit)
#endif
{
    if (inputPath.isEmpty())
        return 0;

    lt::create_flags_t flags = {};
#ifdef QBT_USES_LIBTORRENT2
    flags |= toNativeTorrentFormatFlag(torrentFormat);
#else
    if (isAlignmentOptimized)
        flags |= lt::create_torrent::optimize_alignment;
#endif

    lt::file_storage fs;
    lt::add_files(fs, inputPath.toString().toStdString()
            , (ignoreDotfiles ? std::function(fileFilter) : std::function<bool (const std::string &)>()), flags);

#ifdef QBT_USES_LIBTORRENT2
    return lt::create_torrent {fs, pieceSize, flags}.num_pieces();
#else
    return lt::create_torrent {fs, pieceSize
            , (isAlignmentOptimized ? paddedFileSizeLimit : -1), flags}.num_pieces();
#endif
}
