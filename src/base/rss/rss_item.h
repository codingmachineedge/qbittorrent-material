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
#include <QObject>

/// RSS subsystem. `Session` owns a tree of `Item`s; concrete items are `Folder`
/// (a branch) and `Feed` (a leaf holding `Article`s). `AutoDownloader` matches new
/// articles against `AutoDownloadRule`s. All types live in the `RSS` namespace and
/// are bridged to QML via `RSSController` and the RSS models (subscribe, never poll).
namespace RSS
{
    class Article;
    class Folder;
    class Session;

    /// Abstract base for any node in the RSS tree. Identified by a `/`-separated
    /// path; exposes aggregate unread state and the article list of its subtree.
    class Item : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(Item)

        friend class Folder;
        friend class Session;

    public:
        virtual QList<Article *> articles() const = 0;
        virtual int unreadCount() const = 0;
        virtual void markAsRead() = 0;
        virtual void refresh() = 0;
        virtual void updateFetchDelay() = 0;

        /// Full tree path (e.g. `folder/subfolder/Feed name`).
        QString path() const;
        /// Leaf name (the last path segment).
        QString name() const;

        virtual QJsonValue toJsonValue(bool withData = false) const = 0;

        static const QChar PathSeparator;

        static bool isValidPath(const QString &path);
        static QString joinPath(const QString &path1, const QString &path2);
        static QStringList expandPath(const QString &path);
        static QString parentPath(const QString &path);
        static QString relativeName(const QString &path);

    signals:
        void pathChanged(Item *item = nullptr);
        void unreadCountChanged(Item *item = nullptr);
        void aboutToBeDestroyed(Item *item = nullptr);
        void newArticle(Article *article);
        void articleRead(Article *article);
        void articleAboutToBeRemoved(Article *article);

    protected:
        explicit Item(const QString &path);
        ~Item() override = default;

        virtual void cleanup() = 0;

    private:
        void setPath(const QString &path);

        QString m_path;
    };
}
