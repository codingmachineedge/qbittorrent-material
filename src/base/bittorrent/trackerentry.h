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

#include <QtContainerFwd>
#include <QString>

class QStringView;

namespace BitTorrent
{
    struct TrackerEntry
    {
        QString url {};
        int tier = 0;
    };

    QList<TrackerEntry> parseTrackerEntries(QStringView str);

    bool operator==(const TrackerEntry &left, const TrackerEntry &right);
    std::size_t qHash(const TrackerEntry &key, std::size_t seed = 0);
}
