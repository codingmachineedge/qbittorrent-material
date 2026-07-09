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

#include "trackerlistmodel.h"

#include <algorithm>
#include <chrono>

#include <QFuture>
#include <QFutureWatcher>
#include <QPointer>
#include <QTimer>

#include "base/bittorrent/peerinfo.h"
#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/bittorrent/trackerentry.h"
#include "base/bittorrent/trackerentrystatus.h"
#include "base/logging.h"
#include "base/utils/misc.h"

using namespace std::chrono_literals;

namespace
{
    const std::chrono::milliseconds ANNOUNCE_TIME_REFRESH_INTERVAL = 4s;

    QString prettyCount(const int val)
    {
        return (val > -1) ? QString::number(val) : TrackerListModel::tr("N/A");
    }

    QString statusDHT(const BitTorrent::Torrent *torrent)
    {
        if (!torrent->session()->isDHTEnabled())
            return TrackerListModel::tr("Disabled");
        if (torrent->isPrivate() || torrent->isDHTDisabled())
            return TrackerListModel::tr("Disabled for this torrent");
        return TrackerListModel::tr("Working");
    }

    QString statusPeX(const BitTorrent::Torrent *torrent)
    {
        if (!torrent->session()->isPeXEnabled())
            return TrackerListModel::tr("Disabled");
        if (torrent->isPrivate() || torrent->isPEXDisabled())
            return TrackerListModel::tr("Disabled for this torrent");
        return TrackerListModel::tr("Working");
    }

    QString statusLSD(const BitTorrent::Torrent *torrent)
    {
        if (!torrent->session()->isLSDEnabled())
            return TrackerListModel::tr("Disabled");
        if (torrent->isPrivate() || torrent->isLSDDisabled())
            return TrackerListModel::tr("Disabled for this torrent");
        return TrackerListModel::tr("Working");
    }

    bool isNumericColumn(const int column)
    {
        switch (column)
        {
        case TrackerListModel::COL_TIER:
        case TrackerListModel::COL_PEERS:
        case TrackerListModel::COL_SEEDS:
        case TrackerListModel::COL_LEECHES:
        case TrackerListModel::COL_TIMES_DOWNLOADED:
        case TrackerListModel::COL_NEXT_ANNOUNCE:
        case TrackerListModel::COL_MIN_ANNOUNCE:
            return true;
        default:
            return false;
        }
    }
}

/// One node of the tracker tree — either a tracker (top-level) or an announce
/// endpoint (child). Endpoints carry a valid @c parentItem.
struct TrackerListModel::Item final
{
    QString name;
    int tier = -1;
    int btVersion = -1;
    bool isUpdating = false;
    BitTorrent::TrackerEndpointState status = BitTorrent::TrackerEndpointState::NotContacted;
    QString message;

    int numPeers = -1;
    int numSeeds = -1;
    int numLeeches = -1;
    int numDownloaded = -1;

    BitTorrent::AnnounceTimePoint nextAnnounceTime;
    BitTorrent::AnnounceTimePoint minAnnounceTime;

    qint64 secsToNextAnnounce = 0;
    qint64 secsToMinAnnounce = 0;
    BitTorrent::AnnounceTimePoint announceTimestamp;

    std::weak_ptr<Item> parentItem;
    QList<std::shared_ptr<Item>> childItems;

    Item(const QString &name, const QString &message)
        : name {name}
        , message {message}
    {
    }

    explicit Item(const BitTorrent::TrackerEntryStatus &status)
        : name {status.url}
    {
        fillFrom(status);
    }

    Item(const std::shared_ptr<Item> &parent, const BitTorrent::TrackerEndpointStatus &endpoint)
        : name {endpoint.name}
        , btVersion {endpoint.btVersion}
        , parentItem {parent}
    {
        fillFrom(endpoint);
    }

