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

#include "transferlistmodel.h"

#include <QDateTime>
#include <QLocale>
#include <QMetaEnum>

#include "base/bittorrent/infohash.h"
#include "base/bittorrent/session.h"
#include "base/bittorrent/sharelimits.h"
#include "base/logging.h"
#include "base/preferences.h"
#include "base/unicodestrings.h"
#include "base/utils/misc.h"
#include "base/utils/string.h"

using namespace BitTorrent;
using namespace Qt::StringLiterals;

namespace
{
    // 100 days, in seconds — ETA at/above this is treated as "infinite".
    constexpr qlonglong MAX_ETA = 8640000;

    QString formatDuration(qlonglong seconds, qlonglong maxCap = -1)
    {
        return Utils::Misc::userFriendlyDuration(seconds, maxCap);
    }

    QString formatSize(qlonglong bytes)
    {
        return Utils::Misc::friendlyUnit(bytes);
    }

    QString formatSpeed(qlonglong bytesPerSec)
    {
        return Utils::Misc::friendlyUnit(bytesPerSec, true);
    }

    QString formatRatio(qreal ratio)
    {
        if ((ratio < 0) || (ratio > Torrent::MAX_RATIO))
            return C_INFINITY;
        return Utils::String::fromDouble(ratio, 2);
    }

    QString formatDateTime(const QDateTime &dt)
    {
        return dt.isValid() ? QLocale().toString(dt, QLocale::ShortFormat) : QString();
    }
}

TransferListModel *TransferListModel::m_instance = nullptr;

TransferListModel::TransferListModel(QObject *parent)
    : QAbstractListModel(parent)
{
    qCDebug(lcModel) << "TransferListModel created";
    configure();
    connectToSession();

    if (Preferences *const pref = Preferences::instance())
        connect(pref, &Preferences::changed, this, &TransferListModel::onPreferencesChanged);
}

TransferListModel *TransferListModel::create(QQmlEngine *, QJSEngine *)
{
    qCInfo(lcModel) << "TransferListModel QML singleton requested";
    return instance();
}

TransferListModel *TransferListModel::instance()
{
    if (!m_instance)
        m_instance = new TransferListModel;
    return m_instance;
}

void TransferListModel::connectToSession()
{
    Session *const session = Session::instance();
    if (!session)
    {
        qCWarning(lcModel) << "TransferListModel: no Session instance to subscribe to";
        return;
    }

    connect(session, &Session::torrentsLoaded, this, &TransferListModel::onTorrentsLoaded);
    connect(session, &Session::torrentAdded, this, &TransferListModel::onTorrentAdded);
    connect(session, &Session::torrentAboutToBeRemoved, this, &TransferListModel::onTorrentAboutToBeRemoved);
    connect(session, &Session::torrentsUpdated, this, &TransferListModel::onTorrentsUpdated);

    // Prime the model with whatever is already loaded.
    const QList<Torrent *> existing = session->torrents();
    if (!existing.isEmpty())
        onTorrentsLoaded(existing);

    qCDebug(lcModel) << "TransferListModel subscribed to Session signals; primed with"
                     << existing.size() << "torrent(s)";
}

void TransferListModel::configure()
{
    const Preferences *const pref = Preferences::instance();
    if (!pref)
        return;

    HideZeroMode mode = HideZeroMode::Never;
    if (pref->getHideZeroValues())
        mode = (pref->getHideZeroComboValues() == 1) ? HideZeroMode::Stopped : HideZeroMode::Always;

    if (mode != m_hideZeroMode)
    {
        m_hideZeroMode = mode;
        qCDebug(lcModel) << "TransferListModel hide-zero mode ->" << static_cast<int>(mode);
    }
}

void TransferListModel::onPreferencesChanged()
{
    configure();
    if (!m_torrents.isEmpty())
    {
        // Formatting policy may have changed — refresh every visible cell.
        emit dataChanged(index(0), index(rowCount() - 1));
        qCDebug(lcModel) << "TransferListModel refreshed after preferences change";
    }
}

int TransferListModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_torrents.size());
}

void TransferListModel::onTorrentsLoaded(const QList<Torrent *> &torrents)
{
    if (torrents.isEmpty())
        return;

    qCInfo(lcModel) << "TransferListModel loading" << torrents.size() << "torrent(s)";
    beginInsertRows({}, static_cast<int>(m_torrents.size())
            , static_cast<int>(m_torrents.size() + torrents.size() - 1));
    for (Torrent *const torrent : torrents)
    {
        m_rowByTorrent.insert(torrent, static_cast<int>(m_torrents.size()));
        m_torrents.append(torrent);
    }
    endInsertRows();
    emit countChanged();
}

void TransferListModel::onTorrentAdded(Torrent *torrent)
{
    if (!torrent || m_rowByTorrent.contains(torrent))
        return;

    qCInfo(lcModel) << "TransferListModel: torrent added" << torrent->name();
    addTorrentRow(torrent);
    emit countChanged();
}

void TransferListModel::addTorrentRow(Torrent *torrent)
{
    const int row = static_cast<int>(m_torrents.size());
    beginInsertRows({}, row, row);
    m_rowByTorrent.insert(torrent, row);
    m_torrents.append(torrent);
    endInsertRows();
}

void TransferListModel::onTorrentAboutToBeRemoved(Torrent *torrent)
{
    const int row = rowOf(torrent);
    if (row < 0)
        return;

    qCInfo(lcModel) << "TransferListModel: removing torrent" << torrent->name() << "at row" << row;
    beginRemoveRows({}, row, row);
    m_torrents.removeAt(row);
    m_rowByTorrent.remove(torrent);
    // Re-index everything after the removed row.
    for (int i = row; i < m_torrents.size(); ++i)
        m_rowByTorrent[m_torrents[i]] = i;
    endRemoveRows();
    emit countChanged();
}

void TransferListModel::onTorrentsUpdated(const QList<Torrent *> &torrents)
{
    if (torrents.isEmpty() || m_torrents.isEmpty())
        return;

    // Emit a coalesced dataChanged spanning the affected rows. For the common
    // bulk-update case (all torrents), collapse into a single full-range signal.
    if (torrents.size() >= m_torrents.size())
    {
        emit dataChanged(index(0), index(rowCount() - 1));
        qCDebug(lcModel) << "TransferListModel bulk-updated" << m_torrents.size() << "row(s)";
        return;
    }

    for (Torrent *const torrent : torrents)
    {
        const int row = rowOf(torrent);
        if (row >= 0)
            emit dataChanged(index(row), index(row));
    }
    qCDebug(lcModel) << "TransferListModel updated" << torrents.size() << "row(s)";
}

BitTorrent::Torrent *TransferListModel::torrentAtRow(int row) const
{
    return ((row >= 0) && (row < m_torrents.size())) ? m_torrents.at(row) : nullptr;
}

int TransferListModel::rowOf(const BitTorrent::Torrent *torrent) const
{
    return m_rowByTorrent.value(torrent, -1);
}

QString TransferListModel::idAtRow(int row) const
{
    const Torrent *const torrent = torrentAtRow(row);
    return torrent ? torrent->id().toString() : QString();
}

