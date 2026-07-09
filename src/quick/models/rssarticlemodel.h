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

#include <QAbstractListModel>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QLocale>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QVariantMap>

#include "base/logging.h"
#include "base/rss/rss_article.h"
#include "base/rss/rss_feed.h"
#include "base/rss/rss_folder.h"
#include "base/rss/rss_item.h"
#include "base/rss/rss_session.h"

/**
 * @file rssarticlemodel.h
 * @brief Flat list model of the articles belonging to the currently-selected
 *        feed / folder / sticky node.
 *
 * The current node is set with @c setFeed(path, unreadOnly); an empty path maps
 * to the root folder (the "All"/"Unread" sticky views). A case-insensitive
 * title filter is applied on top. The model subscribes to the node's
 * @c newArticle / @c articleRead / @c articleAboutToBeRemoved signals so it stays
 * live — new articles are inserted at the top.
 *
 * Read/unread styling is left to QML (via the @c isRead role and the
 * DESIGN_SYSTEM RSS colors); this model only reports state. Article-level verbs
 * that touch the engine (download torrent, open URL) are performed by
 * @c RSSController using the strings this model exposes via @c get(row); marking
 * read is done here because it owns the @c RSS::Article pointers.
 *
 * Header-only so it registers as a @c QML_ELEMENT with no separate TU.
 */
class RSSArticleModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    /// Number of articles currently listed (bindable in QML headers/labels).
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles
    {
        TitleRole = Qt::UserRole + 1, ///< "title"
        GuidRole,                     ///< "guid"
        FeedNameRole,                 ///< "feedName"
        FeedUrlRole,                  ///< "feedUrl"
        IsReadRole,                   ///< "isRead"
        DateTextRole,                 ///< "dateText" — localized short date
        AuthorRole,                   ///< "author"
        DescriptionRole,              ///< "description"
        TorrentUrlRole,               ///< "torrentUrl"
        LinkRole,                     ///< "link"
        HasTorrentRole,               ///< "hasTorrent"
        HasLinkRole                   ///< "hasLink"
    };

    explicit RSSArticleModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        qCDebug(lcModel) << "RSSArticleModel constructed";
    }

    ~RSSArticleModel() override
    {
        qCDebug(lcModel) << "RSSArticleModel destroyed";
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {TitleRole, "title"}, {GuidRole, "guid"}, {FeedNameRole, "feedName"},
            {FeedUrlRole, "feedUrl"}, {IsReadRole, "isRead"}, {DateTextRole, "dateText"},
            {AuthorRole, "author"}, {DescriptionRole, "description"}, {TorrentUrlRole, "torrentUrl"},
            {LinkRole, "link"}, {HasTorrentRole, "hasTorrent"}, {HasLinkRole, "hasLink"}
        };
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : int(m_articles.size());
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || (index.row() < 0) || (index.row() >= m_articles.size()))
            return {};
        RSS::Article *a = m_articles.at(index.row());
        if (!a)
            return {};

        switch (role)
        {
        case TitleRole:
        case Qt::DisplayRole:
            return a->title();
        case GuidRole:
            return a->guid();
        case FeedNameRole:
            return a->feed() ? a->feed()->title() : QString();
        case FeedUrlRole:
            return a->feed() ? a->feed()->url() : QString();
        case IsReadRole:
            return a->isRead();
        case DateTextRole:
            return a->date().isValid()
                ? QLocale::system().toString(a->date().toLocalTime(), QLocale::ShortFormat)
                : QString();
        case AuthorRole:
            return a->author();
        case DescriptionRole:
            return a->description();
        case TorrentUrlRole:
            return a->torrentUrl();
        case LinkRole:
            return a->link();
        case HasTorrentRole:
            return !a->torrentUrl().isEmpty();
        case HasLinkRole:
            return !a->link().isEmpty();
        default:
            return {};
        }
    }

    /// Select the node whose articles are listed. Empty @p path = root folder.
    Q_INVOKABLE void setFeed(const QString &path, const bool unreadOnly)
    {
        RSS::Item *item = path.isEmpty()
            ? static_cast<RSS::Item *>(RSS::Session::instance()->rootFolder())
            : RSS::Session::instance()->itemByPath(path);

        qCDebug(lcModel) << "RSSArticleModel::setFeed path=" << path << "unreadOnly=" << unreadOnly;
        m_unreadOnly = unreadOnly;

        if (m_item)
            disconnect(m_item, nullptr, this, nullptr);
        m_item = item;
        if (m_item)
        {
            connect(m_item, &RSS::Item::newArticle, this, &RSSArticleModel::onNewArticle);
            connect(m_item, &RSS::Item::articleRead, this, &RSSArticleModel::onArticleRead);
            connect(m_item, &RSS::Item::articleAboutToBeRemoved, this, &RSSArticleModel::onArticleRemoved);
        }
        reload();
    }

    /// Apply a case-insensitive title substring filter ("" clears it).
    Q_INVOKABLE void setFilter(const QString &filter)
    {
        if (filter == m_filter)
            return;
        qCDebug(lcModel) << "RSSArticleModel::setFilter" << filter;
        m_filter = filter;
        reload();
    }

    /// Snapshot of the article at @p row (strings for the controller + preview).
    Q_INVOKABLE QVariantMap get(const int row) const
    {
        QVariantMap m;
        if ((row < 0) || (row >= m_articles.size()))
            return m;
        RSS::Article *a = m_articles.at(row);
        if (!a)
            return m;
        m.insert(QStringLiteral("title"), a->title());
        m.insert(QStringLiteral("guid"), a->guid());
        m.insert(QStringLiteral("feedName"), a->feed() ? a->feed()->title() : QString());
        m.insert(QStringLiteral("author"), a->author());
        m.insert(QStringLiteral("description"), a->description());
        m.insert(QStringLiteral("torrentUrl"), a->torrentUrl());
        m.insert(QStringLiteral("link"), a->link());
        m.insert(QStringLiteral("isRead"), a->isRead());
        m.insert(QStringLiteral("dateText"), a->date().isValid()
            ? QLocale::system().toString(a->date().toLocalTime(), QLocale::ShortFormat)
            : QString());
        return m;
    }

    /// Mark the article at @p row read (used on navigation-away and explicitly).
    Q_INVOKABLE void markRead(const int row)
    {
        if ((row < 0) || (row >= m_articles.size()))
            return;
        if (RSS::Article *a = m_articles.at(row); a && !a->isRead())
        {
            qCDebug(lcModel) << "RSSArticleModel::markRead row" << row;
            a->markAsRead();
        }
    }

