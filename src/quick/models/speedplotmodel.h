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
#include <chrono>
#include <cmath>
#include <deque>

#include <QElapsedTimer>
#include <QLocale>
#include <QObject>
#include <QPointF>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>

#include "base/bittorrent/session.h"
#include "base/bittorrent/sessionstatus.h"
#include "base/logging.h"
#include "base/preferences.h"
#include "base/utils/misc.h"

/**
 * @file speedplotmodel.h
 * @brief The @c SpeedPlotModel — session-wide speed-graph data source for the
 *        Properties → Speed tab.
 *
 * Ten series (total/payload/overhead/DHT/tracker × up/down) are sampled off
 * @c Session::statsUpdated and averaged into per-resolution ring buffers, exactly
 * mirroring the legacy @c SpeedPlotView averagers (5 min\@1 s, 30 min\@6 s,
 * 6 h\@36 s, 12 h\@72 s, 24 h\@144 s). QML paints the plot on a @c Canvas by
 * reading @ref seriesPoints (normalized x, byte-valued y) and @ref yScale for the
 * labelled Y axis; no widgets, no polling.
 *
 * Enabled series and the selected period are persisted through @c Preferences
 * (`getSpeedWidgetGraphEnable` / `getSpeedWidgetPeriod`).
 */
class SpeedPlotModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int period READ period WRITE setPeriod NOTIFY periodChanged FINAL)

public:
    /// The ten plotted series, in a stable order matched by @c SampleData.
    enum GraphId
    {
        Up = 0,
        Down,
        PayloadUp,
        PayloadDown,
        OverheadUp,
        OverheadDown,
        DhtUp,
        DhtDown,
        TrackerUp,
        TrackerDown,

        NumGraphs
    };
    Q_ENUM(GraphId)

    /// Selectable time window.
    enum Period
    {
        Min1 = 0,
        Min5,
        Min30,
        Hour3,
        Hour6,
        Hour12,
        Hour24
    };
    Q_ENUM(Period)

    explicit SpeedPlotModel(QObject *parent = nullptr)
        : QObject(parent)
    {
        const Preferences *pref = Preferences::instance();
        for (int id = 0; id < NumGraphs; ++id)
            m_enabled[id] = pref->getSpeedWidgetGraphEnable(id);

        applyPeriod(static_cast<Period>(pref->getSpeedWidgetPeriod()));

        connect(BitTorrent::Session::instance(), &BitTorrent::Session::statsUpdated
                , this, &SpeedPlotModel::onStatsUpdated);
        qCDebug(lcModel) << "SpeedPlotModel constructed; period" << m_period;
    }

    [[nodiscard]] int period() const { return m_period; }
    void setPeriod(const int period)
    {
        if ((period < Min1) || (period > Hour24) || (period == m_period))
            return;

        applyPeriod(static_cast<Period>(period));
        Preferences::instance()->setSpeedWidgetPeriod(m_period);
        qCDebug(lcModel) << "SpeedPlotModel period ->" << m_period;
        emit periodChanged();
        emit updated();
    }

    /// Total number of series (== NumGraphs).
    Q_INVOKABLE int graphCount() const { return NumGraphs; }

    Q_INVOKABLE bool isGraphEnabled(const int id) const
    {
        return ((id >= 0) && (id < NumGraphs)) && m_enabled[id];
    }

    Q_INVOKABLE void setGraphEnabled(const int id, const bool enable)
    {
        if ((id < 0) || (id >= NumGraphs) || (m_enabled[id] == enable))
            return;

        m_enabled[id] = enable;
        Preferences::instance()->setSpeedWidgetGraphEnable(id, enable);
        qCDebug(lcModel) << "SpeedPlotModel series" << id << (enable ? "enabled" : "disabled");
        emit updated();
    }

    /// Translated display name for a series (used by the legend / toggle menu).
    Q_INVOKABLE QString graphName(const int id) const
    {
        switch (id)
        {
        case Up:           return tr("Total Upload");
        case Down:         return tr("Total Download");
        case PayloadUp:    return tr("Payload Upload");
        case PayloadDown:  return tr("Payload Download");
        case OverheadUp:   return tr("Overhead Upload");
        case OverheadDown: return tr("Overhead Download");
        case DhtUp:        return tr("DHT Upload");
        case DhtDown:      return tr("DHT Download");
        case TrackerUp:    return tr("Tracker Upload");
        case TrackerDown:  return tr("Tracker Download");
        default:           return {};
        }
    }

    /// The plotted window length, in seconds (for the time axis).
    Q_INVOKABLE int windowSeconds() const
    {
        return static_cast<int>(std::chrono::duration_cast<std::chrono::seconds>(m_currentMaxDuration).count());
    }

    /**
     * Points for series @p id within the current window, most-recent first.
     * Each point is a @c QPointF where @c x is a left-to-right fraction in [0, 1]
     * (1 == newest, at the right edge) and @c y is the raw speed in bytes/s.
     */
    Q_INVOKABLE QVariantList seriesPoints(const int id) const
    {
        QVariantList points;
        if ((id < 0) || (id >= NumGraphs))
            return points;

        const std::deque<Sample> &queue = currentData();
        const qreal windowMs = static_cast<qreal>(m_currentMaxDuration.count());
        if (windowMs <= 0)
            return points;

        std::chrono::milliseconds fromRight {0};
        for (int i = static_cast<int>(queue.size()) - 1; i >= 0; --i)
        {
            const qreal frac = std::clamp(1.0 - (fromRight.count() / windowMs), 0.0, 1.0);
            points.append(QPointF(frac, static_cast<qreal>(queue[i].data[id])));

            fromRight += queue[i].duration;
            if (fromRight >= m_currentMaxDuration)
                break;
        }
        return points;
    }

    /// Peak byte/s value across all *enabled* series inside the current window.
    Q_INVOKABLE qreal maxValue() const
    {
        const std::deque<Sample> &queue = currentData();

        quint64 maxY = 0;
        for (int id = 0; id < NumGraphs; ++id)
        {
            if (!m_enabled[id])
                continue;

            std::chrono::milliseconds duration {0};
            for (int i = static_cast<int>(queue.size()) - 1; i >= 0; --i)
            {
                maxY = std::max(maxY, queue[i].data[id]);
                duration += queue[i].duration;
                if (duration >= m_currentMaxDuration)
                    break;
            }
        }
        return static_cast<qreal>(maxY);
    }

    /**
     * A nicely rounded Y-axis scale for the current data. Returns a map:
     *  - @c max    : the rounded maximum, in bytes/s (use to normalize @c y)
     *  - @c labels : five top-to-bottom axis captions (already localized)
     */
    Q_INVOKABLE QVariantMap yScale() const
    {
        const SplitValue nice = roundedYScale(maxValue());
        const qreal maxBytes = static_cast<qreal>(Utils::Misc::sizeInBytes(nice.arg, nice.unit));

        const QStringList labels = {
            formatLabel(nice.arg, nice.unit),
            formatLabel(0.75 * nice.arg, nice.unit),
            formatLabel(0.50 * nice.arg, nice.unit),
            formatLabel(0.25 * nice.arg, nice.unit),
            formatLabel(0.0, nice.unit)
        };

        return {{QStringLiteral("max"), maxBytes}, {QStringLiteral("labels"), labels}};
    }

