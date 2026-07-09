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

#include "base/global.h"
#include "base/settingvalue.h"
#include "proxytype.h"

namespace Net
{
    /// Immutable-ish value describing the application-wide network proxy.
    struct ProxyConfiguration
    {
        ProxyType type = ProxyType::None;
        QString ip;
        ushort port = 8080;
        bool authEnabled = false;
        QString username;
        QString password;
        bool hostnameLookupEnabled = true;
    };
    bool operator==(const ProxyConfiguration &left, const ProxyConfiguration &right);
    inline bool operator!=(const ProxyConfiguration &left, const ProxyConfiguration &right)
    {
        return !(left == right);
    }

    /// Singleton owner of the global proxy configuration. Persists each field via
    /// `SettingValue<>` (legacy keys preserved) and notifies on change so the
    /// download manager, session and other consumers re-apply their proxy.
    class ProxyConfigurationManager final : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(ProxyConfigurationManager)

        explicit ProxyConfigurationManager(QObject *parent = nullptr);
        ~ProxyConfigurationManager() override = default;

    public:
        static void initInstance();
        static void freeInstance();
        static ProxyConfigurationManager *instance();

        ProxyConfiguration proxyConfiguration() const;
        void setProxyConfiguration(const ProxyConfiguration &config);

    signals:
        void proxyConfigurationChanged();

    private:
        static ProxyConfigurationManager *m_instance;
        ProxyConfiguration m_config;
        SettingValue<ProxyType> m_storeProxyType;
        SettingValue<QString> m_storeProxyIP;
        SettingValue<ushort> m_storeProxyPort;
        SettingValue<bool> m_storeProxyAuthEnabled;
        SettingValue<QString> m_storeProxyUsername;
        SettingValue<QString> m_storeProxyPassword;
        SettingValue<bool> m_storeProxyHostnameLookupEnabled;
    };
}
