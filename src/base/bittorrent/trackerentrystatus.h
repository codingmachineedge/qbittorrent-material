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

#include <QHash>
#include <QString>

#include "announcetimepoint.h"

class QStringView;

namespace BitTorrent
{
    enum class TrackerEndpointState
    {
        NotContacted = 1,
        Working = 2,
        NotWorking = 4,
        TrackerError = 5,
        Unreachable = 6
    };

    struct TrackerEndpointStatus
    {
        QString name {};
        int btVersion = 1;

        bool isUpdating = false;
        TrackerEndpointState state = TrackerEndpointState::NotContacted;
        QString message {};

        int numPeers = -1;
        int numSeeds = -1;
        int numLeeches = -1;
        int numDownloaded = -1;

        AnnounceTimePoint nextAnnounceTime {};
        AnnounceTimePoint minAnnounceTime {};
    };

    struct TrackerEntryStatus
    {
        QString url {};
        int tier = 0;

        bool isUpdating = false;
        TrackerEndpointState state = TrackerEndpointState::NotContacted;
        QString message {};

        int numPeers = -1;
        int numSeeds = -1;
        int numLeeches = -1;
        int numDownloaded = -1;

        AnnounceTimePoint nextAnnounceTime {};
        AnnounceTimePoint minAnnounceTime {};

        QHash<std::pair<QString, int>, TrackerEndpointStatus> endpoints {};

        void clear();
    };

    bool operator==(const TrackerEntryStatus &left, const TrackerEntryStatus &right);
    std::size_t qHash(const TrackerEntryStatus &key, std::size_t seed = 0);
}
