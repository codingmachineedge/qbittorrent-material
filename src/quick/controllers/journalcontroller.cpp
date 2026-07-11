/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "journalcontroller.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "base/logging.h"
#include "base/torrentjournal/torrentjournal.h"
#include "base/torrentjournal/torrentundomanager.h"

using namespace Qt::Literals::StringLiterals;
using namespace TorrentJournalNS;

namespace
{
    TorrentJournal::Repo repoFromString(const QString &repo)
    {
        return (repo == u"settings"_s) ? TorrentJournal::Repo::Settings
                                       : TorrentJournal::Repo::Actions;
    }
}

JournalController *JournalController::m_instance = nullptr;

JournalController *JournalController::create(QQmlEngine *, QJSEngine *)
{
    qCInfo(lcUi) << "JournalController QML singleton requested";
    return instance();
}

JournalController *JournalController::instance()
{
    if (!m_instance)
        m_instance = new JournalController;
    return m_instance;
}

JournalController::JournalController()
{
    if (TorrentJournal *journal = TorrentJournal::instance())
    {
        connect(journal, &TorrentJournal::historyChanged, this, [this](TorrentJournal::Repo)
        {
            m_cachedActionsCount = -1;
            m_cachedSettingsCount = -1;
            emit historyChanged();
        });
        connect(journal, &TorrentJournal::statusChanged, this, &JournalController::statusChanged);
        connect(journal, &TorrentJournal::entryCommitted, this,
            [this](const TorrentJournal::Repo repo, const JournalEntry &entry)
        {
            if ((repo != TorrentJournal::Repo::Actions)
                || (entry.origin != JournalOrigin::User))
                return;
            emit actionJournaled(entry.commitId, entry.summary,
                TorrentUndoManager::isEntryUndoable(entry));
        });
    }
    if (TorrentUndoManager *undoManager = TorrentUndoManager::instance())
    {
        connect(undoManager, &TorrentUndoManager::busyChanged,
            this, &JournalController::busyChanged);
        connect(undoManager, &TorrentUndoManager::operationCompleted,
            this, &JournalController::operationFinished);
    }
}

bool JournalController::isAvailable() const
{
    const TorrentJournal *journal = TorrentJournal::instance();
    return journal && journal->isAvailable();
}

bool JournalController::isBusy() const
{
    const TorrentUndoManager *undoManager = TorrentUndoManager::instance();
    return undoManager && undoManager->isBusy();
}

bool JournalController::canUndo() const
{
    const TorrentUndoManager *undoManager = TorrentUndoManager::instance();
    return undoManager && undoManager->canUndo();
}

QString JournalController::lastActionDescription() const
{
    const TorrentUndoManager *undoManager = TorrentUndoManager::instance();
    return undoManager ? undoManager->lastUndoableEntry().summary : QString();
}

int JournalController::actionsCount() const
{
    if (m_cachedActionsCount < 0)
    {
        const TorrentJournal *journal = TorrentJournal::instance();
        m_cachedActionsCount = (journal && journal->isAvailable())
            ? static_cast<int>(journal->history(TorrentJournal::Repo::Actions, 0).size()) : 0;
    }
    return m_cachedActionsCount;
}

int JournalController::settingsCount() const
{
    if (m_cachedSettingsCount < 0)
    {
        const TorrentJournal *journal = TorrentJournal::instance();
        m_cachedSettingsCount = (journal && journal->isAvailable())
            ? static_cast<int>(journal->history(TorrentJournal::Repo::Settings, 0).size()) : 0;
    }
    return m_cachedSettingsCount;
}

int JournalController::journaledOnlyCount() const
{
    const TorrentJournal *journal = TorrentJournal::instance();
    return journal ? static_cast<int>(journal->journaledOnlyTorrentIds().size()) : 0;
}

bool JournalController::isAutoCommitEnabled() const
{
    const TorrentJournal *journal = TorrentJournal::instance();
    return journal && journal->isSettingsAutoCommitEnabled();
}

void JournalController::setAutoCommitEnabled(const bool enabled)
{
    if (TorrentJournal *journal = TorrentJournal::instance())
        journal->setSettingsAutoCommitEnabled(enabled);
}

QString JournalController::retention() const
{
    const TorrentJournal *journal = TorrentJournal::instance();
    return journal ? journal->retention() : u"Forever"_s;
}

void JournalController::setRetention(const QString &retention)
{
    if (TorrentJournal *journal = TorrentJournal::instance())
        journal->setRetention(retention);
}

void JournalController::undoLast()
{
    qCInfo(lcUi) << "Journal: undo last requested";
    if (TorrentUndoManager *undoManager = TorrentUndoManager::instance())
        (void)undoManager->undoLast();
}

void JournalController::undoEntry(const QString &commitId)
{
    qCInfo(lcUi) << "Journal: undo entry requested" << commitId.left(8);
    if (TorrentUndoManager *undoManager = TorrentUndoManager::instance())
        (void)undoManager->undoEntry(commitId);
}

void JournalController::restoreTo(const QString &commitId)
{
    qCInfo(lcUi) << "Journal: restore-to-point requested" << commitId.left(8);
    if (TorrentUndoManager *undoManager = TorrentUndoManager::instance())
        (void)undoManager->restoreToCommit(commitId);
}

void JournalController::undoSettingsEntry(const QString &commitId)
{
    qCInfo(lcUi) << "Journal: settings revert requested" << commitId.left(8);
    if (TorrentUndoManager *undoManager = TorrentUndoManager::instance())
        (void)undoManager->undoSettingsEntry(commitId);
}

void JournalController::restoreMissingTorrents()
{
    qCInfo(lcUi) << "Journal: restore missing torrents requested";
    if (TorrentUndoManager *undoManager = TorrentUndoManager::instance())
        (void)undoManager->restoreMissingTorrents();
}

bool JournalController::openRepository(const QString &repo) const
{
    const TorrentJournal *journal = TorrentJournal::instance();
    if (!journal)
        return false;
    return QDesktopServices::openUrl(
        QUrl::fromLocalFile(journal->repositoryPath(repoFromString(repo))));
}

void JournalController::copyToClipboard(const QString &text) const
{
    if (QClipboard *clipboard = QGuiApplication::clipboard())
        clipboard->setText(text);
}

int JournalController::exportHistoryJson(const QString &repo) const
{
    const TorrentJournal *journal = TorrentJournal::instance();
    if (!journal || !journal->isAvailable())
        return 0;

    const QList<JournalEntry> entries = journal->history(repoFromString(repo), 0);
    QJsonArray array;
    for (const JournalEntry &entry : entries)
    {
        QJsonArray ops;
        for (const JournalOpRecord &op : entry.ops)
        {
            ops.append(QJsonObject {
                {u"op"_s, journalOpKindToString(op.kind)},
                {u"id"_s, op.torrentId},
                {u"name"_s, op.torrentName},
                {u"old"_s, op.oldValue},
                {u"new"_s, op.newValue}
            });
        }
        array.append(QJsonObject {
            {u"sha"_s, entry.shortId},
            {u"commitId"_s, entry.commitId},
            {u"time"_s, entry.timestamp.toString(Qt::ISODate)},
            {u"message"_s, entry.summary},
            {u"origin"_s, journalOriginToString(entry.origin)},
            {u"ops"_s, ops}
        });
    }
    copyToClipboard(QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Indented)));
    return static_cast<int>(entries.size());
}
