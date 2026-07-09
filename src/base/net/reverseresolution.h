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

#include <boost/multi_index/hashed_index.hpp>
#include <boost/multi_index/key.hpp>
#include <boost/multi_index/tag.hpp>
#include <boost/multi_index_container.hpp>

#include <QCache>
#include <QHostAddress>
#include <QObject>

class QHostInfo;
class QString;

namespace Net
{
    /// Singleton asynchronous reverse-DNS resolver used by the peer list to show
    /// host names instead of raw IPs. Results are cached; a pending lookup returns
    /// empty and emits `ipResolved` when done (subscribe — never poll).
    class ReverseResolution final : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(ReverseResolution)

    public:
        static void initInstance();
        static void freeInstance();
        static ReverseResolution *instance();

        /// Returns the cached host name for `ip`, or empty while a lookup is queued.
        QString resolve(const QHostAddress &ip);

    signals:
        void ipResolved(const QHostAddress &ip, const QString &hostname);

    private slots:
        void hostResolved(const QHostInfo &host);

    private:
        ReverseResolution();
        ~ReverseResolution() override;

        static ReverseResolution *m_instance;

        struct LookupRequest
        {
            int id = -1; // lookup ID
            QHostAddress address;
        };

        using Lookups = boost::multi_index_container<
            LookupRequest,
            boost::multi_index::indexed_by<
                boost::multi_index::hashed_unique<boost::multi_index::tag<struct ByLookupID>, boost::multi_index::key<&LookupRequest::id>>,
                boost::multi_index::hashed_unique<boost::multi_index::tag<struct ByAddress>, boost::multi_index::key<&LookupRequest::address>>>>;
        Lookups m_lookups;

        QCache<QHostAddress, QString> m_cache; // <IP, HostName>
    };
}
