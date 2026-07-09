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

#include <QSet>
#include <QString>
#include <QUrl>

#include "base/bittorrent/infohash.h"
#include "base/bittorrent/torrent.h"
#include "base/bittorrent/torrentannouncestatus.h"
#include "base/bittorrent/trackerentrystatus.h"
#include "base/tag.h"

/**
 * @file torrentfilter.h
 * @brief The self-contained @c TorrentFilter used by @c TorrentFilterProxyModel.
 *
 * This mirrors the semantics of legacy qBittorrent's @c TorrentFilter (status +
 * category + tag + tracker host + announce status + id set + private flag), all
 * ANDed together in @ref TorrentFilter::match. It is intentionally header-only so
 * the bridge layer does not depend on a base-engine filter class.
 */

using TorrentIDSet = QSet<BitTorrent::TorrentID>;

/// Extract the host part of a tracker URL (used to bucket torrents by tracker).
inline QString trackerHostFromURL(const QString &url)
{
    const QString host = QUrl(url).host();
    return host.isEmpty() ? url : host;
}

/// A conjunction of independent predicates applied to a @c BitTorrent::Torrent.
/// Every unset (`std::nullopt`) criterion matches everything.
struct TorrentFilter
{
    /// Fixed order — matches the Status filter sidebar row order (FEATURE_SPEC §6a).
    enum Status
    {
        All = 0,
        Downloading,
        Seeding,
        Completed,
        Running,
        Stopped,
        Active,
        Inactive,
        Stalled,
        StalledUploading,
        StalledDownloading,
        Checking,
        Moving,
        Errored,

        _Count
    };

    Status status = All;
    std::optional<QString> category;                                ///< empty string == uncategorized
    std::optional<Tag> tag;                                         ///< empty tag == untagged
    std::optional<TorrentIDSet> idSet;                              ///< restrict to a set of ids
    std::optional<bool> isPrivate;
    std::optional<QString> trackerHost;                            ///< empty string == trackerless
    std::optional<BitTorrent::TorrentAnnounceStatus> announceStatus;

    /// Full conjunction — true when every set criterion accepts @p torrent.
    [[nodiscard]] bool match(const BitTorrent::Torrent *torrent) const
    {
        if (!torrent)
            return false;

        return matchStatus(torrent) && matchHash(torrent) && matchCategory(torrent)
                && matchTag(torrent) && matchPrivate(torrent) && matchTracker(torrent)
                && matchAnnounceStatus(torrent);
    }

private:
    [[nodiscard]] bool matchStatus(const BitTorrent::Torrent *t) const
    {
        using BitTorrent::TorrentState;

        switch (status)
        {
        case All:                return true;
        case Downloading:        return t->isDownloading();
        case Seeding:            return t->isUploading();
        case Completed:          return t->isCompleted();
        case Running:            return t->isRunning();
        case Stopped:            return t->isStopped();
        case Active:             return t->isActive();
        case Inactive:           return t->isInactive();
        case Stalled:            return (t->state() == TorrentState::StalledUploading)
                                        || (t->state() == TorrentState::StalledDownloading);
        case StalledUploading:   return t->state() == TorrentState::StalledUploading;
        case StalledDownloading: return t->state() == TorrentState::StalledDownloading;
        case Checking:           return t->isChecking();
        case Moving:             return t->isMoving();
        case Errored:            return t->isErrored();
        default:                 return true;
        }
    }

    [[nodiscard]] bool matchHash(const BitTorrent::Torrent *t) const
    {
        return !idSet.has_value() || idSet->contains(t->id());
    }

    [[nodiscard]] bool matchCategory(const BitTorrent::Torrent *t) const
    {
        return !category.has_value() || t->belongsToCategory(*category);
    }

    [[nodiscard]] bool matchTag(const BitTorrent::Torrent *t) const
    {
        if (!tag.has_value())
            return true;
        if (tag->isEmpty())
            return t->tags().isEmpty();
        return t->hasTag(*tag);
    }

    [[nodiscard]] bool matchPrivate(const BitTorrent::Torrent *t) const
    {
        return !isPrivate.has_value() || (t->isPrivate() == *isPrivate);
    }

    [[nodiscard]] bool matchTracker(const BitTorrent::Torrent *t) const
    {
        if (!trackerHost.has_value())
            return true;

        const QString &wanted = *trackerHost;
        const QList<BitTorrent::TrackerEntryStatus> trackers = t->trackers();
        if (wanted.isEmpty())
            return trackers.isEmpty();

        for (const BitTorrent::TrackerEntryStatus &status : trackers)
        {
            if (trackerHostFromURL(status.url) == wanted)
                return true;
        }
        return false;
    }

    [[nodiscard]] bool matchAnnounceStatus(const BitTorrent::Torrent *t) const
    {
        if (!announceStatus.has_value())
            return true;
        return (t->announceStatus() & *announceStatus) != BitTorrent::TorrentAnnounceStatus();
    }
};
