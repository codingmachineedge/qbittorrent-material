/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QDateTime>
#include <QList>
#include <QString>

namespace TorrentJournalNS
{
    enum class JournalOpKind
    {
        Add,
        Delete,
        Name,
        Category,
        TagAdd,
        TagRemove,
        SavePath,
        SavingMode,
        Start,
        Stop,
        Metadata,
        Config,
        TrackersAdd,
        TrackersRemove,
        TrackersReset,
        FileRename,
        FolderRename,
        SessionCategory,
        SessionTag,
        Snapshot,
        Restore,
        Unknown
    };

    [[nodiscard]] QString journalOpKindToString(JournalOpKind kind);
    [[nodiscard]] JournalOpKind journalOpKindFromString(const QString &value);

    enum class JournalOrigin
    {
        User,     // a normal (user- or automation-driven) mutation
        Undo,     // applied by the undo manager to invert an earlier entry
        Snapshot, // startup/shutdown state capture
        Restore   // applied by restore-to-point reconciliation
    };

    [[nodiscard]] QString journalOriginToString(JournalOrigin origin);
    [[nodiscard]] JournalOrigin journalOriginFromString(const QString &value);

    struct JournalOpRecord
    {
        JournalOpKind kind = JournalOpKind::Unknown;
        QString torrentId;   // TorrentID hex (empty for session-level ops)
        QString torrentName; // display name at the time of the op
        QString oldValue;
        QString newValue;
    };

    struct JournalEntry
    {
        QString commitId;
        QString shortId;
        QDateTime timestamp;
        QString summary; // first line of the commit message
        JournalOrigin origin = JournalOrigin::User;
        QString undoesCommitId;        // set when origin == Undo
        QString restoreTargetCommitId; // set when origin == Restore
        QList<JournalOpRecord> ops;
        int truncatedOpCount = 0; // ops dropped from the trailer beyond the cap
    };

    // Builds the full commit message: summary line + machine trailers.
    [[nodiscard]] QString encodeCommitMessage(const QString &summary,
        const QList<JournalOpRecord> &ops, JournalOrigin origin,
        const QString &undoesCommitId = {}, const QString &restoreTargetCommitId = {});

    // Parses the trailers back out of a full commit message. commitId, shortId
    // and timestamp are left for the caller to fill from the commit itself.
    [[nodiscard]] JournalEntry decodeCommitMessage(const QString &message);
}