signals:
    void periodChanged();
    /// Fired whenever the plotted data (or its enable/period configuration)
    /// changed and the QML canvas should repaint.
    void updated();

private:
    using SampleData = std::array<quint64, NumGraphs>;

    struct Sample
    {
        std::chrono::milliseconds duration {0};
        SampleData data {};
    };

    /// A single-resolution averaging ring buffer (mirrors SpeedPlotView::Averager).
    class Averager
    {
    public:
        Averager(const std::chrono::milliseconds duration, const std::chrono::milliseconds resolution)
            : m_resolution {resolution}
            , m_maxDuration {duration}
        {
            m_lastSampleTime.start();
        }

        /// Accumulate one raw sample; returns true when a new averaged point was
        /// flushed (i.e. the resolution interval elapsed).
        bool push(const SampleData &sample)
        {
            ++m_counter;
            for (int id = 0; id < NumGraphs; ++id)
                m_accumulator[id] += sample[id];

            const std::chrono::milliseconds updateInterval {
                static_cast<int64_t>(BitTorrent::Session::instance()->refreshInterval() * 1.25)};
            const std::chrono::milliseconds maxElapsed {std::max(updateInterval, m_resolution)};
            const std::chrono::milliseconds elapsed {
                std::min(std::chrono::milliseconds {m_lastSampleTime.elapsed()}, maxElapsed)};
            if (elapsed < m_resolution)
                return false; // still accumulating

            for (int id = 0; id < NumGraphs; ++id)
                m_accumulator[id] /= static_cast<quint64>(m_counter);

            m_currentDuration += elapsed;
            if (m_currentDuration > m_maxDuration)
            {
                while (!m_sink.empty()
                       && ((m_currentDuration - m_sink.front().duration) >= m_maxDuration))
                {
                    m_currentDuration -= m_sink.front().duration;
                    m_sink.pop_front();
                }
            }

            m_sink.push_back({elapsed, m_accumulator});
            m_accumulator = {};
            m_counter = 0;
            m_lastSampleTime.restart();
            return true;
        }

        [[nodiscard]] const std::deque<Sample> &data() const { return m_sink; }

    private:
        const std::chrono::milliseconds m_resolution;
        const std::chrono::milliseconds m_maxDuration;
        std::chrono::milliseconds m_currentDuration {0};
        int m_counter = 0;
        SampleData m_accumulator {};
        std::deque<Sample> m_sink;
        QElapsedTimer m_lastSampleTime;
    };

    struct SplitValue
    {
        qreal arg = 0;
        Utils::Misc::SizeUnit unit = Utils::Misc::SizeUnit::Byte;
    };

    void onStatsUpdated()
    {
        const BitTorrent::SessionStatus &status = BitTorrent::Session::instance()->status();

        SampleData sample {};
        sample[Up] = static_cast<quint64>(std::max<qint64>(0, status.uploadRate));
        sample[Down] = static_cast<quint64>(std::max<qint64>(0, status.downloadRate));
        sample[PayloadUp] = static_cast<quint64>(std::max<qint64>(0, status.payloadUploadRate));
        sample[PayloadDown] = static_cast<quint64>(std::max<qint64>(0, status.payloadDownloadRate));
        sample[OverheadUp] = static_cast<quint64>(std::max<qint64>(0, status.ipOverheadUploadRate));
        sample[OverheadDown] = static_cast<quint64>(std::max<qint64>(0, status.ipOverheadDownloadRate));
        sample[DhtUp] = static_cast<quint64>(std::max<qint64>(0, status.dhtUploadRate));
        sample[DhtDown] = static_cast<quint64>(std::max<qint64>(0, status.dhtDownloadRate));
        sample[TrackerUp] = static_cast<quint64>(std::max<qint64>(0, status.trackerUploadRate));
        sample[TrackerDown] = static_cast<quint64>(std::max<qint64>(0, status.trackerDownloadRate));

        bool currentUpdated = false;
        for (Averager *averager : {&m_averager5Min, &m_averager30Min, &m_averager6Hour, &m_averager12Hour, &m_averager24Hour})
        {
            if (averager->push(sample) && (m_currentAverager == averager))
                currentUpdated = true;
        }

        if (currentUpdated)
            emit updated();
    }

    void applyPeriod(const Period period)
    {
        using namespace std::chrono_literals;
        m_period = period;
        switch (period)
        {
        case Min1:   m_currentMaxDuration = 1min;  m_currentAverager = &m_averager5Min;  break;
        case Min5:   m_currentMaxDuration = 5min;  m_currentAverager = &m_averager5Min;  break;
        case Min30:  m_currentMaxDuration = 30min; m_currentAverager = &m_averager30Min; break;
        case Hour3:  m_currentMaxDuration = 3h;    m_currentAverager = &m_averager6Hour; break;
        case Hour6:  m_currentMaxDuration = 6h;    m_currentAverager = &m_averager6Hour; break;
        case Hour12: m_currentMaxDuration = 12h;   m_currentAverager = &m_averager12Hour; break;
        case Hour24: m_currentMaxDuration = 24h;   m_currentAverager = &m_averager24Hour; break;
        }
    }

    [[nodiscard]] const std::deque<Sample> &currentData() const
    {
        return m_currentAverager->data();
    }

    static SplitValue roundedYScale(qreal value)
    {
        using Utils::Misc::SizeUnit;

        if (value == 0.0)
            return {0, SizeUnit::Byte};
        if (value <= 12.0)
            return {12, SizeUnit::Byte};

        SizeUnit unit = SizeUnit::Byte;
        while (value > 1024)
        {
            value /= 1024;
            unit = static_cast<SizeUnit>(static_cast<int>(unit) + 1);
        }

        if (value > 100)
            return {std::ceil(value / 40) * 40, unit};
        if (value > 10)
            return {std::ceil(value / 4) * 4, unit};

        // nice steps to get evenly-divided quarters of the scale
        static const qreal roundingTable[] = {1.2, 1.6, 2, 2.4, 2.8, 3.2, 4, 6, 8};
        for (const qreal rounded : roundingTable)
        {
            if (value <= rounded)
                return {rounded, unit};
        }
        return {10.0, unit};
    }

    static QString formatLabel(const qreal argValue, const Utils::Misc::SizeUnit unit)
    {
        const int precision = (argValue < 10) ? Utils::Misc::friendlyUnitPrecision(unit) : 0;
        return QLocale::system().toString(argValue, 'f', precision)
                + QChar(QChar::Nbsp) + Utils::Misc::unitString(unit, true);
    }

    Averager m_averager5Min {std::chrono::minutes(5), std::chrono::seconds(1)};
    Averager m_averager30Min {std::chrono::minutes(30), std::chrono::seconds(6)};
    Averager m_averager6Hour {std::chrono::hours(6), std::chrono::seconds(36)};
    Averager m_averager12Hour {std::chrono::hours(12), std::chrono::seconds(72)};
    Averager m_averager24Hour {std::chrono::hours(24), std::chrono::seconds(144)};
    Averager *m_currentAverager = &m_averager5Min;

    std::chrono::milliseconds m_currentMaxDuration {std::chrono::minutes(5)};
    std::array<bool, NumGraphs> m_enabled {};
    Period m_period = Min5;
};
