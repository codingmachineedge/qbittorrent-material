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

#include <QJsonDocument>
#include <QJsonObject>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include "base/bittorrent/addtorrentparams.h"
#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/bittorrent/torrentcontentlayout.h"
#include "base/logging.h"
#include "base/path.h"
#include "base/tag.h"
#include "base/tagset.h"

using namespace Qt::StringLiterals;

class QJSEngine;

/**
 * @file addtorrentparamscontroller.h
 * @brief The @c AddTorrentParamsController QML singleton — a tri-state,
 *        reusable @c BitTorrent::AddTorrentParams editor backend.
 *
 * This is the shared model behind @c AddTorrentParamsForm.qml. Every option can
 * be left "Default" (→ @c std::optional nullopt / absent map key), so the same
 * form is reused by RSS auto-download rules, the category dialog and watched
 * folders. QML edits a plain @c QVariantMap; this controller converts to/from
 * the engine's @c AddTorrentParams gadget and can resolve what each "Default"
 * currently evaluates to via @c Session.
 *
 * Map convention (keys are absent when the field is left at "Default"):
 *  - @c useAutoTMM, @c useDownloadPath, @c addStopped, @c addToQueueTop : bool
 *  - @c contentLayout, @c stopCondition : int (enum value)
 *  - @c savePath, @c downloadPath, @c category : string (always present)
 *  - @c tags : string list; @c skipChecking, @c sequential,
 *    @c firstLastPiecePriority : bool (always present)
 */
class AddTorrentParamsController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