int TransferListModel::roleForColumn(int column)
{
    switch (column)
    {
    case TR_QUEUE_POSITION:            return QueuePositionRole;
    case TR_NAME:                      return NameRole;
    case TR_SIZE:                      return SizeRole;
    case TR_TOTAL_SIZE:                return TotalSizeRole;
    case TR_PROGRESS:                  return ProgressRole;
    case TR_STATUS:                    return StatusRole;
    case TR_SEEDS:                     return SeedsRole;
    case TR_PEERS:                     return PeersRole;
    case TR_DLSPEED:                   return DownSpeedRole;
    case TR_UPSPEED:                   return UpSpeedRole;
    case TR_ETA:                       return EtaRole;
    case TR_RATIO:                     return RatioRole;
    case TR_POPULARITY:                return PopularityRole;
    case TR_CATEGORY:                  return CategoryRole;
    case TR_TAGS:                      return TagsRole;
    case TR_ADD_DATE:                  return AddedOnRole;
    case TR_SEED_DATE:                 return CompletedOnRole;
    case TR_TRACKER:                   return TrackerRole;
    case TR_DLLIMIT:                   return DownLimitRole;
    case TR_UPLIMIT:                   return UpLimitRole;
    case TR_AMOUNT_DOWNLOADED:         return DownloadedRole;
    case TR_AMOUNT_UPLOADED:           return UploadedRole;
    case TR_AMOUNT_DOWNLOADED_SESSION: return SessionDownloadedRole;
    case TR_AMOUNT_UPLOADED_SESSION:   return SessionUploadedRole;
    case TR_AMOUNT_LEFT:               return RemainingRole;
    case TR_TIME_ELAPSED:              return TimeActiveRole;
    case TR_SAVE_PATH:                 return SavePathRole;
    case TR_COMPLETED:                 return CompletedRole;
    case TR_RATIO_LIMIT:               return RatioLimitRole;
    case TR_SEEN_COMPLETE_DATE:        return LastSeenCompleteRole;
    case TR_LAST_ACTIVITY:             return LastActivityRole;
    case TR_AVAILABILITY:              return AvailabilityRole;
    case TR_DOWNLOAD_PATH:             return DownloadPathRole;
    case TR_INFOHASH_V1:               return InfoHashV1Role;
    case TR_INFOHASH_V2:               return InfoHashV2Role;
    case TR_REANNOUNCE:                return ReannounceRole;
    case TR_PRIVATE:                   return PrivateRole;
    case TR_CREATE_DATE:               return CreatedOnRole;
    default:                           return NameRole;
    }
}

namespace
{
    // Reverse of roleForColumn for the display roles (returns -1 for aux roles).
    int columnForRole(int role)
    {
        for (int c = 0; c < TransferListModel::NB_COLUMNS; ++c)
        {
            if (TransferListModel::roleForColumn(c) == role)
                return c;
        }
        return -1;
    }
}

QVariant TransferListModel::data(const QModelIndex &index, int role) const
{
    const Torrent *const torrent = torrentAtRow(index.row());
    if (!torrent)
        return {};

    switch (role)
    {
    case Qt::DisplayRole:
        return displayValue(torrent, TR_NAME);
    case ProgressRole:
        return static_cast<double>(torrent->progress());
    case StateRole:
        return static_cast<int>(torrent->state());
    case IdRole:
        return torrent->id().toString();
    case UnderlyingRole:
        return QVariant::fromValue(static_cast<QObject *>(const_cast<Torrent *>(torrent)));
    case AdditionalUnderlyingRole:
        return {};
    default:
        break;
    }

    const int column = columnForRole(role);
    if (column >= 0)
        return displayValue(torrent, column);

    return {};
}

