/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "journalhistorymodel.h"

#include <QDateTime>
#include <QRegularExpression>
#include <QVariantList>
#include <QVariantMap>

#include "base/logging.h"
#include "base/torrentjournal/torrentjournal.h"
#include "base/torrentjournal/torrentundomanager.h"

using namespace Qt::Literals::StringLiterals;
using namespace TorrentJournalNS;

namespace
{
    QString relativeTimeText(const QDateTime &timestamp)
    {
        const qint64 secs = timestamp.secsTo(QDateTime::currentDateTimeUtc());
        if (secs < 60)
            return QObject::tr("just now");
        if (secs < 3600)
            return QObject::tr("%1m ago").arg(secs / 60);
        if (secs < (24 * 3600))
            return QObject::tr("%1h ago").arg(secs / 3600);
        return QObject::tr("%1d ago").arg(secs / (24 * 3600));
    }

    QString describeOp(const JournalOpRecord &op)
    {
        QString from = op.oldValue;
        QString to = op.newValue;
        const QString subject = op.torrentName.isEmpty() ? op.torrentId : op.torrentName;
        const QString kindText = journalOpKindToString(op.kind);
        if (from.isEmpty())
            from = u"(none)"_s;
        if (to.isEmpty())
            to = u"(none)"_s;
        return kindText + u' ' + subject + u": "_s + from + u" → "_s + to;
    }
}

JournalHistoryModel::JournalHistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
    if (TorrentJournal *journal = TorrentJournal::instance())
    {
        connect(journal, &TorrentJournal::historyChanged, this,
            [this](const TorrentJournal::Repo repo)
        {
            const bool matches = ((repo == TorrentJournal::Repo::Actions) && (m_repo == u"actions"_s))
                || ((repo == TorrentJournal::Repo::Settings) && (m_repo == u"settings"_s));
            if (matches)
                reload();
        });
    }
    reload();
}

int JournalHistoryModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_entries.size());
}

QVariant JournalHistoryModel::data(const QModelIndex &index, const int role) const
{
    if (!index.isValid() || (index.row() < 0) || (index.row() >= m_entries.size()))
        return {};
    const JournalEntry &entry = m_entries.at(index.row());

    switch (role)
    {
    case CommitIdRole: return entry.commitId;
    case ShaRole: return entry.shortId.left(7);
    case MessageRole: return entry.summary;
    case TimeTextRole: return relativeTimeText(entry.timestamp);
    case DateKeyRole:
    {
        const qint64 secs = entry.timestamp.secsTo(QDateTime::currentDateTimeUtc());
        return (secs < (24 * 3600)) ? tr("Today") : tr("Earlier");
    }
    case DiffLinesRole:
    {
        QVariantList lines;
        for (const JournalOpRecord &op : entry.ops)
        {
            QVariantMap line;
            const QString subject = op.torrentName.isEmpty() ? op.torrentId : op.torrentName;
            line[u"from"_s] = QString(journalOpKindToString(op.kind) + u' ' + subject + u": "_s
                + (op.oldValue.isEmpty() ? u"(none)"_s : op.oldValue));
            line[u"to"_s] = QString(journalOpKindToString(op.kind) + u' ' + subject + u": "_s
                + (op.newValue.isEmpty() ? u"(none)"_s : op.newValue));
            lines.append(line);
            if (lines.size() >= 12)
                break;
        }
        return lines;
    }
    case UndoableRole:
        return (m_repo == u"actions"_s)
            ? TorrentUndoManager::isEntryUndoable(entry)
            : (!entry.ops.isEmpty() && (entry.origin == JournalOrigin::User));
    case CanRestoreRole:
        return (m_repo == u"actions"_s)
            ? (entry.origin != JournalOrigin::Snapshot)
            : !entry.ops.isEmpty();
    case OriginRole: return journalOriginToString(entry.origin);
    case OpCountRole: return static_cast<int>(entry.ops.size()) + entry.truncatedOpCount;
    default: return {};
    }
}

QHash<int, QByteArray> JournalHistoryModel::roleNames() const
{
    return {
        {CommitIdRole, QByteArrayLiteral("commitId")},
        {ShaRole, QByteArrayLiteral("sha")},
        {MessageRole, QByteArrayLiteral("message")},
        {TimeTextRole, QByteArrayLiteral("timeText")},
        {DateKeyRole, QByteArrayLiteral("dateKey")},
        {DiffLinesRole, QByteArrayLiteral("diffLines")},
        {UndoableRole, QByteArrayLiteral("undoable")},
        {CanRestoreRole, QByteArrayLiteral("canRestore")},
        {OriginRole, QByteArrayLiteral("origin")},
        {OpCountRole, QByteArrayLiteral("opCount")}
    };
}

QString JournalHistoryModel::repo() const
{
    return m_repo;
}

void JournalHistoryModel::setRepo(const QString &repo)
{
    const QString normalized = (repo == u"settings"_s) ? u"settings"_s : u"actions"_s;
    if (m_repo == normalized)
        return;
    m_repo = normalized;
    emit repoChanged();
    reload();
}

QString JournalHistoryModel::filterText() const
{
    return m_filterText;
}

void JournalHistoryModel::setFilterText(const QString &text)
{
    if (m_filterText == text)
        return;
    m_filterText = text;
    emit filterChanged();
    reload();
}

bool JournalHistoryModel::filterRegex() const
{
    return m_filterRegex;
}

void JournalHistoryModel::setFilterRegex(const bool regex)
{
    if (m_filterRegex == regex)
        return;
    m_filterRegex = regex;
    emit filterChanged();
    reload();
}

int JournalHistoryModel::count() const
{
    return static_cast<int>(m_entries.size());
}

void JournalHistoryModel::refresh()
{
    reload();
}

bool JournalHistoryModel::matchesFilter(const JournalEntry &entry) const
{
    if (m_filterText.isEmpty())
        return true;

    QString haystack = entry.summary + u' ' + entry.shortId;
    for (const JournalOpRecord &op : entry.ops)
        haystack += u' ' + describeOp(op);

    if (m_filterRegex)
    {
        const QRegularExpression expression {m_filterText,
            QRegularExpression::CaseInsensitiveOption};
        if (!expression.isValid())
            return true; // invalid pattern — show everything, field shows the error state
        return expression.match(haystack).hasMatch();
    }
    return haystack.contains(m_filterText, Qt::CaseInsensitive);
}

void JournalHistoryModel::reload()
{
    beginResetModel();
    m_entries.clear();
    if (const TorrentJournal *journal = TorrentJournal::instance(); journal && journal->isAvailable())
    {
        const TorrentJournal::Repo repo = (m_repo == u"settings"_s)
            ? TorrentJournal::Repo::Settings : TorrentJournal::Repo::Actions;
        const QList<JournalEntry> entries = journal->history(repo, 500);
        for (const JournalEntry &entry : entries)
        {
            if (matchesFilter(entry))
                m_entries.append(entry);
        }
    }
    endResetModel();
    emit countChanged();
}
