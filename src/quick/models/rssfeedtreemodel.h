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

#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QPointer>
#include <QQmlEngine>
#include <QString>

#include "base/logging.h"
#include "base/rss/rss_feed.h"
#include "base/rss/rss_folder.h"
#include "base/rss/rss_item.h"
#include "base/rss/rss_session.h"

/**
 * @file rssfeedtreemodel.h
 * @brief Tree model mirroring the @c RSS::Session item hierarchy, with two
 *        pinned virtual nodes ("All" and "Unread (N)") sorted above every real
 *        feed/folder.
 *
 * The model is a thin, self-rebuilding projection of the engine: structural
 * signals (@c itemAdded / @c itemAboutToBeRemoved / @c itemPathChanged) trigger a
 * reset, while cheap per-node changes (unread counts, feed loading/error state,
 * favicon) emit a targeted @c dataChanged so selection survives. Every node
 * exposes the roles the @c FeedsTree QML view reads by name.
 *
 * Header-only (methods inline) so it can be registered as a @c QML_ELEMENT with
 * no separate translation unit; @c moc still processes the @c Q_OBJECT.
 */
class RSSFeedTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    /// Kind of a tree node — the two sticky nodes are virtual aggregates.
    enum StickyKind
    {
        RegularNode = 0,
        AllArticlesNode = 1,
        UnreadArticlesNode = 2
    };
    Q_ENUM(StickyKind)

    enum Roles
    {
        NameRole = Qt::UserRole + 1, ///< "name" — display name (no count)
        PathRole,                    ///< "path" — engine item path ("" for sticky)
        UnreadCountRole,             ///< "unreadCount"
        IsFeedRole,                  ///< "isFeed"
        IsFolderRole,                ///< "isFolder"
        IsStickyRole,                ///< "isSticky"
        StickyKindRole,              ///< "stickyKind" (see StickyKind)
        HasErrorRole,                ///< "hasError"
        IsLoadingRole,               ///< "isLoading"
        IconPathRole,                ///< "iconPath" — favicon file path (feeds)
        UrlRole                      ///< "url" — feed URL (feeds only)
    };

    explicit RSSFeedTreeModel(QObject *parent = nullptr)
        : QAbstractItemModel(parent)
    {
        m_root = new Node;
        rebuild();

        auto *session = RSS::Session::instance();
        connect(session, &RSS::Session::itemAdded, this, [this](RSS::Item *) { rebuild(); });
        connect(session, &RSS::Session::itemAboutToBeRemoved, this, [this](RSS::Item *) {
            // Defer the rebuild so the engine has finished detaching the item.
            QMetaObject::invokeMethod(this, [this] { rebuild(); }, Qt::QueuedConnection);
        });
        connect(session, &RSS::Session::itemPathChanged, this, [this](RSS::Item *) { rebuild(); });
        connect(session, &RSS::Session::feedStateChanged, this, [this](RSS::Feed *feed) {
            if (feed)
                touchNode(feed->path());
        });
        connect(session, &RSS::Session::feedIconLoaded, this, [this](RSS::Feed *feed) {
            if (feed)
                touchNode(feed->path());
        });
        qCDebug(lcModel) << "RSSFeedTreeModel constructed";
    }

    ~RSSFeedTreeModel() override
    {
        disconnectItems();
        deleteNode(m_root);
        qCDebug(lcModel) << "RSSFeedTreeModel destroyed";
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {NameRole, "name"}, {PathRole, "path"}, {UnreadCountRole, "unreadCount"},
            {IsFeedRole, "isFeed"}, {IsFolderRole, "isFolder"}, {IsStickyRole, "isSticky"},
            {StickyKindRole, "stickyKind"}, {HasErrorRole, "hasError"}, {IsLoadingRole, "isLoading"},
            {IconPathRole, "iconPath"}, {UrlRole, "url"}
        };
    }

    int columnCount(const QModelIndex & = {}) const override { return 1; }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        const Node *node = parent.isValid() ? static_cast<const Node *>(parent.internalPointer()) : m_root;
        return node ? int(node->children.size()) : 0;
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override
    {
        if (column != 0)
            return {};
        const Node *parentNode = parent.isValid() ? static_cast<const Node *>(parent.internalPointer()) : m_root;
        if (!parentNode || (row < 0) || (row >= int(parentNode->children.size())))
            return {};
        return createIndex(row, column, parentNode->children.at(row));
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        if (!child.isValid())
            return {};
        const Node *node = static_cast<const Node *>(child.internalPointer());
        Node *parentNode = node ? node->parent : nullptr;
        if (!parentNode || (parentNode == m_root))
            return {};
        Node *grand = parentNode->parent;
        const int row = grand ? int(grand->children.indexOf(parentNode)) : 0;
        return createIndex(row, 0, parentNode);
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid())
            return {};
        const Node *node = static_cast<const Node *>(index.internalPointer());
        if (!node)
            return {};

        switch (role)
        {
        case NameRole:
        case Qt::DisplayRole:
            return node->name;
        case PathRole:
            return node->path;
        case UnreadCountRole:
            return currentUnread(node);
        case IsFeedRole:
            return node->isFeed;
        case IsFolderRole:
            return node->isFolder;
        case IsStickyRole:
            return node->sticky != RegularNode;
        case StickyKindRole:
            return node->sticky;
        case HasErrorRole:
            return feedFlag(node, &RSS::Feed::hasError);
        case IsLoadingRole:
            return feedFlag(node, &RSS::Feed::isLoading);
        case IconPathRole:
            return node->iconPath;
        case UrlRole:
            return node->url;
        default:
            return {};
        }
    }

