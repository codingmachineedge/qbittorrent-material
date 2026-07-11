/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

#include "torrentjournalop.h"

namespace BitTorrent
{
    class Torrent;
}

/**
 * Computes and applies inverse operations for TorrentJournal entries.
 *
 * Old values are always read from the git trees (an entry's parent commit),
 * never from commit-message trailers. Every undo/restore is applied through
 * the normal engine mutators inside an annotated journal scope, so it lands
 * as a NEW commit — history is append-only and an undo can itself be undone.
 */
class TorrentUndoManager final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TorrentUndoManager)

public:
    enum class ConflictPolicy
    {
        Skip, // leave ops whose current value drifted from the entry's post-state
        Force // apply the inverse regardless
    };

    struct UndoResult
    {
        bool success = false;
        QString message;
        int applied = 0;
        int skipped = 0;
        int conflicted = 0;
    };

    static void initInstance();
    static void freeInstance();
    static TorrentUndoManager *instance();

    [[nodiscard]] bool isBusy() const;
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] TorrentJournalNS::JournalEntry lastUndoableEntry() const;

    UndoResult undoLast();
    UndoResult undoEntry(const QString &commitId, ConflictPolicy policy = ConflictPolicy::Skip);
    UndoResult restoreToCommit(const QString &commitId);
    // Re-adds every torrent present in the journal working tree but missing
    // from the session (the restart persistence gap).
    UndoResult restoreMissingTorrents();
    // Settings repo: writes the entry's keys back to their parent-commit values.
    UndoResult undoSettingsEntry(const QString &commitId);

    [[nodiscard]] static bool isEntryUndoable(const TorrentJournalNS::JournalEntry &entry);

signals:
    void busyChanged();
    void operationCompleted(bool success, const QString &message);

private:
    TorrentUndoManager() = default;
    ~TorrentUndoManager() override = default;

    class BusyGuard;

    UndoResult undoOps(const TorrentJournalNS::JournalEntry &entry, ConflictPolicy policy);
    bool undoSingleOp(const TorrentJournalNS::JournalOpRecord &op, const QString &parentCommitId,
        const QString &undoneCommitId, ConflictPolicy policy, UndoResult *result);

    // Re-adds a torrent from its serialized doc + optional metadata blob.
    bool readdTorrent(const QString &torrentId, const QJsonObject &doc, const QByteArray &blob,
        const QString &annotationCommitId, QString *error);
    // Applies every configuration field of `doc` that differs from the live torrent.
    int applyDocConfig(BitTorrent::Torrent *torrent, const QJsonObject &doc);
    void reconcileSessionDoc(const QJsonObject &targetSessionDoc);

    void setBusy(bool busy);
    void finish(const UndoResult &result);

    static TorrentUndoManager *m_instance;
    bool m_busy = false;
};
