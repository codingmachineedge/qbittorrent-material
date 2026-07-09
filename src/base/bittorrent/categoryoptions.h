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

#include <QString>

#include "base/utils/fs/path.h"
#include "downloadpathoption.h"
#include "sharelimits.h"

class QJsonObject;

namespace BitTorrent
{
    struct CategoryOptions
    {
        Path savePath;
        std::optional<DownloadPathOption> downloadPath;

        ShareLimits shareLimits;

        static CategoryOptions fromJSON(const QJsonObject &jsonObj);
        QJsonObject toJSON() const;

        friend bool operator==(const CategoryOptions &, const CategoryOptions &) = default;
    };
}
