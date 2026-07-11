/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "torrentjournalop.h"

#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>

using namespace Qt::Literals::StringLiterals;

namespace
{
    // The Ops trailer is for display and undo routing; authoritative state
    // always comes from the git trees, so a hard cap keeps messages bounded.
    constexpr int MaximumTrailerOps = 100;

    const QString OpsPrefix = u"Ops: "_s;
    const QString OriginPrefix = u"Origin: "_s;
    const QString UndoesPrefix = u"Undoes: "_s;
    const QString RestoreTargetPrefix = u"RestoreTarget: "_s;
    const QString TruncatedPrefix = u"TruncatedOps: "_s;

    const QHash<TorrentJournalNS::JournalOpKind, QString> &opKindNames()
    {
        static const QHash<TorrentJournalNS::JournalOpKind, QString> names {
            {TorrentJournalNS::JournalOpKind::Add, u"add"_s},
            {TorrentJournalNS::JournalOpKind::Delete, u"delete"_s},
            {TorrentJournalNS::JournalOpKind::Name, u"name"_s},
            {TorrentJournalNS::JournalOpKind::Category, u"category"_s},
            {TorrentJournalNS::JournalOpKind::TagAdd, u"tag-add"_s},
            {TorrentJournalNS::JournalOpKind::TagRemove, u"tag-remove"_s},
            {TorrentJournalNS::JournalOpKind::SavePath, u"save-path"_s},
            {TorrentJournalNS::JournalOpKind::SavingMode, u"saving-mode"_s},
            {TorrentJournalNS::JournalOpKind::Start, u"start"_s},
            {TorrentJournalNS::JournalOpKind::Stop, u"stop"_s},
            {TorrentJournalNS::JournalOpKind::Metadata, u"metadata"_s},
            {TorrentJournalNS::JournalOpKind::Config, u"config"_s},
            {TorrentJournalNS::JournalOpKind::TrackersAdd, u"trackers-add"_s},
            {TorrentJournalNS::JournalOpKind::TrackersRemove, u"trackers-remove"_s},
            {TorrentJournalNS::JournalOpKind::TrackersReset, u"trackers-reset"_s},
            {TorrentJournalNS::JournalOpKind::FileRename, u"file-rename"_s},
            {TorrentJournalNS::JournalOpKind::FolderRename, u"folder-rename"_s},
            {TorrentJournalNS::JournalOpKind::SessionCategory, u"session-category"_s},
            {TorrentJournalNS::JournalOpKind::SessionTag, u"session-tag"_s},
            {TorrentJournalNS::JournalOpKind::Snapshot, u"snapshot"_s},
            {TorrentJournalNS::JournalOpKind::Restore, u"restore"_s}
        };
        return names;
    }
}

namespace TorrentJournalNS
{
    QString journalOpKindToString(const JournalOpKind kind)
    {
        return opKindNames().value(kind, u"unknown"_s);
    }

    JournalOpKind journalOpKindFromString(const QString &value)
    {
        const auto &names = opKindNames();
        for (auto it = names.cbegin(); it != names.cend(); ++it)
        {
            if (it.value() == value)
                return it.key();
        }
        return JournalOpKind::Unknown;
    }

    QString journalOriginToString(const JournalOrigin origin)
    {
        switch (origin)
        {
        case JournalOrigin::Undo: return u"undo"_s;
        case JournalOrigin::Snapshot: return u"snapshot"_s;
        case JournalOrigin::Restore: return u"restore"_s;
        case JournalOrigin::User:
        default: return u"user"_s;
        }
    }

    JournalOrigin journalOriginFromString(const QString &value)
    {
        if (value == u"undo"_s) return JournalOrigin::Undo;
        if (value == u"snapshot"_s) return JournalOrigin::Snapshot;
        if (value == u"restore"_s) return JournalOrigin::Restore;
        return JournalOrigin::User;
    }

    QString encodeCommitMessage(const QString &summary, const QList<JournalOpRecord> &ops,
        const JournalOrigin origin, const QString &undoesCommitId,
        const QString &restoreTargetCommitId)
    {
        QJsonArray opsArray;
        const int encodedCount = qMin<qsizetype>(ops.size(), MaximumTrailerOps);
        for (int i = 0; i < encodedCount; ++i)
        {
            const JournalOpRecord &op = ops.at(i);
            QJsonObject obj {
                {u"op"_s, journalOpKindToString(op.kind)}
            };
            if (!op.torrentId.isEmpty())
                obj[u"id"_s] = op.torrentId;
            if (!op.torrentName.isEmpty())
                obj[u"name"_s] = op.torrentName;
            if (!op.oldValue.isEmpty())
                obj[u"old"_s] = op.oldValue;
            if (!op.newValue.isEmpty())
                obj[u"new"_s] = op.newValue;
            opsArray.append(obj);
        }

        QString message = summary;
        message += u"\n\n"_s;
        message += OpsPrefix
            + QString::fromUtf8(QJsonDocument(opsArray).toJson(QJsonDocument::Compact)) + u'\n';
        message += OriginPrefix + journalOriginToString(origin) + u'\n';
        if (!undoesCommitId.isEmpty())
            message += UndoesPrefix + undoesCommitId + u'\n';
        if (!restoreTargetCommitId.isEmpty())
            message += RestoreTargetPrefix + restoreTargetCommitId + u'\n';
        if (ops.size() > MaximumTrailerOps)
            message += TruncatedPrefix + QString::number(ops.size() - MaximumTrailerOps) + u'\n';
        return message;
    }

    JournalEntry decodeCommitMessage(const QString &message)
    {
        JournalEntry entry;
        const QStringList lines = message.split(u'\n');
        entry.summary = lines.isEmpty() ? QString() : lines.first();

        for (const QString &line : lines)
        {
            if (line.startsWith(OpsPrefix))
            {
                const QJsonDocument doc = QJsonDocument::fromJson(
                    line.mid(OpsPrefix.size()).toUtf8());
                const QJsonArray array = doc.array();
                for (const QJsonValue &value : array)
                {
                    const QJsonObject obj = value.toObject();
                    JournalOpRecord op;
                    op.kind = journalOpKindFromString(obj.value(u"op"_s).toString());
                    op.torrentId = obj.value(u"id"_s).toString();
                    op.torrentName = obj.value(u"name"_s).toString();
                    op.oldValue = obj.value(u"old"_s).toString();
                    op.newValue = obj.value(u"new"_s).toString();
                    entry.ops.append(op);
                }
            }
            else if (line.startsWith(OriginPrefix))
            {
                entry.origin = journalOriginFromString(line.mid(OriginPrefix.size()).trimmed());
            }
            else if (line.startsWith(UndoesPrefix))
            {
                entry.undoesCommitId = line.mid(UndoesPrefix.size()).trimmed();
            }
            else if (line.startsWith(RestoreTargetPrefix))
            {
                entry.restoreTargetCommitId = line.mid(RestoreTargetPrefix.size()).trimmed();
            }
            else if (line.startsWith(TruncatedPrefix))
            {
                entry.truncatedOpCount = line.mid(TruncatedPrefix.size()).trimmed().toInt();
            }
        }
        return entry;
    }
}
