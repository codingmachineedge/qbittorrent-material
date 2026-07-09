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

#include <QAbstractListModel>
#include <QList>
#include <QMap>
#include <QSet>
#include <QString>

#include <qqmlintegration.h>

#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/bittorrent/torrentannouncestatus.h"
#include "base/bittorrent/trackerentry.h"
#include "base/bittorrent/trackerentrystatus.h"
#include "base/logging.h"
#include "base/preferences.h"
#include "torrentfilter.h"

/**
 * @file trackersfiltermodel.h
 * @brief @c TrackersFilterModel — the Trackers section of the sidebar.
 *
 * Row 0 *All*, row 1 *Trackerless*. When the tracker-status filter is **not**
 * split out (`separateStatusFilter == false`), three merged status rows are
 * inserted (Tracker error, Other error, Warning). Below the specials, one row per
 * distinct tracker host, natural-sorted, each with a live count. Selecting a row
 * drives `TorrentFilterProxyModel::setTrackerFilter` /
 * `setAnnounceStatusFilter`.
 */
class TrackersFilterModel final : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool separateStatusFilter READ separateStatusFilter WRITE setSeparateStatusFilter NOTIFY separateStatusFilterChanged)

public:
    enum FilterType
    {
        AllTrackers = 0,
        Trackerless = 1,
        StatusTrackerError = 2,
        StatusOtherError = 3,
        StatusWarning = 4,
        Host = 5
    };
    Q_ENUM(FilterType)

    enum Roles
    {
        LabelRole = Qt::UserRole + 1, // "label"
        CountRole,                    // "count"
        IconRole,                     // "icon"
        ValueRole,                    // "value" (host string; empty otherwise)
        TypeRole,                     // "type"  (FilterType)
        AnnounceFlagRole              // "announceFlag" (TorrentAnnounceStatusFlag or -1)
    };
    Q_ENUM(Roles)

    explicit TrackersFilterModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        qCDebug(lcModel) << "TrackersFilterModel created";
        if (const Preferences *const pref = Preferences::instance())
            m_separate = pref->useSeparateTrackerStatusFilter();
        subscribe();
        rebuild();
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if ((index.row() < 0) || (index.row() >= m_rows.size()))
            return {};

        const Row &row = m_rows.at(index.row());
        switch (role)
        {
        case LabelRole:
        case Qt::DisplayRole:
            return QStringLiteral("%1 (%2)").arg(labelFor(row)).arg(row.count);
        case CountRole:
            return row.count;
        case IconRole:
            return iconFor(row);
        case ValueRole:
            return row.host;
        case TypeRole:
            return row.type;
        case AnnounceFlagRole:
            return announceFlagFor(row);
        default:
            return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {{LabelRole, "label"}, {CountRole, "count"}, {IconRole, "icon"}
                , {ValueRole, "value"}, {TypeRole, "type"}, {AnnounceFlagRole, "announceFlag"}};
    }

    [[nodiscard]] bool separateStatusFilter() const { return m_separate; }

    void setSeparateStatusFilter(bool value)
    {
        if (m_separate == value)
            return;
        m_separate = value;
        qCDebug(lcModel) << "TrackersFilterModel separateStatusFilter ->" << value;
        rebuild();
        emit separateStatusFilterChanged();
    }

signals:
    void separateStatusFilterChanged();

