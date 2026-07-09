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

#include <QtTypes>

namespace BitTorrent
{
    /// Aggregate, session-wide transfer statistics (speeds and cumulative totals).
    struct SessionStatus
    {
        bool hasIncomingConnections = false;

        // Current download rate for the BT
        // session. Payload means that it only take into
        // account "useful" part of the rate
        qint64 payloadDownloadRate = 0;

        // Current upload rate for the BT
        // session. Payload means that it only take into
        // account "useful" part of the rate
        qint64 payloadUploadRate = 0;

        // Additional download/upload rates
        qint64 uploadRate = 0;
        qint64 downloadRate = 0;
        qint64 ipOverheadUploadRate = 0;
        qint64 ipOverheadDownloadRate = 0;
        qint64 dhtUploadRate = 0;
        qint64 dhtDownloadRate = 0;
        qint64 trackerUploadRate = 0;
        qint64 trackerDownloadRate = 0;

        qint64 allTimeDownload = 0;
        qint64 allTimeUpload = 0;
        qint64 totalDownload = 0;
        qint64 totalUpload = 0;
        qint64 totalPayloadDownload = 0;
        qint64 totalPayloadUpload = 0;
        qint64 ipOverheadUpload = 0;
        qint64 ipOverheadDownload = 0;
        qint64 dhtUpload = 0;
        qint64 dhtDownload = 0;
        qint64 trackerUpload = 0;
        qint64 trackerDownload = 0;
        qint64 totalWasted = 0;
        qint64 diskReadQueue = 0;
        qint64 diskWriteQueue = 0;
        qint64 dhtNodes = 0;
        qint64 peersCount = 0;

        qint64 queuedTrackerAnnounces = 0;
    };

    /// Disk cache statistics reported by the libtorrent session.
    ///
    /// @note In the legacy engine this lived in a separate `cachestatus.h`. It is
    /// consolidated here so it stays with `SessionStatus`; includes that referenced
    /// `cachestatus.h` should include this header instead.
    struct CacheStatus
    {
        qint64 totalUsedBuffers = 0;
        qint64 jobQueueLength = 0;
        qint64 averageJobTime = 0;
        qint64 queuedBytes = 0;
        qreal readRatio = 0;  // TODO(engine): remove when LIBTORRENT_VERSION_NUM >= 20000
        qint64 requestLatency = 0;
    };
}