signals:
    void countChanged();

private slots:
    void onNewArticle(RSS::Article *article)
    {
        if (!article || !passesFilter(article))
            return;
        beginInsertRows({}, 0, 0);
        m_articles.prepend(article);
        endInsertRows();
        emit countChanged();
    }

    void onArticleRead(RSS::Article *article)
    {
        const int row = int(m_articles.indexOf(article));
        if (row < 0)
        {
            // In "unread only" mode a freshly-read article must disappear.
            return;
        }
        if (m_unreadOnly)
        {
            beginRemoveRows({}, row, row);
            m_articles.removeAt(row);
            endRemoveRows();
            emit countChanged();
        }
        else
        {
            const QModelIndex idx = index(row);
            emit dataChanged(idx, idx, {IsReadRole});
        }
    }

    void onArticleRemoved(RSS::Article *article)
    {
        const int row = int(m_articles.indexOf(article));
        if (row < 0)
            return;
        beginRemoveRows({}, row, row);
        m_articles.removeAt(row);
        endRemoveRows();
        emit countChanged();
    }

private:
    bool passesFilter(RSS::Article *a) const
    {
        if (!a)
            return false;
        if (m_unreadOnly && a->isRead())
            return false;
        if (!m_filter.isEmpty() && !a->title().contains(m_filter, Qt::CaseInsensitive))
            return false;
        return true;
    }

    void reload()
    {
        beginResetModel();
        m_articles.clear();
        if (m_item)
        {
            const QList<RSS::Article *> all = m_item->articles();
            for (RSS::Article *a : all)
            {
                if (passesFilter(a))
                    m_articles.append(a);
            }
        }
        endResetModel();
        emit countChanged();
        qCDebug(lcModel) << "RSSArticleModel reloaded:" << m_articles.size() << "articles";
    }

    QPointer<RSS::Item> m_item;
    QList<RSS::Article *> m_articles;
    bool m_unreadOnly = false;
    QString m_filter;
};