public:
    /// QML singleton factory — returns the one app-owned instance.
    static AddTorrentParamsController *create(QQmlEngine *, QJSEngine *)
    {
        return instance();
    }

    static AddTorrentParamsController *instance()
    {
        static AddTorrentParamsController s_instance;
        return &s_instance;
    }

    explicit AddTorrentParamsController(QObject *parent = nullptr)
        : QObject(parent)
    {
        qCDebug(lcUi) << "AddTorrentParamsController constructed";
    }

    // ---- C++ helpers (used by RSS / category / watched-folder controllers) --

    /// Serialize an @c AddTorrentParams to the QVariantMap the QML form edits.
    /// Optional fields that are @c nullopt are simply left out of the map so
    /// QML sees them as @c undefined ("Default").
    static QVariantMap toMap(const BitTorrent::AddTorrentParams &p)
    {
        QVariantMap m;
        if (p.useAutoTMM.has_value())
            m.insert(u"useAutoTMM"_s, *p.useAutoTMM);
        m.insert(u"savePath"_s, p.savePath.toString());
        if (p.useDownloadPath.has_value())
            m.insert(u"useDownloadPath"_s, *p.useDownloadPath);
        m.insert(u"downloadPath"_s, p.downloadPath.toString());
        m.insert(u"category"_s, p.category);

        QStringList tags;
        for (const Tag &tag : p.tags)
            tags.append(tag.toString());
        m.insert(u"tags"_s, tags);

        m.insert(u"skipChecking"_s, p.skipChecking);
        m.insert(u"sequential"_s, p.sequential);
        m.insert(u"firstLastPiecePriority"_s, p.firstLastPiecePriority);

        if (p.contentLayout.has_value())
            m.insert(u"contentLayout"_s, static_cast<int>(*p.contentLayout));
        if (p.addStopped.has_value())
            m.insert(u"addStopped"_s, *p.addStopped);
        if (p.stopCondition.has_value())
            m.insert(u"stopCondition"_s, static_cast<int>(*p.stopCondition));
        if (p.addToQueueTop.has_value())
            m.insert(u"addToQueueTop"_s, *p.addToQueueTop);
        return m;
    }

    /// Build an @c AddTorrentParams from the QML form's map. A missing key means
    /// "Default" (the corresponding @c std::optional stays @c nullopt).
    static BitTorrent::AddTorrentParams fromMap(const QVariantMap &m)
    {
        BitTorrent::AddTorrentParams p;
        if (m.contains(u"useAutoTMM"_s))
            p.useAutoTMM = m.value(u"useAutoTMM"_s).toBool();
        p.savePath = Path(m.value(u"savePath"_s).toString());
        if (m.contains(u"useDownloadPath"_s))
            p.useDownloadPath = m.value(u"useDownloadPath"_s).toBool();
        p.downloadPath = Path(m.value(u"downloadPath"_s).toString());
        p.category = m.value(u"category"_s).toString();

        TagSet tags;
        const QStringList tagList = m.value(u"tags"_s).toStringList();
        for (const QString &tag : tagList)
        {
            const QString trimmed = tag.trimmed();
            if (!trimmed.isEmpty())
                tags.insert(Tag(trimmed));
        }
        p.tags = tags;

        p.skipChecking = m.value(u"skipChecking"_s, false).toBool();
        p.sequential = m.value(u"sequential"_s, false).toBool();
        p.firstLastPiecePriority = m.value(u"firstLastPiecePriority"_s, false).toBool();

        if (m.contains(u"contentLayout"_s))
        {
            p.contentLayout = static_cast<BitTorrent::TorrentContentLayout>(
                    m.value(u"contentLayout"_s).toInt());
        }
        if (m.contains(u"addStopped"_s))
            p.addStopped = m.value(u"addStopped"_s).toBool();
        if (m.contains(u"stopCondition"_s))
        {
            p.stopCondition = static_cast<BitTorrent::Torrent::StopCondition>(
                    m.value(u"stopCondition"_s).toInt());
        }
        if (m.contains(u"addToQueueTop"_s))
            p.addToQueueTop = m.value(u"addToQueueTop"_s).toBool();
        return p;
    }

    // ---- QML-invokable API -------------------------------------------------

    /// A fresh, all-"Default" parameter map for a new rule / category.
    Q_INVOKABLE QVariantMap newParams() const
    {
        QVariantMap m;
        m.insert(u"savePath"_s, QString());
        m.insert(u"downloadPath"_s, QString());
        m.insert(u"category"_s, QString());
        m.insert(u"tags"_s, QStringList());
        m.insert(u"skipChecking"_s, false);
        m.insert(u"sequential"_s, false);
        m.insert(u"firstLastPiecePriority"_s, false);
        return m;
    }

    /// Resolve what every "Default" currently evaluates to (for hint text).
    Q_INVOKABLE QVariantMap resolvedDefaults() const
    {
        const auto *session = BitTorrent::Session::instance();
        QVariantMap m;
        if (!session)
            return m;

        m.insert(u"useAutoTMM"_s, !session->isAutoTMMDisabledByDefault());
        m.insert(u"savePath"_s, session->savePath().toString());
        m.insert(u"useDownloadPath"_s, session->isDownloadPathEnabled());
        m.insert(u"downloadPath"_s, session->downloadPath().toString());
        m.insert(u"addStopped"_s, session->isAddTorrentStopped());
        m.insert(u"addToQueueTop"_s, session->isAddTorrentToQueueTop());
        m.insert(u"contentLayout"_s, static_cast<int>(session->torrentContentLayout()));
        m.insert(u"stopCondition"_s, static_cast<int>(session->torrentStopCondition()));
        return m;
    }

    /// Sorted list of existing category names (with an empty "uncategorized").
    Q_INVOKABLE QStringList categories() const
    {
        const auto *session = BitTorrent::Session::instance();
        if (!session)
            return {};
        QStringList result = session->categories();
        std::sort(result.begin(), result.end());
        result.prepend(QString());
        return result;
    }

    /// Automatic save / download paths for @p category (AutoTMM preview).
    Q_INVOKABLE QVariantMap categoryPaths(const QString &category) const
    {
        const auto *session = BitTorrent::Session::instance();
        QVariantMap m;
        if (!session)
            return m;
        m.insert(u"savePath"_s, session->categorySavePath(category).toString());
        m.insert(u"downloadPath"_s, session->categoryDownloadPath(category).toString());
        return m;
    }

    /// Persist a parameter map into a JSON string (for RSS rules etc.).
    Q_INVOKABLE QString serialize(const QVariantMap &map) const
    {
        const QJsonObject obj = BitTorrent::serializeAddTorrentParams(fromMap(map));
        return QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    }

    /// Parse a JSON string back into a parameter map.
    Q_INVOKABLE QVariantMap deserialize(const QString &json) const
    {
        const QJsonObject obj = QJsonDocument::fromJson(json.toUtf8()).object();
        return toMap(BitTorrent::parseAddTorrentParams(obj));
    }
};
