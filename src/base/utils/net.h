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

#include <optional>
#include <utility>

#include <QtContainerFwd>
#include <QHostAddress>

class QSslCertificate;
class QSslKey;
class QString;

namespace Utils::Net
{
    // alias for `QHostAddress::parseSubnet()` return type
    using Subnet = std::pair<QHostAddress, int>;
    using IPRange = std::pair<QHostAddress, QHostAddress>;

    bool isValidIP(const QString &ip);
    std::optional<Subnet> parseSubnet(const QString &subnetStr);
    bool isIPInSubnets(const QHostAddress &addr, const QList<Subnet> &subnets);
    QString subnetToString(const Subnet &subnet);
    IPRange subnetToIPRange(const Subnet &subnet);
    QHostAddress canonicalIPv6Addr(const QHostAddress &addr);

    std::optional<IPRange> parseIPRange(QStringView filterStr, bool isStrictIPv4 = false);
    QString ipRangeToString(const IPRange &ipRange);

    inline const int MAX_SSL_FILE_SIZE = 1024 * 1024;
    QList<QSslCertificate> loadSSLCertificate(const QByteArray &data);
    bool isSSLCertificatesValid(const QByteArray &data);
}