QString TransferListModel::displayValue(const Torrent *torrent, int column) const
{
    const bool hideZero = shouldHideZero(torrent);
    const auto hidden = [hideZero](bool isZero, const QString &value) -> QString {
        return (hideZero && isZero) ? QString() : value;
    };

    switch (column)
    {
    case TR_QUEUE_POSITION:
        return (torrent->queuePosition() >= 0) ? QString::number(torrent->queuePosition() + 1) : u"*"_s;
    case TR_NAME:
        return torrent->name();
    case TR_SIZE:
        return formatSize(torrent->wantedSize());
    case TR_TOTAL_SIZE:
        return formatSize(torrent->totalSize());
    case TR_PROGRESS:
        return (torrent->progress() >= 1)
                ? u"100%"_s
                : (Utils::String::fromDouble(torrent->progress() * 100, 1) + u"%"_s);
    case TR_STATUS:
    {
        QString text = statusText(torrent->state());
        if (torrent->state() == TorrentState::Error)
            text += u": "_s + torrent->error();
        return text;
    }
    case TR_SEEDS:
        return hidden((torrent->seedsCount() == 0) && (torrent->totalSeedsCount() == 0)
                , u"%1 (%2)"_s.arg(torrent->seedsCount()).arg(torrent->totalSeedsCount()));
    case TR_PEERS:
        return hidden((torrent->leechsCount() == 0) && (torrent->totalLeechersCount() == 0)
                , u"%1 (%2)"_s.arg(torrent->leechsCount()).arg(torrent->totalLeechersCount()));
    case TR_DLSPEED:
        return hidden(torrent->downloadPayloadRate() <= 0, formatSpeed(torrent->downloadPayloadRate()));
    case TR_UPSPEED:
        return hidden(torrent->uploadPayloadRate() <= 0, formatSpeed(torrent->uploadPayloadRate()));
    case TR_ETA:
        return hidden(torrent->eta() >= MAX_ETA, formatDuration(torrent->eta(), MAX_ETA));
    case TR_RATIO:
        return hidden(torrent->realRatio() <= 0, formatRatio(torrent->realRatio()));
    case TR_POPULARITY:
        return hidden(torrent->popularity() <= 0
                , ((torrent->popularity() < 0) ? C_INFINITY : Utils::String::fromDouble(torrent->popularity(), 2)));
    case TR_CATEGORY:
        return torrent->category();
    case TR_TAGS:
    {
        QStringList tags;
        for (const Tag &tag : torrent->tags())
            tags.append(tag.toString());
        tags.sort(Qt::CaseInsensitive);
        return tags.join(u", "_s);
    }
    case TR_ADD_DATE:
        return formatDateTime(torrent->addedTime());
    case TR_SEED_DATE:
        return formatDateTime(torrent->completedTime());
    case TR_TRACKER:
        return torrent->currentTracker();
    case TR_DLLIMIT:
        return hidden(torrent->downloadLimit() <= 0, formatSpeed(torrent->downloadLimit()));
    case TR_UPLIMIT:
        return hidden(torrent->uploadLimit() <= 0, formatSpeed(torrent->uploadLimit()));
    case TR_AMOUNT_DOWNLOADED:
        return hidden(torrent->totalDownload() <= 0, formatSize(torrent->totalDownload()));
    case TR_AMOUNT_UPLOADED:
        return hidden(torrent->totalUpload() <= 0, formatSize(torrent->totalUpload()));
    case TR_AMOUNT_DOWNLOADED_SESSION:
        return hidden(torrent->totalPayloadDownload() <= 0, formatSize(torrent->totalPayloadDownload()));
    case TR_AMOUNT_UPLOADED_SESSION:
        return hidden(torrent->totalPayloadUpload() <= 0, formatSize(torrent->totalPayloadUpload()));
    case TR_AMOUNT_LEFT:
        return hidden(torrent->remainingSize() <= 0, formatSize(torrent->remainingSize()));
    case TR_TIME_ELAPSED:
        return (torrent->finishedTime() > 0)
                ? tr("%1 (seeded for %2)", "e.g. 4m (seeded for 3m)")
                        .arg(formatDuration(torrent->activeTime()), formatDuration(torrent->finishedTime()))
                : formatDuration(torrent->activeTime());
    case TR_SAVE_PATH:
        return torrent->savePath().toString();
    case TR_COMPLETED:
        return hidden(torrent->completedSize() <= 0, formatSize(torrent->completedSize()));
    case TR_RATIO_LIMIT:
        return formatRatio(torrent->effectiveShareLimits().ratioLimit);
    case TR_SEEN_COMPLETE_DATE:
        return formatDateTime(torrent->lastSeenComplete());
    case TR_LAST_ACTIVITY:
    {
        const qlonglong t = torrent->timeSinceActivity();
        if (t < 0)
            return {};
        return (t == 0) ? tr("< 1m ago") : tr("%1 ago").arg(formatDuration(t));
    }
    case TR_AVAILABILITY:
        return hidden(torrent->distributedCopies() < 0
                , ((torrent->distributedCopies() < 0) ? tr("N/A")
                        : Utils::String::fromDouble(torrent->distributedCopies(), 3)));
    case TR_DOWNLOAD_PATH:
        return torrent->downloadPath().toString();
    case TR_INFOHASH_V1:
        return torrent->infoHash().v1().isValid() ? torrent->infoHash().v1().toString() : tr("N/A");
    case TR_INFOHASH_V2:
        return torrent->infoHash().v2().isValid() ? torrent->infoHash().v2().toString() : tr("N/A");
    case TR_REANNOUNCE:
        return formatDuration(torrent->nextAnnounce());
    case TR_PRIVATE:
        return torrent->hasMetadata() ? (torrent->isPrivate() ? tr("Yes") : tr("No")) : tr("N/A");
    case TR_CREATE_DATE:
        return formatDateTime(torrent->creationDate());
    default:
        return {};
    }
}

