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

#include "logger.h"

#include <algorithm>
#include <iterator>
#include <mutex>
#include <ranges>

#include <QDateTime>
#include <QList>
#include <QMutex>
#include <QMutexLocker>

namespace
{
    /// Copy a circular buffer (optionally skipping @p offset leading items)
    /// into a QList in chronological order.
    template <typename T>
    QList<T> loadFromBuffer(const boost::circular_buffer<T> &src, const int offset = 0)
    {
        QList<T> out;
        const int count = static_cast<int>(src.size()) - offset;
        if (count <= 0)
            return out;
        out.reserve(count);
        std::ranges::copy(std::views::drop(src, offset), std::back_inserter(out));
        return out;
    }
}

Logger *Logger::m_instance = nullptr;

Logger::Logger()
    : m_messages(MAX_LOG_MESSAGES)
    , m_peers(MAX_LOG_MESSAGES)
{
    // Required so newLogMessage/newLogPeer can cross thread boundaries via
    // queued connections (addMessage may be invoked from the engine IO thread).
    qRegisterMetaType<Logger::Message>();
    qRegisterMetaType<Logger::Peer>();
}

Logger *Logger::instance()
{
    // Guarded lazy init so the message handler can reach us at any lifetime
    // point (very early startup / very late shutdown) from any thread.
    static QMutex mutex;
    const QMutexLocker locker(&mutex);
    if (m_instance == nullptr)
        m_instance = new Logger;
    return m_instance;
}

void Logger::initInstance()
{
    (void)instance();
}

void Logger::freeInstance()
{
    delete m_instance;
    m_instance = nullptr;
}

void Logger::addMessage(const QString &message, const MsgType type)
{
    Message msg;
    {
        const QWriteLocker locker(&m_lock);
        msg = {m_msgCounter++, type, QDateTime::currentSecsSinceEpoch(), message};
        m_messages.push_back(msg);
    }
    emit newLogMessage(msg);
}

void Logger::addPeer(const QString &ip, const bool blocked, const QString &reason)
{
    Peer peer;
    {
        const QWriteLocker locker(&m_lock);
        peer = {m_peerCounter++, blocked, QDateTime::currentSecsSinceEpoch(), ip, reason};
        m_peers.push_back(peer);
    }
    emit newLogPeer(peer);
}

QList<Logger::Message> Logger::getMessages(const int lastKnownId) const
{
    const QReadLocker locker(&m_lock);

    const int diff = m_msgCounter - lastKnownId - 1;
    const int size = static_cast<int>(m_messages.size());

    if ((lastKnownId == -1) || (diff >= size))
        return loadFromBuffer(m_messages);
    if (diff <= 0)
        return {};
    return loadFromBuffer(m_messages, (size - diff));
}

QList<Logger::Peer> Logger::getPeers(const int lastKnownId) const
{
    const QReadLocker locker(&m_lock);

    const int diff = m_peerCounter - lastKnownId - 1;
    const int size = static_cast<int>(m_peers.size());

    if ((lastKnownId == -1) || (diff >= size))
        return loadFromBuffer(m_peers);
    if (diff <= 0)
        return {};
    return loadFromBuffer(m_peers, (size - diff));
}

// Upstream-compatibility free function (see logger.h).
void LogMsg(const QString &message, Logger::MsgType type)
{
    Logger::instance()->addMessage(message, type);
}
