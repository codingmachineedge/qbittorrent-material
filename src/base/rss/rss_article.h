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

#include <QDateTime>
#include <QObject>
#include <QString>
#include <QVariantHash>

namespace RSS
{
    class Feed;

    /// A single RSS article/item belonging to a `Feed`. Immutable value data with a
    /// read/unread flag. The string `Key*` constants are the JSON/persistence field
    /// names and the keys expected by `AutoDownloadRule::matches(articleData)`.
    class Article final : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(Article)

        friend class Feed;

        Article(Feed *feed, const QVariantHash &varHash);

    public:
        static const QString KeyId;
        static const QString KeyDate;
        static const QString KeyTitle;
        static const QString KeyAuthor;
        static const QString KeyDescription;
        static const QString KeyTorrentURL;
        static const QString KeyLink;
        static const QString KeyIsRead;

        Feed *feed() const;
        QString guid() const;
        QDateTime date() const;
        QString title() const;
        QString author() const;
        QString description() const;
        QString torrentUrl() const;
        QString link() const;
        bool isRead() const;
        /// The full field map (used for auto-download rule matching and persistence).
        QVariantHash data() const;

        void markAsRead();

        static bool articleDateRecentThan(const Article *article, const QDateTime &date);

    signals:
        void read(Article *article = nullptr);

    private:
        Feed *m_feed = nullptr;
        QString m_guid;
        QDateTime m_date;
        QString m_title;
        QString m_author;
        QString m_description;
        QString m_torrentURL;
        QString m_link;
        bool m_isRead = false;
        QVariantHash m_data;
    };
}