    void fillFrom(const BitTorrent::TrackerEntryStatus &status)
    {
        tier = status.tier;
        isUpdating = status.isUpdating;
        this->status = status.state;
        message = status.message;
        numPeers = status.numPeers;
        numSeeds = status.numSeeds;
        numLeeches = status.numLeeches;
        numDownloaded = status.numDownloaded;
        nextAnnounceTime = status.nextAnnounceTime;
        minAnnounceTime = status.minAnnounceTime;
        secsToNextAnnounce = 0;
        secsToMinAnnounce = 0;
        announceTimestamp = {};
    }

    void fillFrom(const BitTorrent::TrackerEndpointStatus &endpoint)
    {
        isUpdating = endpoint.isUpdating;
        status = endpoint.state;
        message = endpoint.message;
        numPeers = endpoint.numPeers;
        numSeeds = endpoint.numSeeds;
        numLeeches = endpoint.numLeeches;
        numDownloaded = endpoint.numDownloaded;
        nextAnnounceTime = endpoint.nextAnnounceTime;
        minAnnounceTime = endpoint.minAnnounceTime;
        secsToNextAnnounce = 0;
        secsToMinAnnounce = 0;
        announceTimestamp = {};
    }

    [[nodiscard]] QString statusText() const
    {
        if (isUpdating)
            return TrackerListModel::tr("Updating...");

        switch (status)
        {
        case BitTorrent::TrackerEndpointState::Working:
            return TrackerListModel::tr("Working");
        case BitTorrent::TrackerEndpointState::NotWorking:
            return TrackerListModel::tr("Not working");
        case BitTorrent::TrackerEndpointState::TrackerError:
            return TrackerListModel::tr("Tracker error");
        case BitTorrent::TrackerEndpointState::Unreachable:
            return TrackerListModel::tr("Unreachable");
        case BitTorrent::TrackerEndpointState::NotContacted:
            return TrackerListModel::tr("Not contacted yet");
        }
        return TrackerListModel::tr("Invalid state!");
    }
};

TrackerListModel::TrackerListModel(QObject *parent)
    : QAbstractItemModel(parent)
    , m_btSession {BitTorrent::Session::instance()}
    , m_announceRefreshTimer {new QTimer(this)}
{
    m_announceRefreshTimer->setSingleShot(true);
    connect(m_announceRefreshTimer, &QTimer::timeout, this, &TrackerListModel::refreshAnnounceTimes);

    connect(m_btSession, &BitTorrent::Session::trackersAdded, this
            , [this](BitTorrent::Torrent *torrent, const QList<BitTorrent::TrackerEntry> &newTrackers)
    {
        if (torrent == m_torrent)
            onTrackersAdded(newTrackers);
    });
    connect(m_btSession, &BitTorrent::Session::trackersRemoved, this
            , [this](BitTorrent::Torrent *torrent, const QStringList &deletedTrackers)
    {
        if (torrent == m_torrent)
            onTrackersRemoved(deletedTrackers);
    });
    connect(m_btSession, &BitTorrent::Session::trackersReset, this
            , [this](BitTorrent::Torrent *torrent, const QList<BitTorrent::TrackerEntryStatus> &, const QList<BitTorrent::TrackerEntry> &)
    {
        if (torrent == m_torrent)
            onTrackersChanged();
    });
    connect(m_btSession, &BitTorrent::Session::trackerEntryStatusesUpdated, this
            , [this](BitTorrent::Torrent *torrent, const QHash<QString, BitTorrent::TrackerEntryStatus> &updatedTrackers)
    {
        if (torrent == m_torrent)
            onTrackersUpdated(updatedTrackers);
    });

    qCDebug(lcModel) << "TrackerListModel constructed";
}

TrackerListModel::~TrackerListModel() = default;

void TrackerListModel::setTorrent(BitTorrent::Torrent *torrent)
{
    beginResetModel();
    m_items.clear();
    m_torrent = torrent;
    if (m_torrent)
    {
        populate();
    }
    else
    {
        m_announceRefreshTimer->stop();
    }
    endResetModel();

    qCDebug(lcModel) << "TrackerListModel torrent ->" << (torrent ? torrent->name() : QStringLiteral("<none>"));
}

BitTorrent::Torrent *TrackerListModel::torrent() const
{
    return m_torrent;
}

