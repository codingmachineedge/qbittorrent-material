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
#include <QByteArray>
#include <QHash>
#include <QList>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

#include "base/search/searchpluginmanager.h"
#include "base/logging.h"

/**
 * @file searchpluginsmodel.h
 * @brief List model of installed search plugins, backing the Search Plugins
 *        dialog table (Name+icon / Version / Url / Enabled).
 *
 * It subscribes to @c SearchPluginManager's install/uninstall/update/enable
 * signals and refreshes live — it never polls. Mutations (enable, install,
 * update, uninstall) are performed through @c SearchController so all logging
 * and user feedback stays in one place; this model is read-only state.
 *
 * QML-creatable: the dialog does `SearchPluginsModel { id: pluginsModel }`.
 */
class SearchPluginsModel final : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles
    {
        PluginIdRole = Qt::UserRole + 1, ///< "pluginId"  — short id / key
        FullNameRole,                    ///< "fullName"  — human display name
        VersionRole,                     ///< "version"   — e.g. "2.11"
        UrlRole,                         ///< "url"        — engine site URL
        EnabledRole,                     ///< "enabled"    — bool
        IconPathRole                     ///< "iconPath"   — local favicon (file URL)
    };
    Q_ENUM(Roles)

    explicit SearchPluginsModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        if (auto *mgr = SearchPluginManager::instance())
        {
            connect(mgr, &SearchPluginManager::pluginInstalled, this, [this] { reload(); });
            connect(mgr, &SearchPluginManager::pluginUninstalled, this, [this] { reload(); });
            connect(mgr, &SearchPluginManager::pluginUpdated, this, [this] { reload(); });
            connect(mgr, &SearchPluginManager::pluginEnabled, this,
                    [this](const QString &name, bool) { updateOne(name); });
        }
        reload();
        qCDebug(lcSearch) << "SearchPluginsModel constructed with" << m_ids.size() << "plugins";
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_ids.size());
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        const int row = index.row();
        if ((row < 0) || (row >= m_ids.size()))
            return {};

        auto *mgr = SearchPluginManager::instance();
        if (!mgr)
            return {};
        const SearchPluginInfo *info = mgr->pluginInfo(m_ids.at(row));
        if (!info)
            return {};

        switch (role)
        {
        case PluginIdRole: return info->name;
        case FullNameRole: return info->fullName;
        case VersionRole:  return info->version.toString();
        case UrlRole:      return info->url;
        case EnabledRole:  return info->enabled;
        case IconPathRole:
            return info->iconPath.isEmpty()
                       ? QString()
                       : QUrl::fromLocalFile(info->iconPath.data()).toString();
        default:           return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {PluginIdRole, "pluginId"},
            {FullNameRole, "fullName"},
            {VersionRole, "version"},
            {UrlRole, "url"},
            {EnabledRole, "enabled"},
            {IconPathRole, "iconPath"}
        };
    }

    /// Plugin id for a row (used by context menus / selection).
    Q_INVOKABLE QString pluginId(int row) const
    {
        return ((row >= 0) && (row < m_ids.size())) ? m_ids.at(row) : QString();
    }

    /// Enabled state for a row (lets a cell delegate colour the whole row).
    Q_INVOKABLE bool isEnabled(int row) const
    {
        auto *mgr = SearchPluginManager::instance();
        if (!mgr || (row < 0) || (row >= m_ids.size()))
            return false;
        const SearchPluginInfo *info = mgr->pluginInfo(m_ids.at(row));
        return info && info->enabled;
    }

    /// Favicon file URL for a row (empty string when none).
    Q_INVOKABLE QString iconPathAt(int row) const
    {
        auto *mgr = SearchPluginManager::instance();
        if (!mgr || (row < 0) || (row >= m_ids.size()))
            return {};
        const SearchPluginInfo *info = mgr->pluginInfo(m_ids.at(row));
        if (!info || info->iconPath.isEmpty())
            return {};
        return QUrl::fromLocalFile(info->iconPath.data()).toString();
    }

    /// Rebuild the id list from the manager (sorted by full name, locale-aware).
    Q_INVOKABLE void reload()
    {
        beginResetModel();
        m_ids.clear();
        if (auto *mgr = SearchPluginManager::instance())
        {
            m_ids = mgr->allPlugins();
            std::sort(m_ids.begin(), m_ids.end(), [mgr](const QString &a, const QString &b) {
                return QString::localeAwareCompare(mgr->pluginFullName(a), mgr->pluginFullName(b)) < 0;
            });
        }
        endResetModel();
        emit countChanged();
        qCDebug(lcSearch) << "SearchPluginsModel reloaded:" << m_ids.size() << "plugins";
    }

signals:
    void countChanged();

private:
    void updateOne(const QString &name)
    {
        const int row = static_cast<int>(m_ids.indexOf(name));
        if (row < 0)
        {
            reload();
            return;
        }
        const QModelIndex idx = index(row, 0);
        emit dataChanged(idx, idx, {EnabledRole});
    }

    QStringList m_ids;
};