private:
    struct Row
    {
        int type = Host;
        QString host;
        int count = 0;
    };

    void subscribe()
    {
        BitTorrent::Session *const session = BitTorrent::Session::instance();
        if (!session)
        {
            qCWarning(lcModel) << "TrackersFilterModel: no Session instance";
            return;
        }
        const auto refresh = [this] { rebuild(); };
        connect(session, &BitTorrent::Session::torrentsLoaded, this, refresh);
        connect(session, &BitTorrent::Session::torrentAdded, this, [refresh](BitTorrent::Torrent *) { refresh(); });
        connect(session, &BitTorrent::Session::torrentAboutToBeRemoved, this, [refresh](BitTorrent::Torrent *) { refresh(); });
        connect(session, &BitTorrent::Session::trackersAdded, this, [refresh](BitTorrent::Torrent *, const QList<BitTorrent::TrackerEntry> &) { refresh(); });
        connect(session, &BitTorrent::Session::trackersRemoved, this, [refresh](BitTorrent::Torrent *, const QStringList &) { refresh(); });
        connect(session, &BitTorrent::Session::trackersReset, this, [refresh](BitTorrent::Torrent *, const QList<BitTorrent::TrackerEntryStatus> &, const QList<BitTorrent::TrackerEntry> &) { refresh(); });
        connect(session, &BitTorrent::Session::trackerEntryStatusesUpdated, this, [refresh](BitTorrent::Torrent *, const QHash<QString, BitTorrent::TrackerEntryStatus> &) { refresh(); });
    }

    [[nodiscard]] static QString labelFor(const Row &row)
    {
        switch (row.type)
        {
        case AllTrackers:        return tr("All");
        case Trackerless:        return tr("Trackerless");
        case StatusTrackerError: return tr("Tracker error");
        case StatusOtherError:   return tr("Other error");
        case StatusWarning:      return tr("Warning");
        default:                 return row.host;
        }
    }

    [[nodiscard]] static QString iconFor(const Row &row)
    {
        switch (row.type)
        {
        case AllTrackers:        return QStringLiteral("trackers");
        case Trackerless:        return QStringLiteral("trackerless");
        case StatusTrackerError: return QStringLiteral("tracker_error");
        case StatusOtherError:   return QStringLiteral("tracker_error");
        case StatusWarning:      return QStringLiteral("tracker_warning");
        default:                 return QStringLiteral("trackers");
        }
    }

    [[nodiscard]] static int announceFlagFor(const Row &row)
    {
        switch (row.type)
        {
        case StatusTrackerError: return static_cast<int>(BitTorrent::TorrentAnnounceStatusFlag::HasTrackerError);
        case StatusOtherError:   return static_cast<int>(BitTorrent::TorrentAnnounceStatusFlag::HasOtherError);
        case StatusWarning:      return static_cast<int>(BitTorrent::TorrentAnnounceStatusFlag::HasWarning);
        default:                 return -1;
        }
    }

    void rebuild()
    {
        BitTorrent::Session *const session = BitTorrent::Session::instance();
        const QList<BitTorrent::Torrent *> torrents = session ? session->torrents() : QList<BitTorrent::Torrent *> {};

        QList<Row> rows;
        rows.append({AllTrackers, QString(), static_cast<int>(torrents.size())});

        int trackerlessCount = 0;
        int trackerErrorCount = 0;
        int otherErrorCount = 0;
        int warningCount = 0;
        QMap<QString, int> hostCounts; // QMap keeps hosts naturally ordered

        using BitTorrent::TorrentAnnounceStatusFlag;
        for (const BitTorrent::Torrent *const t : torrents)
        {
            const QList<BitTorrent::TrackerEntryStatus> trackers = t->trackers();
            if (trackers.isEmpty())
                ++trackerlessCount;

            QSet<QString> seenHosts;
            for (const BitTorrent::TrackerEntryStatus &status : trackers)
            {
                const QString host = trackerHostFromURL(status.url);
                if (!seenHosts.contains(host))
                {
                    seenHosts.insert(host);
                    hostCounts[host] += 1;
                }
            }

            const BitTorrent::TorrentAnnounceStatus announce = t->announceStatus();
            if (announce & TorrentAnnounceStatusFlag::HasTrackerError) ++trackerErrorCount;
            if (announce & TorrentAnnounceStatusFlag::HasOtherError)   ++otherErrorCount;
            if (announce & TorrentAnnounceStatusFlag::HasWarning)      ++warningCount;
        }

        rows.append({Trackerless, QString(), trackerlessCount});

        if (!m_separate)
        {
            rows.append({StatusTrackerError, QString(), trackerErrorCount});
            rows.append({StatusOtherError, QString(), otherErrorCount});
            rows.append({StatusWarning, QString(), warningCount});
        }

        for (auto it = hostCounts.cbegin(); it != hostCounts.cend(); ++it)
            rows.append({Host, it.key(), it.value()});

        beginResetModel();
        m_rows = rows;
        endResetModel();
        qCDebug(lcModel) << "TrackersFilterModel rebuilt;" << m_rows.size() << "rows,"
                         << hostCounts.size() << "hosts";
    }

    QList<Row> m_rows;
    bool m_separate = false;
};
