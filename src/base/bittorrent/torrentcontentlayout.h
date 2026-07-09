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
#include <QMetaEnum>

#include "base/utils/fs/path.h"

namespace BitTorrent
{
    // Using `Q_ENUM_NS()` without a wrapper namespace in our case is not advised
    // since `Q_NAMESPACE` cannot be used when the same namespace resides at different files.
    // https://www.kdab.com/new-qt-5-8-meta-object-support-namespaces/#comment-143779
    inline namespace TorrentContentLayoutNS
    {
        Q_NAMESPACE

        enum class TorrentContentLayout
        {
            Original,
            Subfolder,
            NoSubfolder
        };

        Q_ENUM_NS(TorrentContentLayout)
    }
}
