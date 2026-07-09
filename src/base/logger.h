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

#include <QMetaType>
#include <QObject>
#include <QReadWriteLock>
#include <QString>
#include <QtContainerFwd>

/**
 * @file logger.h
 * @brief In-memory backend for the Material "Execution Log" screen.
 *
 * ::Logger is a singleton ring buffer (20 000 entries) of log messages plus a
 * parallel ring of peer-ban events. The dual-sink message handler
 * (Logging::installMessageHandler) forwards INFO+ Qt messages here via
 * addMessage(); the network stack reports blocked/whitelisted peers via
 * addPeer(). The ExecutionLogController bridge subscribes to
 * newLogMessage()/newLogPeer() and exposes the buffers to QML — never polling.
 */

/// Maximum number of retained entries per ring buffer.
inline constexpr int MAX_LOG_MESSAGES = 20000;

class Logger final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Logger)

public:
    /// Severity class of an Execution-Log entry. Values are bit flags so the
    /// Execution-Log filter can select several classes at once.
    enum MsgType
    {
        All = -1,
        Normal = 0x1,   ///< default informational line
        Info = 0x2,     ///< maps from qCInfo / QtInfoMsg
        Warning = 0x4,  ///< maps from qCWarning / QtWarningMsg
        Critical = 0x8  ///< maps from qCCritical / QtCriticalMsg (ERROR clashes with libtorrent)
    };
    Q_ENUM(MsgType)
    Q_DECLARE_FLAGS(MsgTypes, MsgType)

    /// A single Execution-Log message.
    struct Message
    {
        int id = -1;
        MsgType type = Normal;
        qint64 timestamp = -1; ///< Unix seconds
        QString message;
    };

    /// A single peer-ban / peer-whitelist event.
    struct Peer
    {
        int id = -1;
        bool blocked = false;
        qint64 timestamp = -1; ///< Unix seconds
        QString ip;
        QString reason;
    };

    /// Returns the process-wide instance, lazily creating it (thread-safe).
    static Logger *instance();
    /// Optional explicit bootstrap (equivalent to touching instance()).
    static void initInstance();
    /// Tear down the instance (test/shutdown helper).
    static void freeInstance();

    /// Append a message and emit newLogMessage().
    void addMessage(const QString &message, MsgType type = Normal);
    /// Append a peer event and emit newLogPeer().
    void addPeer(const QString &ip, bool blocked, const QString &reason = {});

    /// Messages newer than @p lastKnownId (-1 => the whole buffer).
    QList<Message> getMessages(int lastKnownId = -1) const;
    /// Peer events newer than @p lastKnownId (-1 => the whole buffer).
    QList<Peer> getPeers(int lastKnownId = -1) const;

signals:
    void newLogMessage(const Logger::Message &message);
    void newLogPeer(const Logger::Peer &peer);

private:
    Logger();
    ~Logger() override = default;

    static Logger *m_instance;

    boost::circular_buffer<Message> m_messages;
    boost::circular_buffer<Peer> m_peers;
    mutable QReadWriteLock m_lock;
    int m_msgCounter = 0;
    int m_peerCounter = 0;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(Logger::MsgTypes)
Q_DECLARE_METATYPE(Logger::Message)
Q_DECLARE_METATYPE(Logger::Peer)
