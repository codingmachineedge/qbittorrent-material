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

#pragma once

#include <boost/circular_buffer.hpp>

#include <QAbstractListModel>
#include <QByteArray>
#include <QHash>
#include <QSortFilterProxyModel>
#include <QString>

#include <qqmlintegration.h>

#include "base/logger.h"

/**
 * @file logmodels.h
 * @brief Bridge models for the Material "Execution Log" screen.
 *
 * Three QML-registered types plus a shared base:
 *  - ::BaseLogModel  — a ring-buffered (20 000) QAbstractListModel of log lines;
 *    exposes named roles (time / message / type / logLevel / timestamp).
 *  - ::LogMessageModel — general log; subscribes to Logger::newLogMessage.
 *  - ::LogPeerModel    — blocked/banned peers; subscribes to Logger::newLogPeer.
 *  - ::LogFilterProxy  — filters the general log by the four message-type flags
 *    (Normal/Info/Warning/Critical), bound to ExecutionLogController.
 *
 * All models subscribe to ::Logger signals and never poll (CONTRACTS §7).
 * Newest entries are appended at the bottom so views can auto-scroll like a
 * console.
 */
class BaseLogModel : public QAbstractListModel
{
    Q_OBJECT
    // Intermediate base of the two QML_ELEMENT log models. Registered anonymously
    // so the QML type system knows the hierarchy (and the inherited reset()/
    // rowText() invokables) without letting QML instantiate it directly.
    QML_ANONYMOUS
    Q_DISABLE_COPY_MOVE(BaseLogModel)

public:
    /// Named roles surfaced to QML delegates (CONTRACTS §7.1).
    enum Roles
    {
        TimeRole = Qt::UserRole + 1, ///< "time"      localized short time string
        MessageRole,                 ///< "message"   the log text
        TypeRole,                    ///< "type"      Logger::MsgType int (for the proxy)
        LogLevelRole,                ///< "logLevel"  severity id for StateColors.forLog()
        TimestampRole                ///< "timestamp" raw Unix seconds
    };

    explicit BaseLogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    /// Clear the visible buffer (does not touch the underlying Logger ring).
    Q_INVOKABLE void reset();

    /// "time - message" for a given row — used by the Copy action.
    Q_INVOKABLE QString rowText(int row) const;

protected:
    /// One rendered log line.
    struct Entry
    {
        qint64 timestamp = -1;
        QString time;
        QString message;
        int type = Logger::Normal; ///< Logger::MsgType value (for filtering)
        QString level;             ///< "Normal"/"Info"/"Warning"/"Critical"/"BannedPeer"
    };

    /// Append @p entry at the bottom, dropping the oldest row if the ring is full.
    void append(const Entry &entry);

    /// Map a Logger::MsgType to its StateColors log-level id.
    static QString levelName(Logger::MsgType type);
    /// Localized short-format time string for a Unix-seconds @p timestamp.
    static QString formatTime(qint64 timestamp);

private:
    boost::circular_buffer<Entry> m_entries;
};

/// General execution-log model: mirrors Logger::newLogMessage.
class LogMessageModel final : public BaseLogModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_DISABLE_COPY_MOVE(LogMessageModel)

public:
    explicit LogMessageModel(QObject *parent = nullptr);

private:
    void handleNewMessage(const Logger::Message &message);
};

/// Blocked/banned-peers model: mirrors Logger::newLogPeer.
class LogPeerModel final : public BaseLogModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_DISABLE_COPY_MOVE(LogPeerModel)

public:
    explicit LogPeerModel(QObject *parent = nullptr);

private:
    void handleNewPeer(const Logger::Peer &peer);
};

/// Filters a ::LogMessageModel by the enabled message-type flags. Bind
/// @c messageTypes to @c ExecutionLogController.messageTypes so the visible
/// severities follow the View menu live.
class LogFilterProxy final : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_DISABLE_COPY_MOVE(LogFilterProxy)

    /// A @c Logger::MsgTypes mask (as int). Default @c Logger::All (-1).
    Q_PROPERTY(int messageTypes READ messageTypes WRITE setMessageTypes NOTIFY messageTypesChanged)

public:
    explicit LogFilterProxy(QObject *parent = nullptr);

    [[nodiscard]] int messageTypes() const;
    void setMessageTypes(int types);

    /// Clear the underlying model's buffer (forwarded to ::BaseLogModel::reset).
    Q_INVOKABLE void reset();
    /// "time - message" for a proxy @p row (mapped to the source model).
    Q_INVOKABLE QString rowText(int row) const;

signals:
    void messageTypesChanged();

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    int m_messageTypes = static_cast<int>(Logger::All);
};