QString TransferListModel::statusText(BitTorrent::TorrentState state) const
{
    switch (state)
    {
    case TorrentState::Downloading:               return tr("Downloading");
    case TorrentState::ForcedDownloading:         return tr("[F] Downloading");
    case TorrentState::DownloadingMetadata:       return tr("Downloading metadata");
    case TorrentState::ForcedDownloadingMetadata: return tr("[F] Downloading metadata");
    case TorrentState::StalledDownloading:        return tr("Stalled");
    case TorrentState::Uploading:                 return tr("Seeding");
    case TorrentState::StalledUploading:          return tr("Seeding");
    case TorrentState::ForcedUploading:           return tr("[F] Seeding");
    case TorrentState::QueuedDownloading:
    case TorrentState::QueuedUploading:           return tr("Queued");
    case TorrentState::CheckingDownloading:
    case TorrentState::CheckingUploading:         return tr("Checking");
    case TorrentState::CheckingResumeData:        return tr("Checking resume data");
    case TorrentState::StoppedDownloading:        return tr("Stopped");
    case TorrentState::StoppedUploading:          return tr("Completed");
    case TorrentState::Moving:                    return tr("Moving");
    case TorrentState::MissingFiles:              return tr("Missing Files");
    case TorrentState::Error:                     return tr("Errored");
    default:                                      return tr("Unknown");
    }
}

bool TransferListModel::shouldHideZero(const Torrent *torrent) const
{
    switch (m_hideZeroMode)
    {
    case HideZeroMode::Always:
        return true;
    case HideZeroMode::Stopped:
        return torrent->state() == TorrentState::StoppedDownloading;
    case HideZeroMode::Never:
    default:
        return false;
    }
}