void TrackerListModel::populate()
{
    Q_ASSERT(m_torrent);

    const QList<BitTorrent::TrackerEntryStatus> trackers = m_torrent->trackers();
    m_items.reserve(trackers.size() + STICKY_ROW_COUNT);

    const QString privateMessage = m_torrent->isPrivate() ? tr("This torrent is private") : QString();
    m_items.append(std::make_shared<Item>(QStringLiteral("** [DHT] **"), privateMessage));
    m_items.append(std::make_shared<Item>(QStringLiteral("** [PeX] **"), privateMessage));
    m_items.append(std::make_shared<Item>(QStringLiteral("** [LSD] **"), privateMessage));

    for (const BitTorrent::TrackerEntryStatus &status : trackers)
        addTrackerItem(status);

    refreshStickyPeerCounts();

    m_announceTimestamp = BitTorrent::AnnounceTimePoint::clock::now();
    m_announceRefreshTimer->start(ANNOUNCE_TIME_REFRESH_INTERVAL);
}

void TrackerListModel::refreshStickyPeerCounts()
{
    if (!m_torrent)
        return;

    auto *watcher = new QFutureWatcher<QList<BitTorrent::PeerInfo>>(this);
    connect(watcher, &QFutureWatcherBase::finished, this
            , [this, watcher, torrent = QPointer<BitTorrent::Torrent>(m_torrent)]
    {
        watcher->deleteLater();
        if (!m_torrent || (m_torrent != torrent) || (m_items.size() < STICKY_ROW_COUNT))
            return;

        int seedsDHT = 0, seedsPeX = 0, seedsLSD = 0;
        int peersDHT = 0, peersPeX = 0, peersLSD = 0;
        for (const BitTorrent::PeerInfo &peer : watcher->result())
        {
            if (peer.isConnecting())
                continue;

            if (peer.isSeed())
            {
                if (peer.fromDHT()) ++seedsDHT;
                if (peer.fromPeX()) ++seedsPeX;
                if (peer.fromLSD()) ++seedsLSD;
            }
            else
            {
                if (peer.fromDHT()) ++peersDHT;
                if (peer.fromPeX()) ++peersPeX;
                if (peer.fromLSD()) ++peersLSD;
            }
        }

        m_items[ROW_DHT]->numSeeds = seedsDHT;
        m_items[ROW_DHT]->numLeeches = peersDHT;
        m_items[ROW_PEX]->numSeeds = seedsPeX;
        m_items[ROW_PEX]->numLeeches = peersPeX;
        m_items[ROW_LSD]->numSeeds = seedsLSD;
        m_items[ROW_LSD]->numLeeches = peersLSD;

        emit dataChanged(index(ROW_DHT, COL_SEEDS), index(ROW_LSD, COL_LEECHES));
    });
    watcher->setFuture(m_torrent->fetchPeerInfo());
}

std::shared_ptr<TrackerListModel::Item> TrackerListModel::createTrackerItem(const BitTorrent::TrackerEntryStatus &status)
{
    auto item = std::make_shared<Item>(status);
    item->childItems.reserve(status.endpoints.size());
    for (const auto &endpoint : status.endpoints)
        item->childItems.append(std::make_shared<Item>(item, endpoint));
    return item;
}

void TrackerListModel::addTrackerItem(const BitTorrent::TrackerEntryStatus &status)
{
    m_items.append(createTrackerItem(status));
}

