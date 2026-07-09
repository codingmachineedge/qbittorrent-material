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

#include <QMetaEnum>

namespace Net
{
    inline namespace ProxyTypeNS
    {
        Q_NAMESPACE

        /// Proxy protocol. Numeric values are persisted and MUST NOT change.
        enum class ProxyType
        {
            None = 0,
            HTTP = 1,
            SOCKS5 = 2,
            SOCKS4 = 5
        };
        Q_ENUM_NS(ProxyType)
    }
}
