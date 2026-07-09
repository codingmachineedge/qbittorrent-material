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

#include <QList>

#include "base/global.h"
#include "rss_item.h"

namespace RSS
{
    class Session;

    /// A branch node in the RSS tree. Aggregates the articles and unread counts of
    /// its child items; the root of the tree is a `Folder` with an empty path.
    class Folder final : public Item
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(Folder)

        friend class Session;

        explicit Folder(const QString &path = {});
        ~Folder() override;

    public:
        QList<Article *> articles() const override;
        int unreadCount() const override;
        void markAsRead() override;
        void refresh() override;
        void updateFetchDelay() override;

        /// Direct children (folders and feeds) of this folder.
        QList<Item *> items() const;

        QJsonValue toJsonValue(bool withData = false) const override;

    private slots:
        void handleItemUnreadCountChanged();

    private:
        void cleanup() override;
        void addItem(Item *item);
        void removeItem(Item *item);

        QList<Item *> m_items;
    };
}
