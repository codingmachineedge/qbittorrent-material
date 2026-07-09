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

#include <QList>
#include <QMetaType>
#include <QString>

#include "base/tagset.h"
#include "base/utils/fs/path.h"
#include "sharelimits.h"
#include "sslparameters.h"
#include "torrent.h"
#include "torrentcontentlayout.h"

class QJsonObject;

namespace BitTorrent
{
    enum class DownloadPriority;

    struct AddTorrentParams
    {
        QString name;
        QString category;
        TagSet tags;
        Path savePath;
        std::optional<bool> useDownloadPath;
        Path downloadPath;
        bool sequential = false;
        bool firstLastPiecePriority = false;
        bool addForced = false;
        std::optional<bool> addToQueueTop;
        std::optional<bool> addStopped;
        std::optional<Torrent::StopCondition> stopCondition;
        PathList filePaths; // used if TorrentInfo is set
        QList<DownloadPriority> filePriorities; // used if TorrentInfo is set
        bool skipChecking = false;
        std::optional<BitTorrent::TorrentContentLayout> contentLayout;
        std::optional<bool> useAutoTMM;
        int uploadLimit = -1;
        int downloadLimit = -1;
        ShareLimits shareLimits;
        SSLParameters sslParameters;

        friend bool operator==(const AddTorrentParams &lhs, const AddTorrentParams &rhs) = default;
    };

    AddTorrentParams parseAddTorrentParams(const QJsonObject &jsonObj);
    QJsonObject serializeAddTorrentParams(const AddTorrentParams &params);
}

Q_DECLARE_METATYPE(BitTorrent::AddTorrentParams)
