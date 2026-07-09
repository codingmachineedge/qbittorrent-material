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

#include <QByteArray>
#include <QSslCertificate>
#include <QSslKey>

namespace BitTorrent
{
    struct SSLParameters
    {
        QSslCertificate certificate {{}};
        QSslKey privateKey;
        QByteArray dhParams;

        bool isValid() const;

        friend bool operator==(const SSLParameters &left, const SSLParameters &right) = default;
    };
}
