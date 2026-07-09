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
#include <QHash>
#include <QList>
#include <QVariant>

#include <qqmlintegration.h>

#include "base/bittorrent/torrent.h"

class QQmlEngine;
class QJSEngine;

/**
 * @file transferlistmodel.h
 * @brief The @c TransferListModel QML singleton — the flat, one-row-per-torrent
 *        source model behind the transfer list table.
 *
 * One @c QAbstractListModel row per @c BitTorrent::Torrent. Every visible column
 * of the legacy transfer list is exposed as a named role (see @ref Roles) whose
 * @c data() value is the *formatted, translated* display string. Raw comparable
 * values used for sorting are provided to @c TorrentFilterProxyModel through the
 * C++ helper @ref rawValue (the proxy owns the sort policy, keeping this model a
 * pure presentation surface).
 *
 * The model never polls: it subscribes to @c BitTorrent::Session signals
 * (`torrentsLoaded`, `torrentsUpdated`, `torrentAdded`, `torrentAboutToBeRemoved`,
 * …) and emits `dataChanged` / row insert-remove in reaction.
 *
 * Registered as a QML **singleton** so every view shares one instance
 * (`TransferListModel` by name; proxies wrap it via `sourceModel:`).
 */
class TransferListModel final : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_DISABLE_COPY_MOVE(TransferListModel)

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    /// Column enumeration — the default logical column order of the table.
    /// Numeric order preserved from legacy qBittorrent for config compatibility.
    enum Column
    {
        TR_QUEUE_POSITION = 0,
        TR_NAME,
        TR_SIZE,
        TR_TOTAL_SIZE,
        TR_PROGRESS,
        TR_STATUS,
        TR_SEEDS,
        TR_PEERS,
        TR_DLSPEED,
        TR_UPSPEED,
        TR_ETA,
        TR_RATIO,
        TR_POPULARITY,
        TR_CATEGORY,
        TR_TAGS,
        TR_ADD_DATE,
        TR_SEED_DATE,
        TR_TRACKER,
        TR_DLLIMIT,
        TR_UPLIMIT,
        TR_AMOUNT_DOWNLOADED,
        TR_AMOUNT_UPLOADED,
        TR_AMOUNT_DOWNLOADED_SESSION,
        TR_AMOUNT_UPLOADED_SESSION,
        TR_AMOUNT_LEFT,
        TR_TIME_ELAPSED,
        TR_SAVE_PATH,
        TR_COMPLETED,
        TR_RATIO_LIMIT,
        TR_SEEN_COMPLETE_DATE,
        TR_LAST_ACTIVITY,
        TR_AVAILABILITY,
        TR_DOWNLOAD_PATH,
        TR_INFOHASH_V1,
        TR_INFOHASH_V2,
        TR_REANNOUNCE,
        TR_PRIVATE,
        TR_CREATE_DATE,

        NB_COLUMNS
    };
    Q_ENUM(Column)

    /// Named roles. One display role per column plus a handful of raw/underlying
    /// roles consumed by the proxy, delegates and controllers.
    enum Roles
    {
        // --- one display role per column ---
        QueuePositionRole = Qt::UserRole + 1, // "queuePosition"
        NameRole,                             // "name"
        SizeRole,                             // "size"
        TotalSizeRole,                        // "totalSize"
        ProgressRole,                         // "progress"   (real 0..1 for the bar)
        StatusRole,                           // "status"     (formatted status text)
        SeedsRole,                            // "seeds"
        PeersRole,                            // "peers"
        DownSpeedRole,                        // "downSpeed"
        UpSpeedRole,                          // "upSpeed"
        EtaRole,                              // "eta"
        RatioRole,                            // "ratio"
        PopularityRole,                       // "popularity"
        CategoryRole,                         // "category"
        TagsRole,                             // "tags"
        AddedOnRole,                          // "addedOn"
        CompletedOnRole,                      // "completedOn"
        TrackerRole,                          // "tracker"
        DownLimitRole,                        // "downLimit"
        UpLimitRole,                          // "upLimit"
        DownloadedRole,                       // "downloaded"
        UploadedRole,                         // "uploaded"
        SessionDownloadedRole,                // "sessionDownloaded"
        SessionUploadedRole,                  // "sessionUploaded"
        RemainingRole,                        // "remaining"
        TimeActiveRole,                       // "timeActive"
        SavePathRole,                         // "savePath"
        CompletedRole,                        // "completed"
        RatioLimitRole,                       // "ratioLimit"
        LastSeenCompleteRole,                 // "lastSeenComplete"
        LastActivityRole,                     // "lastActivity"
        AvailabilityRole,                     // "availability"
        DownloadPathRole,                     // "downloadPath"
        InfoHashV1Role,                       // "infoHashV1"
        InfoHashV2Role,                       // "infoHashV2"
        ReannounceRole,                       // "reannounce"
        PrivateRole,                          // "private"
        CreatedOnRole,                        // "createdOn"

        // --- underlying / auxiliary roles ---
        StateRole,                            // "state"       (int TorrentState — color/icon channel)
        IdRole,                               // "id"          (TorrentID hex string)
        UnderlyingRole,                       // "underlying"  (the Torrent* as QObject*)
        AdditionalUnderlyingRole              // "additionalUnderlying"
    };
    Q_ENUM(Roles)

    /// QML singleton factory — returns the shared app-owned instance.
    static TransferListModel *create(QQmlEngine *engine, QJSEngine *jsEngine);
    /// The one shared instance (also usable from pure C++).
    static TransferListModel *instance();

    // --- QAbstractListModel ---
    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;
    QHash<int, QByteArray> roleNames() const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // --- helpers for the proxy / views ---
    /// The torrent backing @p row, or nullptr when out of range.
    [[nodiscard]] BitTorrent::Torrent *torrentAtRow(int row) const;
    /// The row hosting @p torrent, or -1 when unknown.
    [[nodiscard]] int rowOf(const BitTorrent::Torrent *torrent) const;
    /// Raw, comparable value for @p column (used by the sort proxy). When @p alt
    /// is true the secondary sort key is returned (e.g. total seeds vs. seeds).
    [[nodiscard]] QVariant rawValue(const BitTorrent::Torrent *torrent, int column, bool alt = false) const;
    /// Column -> its display role id.
    [[nodiscard]] static int roleForColumn(int column);
    /// Translated header text for @p column.
    [[nodiscard]] Q_INVOKABLE QString headerText(int column) const;
    /// TorrentID hex string of @p row (convenience for QML selection wiring).
    [[nodiscard]] Q_INVOKABLE QString idAtRow(int row) const;

