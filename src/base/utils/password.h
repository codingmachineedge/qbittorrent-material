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

class QByteArray;
class QByteArrayView;
class QString;
class QStringView;

namespace Utils::Password
{
    // Implements constant-time comparison to protect against timing attacks
    // Taken from https://crackstation.net/hashing-security.htm
    bool slowEquals(QByteArrayView left, QByteArrayView right);

    QString generate(int passwordLength);

    namespace PBKDF2
    {
        QByteArray generate(const QString &password);
        QByteArray generate(const QByteArray &password);

        bool verify(const QByteArray &secret, QStringView password);
        bool verify(const QByteArray &secret, const QByteArray &password);
    }
}
