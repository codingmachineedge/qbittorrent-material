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

#include <memory>

#include <QAbstractItemModel>
#include <QHash>
#include <QList>
#include <QString>

#include <qqmlintegration.h>

#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/logging.h"

/**
 * @file categoryfiltermodel.h
 * @brief @c CategoryFilterModel — the Categories tree of the transfer-list sidebar.
 *
 * A @c QAbstractItemModel tree. Row 0 is *All* categories, row 1 is
 * *Uncategorized* (empty category), and below them a nested tree built from the
 * `/`-separated category names reported by @c BitTorrent::Session. Every node
 * carries a live torrent count. Selecting a node drives
 * `TorrentFilterProxyModel::setCategoryFilter` (All -> `clearCategoryFilter`).
 */
class CategoryFilterModel final : public QAbstractItemModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum FilterType
    {
        AllCategories = 0,
        Uncategorized = 1,
        RealCategory = 2
    };
    Q_ENUM(FilterType)

    enum Roles
    {
        LabelRole = Qt::UserRole + 1, // "label"
        CountRole,                    // "count"
        IconRole,                     // "icon"
        ValueRole,                    // "value" (full category name)
        TypeRole                      // "type"  (FilterType)
    };
    Q_ENUM(Roles)

    explicit CategoryFilterModel(QObject *parent = nullptr)
        : QAbstractItemModel(parent)
    {
        qCDebug(lcModel) << "CategoryFilterModel created";
        subscribe();
        rebuild();
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = {}) const override
    {
        if ((column != 0) || (row < 0))
            return {};
        const Node *const parentNode = nodeFor(parent);
        if (!parentNode || (row >= parentNode->children.size()))
            return {};
        return createIndex(row, column, parentNode->children.at(row).get());
    }

    QModelIndex parent(const QModelIndex &child) const override
    {
        const Node *const node = static_cast<const Node *>(child.internalPointer());
        if (!node || !node->parent || (node->parent == m_root.get()))
            return {};
        return createIndex(node->parent->rowInParent(), 0, node->parent);
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        const Node *const node = nodeFor(parent);
        return node ? static_cast<int>(node->children.size()) : 0;
    }

    int columnCount(const QModelIndex & = {}) const override { return 1; }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        const Node *const node = static_cast<const Node *>(index.internalPointer());
        if (!node)
            return {};

        switch (role)
        {
        case LabelRole:
        case Qt::DisplayRole:
            return QStringLiteral("%1 (%2)").arg(node->displayName).arg(node->count);
        case CountRole:
            return node->count;
        case IconRole:
            return QStringLiteral("category");
        case ValueRole:
            return node->fullName;
        case TypeRole:
            return node->type;
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
    struct Node
    {
        int type = RealCategory;
        QString fullName;       ///< full `/`-joined category path ("" for specials)
        QString displayName;    ///< leaf label
        int count = 0;
        Node *parent = nullptr;
        QList<std::unique_ptr<Node>> children;

        [[nodiscard]] int rowInParent() const
        {
            if (!parent)
                return 0;
            for (int i = 0; i < parent->children.size(); ++i)
            {
                if (parent->children.at(i).get() == this)
                    return i;
            }
            return 0;
        }
    };

    [[nodiscard]] const Node *nodeFor(const QModelIndex &index) const
    {
        return index.isValid() ? static_cast<const Node *>(index.internalPointer()) : m_root.get();
    }

    void subscribe()
    {
        BitTorrent::Session *const session = BitTorrent::Session::instance();
        if (!session)
        {
            qCWarning(lcModel) << "CategoryFilterModel: no Session instance";
            return;
        }
        connect(session, &BitTorrent::Session::categoryAdded, this, [this](const QString &) { rebuild(); });
        connect(session, &BitTorrent::Session::categoryRemoved, this, [this](const QString &) { rebuild(); });
        connect(session, &BitTorrent::Session::torrentsLoaded, this, [this] { recount(); });
        connect(session, &BitTorrent::Session::torrentsUpdated, this, [this] { recount(); });
        connect(session, &BitTorrent::Session::torrentCategoryChanged, this, [this](BitTorrent::Torrent *, const QString &) { recount(); });
        connect(session, &BitTorrent::Session::torrentAdded, this, [this](BitTorrent::Torrent *) { recount(); });
        connect(session, &BitTorrent::Session::torrentAboutToBeRemoved, this, [this](BitTorrent::Torrent *) { recount(); });
    }

    void rebuild()
    {
        beginResetModel();

        m_root = std::make_unique<Node>();

        auto all = std::make_unique<Node>();
        all->type = AllCategories;
        all->displayName = tr("All");
        all->parent = m_root.get();
        m_root->children.push_back(std::move(all));

        auto uncat = std::make_unique<Node>();
        uncat->type = Uncategorized;
        uncat->displayName = tr("Uncategorized");
        uncat->parent = m_root.get();
        m_root->children.push_back(std::move(uncat));

        BitTorrent::Session *const session = BitTorrent::Session::instance();
        if (session)
        {
            QStringList categories = session->categories();
            categories.sort(Qt::CaseInsensitive);
            for (const QString &category : std::as_const(categories))
                ensureCategoryNode(category);
        }

        endResetModel();
        qCDebug(lcModel) << "CategoryFilterModel rebuilt";
        recount();
    }

    /// Create (if needed) the node chain for a `/`-separated category path.
    Node *ensureCategoryNode(const QString &fullName)
    {
        const QStringList parts = fullName.split(u'/', Qt::SkipEmptyParts);
        Node *parentNode = m_root.get();
        QString accumulated;
        for (const QString &part : parts)
        {
            accumulated = accumulated.isEmpty() ? part : (accumulated + u'/' + part);
            Node *found = nullptr;
            for (const std::unique_ptr<Node> &child : parentNode->children)
            {
                if ((child->type == RealCategory) && (child->fullName == accumulated))
                {
                    found = child.get();
                    break;
                }
            }
            if (!found)
            {
                auto node = std::make_unique<Node>();
                node->type = RealCategory;
                node->fullName = accumulated;
                node->displayName = part;
                node->parent = parentNode;
                found = node.get();
                parentNode->children.push_back(std::move(node));
            }
            parentNode = found;
        }
        return parentNode;
    }

    void recount()
    {
        if (!m_root)
            return;

        BitTorrent::Session *const session = BitTorrent::Session::instance();
        const QList<BitTorrent::Torrent *> torrents = session ? session->torrents() : QList<BitTorrent::Torrent *> {};

        resetCounts(m_root.get());
        for (const BitTorrent::Torrent *const t : torrents)
            addToCounts(m_root.get(), t);

        // Counts changed on (potentially) every node — refresh the whole tree.
        refreshCountsRecursive(QModelIndex());
    }

    static void resetCounts(Node *node)
    {
        node->count = 0;
        for (const std::unique_ptr<Node> &child : node->children)
            resetCounts(child.get());
    }

    static void addToCounts(Node *node, const BitTorrent::Torrent *torrent)
    {
        for (const std::unique_ptr<Node> &child : node->children)
        {
            switch (child->type)
            {
            case AllCategories:
                child->count += 1;
                break;
            case Uncategorized:
                if (torrent->category().isEmpty())
                    child->count += 1;
                break;
            default:
                if (torrent->belongsToCategory(child->fullName))
                    child->count += 1;
                addToCounts(child.get(), torrent);
                break;
            }
        }
    }

    void refreshCountsRecursive(const QModelIndex &parent)
    {
        const int rows = rowCount(parent);
        if (rows > 0)
            emit dataChanged(index(0, 0, parent), index(rows - 1, 0, parent), {LabelRole, CountRole, Qt::DisplayRole});
        for (int i = 0; i < rows; ++i)
            refreshCountsRecursive(index(i, 0, parent));
    }

    std::unique_ptr<Node> m_root;
};
