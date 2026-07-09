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

namespace BitTorrent
{
    inline const qreal DEFAULT_RATIO_LIMIT = -2;
    inline const qreal NO_RATIO_LIMIT = -1;

    inline const int DEFAULT_SEEDING_TIME_LIMIT = -2;
    inline const int NO_SEEDING_TIME_LIMIT = -1;

    // Using `Q_ENUM_NS()` without a wrapper namespace in our case is not advised
    // since `Q_NAMESPACE` cannot be used when the same namespace resides at different files.
    // https://www.kdab.com/new-qt-5-8-meta-object-support-namespaces/#comment-143779
    inline namespace ShareLimitsNS
    {
        Q_NAMESPACE

        // These values should remain unchanged when adding new items
        // so as not to break the existing user settings.
        enum class ShareLimitAction
        {
            Default = -1, // special value

            Stop = 0,
            Remove = 1,
            RemoveWithContent = 3,
            EnableSuperSeeding = 2
        };

        Q_ENUM_NS(ShareLimitAction)

        enum class ShareLimitsMode
        {
            Default = -1, // special value

            MatchAny = 0,
            MatchAll = 1
        };

        Q_ENUM_NS(ShareLimitsMode)
    }

    struct ShareLimits
    {
        qreal ratioLimit = DEFAULT_RATIO_LIMIT;
        int seedingTimeLimit = DEFAULT_SEEDING_TIME_LIMIT;
        int inactiveSeedingTimeLimit = DEFAULT_SEEDING_TIME_LIMIT;

        ShareLimitsMode mode = ShareLimitsMode::Default;

        ShareLimitAction action = ShareLimitAction::Default;

        friend bool operator==(const ShareLimits &lhs, const ShareLimits &rhs) = default;
    };
}