void TrackerListModel::updateTrackerItem(const std::shared_ptr<Item> &item, const BitTorrent::TrackerEntryStatus &status)
{
    const int trackerRow = rowOfTopItem(item.get());
    if (trackerRow < 0)
        return;
    const QModelIndex trackerIndex = index(trackerRow, 0);

    // Gather the incoming endpoint IDs and update / mark existing ones.
    QSet<std::pair<QString, int>> incomingIDs;
    QList<std::shared_ptr<Item>> newEndpoints;
    for (const auto &endpoint : status.endpoints)
    {
        const std::pair id {endpoint.name, endpoint.btVersion};
        incomingIDs.insert(id);

        const auto it = std::find_if(item->childItems.cbegin(), item->childItems.cend()
                , [&id](const std::shared_ptr<Item> &child)
        {
            return (child->name == id.first) && (child->btVersion == id.second);
        });
        if (it != item->childItems.cend())
            (*it)->fillFrom(endpoint);
        else
            newEndpoints.append(std::make_shared<Item>(item, endpoint));
    }

    // Remove endpoints that disappeared.
    int row = 0;
    while (row < item->childItems.size())
    {
        const auto &child = item->childItems.at(row);
        if (incomingIDs.contains({child->name, child->btVersion}))
        {
            ++row;
        }
        else
        {
            beginRemoveRows(trackerIndex, row, row);
            item->childItems.removeAt(row);
            endRemoveRows();
        }
    }

    if (const int rows = item->childItems.size(); rows > 0)
        emit dataChanged(index(0, 0, trackerIndex), index((rows - 1), (COL_COUNT - 1), trackerIndex));

    if (!newEndpoints.isEmpty())
    {
        const int first = item->childItems.size();
        beginInsertRows(trackerIndex, first, (first + newEndpoints.size() - 1));
        item->childItems.append(newEndpoints);
        endInsertRows();
    }

    item->fillFrom(status);
    emit dataChanged(trackerIndex, index(trackerRow, (COL_COUNT - 1)));
}

void TrackerListModel::refreshAnnounceTimes()
{
    if (!m_torrent)
        return;

    m_announceTimestamp = BitTorrent::AnnounceTimePoint::clock::now();
    emit dataChanged(index(0, COL_NEXT_ANNOUNCE), index((rowCount() - 1), COL_MIN_ANNOUNCE));
    for (int i = 0; i < rowCount(); ++i)
    {
        const QModelIndex parentIndex = index(i, 0);
        if (const int children = rowCount(parentIndex); children > 0)
            emit dataChanged(index(0, COL_NEXT_ANNOUNCE, parentIndex), index((children - 1), COL_MIN_ANNOUNCE, parentIndex));
    }

    m_announceRefreshTimer->start(ANNOUNCE_TIME_REFRESH_INTERVAL);
}

int TrackerListModel::rowOfTopItem(const Item *item) const
{
    for (int i = 0; i < m_items.size(); ++i)
    {
        if (m_items.at(i).get() == item)
            return i;
    }
    return -1;
}

std::shared_ptr<TrackerListModel::Item> TrackerListModel::findTopItem(const QString &name) const
{
    for (const auto &item : m_items)
    {
        if (item->name == name)
            return item;
    }
    return {};
}

int TrackerListModel::columnCount(const QModelIndex &) const
{
    return COL_COUNT;
}

int TrackerListModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
        return static_cast<int>(m_items.size());

    const auto *item = static_cast<Item *>(parent.internalPointer());
    return item ? static_cast<int>(item->childItems.size()) : 0;
}

QVariant TrackerListModel::headerData(const int section, const Qt::Orientation orientation, const int role) const
{
    if (orientation != Qt::Horizontal)
        return {};

    if (role == Qt::DisplayRole)
    {
        switch (section)
        {
        case COL_URL:              return tr("URL/Announce Endpoint");
        case COL_TIER:             return tr("Tier");
        case COL_PROTOCOL:         return tr("BT Protocol");
        case COL_STATUS:           return tr("Status");
        case COL_PEERS:            return tr("Peers");
        case COL_SEEDS:            return tr("Seeds");
        case COL_LEECHES:          return tr("Leeches");
        case COL_TIMES_DOWNLOADED: return tr("Times Downloaded");
        case COL_MSG:              return tr("Message");
        case COL_NEXT_ANNOUNCE:    return tr("Next Announce");
        case COL_MIN_ANNOUNCE:     return tr("Min Announce");
        default:                   return {};
        }
    }

    if (role == AlignRightRole)
        return isNumericColumn(section);

    return {};
}

