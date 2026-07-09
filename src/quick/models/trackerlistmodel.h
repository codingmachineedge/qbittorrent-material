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

#include <memory>

#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QQmlEngine>
#include <QSortFilterProxyModel>
#include <QString>
#include <QStringList>

#include "base/bittorrent/announcetimepoint.h"

class QTimer;

namespace BitTorrent
{
    class Session;
    class Torrent;
    struct TrackerEntry;
    struct TrackerEntryStatus;
    struct TrackerEndpointStatus;
}

/**
 * @file trackerlistmodel.h
 * @brief The @c TrackerListModel — hierarchical model behind Properties → Trackers.
 *
 * A two-level @c QAbstractItemModel: top-level rows are the torrent's trackers
 * (plus three pinned "sticky" pseudo-rows for DHT / PeX / LSD), and each tracker's
 * announce endpoints appear as child rows. It subscribes to the granular
 * @c BitTorrent::Session tracker signals (added/removed/reset/statuses-updated) and
 * to @c fetchPeerInfo() for the DHT/PeX/LSD peer counts — never polling — while a
 * 4-second timer recomputes the relative "Next/Min Announce" countdowns.
 *
 * QML consumes it through a @c TreeView, reading @c display per cell and the
 * @c sortValue / @c sticky / @c endpoint helper roles; @ref TrackerListSortModel
 * keeps the sticky rows pinned regardless of the active sort column.
 */
class TrackerListModel final : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_DISABLE_COPY_MOVE(TrackerListModel)

public:
    /// Column order (mirrors the legacy tracker list).
    enum TrackerListColumn
    {
        COL_URL = 0,
        COL_TIER,
        COL_PROTOCOL,
        COL_STATUS,
        COL_PEERS,
        COL_SEEDS,
        COL_LEECHES,
        COL_TIMES_DOWNLOADED,
        COL_MSG,
        COL_NEXT_ANNOUNCE,
        COL_MIN_ANNOUNCE,

        COL_COUNT
    };
    Q_ENUM(TrackerListColumn)

    /// The three always-present pinned pseudo-trackers.
    enum StickyRow
    {
        ROW_DHT = 0,
        ROW_PEX = 1,
        ROW_LSD = 2,

        STICKY_ROW_COUNT
    };

    /// Named roles exposed to QML (plus @c Qt::DisplayRole == "display").
    enum Roles
    {
        SortRole = Qt::UserRole + 1, // "sortValue"
        IsStickyRole,                // "sticky"     (top-level DHT/PeX/LSD row)
        IsEndpointRole,              // "endpoint"   (announce-endpoint child row)
        AlignRightRole               // "alignRight" (numeric column hint)
    };
    Q_ENUM(Roles)

    explicit TrackerListModel(QObject *parent = nullptr);
    ~TrackerListModel() override;

    /// Bind the model to a torrent (nullptr clears it).
    void setTorrent(BitTorrent::Torrent *torrent);
    [[nodiscard]] BitTorrent::Torrent *torrent() const;

    // --- QAbstractItemModel ---
    int columnCount(const QModelIndex &parent = {}) const override;
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;
    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override;
    QModelIndex parent(const QModelIndex &index) const override;

private:
    struct Item;

    void populate();
    std::shared_ptr<Item> createTrackerItem(const BitTorrent::TrackerEntryStatus &status);
    void addTrackerItem(const BitTorrent::TrackerEntryStatus &status);
    void updateTrackerItem(const std::shared_ptr<Item> &item, const BitTorrent::TrackerEntryStatus &status);
    void refreshAnnounceTimes();
    void refreshStickyPeerCounts();

    void onTrackersAdded(const QList<BitTorrent::TrackerEntry> &newTrackers);
    void onTrackersRemoved(const QStringList &deletedTrackers);
    void onTrackersChanged();
    void onTrackersUpdated(const QHash<QString, BitTorrent::TrackerEntryStatus> &updatedTrackers);

    [[nodiscard]] int rowOfTopItem(const Item *item) const;
    [[nodiscard]] std::shared_ptr<Item> findTopItem(const QString &name) const;

    BitTorrent::Session *m_btSession = nullptr;
    BitTorrent::Torrent *m_torrent = nullptr;

    QList<std::shared_ptr<Item>> m_items; ///< top-level rows (sticky first, then trackers)

    BitTorrent::AnnounceTimePoint m_announceTimestamp;
    QTimer *m_announceRefreshTimer = nullptr;
};

/**
 * @brief Sort/filter proxy for @c TrackerListModel that keeps the DHT/PeX/LSD
 *        pseudo-rows pinned to the top regardless of the active sort column.
 */
class TrackerListSortModel final : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    explicit TrackerListSortModel(QObject *parent = nullptr);

protected:
    bool lessThan(const QModelIndex &left, const QModelIndex &right) const override;
};
