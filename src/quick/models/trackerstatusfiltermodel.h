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

#include <array>

#include <QAbstractListModel>
#include <QString>

#include <qqmlintegration.h>

#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/bittorrent/torrentannouncestatus.h"
#include "base/bittorrent/trackerentrystatus.h"
#include "base/logging.h"

/**
 * @file trackerstatusfiltermodel.h
 * @brief @c TrackerStatusFilterModel — the separate "Tracker status" sidebar
 *        panel (only shown when `TransferListFilters/SeparateTrackerStatusFilter`
 *        is on).
 *
 * Four fixed rows: All, Warning, Tracker error, Other error — counts derived from
 * each torrent's `announceStatus()` flags. Selecting a row drives
 * `TorrentFilterProxyModel::setAnnounceStatusFilter` (All clears it).
 */
class TrackerStatusFilterModel final : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum RowKind
    {
        AllStatuses = 0,
        Warning = 1,
        TrackerError = 2,
        OtherError = 3,

        RowCount
    };
    Q_ENUM(RowKind)

    enum Roles
    {
        LabelRole = Qt::UserRole + 1, // "label"
        CountRole,                    // "count"
        IconRole,                     // "icon"
        AnnounceFlagRole              // "announceFlag" (TorrentAnnounceStatusFlag or -1 for All)
    };
    Q_ENUM(Roles)

    explicit TrackerStatusFilterModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        qCDebug(lcModel) << "TrackerStatusFilterModel created";
        subscribe();
        recount();
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : RowCount;
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if ((index.row() < 0) || (index.row() >= RowCount))
            return {};

        const int kind = index.row();
        switch (role)
        {
        case LabelRole:
        case Qt::DisplayRole:
            return QStringLiteral("%1 (%2)").arg(labelFor(kind)).arg(m_counts.at(kind));
        case CountRole:
            return m_counts.at(kind);
        case IconRole:
            return iconFor(kind);
        case AnnounceFlagRole:
            return announceFlagFor(kind);
        default:
            return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {{LabelRole, "label"}, {CountRole, "count"}, {IconRole, "icon"}
                , {AnnounceFlagRole, "announceFlag"}};
    }

private:
    void subscribe()
    {
        BitTorrent::Session *const session = BitTorrent::Session::instance();
        if (!session)
        {
            qCWarning(lcModel) << "TrackerStatusFilterModel: no Session instance";
            return;
        }
        connect(session, &BitTorrent::Session::torrentsLoaded, this, [this] { recount(); });
        connect(session, &BitTorrent::Session::torrentsUpdated, this, [this] { recount(); });
        connect(session, &BitTorrent::Session::torrentAdded, this, [this](BitTorrent::Torrent *) { recount(); });
        connect(session, &BitTorrent::Session::torrentAboutToBeRemoved, this, [this](BitTorrent::Torrent *) { recount(); });
        connect(session, &BitTorrent::Session::trackerEntryStatusesUpdated, this, [this](BitTorrent::Torrent *, const QHash<QString, BitTorrent::TrackerEntryStatus> &) { recount(); });
    }

    [[nodiscard]] static QString labelFor(int kind)
    {
        switch (kind)
        {
        case AllStatuses:  return tr("All");
        case Warning:      return tr("Warning");
        case TrackerError: return tr("Tracker error");
        case OtherError:   return tr("Other error");
        default:           return {};
        }
    }

    [[nodiscard]] static QString iconFor(int kind)
    {
        switch (kind)
        {
        case AllStatuses:  return QStringLiteral("trackers");
        case Warning:      return QStringLiteral("tracker_warning");
        case TrackerError: return QStringLiteral("tracker_error");
        case OtherError:   return QStringLiteral("tracker_error");
        default:           return {};
        }
    }

    [[nodiscard]] static int announceFlagFor(int kind)
    {
        switch (kind)
        {
        case Warning:      return static_cast<int>(BitTorrent::TorrentAnnounceStatusFlag::HasWarning);
        case TrackerError: return static_cast<int>(BitTorrent::TorrentAnnounceStatusFlag::HasTrackerError);
        case OtherError:   return static_cast<int>(BitTorrent::TorrentAnnounceStatusFlag::HasOtherError);
        default:           return -1;
        }
    }

    void recount()
    {
        std::array<int, RowCount> counts {};
        BitTorrent::Session *const session = BitTorrent::Session::instance();
        const QList<BitTorrent::Torrent *> torrents = session ? session->torrents() : QList<BitTorrent::Torrent *> {};

        using BitTorrent::TorrentAnnounceStatusFlag;
        counts[AllStatuses] = static_cast<int>(torrents.size());
        for (const BitTorrent::Torrent *const t : torrents)
        {
            const BitTorrent::TorrentAnnounceStatus announce = t->announceStatus();
            if (announce & TorrentAnnounceStatusFlag::HasWarning)      ++counts[Warning];
            if (announce & TorrentAnnounceStatusFlag::HasTrackerError) ++counts[TrackerError];
            if (announce & TorrentAnnounceStatusFlag::HasOtherError)   ++counts[OtherError];
        }

        if (counts == m_counts)
            return;

        m_counts = counts;
        emit dataChanged(index(0), index(RowCount - 1), {LabelRole, CountRole, Qt::DisplayRole});
        qCDebug(lcModel) << "TrackerStatusFilterModel recounted";
    }

    std::array<int, RowCount> m_counts {};
};
