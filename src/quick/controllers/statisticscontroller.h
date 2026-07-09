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

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QVariantMap>

#include "base/bittorrent/session.h"
#include "base/bittorrent/sessionstatus.h"
#include "base/logging.h"
#include "base/utils/misc.h"
#include "base/utils/string.h"

/**
 * @file statisticscontroller.h
 * @brief Bridge backing the Material @c StatisticsDialog.
 *
 * @c StatisticsController is a QML singleton that mirrors the legacy
 * @c StatsDialog. It subscribes to @c BitTorrent::Session::statsUpdated (never
 * polls) and republishes every user / cache / performance / tracker figure as a
 * single, already-formatted @c QVariantMap that the dialog binds by key. All
 * numbers are formatted here (via @c Utils::Misc::friendlyUnit /
 * @c Utils::String::fromDouble) exactly like the original dialog so QML never
 * has to touch engine types.
 *
 * Map keys (all values are display strings):
 * @li @c allTimeUpload, @c allTimeDownload, @c allTimeShareRatio,
 *     @c sessionWaste, @c connectedPeers            (User statistics)
 * @li @c readCacheHits (libtorrent1 only), @c totalBufferSize (Cache statistics)
 * @li @c writeCacheOverload, @c readCacheOverload, @c queuedIOJobs,
 *     @c averageTimeInQueue, @c totalQueuedSize, @c requestLatency, @c dhtNodes
 *     (Performance statistics)
 * @li @c queuedTrackerAnnounces                     (Tracker statistics)
 */
class StatisticsController final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    /// All statistics, pre-formatted, keyed by the ids documented above.
    Q_PROPERTY(QVariantMap stats READ stats NOTIFY updated)

    /// Whether the "Read cache hits" row is meaningful (false on libtorrent 2.x).
    Q_PROPERTY(bool readCacheHitsVisible READ readCacheHitsVisible CONSTANT)

public:
    /// QML singleton factory — returns the app-owned instance.
    static StatisticsController *create(QQmlEngine *, QJSEngine *)
    {
        qCDebug(lcUi) << "Creating StatisticsController";
        return new StatisticsController;
    }

    explicit StatisticsController(QObject *parent = nullptr)
        : QObject(parent)
    {
        auto *session = BitTorrent::Session::instance();
        connect(session, &BitTorrent::Session::statsUpdated
                , this, &StatisticsController::refresh);
        qCDebug(lcUi) << "StatisticsController subscribed to Session::statsUpdated";
        refresh();
    }

    [[nodiscard]] QVariantMap stats() const { return m_stats; }

    [[nodiscard]] static constexpr bool readCacheHitsVisible()
    {
#ifdef QBT_USES_LIBTORRENT2
        return false;
#else
        return true;
#endif
    }

public slots:
    /// Recompute every figure from the live session status and republish.
    void refresh()
    {
        const auto *session = BitTorrent::Session::instance();
        const BitTorrent::SessionStatus &status = session->status();
        const BitTorrent::CacheStatus &cache = session->cacheStatus();

        QVariantMap m;

        // ---- User statistics ------------------------------------------------
        m[QStringLiteral("allTimeUpload")] = Utils::Misc::friendlyUnit(status.allTimeUpload);
        m[QStringLiteral("allTimeDownload")] = Utils::Misc::friendlyUnit(status.allTimeDownload);

        const qint64 atd = status.allTimeDownload;
        const qint64 atu = status.allTimeUpload;
        m[QStringLiteral("allTimeShareRatio")] = ((atd > 0) && (atu > 0))
                ? Utils::String::fromDouble((static_cast<double>(atu) / static_cast<double>(atd)), 2)
                : QString::fromLatin1("-");

        m[QStringLiteral("sessionWaste")] = Utils::Misc::friendlyUnit(status.totalWasted);
        m[QStringLiteral("connectedPeers")] = QString::number(status.peersCount);
        m[QStringLiteral("dhtNodes")] = QString::number(status.dhtNodes);

        // ---- Cache statistics ----------------------------------------------
        // Read cache hits is only reported by libtorrent 1.x.
        m[QStringLiteral("readCacheHits")] = QStringLiteral("%1%").arg((cache.readRatio > 0)
                ? Utils::String::fromDouble((100 * cache.readRatio), 2)
                : QString::fromLatin1("0"));
        m[QStringLiteral("totalBufferSize")] = Utils::Misc::friendlyUnit(cache.totalUsedBuffers * 16 * 1024);

        // ---- Performance statistics ----------------------------------------
        const qint64 peers = status.peersCount;
        m[QStringLiteral("writeCacheOverload")] = QStringLiteral("%1%").arg(((status.diskWriteQueue > 0) && (peers > 0))
                ? Utils::String::fromDouble(((100.0 * static_cast<double>(status.diskWriteQueue)) / static_cast<double>(peers)), 2)
                : QString::fromLatin1("0"));
        m[QStringLiteral("readCacheOverload")] = QStringLiteral("%1%").arg(((status.diskReadQueue > 0) && (peers > 0))
                ? Utils::String::fromDouble(((100.0 * static_cast<double>(status.diskReadQueue)) / static_cast<double>(peers)), 2)
                : QString::fromLatin1("0"));
        m[QStringLiteral("queuedIOJobs")] = QString::number(cache.jobQueueLength);
        m[QStringLiteral("averageTimeInQueue")] = tr("%1 ms").arg(cache.averageJobTime);
        m[QStringLiteral("totalQueuedSize")] = Utils::Misc::friendlyUnit(cache.queuedBytes);
        m[QStringLiteral("requestLatency")] = tr("%1 ms").arg(cache.requestLatency);

        // ---- Tracker statistics --------------------------------------------
        m[QStringLiteral("queuedTrackerAnnounces")] = QString::number(status.queuedTrackerAnnounces);

        m_stats = m;
        qCDebug(lcUi) << "StatisticsController refreshed statistics";
        emit updated();
    }

signals:
    /// Emitted after each recompute so the dialog re-binds every row.
    void updated();

private:
    QVariantMap m_stats;
};