QVariant TransferListModel::rawValue(const Torrent *torrent, int column, bool alt) const
{
    switch (column)
    {
    case TR_QUEUE_POSITION:            return torrent->queuePosition();
    case TR_NAME:                      return torrent->name();
    case TR_SIZE:                      return torrent->wantedSize();
    case TR_TOTAL_SIZE:                return torrent->totalSize();
    case TR_PROGRESS:                  return static_cast<double>(torrent->progress());
    case TR_STATUS:                    return static_cast<int>(torrent->state());
    case TR_SEEDS:                     return alt ? torrent->totalSeedsCount() : torrent->seedsCount();
    case TR_PEERS:                     return alt ? torrent->totalLeechersCount() : torrent->leechsCount();
    case TR_DLSPEED:                   return torrent->downloadPayloadRate();
    case TR_UPSPEED:                   return torrent->uploadPayloadRate();
    case TR_ETA:                       return torrent->eta();
    case TR_RATIO:                     return static_cast<double>(torrent->realRatio());
    case TR_POPULARITY:                return static_cast<double>(torrent->popularity());
    case TR_CATEGORY:                  return torrent->category();
    case TR_TAGS:
    {
        QStringList tags;
        for (const Tag &tag : torrent->tags())
            tags.append(tag.toString());
        tags.sort(Qt::CaseInsensitive);
        return tags.join(u", "_s);
    }
    case TR_ADD_DATE:                  return torrent->addedTime();
    case TR_SEED_DATE:                 return torrent->completedTime();
    case TR_TRACKER:                   return torrent->currentTracker();
    case TR_DLLIMIT:                   return torrent->downloadLimit();
    case TR_UPLIMIT:                   return torrent->uploadLimit();
    case TR_AMOUNT_DOWNLOADED:         return torrent->totalDownload();
    case TR_AMOUNT_UPLOADED:           return torrent->totalUpload();
    case TR_AMOUNT_DOWNLOADED_SESSION: return torrent->totalPayloadDownload();
    case TR_AMOUNT_UPLOADED_SESSION:   return torrent->totalPayloadUpload();
    case TR_AMOUNT_LEFT:               return torrent->remainingSize();
    case TR_TIME_ELAPSED:              return alt ? torrent->finishedTime() : torrent->activeTime();
    case TR_SAVE_PATH:                 return torrent->savePath().toString();
    case TR_COMPLETED:                 return torrent->completedSize();
    case TR_RATIO_LIMIT:               return static_cast<double>(torrent->effectiveShareLimits().ratioLimit);
    case TR_SEEN_COMPLETE_DATE:        return torrent->lastSeenComplete();
    case TR_LAST_ACTIVITY:             return torrent->timeSinceActivity();
    case TR_AVAILABILITY:              return static_cast<double>(torrent->distributedCopies());
    case TR_DOWNLOAD_PATH:             return torrent->downloadPath().toString();
    case TR_INFOHASH_V1:               return torrent->infoHash().v1().toString();
    case TR_INFOHASH_V2:               return torrent->infoHash().v2().toString();
    case TR_REANNOUNCE:                return torrent->nextAnnounce();
    case TR_PRIVATE:                   return torrent->isPrivate();
    case TR_CREATE_DATE:               return torrent->creationDate();
    default:                           return {};
    }
}

bool TransferListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    Torrent *const torrent = torrentAtRow(index.row());
    if (!torrent)
        return false;

    const int column = (role == Qt::EditRole) ? columnForRole(NameRole) : columnForRole(role);
    if ((role == NameRole) || ((role == Qt::EditRole) && (column == TR_NAME)))
    {
        const QString name = value.toString().trimmed();
        if (name.isEmpty() || (name == torrent->name()))
            return false;
        qCInfo(lcModel) << "TransferListModel: renaming torrent to" << name;
        torrent->setName(name);
        emit dataChanged(index, index, {NameRole, Qt::DisplayRole});
        return true;
    }
    if (role == CategoryRole)
    {
        qCInfo(lcModel) << "TransferListModel: setting category" << value.toString();
        torrent->setCategory(value.toString());
        emit dataChanged(index, index, {CategoryRole});
        return true;
    }
    return false;
}

Qt::ItemFlags TransferListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return QAbstractListModel::flags(index) | Qt::ItemIsEditable;
}