private:
    struct Node
    {
        QString name;
        QString path;               ///< engine item path ("" for sticky/root)
        QString url;                ///< feed URL
        QString iconPath;           ///< favicon path
        bool isFeed = false;
        bool isFolder = false;
        int sticky = RegularNode;
        Node *parent = nullptr;
        QList<Node *> children;
    };

    // ---- Live-value helpers (read the engine so counts stay fresh) ---------

    int currentUnread(const Node *node) const
    {
        if (node->sticky == AllArticlesNode)
            return 0; // "All" shows no count
        if (node->sticky == UnreadArticlesNode)
            return RSS::Session::instance()->rootFolder()->unreadCount();
        if (auto *item = RSS::Session::instance()->itemByPath(node->path))
            return item->unreadCount();
        return 0;
    }

    template <typename Fn>
    bool feedFlag(const Node *node, Fn fn) const
    {
        if (!node->isFeed)
            return false;
        auto *feed = qobject_cast<RSS::Feed *>(RSS::Session::instance()->itemByPath(node->path));
        return feed ? (feed->*fn)() : false;
    }

    // ---- Tree (re)building -------------------------------------------------

    void rebuild()
    {
        beginResetModel();
        disconnectItems();

        deleteChildren(m_root);
        m_byPath.clear();

        // Sticky nodes first — they always sort above real items.
        auto *all = new Node;
        all->name = tr("All");
        all->sticky = AllArticlesNode;
        all->parent = m_root;
        m_root->children.append(all);

        auto *unread = new Node;
        unread->name = tr("Unread");
        unread->sticky = UnreadArticlesNode;
        unread->parent = m_root;
        m_root->children.append(unread);

        RSS::Folder *root = RSS::Session::instance()->rootFolder();
        if (root)
        {
            addFolderChildren(root, m_root);
            connectItem(root); // for the Unread sticky count
        }

        endResetModel();
        qCDebug(lcModel) << "RSSFeedTreeModel rebuilt:" << m_byPath.size() << "real nodes";
    }

    void addFolderChildren(RSS::Folder *folder, Node *parentNode)
    {
        for (RSS::Item *item : folder->items())
        {
            auto *node = new Node;
            node->name = item->name();
            node->path = item->path();
            node->parent = parentNode;

            if (auto *feed = qobject_cast<RSS::Feed *>(item))
            {
                node->isFeed = true;
                node->url = feed->url();
                node->iconPath = feed->iconPath().toString();
            }
            else if (auto *sub = qobject_cast<RSS::Folder *>(item))
            {
                node->isFolder = true;
                parentNode->children.append(node);
                m_byPath.insert(node->path, node);
                connectItem(item);
                addFolderChildren(sub, node);
                continue;
            }

            parentNode->children.append(node);
            m_byPath.insert(node->path, node);
            connectItem(item);
        }
    }

    // ---- Targeted refresh (unread / icon / state) --------------------------

    void touchNode(const QString &path)
    {
        Node *node = m_byPath.value(path, nullptr);
        if (!node)
            return;
        if (node->isFeed)
        {
            if (auto *feed = qobject_cast<RSS::Feed *>(RSS::Session::instance()->itemByPath(path)))
                node->iconPath = feed->iconPath().toString();
        }
        const QModelIndex idx = indexForNode(node);
        if (idx.isValid())
            emit dataChanged(idx, idx);
    }

    void onItemUnreadChanged(RSS::Item *item)
    {
        if (!item)
            return;
        RSS::Folder *root = RSS::Session::instance()->rootFolder();
        if (item == root)
        {
            // Refresh the "Unread" sticky node (row 1).
            const QModelIndex idx = index(UnreadArticlesNode, 0);
            if (idx.isValid())
                emit dataChanged(idx, idx);
            return;
        }
        touchNode(item->path());
    }

    QModelIndex indexForNode(Node *node) const
    {
        if (!node || !node->parent)
            return {};
        const int row = int(node->parent->children.indexOf(node));
        if (row < 0)
            return {};
        return createIndex(row, 0, node);
    }

    // ---- Per-item signal bookkeeping ---------------------------------------

    void connectItem(RSS::Item *item)
    {
        connect(item, &RSS::Item::unreadCountChanged, this, [this, item] { onItemUnreadChanged(item); });
        m_connectedItems.append(item);
    }

    void disconnectItems()
    {
        for (const QPointer<RSS::Item> &item : m_connectedItems)
        {
            if (item)
                disconnect(item, nullptr, this, nullptr);
        }
        m_connectedItems.clear();
    }

    void deleteChildren(Node *node)
    {
        for (Node *child : node->children)
            deleteNode(child);
        node->children.clear();
    }

    void deleteNode(Node *node)
    {
        if (!node)
            return;
        for (Node *child : node->children)
            deleteNode(child);
        delete node;
    }

    Node *m_root = nullptr;
    QHash<QString, Node *> m_byPath;
    QList<QPointer<RSS::Item>> m_connectedItems;
};
