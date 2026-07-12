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
#include <QList>
#include <QString>

#include <qqmlintegration.h>

#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/logging.h"
#include "base/preferences.h"
#include "torrentfilter.h"

/**
 * @file statusfiltermodel.h
 * @brief @c StatusFilterModel — the Status section of the transfer-list sidebar.
 *
 * Fourteen fixed rows in @c TorrentFilter::Status order (All, Downloading,
 * Seeding, Completed, Running, Stopped, Active, Inactive, Stalled,
 * Stalled Uploading, Stalled Downloading, Checking, Moving, Errored), each with a
 * live torrent count. Rows with a zero count (except *All*) are hidden when the
 * `TransferListFilters/HideZeroStatusFilters` preference is on. Selecting a row
 * feeds `TorrentFilterProxyModel::setStatusFilter`.
 */
class StatusFilterModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool hideZero READ hideZero WRITE setHideZero NOTIFY hideZeroChanged)

public:
    enum Roles
    {
        LabelRole = Qt::UserRole + 1, // "label"
        CountRole,                    // "count"
        IconRole,                     // "icon"
        ValueRole                     // "value" (TorrentFilter::Status int)
    };
    Q_ENUM(Roles)

    explicit StatusFilterModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        qCDebug(lcModel) << "StatusFilterModel created";
        if (const Preferences *const pref = Preferences::instance())
            m_hideZero = pref->getHideZeroStatusFilters();
        subscribe();
        recount();
        rebuildVisible();   // guarantee rows exist even for an empty session
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_visible.size());
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if ((index.row() < 0) || (index.row() >= m_visible.size()))
            return {};

        const int status = m_visible.at(index.row());
        switch (role)
        {
        case LabelRole:
        case Qt::DisplayRole:
            return labelFor(status);
        case CountRole:
            return m_counts.at(status);
        case IconRole:
            return iconFor(status);
        case ValueRole:
            return status;
        default:
            return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {{LabelRole, "label"}, {CountRole, "count"}, {IconRole, "icon"}, {ValueRole, "value"}};
    }

    [[nodiscard]] bool hideZero() const { return m_hideZero; }

    void setHideZero(bool value)
    {
        if (m_hideZero == value)
            return;
        m_hideZero = value;
        qCDebug(lcModel) << "StatusFilterModel hideZero ->" << value;
        rebuildVisible();
        emit hideZeroChanged();
    }

signals:
    void hideZeroChanged();

private:
    void subscribe()
    {
        BitTorrent::Session *const session = BitTorrent::Session::instance();
        if (!session)
        {
            qCWarning(lcModel) << "StatusFilterModel: no Session instance";
            return;
        }
        connect(session, &BitTorrent::Session::torrentsLoaded, this, [this] { recount(); });
        connect(session, &BitTorrent::Session::torrentsUpdated, this, [this] { recount(); });
        connect(session, &BitTorrent::Session::torrentAdded, this, [this](BitTorrent::Torrent *) { recount(); });
        connect(session, &BitTorrent::Session::torrentRemoved, this, [this](const BitTorrent::TorrentID &) { recount(); });
    }

    [[nodiscard]] static QString labelFor(int status)
    {
        switch (status)
        {
        case TorrentFilter::All:                return tr("All");
        case TorrentFilter::Downloading:        return tr("Downloading");
        case TorrentFilter::Seeding:            return tr("Seeding");
        case TorrentFilter::Completed:          return tr("Completed");
        case TorrentFilter::Running:            return tr("Running");
        case TorrentFilter::Stopped:            return tr("Stopped");
        case TorrentFilter::Active:             return tr("Active");
        case TorrentFilter::Inactive:           return tr("Inactive");
        case TorrentFilter::Stalled:            return tr("Stalled");
        case TorrentFilter::StalledUploading:   return tr("Stalled Uploading");
        case TorrentFilter::StalledDownloading: return tr("Stalled Downloading");
        case TorrentFilter::Checking:           return tr("Checking");
        case TorrentFilter::Moving:             return tr("Moving");
        case TorrentFilter::Errored:            return tr("Errored");
        default:                                return {};
        }
    }

    [[nodiscard]] static QString iconFor(int status)
    {
        switch (status)
        {
        case TorrentFilter::All:                return QStringLiteral("filter_all");
        case TorrentFilter::Downloading:        return QStringLiteral("downloading");
        case TorrentFilter::Seeding:            return QStringLiteral("uploading");
        case TorrentFilter::Completed:          return QStringLiteral("completed");
        case TorrentFilter::Running:            return QStringLiteral("play_arrow");
        case TorrentFilter::Stopped:            return QStringLiteral("stopped");
        case TorrentFilter::Active:             return QStringLiteral("filter_active");
        case TorrentFilter::Inactive:           return QStringLiteral("filter_inactive");
        case TorrentFilter::Stalled:            return QStringLiteral("filter_stalled");
        case TorrentFilter::StalledUploading:   return QStringLiteral("stalled_up");
        case TorrentFilter::StalledDownloading: return QStringLiteral("stalled_dl");
        case TorrentFilter::Checking:           return QStringLiteral("force_recheck");
        case TorrentFilter::Moving:             return QStringLiteral("set_location");
        case TorrentFilter::Errored:            return QStringLiteral("error");
        default:                                return {};
        }
    }

    void recount()
    {
        std::array<int, TorrentFilter::_Count> counts {};
        BitTorrent::Session *const session = BitTorrent::Session::instance();
        const QList<BitTorrent::Torrent *> torrents = session ? session->torrents() : QList<BitTorrent::Torrent *> {};

        using BitTorrent::TorrentState;
        for (const BitTorrent::Torrent *const t : torrents)
        {
            ++counts[TorrentFilter::All];
            if (t->isDownloading())  ++counts[TorrentFilter::Downloading];
            if (t->isUploading())    ++counts[TorrentFilter::Seeding];
            if (t->isCompleted())    ++counts[TorrentFilter::Completed];
            if (t->isRunning())      ++counts[TorrentFilter::Running];
            if (t->isStopped())      ++counts[TorrentFilter::Stopped];
            if (t->isActive())       ++counts[TorrentFilter::Active];
            if (t->isInactive())     ++counts[TorrentFilter::Inactive];
            if (t->isChecking())     ++counts[TorrentFilter::Checking];
            if (t->isMoving())       ++counts[TorrentFilter::Moving];
            if (t->isErrored())      ++counts[TorrentFilter::Errored];
            if (t->state() == TorrentState::StalledUploading)
            {
                ++counts[TorrentFilter::StalledUploading];
                ++counts[TorrentFilter::Stalled];
            }
            else if (t->state() == TorrentState::StalledDownloading)
            {
                ++counts[TorrentFilter::StalledDownloading];
                ++counts[TorrentFilter::Stalled];
            }
        }

        if (counts == m_counts)
            return;

        m_counts = counts;
        rebuildVisible();
    }

    void rebuildVisible()
    {
        QList<int> visible;
        visible.reserve(TorrentFilter::_Count);
        for (int s = 0; s < TorrentFilter::_Count; ++s)
        {
            if (!m_hideZero || (s == TorrentFilter::All) || (m_counts.at(s) > 0))
                visible.append(s);
        }

        beginResetModel();
        m_visible = visible;
        endResetModel();
        qCDebug(lcModel) << "StatusFilterModel recomputed;" << m_visible.size() << "visible rows";
    }

    std::array<int, TorrentFilter::_Count> m_counts {};
    QList<int> m_visible;
    bool m_hideZero = false;
};
