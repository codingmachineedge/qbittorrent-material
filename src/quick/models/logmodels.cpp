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

#include "logmodels.h"

#include <QDateTime>
#include <QLocale>

#include "base/logging.h"

namespace
{
    /// Row separator used when copying / assembling a full log line.
    const QString kSeparator = QStringLiteral(" - ");

    /// StateColors log-level id for a peer-ban line.
    const QString kBannedPeerLevel = QStringLiteral("BannedPeer");
}

// =============================================================================
// BaseLogModel
// =============================================================================

BaseLogModel::BaseLogModel(QObject *parent)
    : QAbstractListModel(parent)
    , m_entries(MAX_LOG_MESSAGES)
{
    qCDebug(lcModel) << "BaseLogModel created; capacity=" << MAX_LOG_MESSAGES;
}

int BaseLogModel::rowCount(const QModelIndex &parent) const
{
    // A flat list has no children under a valid parent.
    if (parent.isValid())
        return 0;
    return static_cast<int>(m_entries.size());
}

QVariant BaseLogModel::data(const QModelIndex &index, const int role) const
{
    if (!index.isValid())
        return {};

    const int row = index.row();
    if ((row < 0) || (row >= static_cast<int>(m_entries.size())))
        return {};

    const Entry &entry = m_entries[row];
    switch (role)
    {
    case TimeRole:
        return entry.time;
    case MessageRole:
        return entry.message;
    case TypeRole:
        return entry.type;
    case LogLevelRole:
        return entry.level;
    case TimestampRole:
        return entry.timestamp;
    default:
        return {};
    }
}

QHash<int, QByteArray> BaseLogModel::roleNames() const
{
    return {
        {TimeRole, QByteArrayLiteral("time")},
        {MessageRole, QByteArrayLiteral("message")},
        {TypeRole, QByteArrayLiteral("type")},
        {LogLevelRole, QByteArrayLiteral("logLevel")},
        {TimestampRole, QByteArrayLiteral("timestamp")}
    };
}

void BaseLogModel::append(const Entry &entry)
{
    // A full circular buffer would silently overwrite its front element on
    // push_back; emit the matching row removal first so the view stays in sync.
    if (m_entries.size() == m_entries.capacity())
    {
        beginRemoveRows({}, 0, 0);
        m_entries.pop_front();
        endRemoveRows();
    }

    const int row = static_cast<int>(m_entries.size());
    beginInsertRows({}, row, row);
    m_entries.push_back(entry);
    endInsertRows();
}

void BaseLogModel::reset()
{
    qCDebug(lcModel) << "Clearing log view buffer (" << m_entries.size() << "rows)";
    beginResetModel();
    m_entries.clear();
    endResetModel();
}

QString BaseLogModel::rowText(const int row) const
{
    if ((row < 0) || (row >= static_cast<int>(m_entries.size())))
        return {};

    const Entry &entry = m_entries[row];
    return entry.time + kSeparator + entry.message;
}

QString BaseLogModel::levelName(const Logger::MsgType type)
{
    switch (type)
    {
    case Logger::Info:
        return QStringLiteral("Info");
    case Logger::Warning:
        return QStringLiteral("Warning");
    case Logger::Critical:
        return QStringLiteral("Critical");
    case Logger::Normal:
    case Logger::All:
        break;
    }
    return QStringLiteral("Normal");
}

QString BaseLogModel::formatTime(const qint64 timestamp)
{
    const QDateTime dateTime = QDateTime::fromSecsSinceEpoch(timestamp);
    return QLocale::system().toString(dateTime, QLocale::ShortFormat);
}

// =============================================================================
// LogMessageModel
// =============================================================================

LogMessageModel::LogMessageModel(QObject *parent)
    : BaseLogModel(parent)
{
    // Preload the existing ring in chronological order, then subscribe for more.
    const QList<Logger::Message> history = Logger::instance()->getMessages();
    for (const Logger::Message &message : history)
        handleNewMessage(message);

    connect(Logger::instance(), &Logger::newLogMessage, this, &LogMessageModel::handleNewMessage);
    qCDebug(lcModel) << "LogMessageModel ready; preloaded" << history.size() << "message(s)";
}

void LogMessageModel::handleNewMessage(const Logger::Message &message)
{
    Entry entry;
    entry.timestamp = message.timestamp;
    entry.time = formatTime(message.timestamp);
    entry.message = message.message;
    entry.type = message.type;
    entry.level = levelName(message.type);
    append(entry);
}

// =============================================================================
// LogPeerModel
// =============================================================================

LogPeerModel::LogPeerModel(QObject *parent)
    : BaseLogModel(parent)
{
    const QList<Logger::Peer> history = Logger::instance()->getPeers();
    for (const Logger::Peer &peer : history)
        handleNewPeer(peer);

    connect(Logger::instance(), &Logger::newLogPeer, this, &LogPeerModel::handleNewPeer);
    qCDebug(lcModel) << "LogPeerModel ready; preloaded" << history.size() << "peer event(s)";
}

void LogPeerModel::handleNewPeer(const Logger::Peer &peer)
{
    Entry entry;
    entry.timestamp = peer.timestamp;
    entry.time = formatTime(peer.timestamp);
    entry.message = peer.blocked
        ? tr("%1 was blocked. Reason: %2.", "0.0.0.0 was blocked. Reason: reason for blocking.")
              .arg(peer.ip, peer.reason)
        : tr("%1 was banned", "0.0.0.0 was banned").arg(peer.ip);
    entry.type = Logger::Normal;
    entry.level = kBannedPeerLevel; // always the banned-peer color
    append(entry);
}

// =============================================================================
// LogFilterProxy
// =============================================================================

LogFilterProxy::LogFilterProxy(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    qCDebug(lcModel) << "LogFilterProxy created; messageTypes=" << m_messageTypes;
}

int LogFilterProxy::messageTypes() const
{
    return m_messageTypes;
}

void LogFilterProxy::setMessageTypes(const int types)
{
    if (m_messageTypes == types)
        return;

    m_messageTypes = types;
    qCDebug(lcModel) << "LogFilterProxy message-type mask ->" << types;
    invalidateRowsFilter();
    emit messageTypesChanged();
}

void LogFilterProxy::reset()
{
    if (auto *model = qobject_cast<BaseLogModel *>(sourceModel()))
        model->reset();
    else
        qCWarning(lcModel) << "LogFilterProxy::reset() called with no BaseLogModel source";
}

QString LogFilterProxy::rowText(const int row) const
{
    auto *model = qobject_cast<BaseLogModel *>(sourceModel());
    if (model == nullptr)
        return {};

    const QModelIndex sourceIndex = mapToSource(index(row, 0));
    return model->rowText(sourceIndex.row());
}

bool LogFilterProxy::filterAcceptsRow(const int sourceRow, const QModelIndex &sourceParent) const
{
    const QAbstractItemModel *model = sourceModel();
    if (model == nullptr)
        return false;

    const QModelIndex sourceIndex = model->index(sourceRow, 0, sourceParent);
    const auto type = static_cast<Logger::MsgType>(
        model->data(sourceIndex, BaseLogModel::TypeRole).toInt());

    return Logger::MsgTypes::fromInt(m_messageTypes).testFlag(type);
}