QVariant TrackerListModel::data(const QModelIndex &index, const int role) const
{
    if (!index.isValid())
        return {};

    auto *item = static_cast<Item *>(index.internalPointer());
    if (!item)
        return {};

    // Lazily recompute the relative announce countdowns against the last tick.
    if (item->announceTimestamp != m_announceTimestamp)
    {
        const auto toNext = std::chrono::duration_cast<std::chrono::seconds>(item->nextAnnounceTime - m_announceTimestamp);
        item->secsToNextAnnounce = std::max<qint64>(0, toNext.count());
        const auto toMin = std::chrono::duration_cast<std::chrono::seconds>(item->minAnnounceTime - m_announceTimestamp);
        item->secsToMinAnnounce = std::max<qint64>(0, toMin.count());
        item->announceTimestamp = m_announceTimestamp;
    }

    const bool isEndpoint = !item->parentItem.expired();
    const bool isSticky = !isEndpoint && (index.row() < STICKY_ROW_COUNT);

    switch (role)
    {
    case IsStickyRole:
        return isSticky;
    case IsEndpointRole:
        return isEndpoint;
    case AlignRightRole:
        return isNumericColumn(index.column());

    case Qt::DisplayRole:
        switch (index.column())
        {
        case COL_URL:
            return item->name;
        case COL_TIER:
            return (isEndpoint || isSticky) ? QString() : QString::number(item->tier);
        case COL_PROTOCOL:
            return isEndpoint ? (u'v' + QString::number(item->btVersion)) : QString();
        case COL_STATUS:
            if (isEndpoint)
                return item->statusText();
            if (index.row() == ROW_DHT)
                return statusDHT(m_torrent);
            if (index.row() == ROW_PEX)
                return statusPeX(m_torrent);
            if (index.row() == ROW_LSD)
                return statusLSD(m_torrent);
            return item->statusText();
        case COL_PEERS:
            return prettyCount(item->numPeers);
        case COL_SEEDS:
            return prettyCount(item->numSeeds);
        case COL_LEECHES:
            return prettyCount(item->numLeeches);
        case COL_TIMES_DOWNLOADED:
            return prettyCount(item->numDownloaded);
        case COL_MSG:
            return item->message;
        case COL_NEXT_ANNOUNCE:
            return Utils::Misc::userFriendlyDuration(item->secsToNextAnnounce, -1, Utils::Misc::TimeResolution::Seconds);
        case COL_MIN_ANNOUNCE:
            return Utils::Misc::userFriendlyDuration(item->secsToMinAnnounce, -1, Utils::Misc::TimeResolution::Seconds);
        default:
            return {};
        }

    case SortRole:
        switch (index.column())
        {
        case COL_URL:              return item->name;
        case COL_TIER:             return isEndpoint ? -1 : item->tier;
        case COL_PROTOCOL:         return isEndpoint ? item->btVersion : -1;
        case COL_STATUS:           return item->statusText();
        case COL_PEERS:            return item->numPeers;
        case COL_SEEDS:            return item->numSeeds;
        case COL_LEECHES:          return item->numLeeches;
        case COL_TIMES_DOWNLOADED: return item->numDownloaded;
        case COL_MSG:              return item->message;
        case COL_NEXT_ANNOUNCE:    return static_cast<qlonglong>(item->secsToNextAnnounce);
        case COL_MIN_ANNOUNCE:     return static_cast<qlonglong>(item->secsToMinAnnounce);
        default:                   return {};
        }

    default:
        return {};
    }
}

QHash<int, QByteArray> TrackerListModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {SortRole, "sortValue"},
        {IsStickyRole, "sticky"},
        {IsEndpointRole, "endpoint"},
        {AlignRightRole, "alignRight"}
    };
}

QModelIndex TrackerListModel::index(const int row, const int column, const QModelIndex &parent) const
{
    if ((column < 0) || (column >= COL_COUNT) || (row < 0))
        return {};

    if (parent.isValid())
    {
        const auto *parentItem = static_cast<Item *>(parent.internalPointer());
        if (!parentItem || (row >= parentItem->childItems.size()))
            return {};
        return createIndex(row, column, parentItem->childItems.at(row).get());
    }

    if (row >= m_items.size())
        return {};
    return createIndex(row, column, m_items.at(row).get());
}