QString TransferListModel::headerText(int column) const
{
    switch (column)
    {
    case TR_QUEUE_POSITION:            return u"#"_s;
    case TR_NAME:                      return tr("Name");
    case TR_SIZE:                      return tr("Size");
    case TR_TOTAL_SIZE:                return tr("Total Size");
    case TR_PROGRESS:                  return tr("Progress");
    case TR_STATUS:                    return tr("Status");
    case TR_SEEDS:                     return tr("Seeds");
    case TR_PEERS:                     return tr("Peers");
    case TR_DLSPEED:                   return tr("Down Speed");
    case TR_UPSPEED:                   return tr("Up Speed");
    case TR_ETA:                       return tr("ETA");
    case TR_RATIO:                     return tr("Ratio");
    case TR_POPULARITY:                return tr("Popularity");
    case TR_CATEGORY:                  return tr("Category");
    case TR_TAGS:                      return tr("Tags");
    case TR_ADD_DATE:                  return tr("Added On");
    case TR_SEED_DATE:                 return tr("Completed On");
    case TR_TRACKER:                   return tr("Tracker");
    case TR_DLLIMIT:                   return tr("Down Limit");
    case TR_UPLIMIT:                   return tr("Up Limit");
    case TR_AMOUNT_DOWNLOADED:         return tr("Downloaded");
    case TR_AMOUNT_UPLOADED:           return tr("Uploaded");
    case TR_AMOUNT_DOWNLOADED_SESSION: return tr("Session Downloaded");
    case TR_AMOUNT_UPLOADED_SESSION:   return tr("Session Uploaded");
    case TR_AMOUNT_LEFT:               return tr("Remaining");
    case TR_TIME_ELAPSED:              return tr("Time Active");
    case TR_SAVE_PATH:                 return tr("Save Path");
    case TR_COMPLETED:                 return tr("Completed");
    case TR_RATIO_LIMIT:               return tr("Ratio Limit");
    case TR_SEEN_COMPLETE_DATE:        return tr("Last Seen Complete");
    case TR_LAST_ACTIVITY:             return tr("Last Activity");
    case TR_AVAILABILITY:              return tr("Availability");
    case TR_DOWNLOAD_PATH:             return tr("Incomplete Save Path");
    case TR_INFOHASH_V1:               return tr("Info Hash v1");
    case TR_INFOHASH_V2:               return tr("Info Hash v2");
    case TR_REANNOUNCE:                return tr("Reannounce In");
    case TR_PRIVATE:                   return tr("Private");
    case TR_CREATE_DATE:               return tr("Created On");
    default:                           return {};
    }
}

QHash<int, QByteArray> TransferListModel::roleNames() const
{
    return {
        {QueuePositionRole, "queuePosition"},
        {NameRole, "name"},
        {SizeRole, "size"},
        {TotalSizeRole, "totalSize"},
        {ProgressRole, "progress"},
        {StatusRole, "status"},
        {SeedsRole, "seeds"},
        {PeersRole, "peers"},
        {DownSpeedRole, "downSpeed"},
        {UpSpeedRole, "upSpeed"},
        {EtaRole, "eta"},
        {RatioRole, "ratio"},
        {PopularityRole, "popularity"},
        {CategoryRole, "category"},
        {TagsRole, "tags"},
        {AddedOnRole, "addedOn"},
        {CompletedOnRole, "completedOn"},
        {TrackerRole, "tracker"},
        {DownLimitRole, "downLimit"},
        {UpLimitRole, "upLimit"},
        {DownloadedRole, "downloaded"},
        {UploadedRole, "uploaded"},
        {SessionDownloadedRole, "sessionDownloaded"},
        {SessionUploadedRole, "sessionUploaded"},
        {RemainingRole, "remaining"},
        {TimeActiveRole, "timeActive"},
        {SavePathRole, "savePath"},
        {CompletedRole, "completed"},
        {RatioLimitRole, "ratioLimit"},
        {LastSeenCompleteRole, "lastSeenComplete"},
        {LastActivityRole, "lastActivity"},
        {AvailabilityRole, "availability"},
        {DownloadPathRole, "downloadPath"},
        {InfoHashV1Role, "infoHashV1"},
        {InfoHashV2Role, "infoHashV2"},
        {ReannounceRole, "reannounce"},
        {PrivateRole, "private"},
        {CreatedOnRole, "createdOn"},
        {StateRole, "state"},
        {IdRole, "id"},
        {UnderlyingRole, "underlying"},
        {AdditionalUnderlyingRole, "additionalUnderlying"}
    };
}
