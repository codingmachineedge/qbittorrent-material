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

#include <QDateTime>
#include <QHostAddress>
#include <QObject>
#include <QTimer>

#include "base/preferences.h"

namespace Net
{
    struct DownloadResult;

    /// Dynamic DNS updater (DynDNS / No-IP style). Periodically checks the public
    /// IP and, on change, pushes an update to the configured DDNS provider. Based on
    /// the dyndns.com update protocol. Credentials/service come from `Preferences`.
    class DNSUpdater final : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(DNSUpdater)

    public:
        explicit DNSUpdater(QObject *parent = nullptr);
        ~DNSUpdater() override;

        /// Provider sign-up URL for the given service (used by the options UI).
        static QUrl getRegistrationUrl(DNS::Service service);

    public slots:
        /// Re-reads service/domain/username/password from `Preferences`.
        void updateCredentials();

    private slots:
        void checkPublicIP();
        void ipRequestFinished(const DownloadResult &result);
        void updateDNSService();
        void ipUpdateFinished(const DownloadResult &result);

    private:
        enum State
        {
            OK,
            INVALID_CREDS,
            FATAL
        };

        QString getUpdateUrl() const;
        void processIPUpdateReply(const QString &reply);

        QHostAddress m_lastIP;
        QDateTime m_lastIPCheckTime;
        QTimer m_ipCheckTimer;
        int m_state = OK;
        // Service credentials.
        DNS::Service m_service = DNS::Service::None;
        QString m_domain;
        QString m_username;
        QString m_password;
    };
}
