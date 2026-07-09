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

#include <utility>

#include <QAbstractListModel>
#include <QList>
#include <QString>

#include <qqmlintegration.h>

#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/logging.h"
#include "base/tag.h"
#include "base/tagset.h"

/**
 * @file tagfiltermodel.h
 * @brief @c TagFilterModel — the Tags section of the transfer-list sidebar.
 *
 * A flat list: row 0 *All* (no tag filter), row 1 *Untagged* (torrents with no
 * tags), then one row per @c BitTorrent::Session tag, each with a live count.
 * Selecting a row drives `TorrentFilterProxyModel::setTagFilter` (All ->
 * `clearTagFilter`).
 */
class TagFilterModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    /// Row kind — lets QML wire the correct proxy call.
    enum FilterType
    {
        AllTags = 0,
        Untagged = 1,
        RealTag = 2
    };
    Q_ENUM(FilterType)

    enum Roles
    {
        LabelRole = Qt::UserRole + 1, // "label"
        CountRole,                    // "count"
        IconRole,                     // "icon"
        ValueRole,                    // "value" (tag string; empty for All/Untagged)
        TypeRole                      // "type"  (FilterType)
    };
    Q_ENUM(Roles)

    explicit TagFilterModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        qCDebug(lcModel) << "TagFilterModel created";
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
            return QStringLiteral("tags");
        case ValueRole:
            return row.tag;
        case TypeRole:
            return row.type;
        default:
            return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {{LabelRole, "label"}, {CountRole, "count"}, {IconRole, "icon"}
                , {ValueRole, "value"}, {TypeRole, "type"}};
    }

private:
    struct Row
    {
        int type = RealTag;
        QString tag;
        int count = 0;
    };

    void subscribe()
    {
        BitTorrent::Session *const session = BitTorrent::Session::instance();
        if (!session)
        {
            qCWarning(lcModel) << "TagFilterModel: no Session instance";
            return;
        }
        connect(session, &BitTorrent::Session::tagAdded, this, [this](const Tag &) { rebuild(); });
        connect(session, &BitTorrent::Session::tagRemoved, this, [this](const Tag &) { rebuild(); });
        connect(session, &BitTorrent::Session::torrentsLoaded, this, [this] { recount(); });
        connect(session, &BitTorrent::Session::torrentsUpdated, this, [this] { recount(); });
        connect(session, &BitTorrent::Session::torrentTagAdded, this, [this](BitTorrent::Torrent *, const Tag &) { recount(); });
        connect(session, &BitTorrent::Session::torrentTagRemoved, this, [this](BitTorrent::Torrent *, const Tag &) { recount(); });
        connect(session, &BitTorrent::Session::torrentAdded, this, [this](BitTorrent::Torrent *) { recount(); });
        connect(session, &BitTorrent::Session::torrentAboutToBeRemoved, this, [this](BitTorrent::Torrent *) { recount(); });
    }

    [[nodiscard]] static QString labelFor(const Row &row)
    {
        switch (row.type)
        {
        case AllTags:  return tr("All");
        case Untagged: return tr("Untagged");
        default:       return row.tag;
        }
    }

    void rebuild()
    {
        QList<Row> rows;
        rows.append({AllTags, QString(), 0});
        rows.append({Untagged, QString(), 0});

        BitTorrent::Session *const session = BitTorrent::Session::instance();
        if (session)
        {
            TagSet tags = session->tags();
            QStringList sorted;
            for (const Tag &tag : tags)
                sorted.append(tag.toString());
            sorted.sort(Qt::CaseInsensitive);
            for (const QString &tag : std::as_const(sorted))
                rows.append({RealTag, tag, 0});
        }

        beginResetModel();
        m_rows = rows;
        endResetModel();
        qCDebug(lcModel) << "TagFilterModel rebuilt;" << m_rows.size() << "rows";
        recount();
    }

    void recount()
    {
        BitTorrent::Session *const session = BitTorrent::Session::instance();
        const QList<BitTorrent::Torrent *> torrents = session ? session->torrents() : QList<BitTorrent::Torrent *> {};

        for (Row &row : m_rows)
            row.count = 0;

        for (const BitTorrent::Torrent *const t : torrents)
        {
            m_rows[AllTags].count += 1;
            const TagSet tags = t->tags();
            if (tags.isEmpty())
            {
                m_rows[Untagged].count += 1;
                continue;
            }
            for (int i = RealTag; i < m_rows.size(); ++i)
            {
                if (t->hasTag(Tag(m_rows[i].tag)))
                    m_rows[i].count += 1;
            }
        }

        if (!m_rows.isEmpty())
            emit dataChanged(index(0), index(rowCount() - 1), {LabelRole, CountRole, Qt::DisplayRole});
    }

    QList<Row> m_rows;
};