QModelIndex TrackerListModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return {};

    const auto *item = static_cast<Item *>(index.internalPointer());
    if (!item)
        return {};

    const std::shared_ptr<Item> parentItem = item->parentItem.lock();
    if (!parentItem)
        return {};

    const int row = rowOfTopItem(parentItem.get());
    if (row < 0)
        return {};

    // Convention: only column 0 carries children.
    return createIndex(row, 0, parentItem.get());
}

void TrackerListModel::onTrackersAdded(const QList<BitTorrent::TrackerEntry> &newTrackers)
{
    if (newTrackers.isEmpty())
        return;

    const int first = rowCount();
    beginInsertRows({}, first, (first + newTrackers.size() - 1));
    for (const BitTorrent::TrackerEntry &entry : newTrackers)
        addTrackerItem({entry.url, entry.tier});
    endInsertRows();

    qCDebug(lcModel) << "TrackerListModel: added" << newTrackers.size() << "tracker(s)";
}

void TrackerListModel::onTrackersRemoved(const QStringList &deletedTrackers)
{
    for (const QString &url : deletedTrackers)
    {
        for (int row = STICKY_ROW_COUNT; row < m_items.size(); ++row)
        {
            if (m_items.at(row)->name == url)
            {
                beginRemoveRows({}, row, row);
                m_items.removeAt(row);
                endRemoveRows();
                break;
            }
        }
    }

    qCDebug(lcModel) << "TrackerListModel: removed" << deletedTrackers.size() << "tracker(s)";
}

void TrackerListModel::onTrackersChanged()
{
    QSet<QString> keptNames;
    for (int i = 0; i < STICKY_ROW_COUNT; ++i)
        keptNames.insert(m_items.at(i)->name);

    QList<std::shared_ptr<Item>> newTrackerItems;
    for (const BitTorrent::TrackerEntryStatus &status : m_torrent->trackers())
    {
        keptNames.insert(status.url);
        if (const std::shared_ptr<Item> existing = findTopItem(status.url))
            updateTrackerItem(existing, status);
        else
            newTrackerItems.append(createTrackerItem(status));
    }

    // Remove trackers no longer present.
    int row = STICKY_ROW_COUNT;
    while (row < m_items.size())
    {
        if (keptNames.contains(m_items.at(row)->name))
        {
            ++row;
        }
        else
        {
            beginRemoveRows({}, row, row);
            m_items.removeAt(row);
            endRemoveRows();
        }
    }

    if (!newTrackerItems.isEmpty())
    {
        const int first = rowCount();
        beginInsertRows({}, first, (first + newTrackerItems.size() - 1));
        m_items.append(newTrackerItems);
        endInsertRows();
    }
}

void TrackerListModel::onTrackersUpdated(const QHash<QString, BitTorrent::TrackerEntryStatus> &updatedTrackers)
{
    for (const auto &[url, status] : updatedTrackers.asKeyValueRange())
    {
        if (const std::shared_ptr<Item> existing = findTopItem(url))
            updateTrackerItem(existing, status);
    }
}

// --- TrackerListSortModel ---------------------------------------------------

TrackerListSortModel::TrackerListSortModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    setDynamicSortFilter(true);
    setSortCaseSensitivity(Qt::CaseInsensitive);
    setSortRole(TrackerListModel::SortRole);
}

bool TrackerListSortModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    // Keep the DHT/PeX/LSD sticky rows pinned at the top of the root level,
    // preserving their fixed order regardless of the active sort column.
    if (!left.parent().isValid() && !right.parent().isValid())
    {
        if ((left.row() < TrackerListModel::STICKY_ROW_COUNT) || (right.row() < TrackerListModel::STICKY_ROW_COUNT))
            return (left.row() < right.row()) && (sortOrder() == Qt::AscendingOrder);
    }

    return QSortFilterProxyModel::lessThan(left, right);
}