signals:
    void countChanged();

private slots:
    void onTorrentsLoaded(const QList<BitTorrent::Torrent *> &torrents);
    void onTorrentAdded(BitTorrent::Torrent *torrent);
    void onTorrentAboutToBeRemoved(BitTorrent::Torrent *torrent);
    void onTorrentsUpdated(const QList<BitTorrent::Torrent *> &torrents);
    void onPreferencesChanged();

private:
    explicit TransferListModel(QObject *parent = nullptr);

    void connectToSession();
    void configure();
    void addTorrentRow(BitTorrent::Torrent *torrent);

    [[nodiscard]] QString displayValue(const BitTorrent::Torrent *torrent, int column) const;
    /// Translated status string for a torrent state (TR_STATUS text).
    [[nodiscard]] QString statusText(BitTorrent::TorrentState state) const;
    /// Whether zero/invalid values in @p column should render as an empty string
    /// under the currently configured hide-zero mode.
    [[nodiscard]] bool shouldHideZero(const BitTorrent::Torrent *torrent) const;

    /// How zero/invalid numeric cells are shown.
    enum class HideZeroMode
    {
        Never,
        Stopped,
        Always
    };

    QList<BitTorrent::Torrent *> m_torrents;         ///< row order == list order
    QHash<const BitTorrent::Torrent *, int> m_rowByTorrent;

    HideZeroMode m_hideZeroMode = HideZeroMode::Never;

    static TransferListModel *m_instance;
};
