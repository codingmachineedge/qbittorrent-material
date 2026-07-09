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

#include <QObject>

class QHostAddress;
class QString;

class GeoIPDatabase;

namespace Net
{
    struct DownloadResult;

    /// Singleton GeoIP resolver. Loads a local MaxMind DB (auto-downloading/updating
    /// it when enabled) and maps peer IPs to ISO country codes for the peer list's
    /// country-flag column (rendered via the QML `FlagImageProvider`).
    class GeoIPManager final : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(GeoIPManager)

    public:
        static void initInstance();
        static void freeInstance();
        static GeoIPManager *instance();

        /// Returns the ISO 3166-1 alpha-2 country code for `hostAddr` (or empty).
        QString lookup(const QHostAddress &hostAddr) const;

        /// Localized, human-readable country name for an ISO code.
        static QString CountryName(const QString &countryISOCode);

    private slots:
        void configure();
        void downloadFinished(const DownloadResult &result);

    private:
        GeoIPManager();
        ~GeoIPManager() override;

        void loadDatabase();
        void manageDatabaseUpdate();
        void downloadDatabaseFile();

        bool m_enabled = false;
        GeoIPDatabase *m_geoIPDatabase = nullptr;

        static GeoIPManager *m_instance;
    };
}
