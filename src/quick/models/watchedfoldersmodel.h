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

#include <optional>

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantMap>

#include <qqmlintegration.h>

#include "base/bittorrent/addtorrentparams.h"
#include "base/logging.h"
#include "base/torrentfileswatcher.h"
#include "base/utils/fs/path.h"

/**
 * @file watchedfoldersmodel.h
 * @brief List model backing the Options → Downloads "Automatically add torrents
 *        from:" watched-folders view.
 *
 * Wraps @c TorrentFilesWatcher (the engine singleton that scans folders for
 * dropped `.torrent` files). The model mirrors the Qt-Widgets `WatchedFoldersModel`
 * behaviour: a single visible column whose display value is the folder path, plus
 * per-folder options (recursive mode + a torrent-parameters override) exposed as
 * roles / helper accessors.
 *
 * Editing is **staged** — `addFolder`/`setFolderOptions`/`removeFolder` mutate an
 * in-memory copy and mark the model dirty; nothing is persisted until `apply()`
 * is called from the Options dialog's OK/Apply button. `reset()` reloads the live
 * watcher state, discarding staged edits. The model subscribes to the watcher's
 * `watchedFolderSet`/`watchedFolderRemoved` signals so external changes reflect
 * live (it never polls).
 */
class WatchedFoldersModel final : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    /// Named roles consumed by QML delegates (`model.<role>`).
    enum Roles
    {
        PathRole = Qt::UserRole + 1,   ///< "path"        — folder path (also the display value)
        RecursiveRole,                 ///< "recursive"   — bool, scan subfolders
        SavePathRole,                  ///< "savePath"    — override save path (may be empty)
        DownloadPathRole,              ///< "downloadPath"— override incomplete path
        UseAutoTMMRole,                ///< "useAutoTMM"  — tri-state int (-1 unset / 0 / 1)
        CategoryRole                   ///< "category"    — override category
    };
    Q_ENUM(Roles)

    explicit WatchedFoldersModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        qCDebug(lcModel) << "WatchedFoldersModel: constructing";
        reload();

        auto *watcher = TorrentFilesWatcher::instance();
        connect(watcher, &TorrentFilesWatcher::watchedFolderSet, this
                , [this](const Path &, const TorrentFilesWatcher::WatchedFolderOptions &)
        {
            // Only react to external changes once we are clean; staged edits own
            // the model while the user is editing.
            if (!m_modified)
                reload();
        });
        connect(watcher, &TorrentFilesWatcher::watchedFolderRemoved, this, [this](const Path &)
        {
            if (!m_modified)
                reload();
        });
    }

    // --- QAbstractListModel ------------------------------------------------

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
    }

    int columnCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : 1;
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role) const override
    {
        if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole) && (section == 0))
            return tr("Watched Folder");
        return {};
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || (index.row() < 0) || (index.row() >= static_cast<int>(m_rows.size())))
            return {};

        const Row &row = m_rows.at(index.row());
        switch (role)
        {
        case Qt::DisplayRole:
        case PathRole:
            return row.path;
        case RecursiveRole:
            return row.options.recursive;
        case SavePathRole:
            return row.options.addTorrentParams.savePath.toString();
        case DownloadPathRole:
            return row.options.addTorrentParams.downloadPath.toString();
        case UseAutoTMMRole:
            return triToInt(row.options.addTorrentParams.useAutoTMM);
        case CategoryRole:
            return row.options.addTorrentParams.category;
        default:
            return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {PathRole, "path"},
            {RecursiveRole, "recursive"},
            {SavePathRole, "savePath"},
            {DownloadPathRole, "downloadPath"},
            {UseAutoTMMRole, "useAutoTMM"},
            {CategoryRole, "category"}
        };
    }

    bool isModified() const { return m_modified; }

    // --- QML API -----------------------------------------------------------

    /// The folder path shown at @p row (empty for an out-of-range row).
    Q_INVOKABLE QString folderPath(int row) const
    {
        if ((row < 0) || (row >= static_cast<int>(m_rows.size())))
            return {};
        return m_rows.at(row).path;
    }

    /// Whether the folder at @p row is scanned recursively.
    Q_INVOKABLE bool isRecursive(int row) const
    {
        if ((row < 0) || (row >= static_cast<int>(m_rows.size())))
            return false;
        return m_rows.at(row).options.recursive;
    }

    /// The full option set for @p row as a QML-friendly map (seeds the edit dialog).
    Q_INVOKABLE QVariantMap folderOptions(int row) const
    {
        if ((row < 0) || (row >= static_cast<int>(m_rows.size())))
            return {};
        return toMap(m_rows.at(row).options);
    }

    /// Stage a new watched folder. No-op (returns false) if @p path is empty or
    /// already watched. Persisted on `apply()`.
    Q_INVOKABLE bool addFolder(const QString &path, const QVariantMap &options = {})
    {
        const QString trimmed = path.trimmed();
        if (trimmed.isEmpty())
        {
            qCWarning(lcModel) << "WatchedFoldersModel: refusing to add empty folder path";
            return false;
        }
        if (indexOfPath(trimmed) >= 0)
        {
            qCWarning(lcModel) << "WatchedFoldersModel: folder already watched" << trimmed;
            return false;
        }

        qCInfo(lcModel) << "WatchedFoldersModel: staging add" << trimmed;
        beginInsertRows({}, static_cast<int>(m_rows.size()), static_cast<int>(m_rows.size()));
        m_rows.append({trimmed, fromMap(options)});
        endInsertRows();
        markModified();
        emit countChanged();
        return true;
    }

    /// Replace the options for the folder at @p row (recursive + torrent params).
    Q_INVOKABLE void setFolderOptions(int row, const QVariantMap &options)
    {
        if ((row < 0) || (row >= static_cast<int>(m_rows.size())))
            return;
        qCInfo(lcModel) << "WatchedFoldersModel: staging option change for" << m_rows.at(row).path;
        m_rows[row].options = fromMap(options);
        const QModelIndex idx = index(row, 0);
        emit dataChanged(idx, idx);
        markModified();
    }

    /// Stage removal of the folder at @p row. Persisted on `apply()`.
    Q_INVOKABLE void removeFolder(int row)
    {
        if ((row < 0) || (row >= static_cast<int>(m_rows.size())))
            return;
        qCInfo(lcModel) << "WatchedFoldersModel: staging remove" << m_rows.at(row).path;
        beginRemoveRows({}, row, row);
        m_rows.removeAt(row);
        endRemoveRows();
        markModified();
        emit countChanged();
    }

    /// Commit staged edits to the engine watcher (diff add/change/remove).
    Q_INVOKABLE void apply()
    {
        if (!m_modified)
        {
            qCDebug(lcModel) << "WatchedFoldersModel: apply() with no changes";
            return;
        }

        qCInfo(lcModel) << "WatchedFoldersModel: applying" << m_rows.size() << "watched folder(s)";
        auto *watcher = TorrentFilesWatcher::instance();
        const QHash<Path, TorrentFilesWatcher::WatchedFolderOptions> existing = watcher->folders();

        // Remove folders no longer present.
        for (auto it = existing.cbegin(); it != existing.cend(); ++it)
        {
            if (indexOfPath(it.key().toString()) < 0)
            {
                qCDebug(lcModel) << "WatchedFoldersModel: removing" << it.key().toString();
                watcher->removeWatchedFolder(it.key());
            }
        }

        // Add / update the rest.
        for (const Row &row : m_rows)
        {
            qCDebug(lcModel) << "WatchedFoldersModel: setting" << row.path;
            watcher->setWatchedFolder(Path(row.path), row.options);
        }

        m_modified = false;
        emit modifiedChanged();
    }

    /// Discard staged edits and reload the live watcher state.
    Q_INVOKABLE void reset()
    {
        qCDebug(lcModel) << "WatchedFoldersModel: reset() — reloading from watcher";
        reload();
    }

signals:
    void modifiedChanged();
    void countChanged();

private:
    struct Row
    {
        QString path;
        TorrentFilesWatcher::WatchedFolderOptions options;
    };

    void reload()
    {
        beginResetModel();
        m_rows.clear();
        const QHash<Path, TorrentFilesWatcher::WatchedFolderOptions> folders =
            TorrentFilesWatcher::instance()->folders();
        for (auto it = folders.cbegin(); it != folders.cend(); ++it)
            m_rows.append({it.key().toString(), it.value()});
        endResetModel();

        if (m_modified)
        {
            m_modified = false;
            emit modifiedChanged();
        }
        emit countChanged();
    }

    void markModified()
    {
        if (!m_modified)
        {
            m_modified = true;
            emit modifiedChanged();
        }
    }

    int indexOfPath(const QString &path) const
    {
        for (int i = 0; i < static_cast<int>(m_rows.size()); ++i)
        {
            if (m_rows.at(i).path == path)
                return i;
        }
        return -1;
    }

    static int triToInt(const std::optional<bool> &value)
    {
        if (!value.has_value())
            return -1;
        return value.value() ? 1 : 0;
    }

    static std::optional<bool> intToTri(int value)
    {
        if (value < 0)
            return std::nullopt;
        return (value != 0);
    }

    static QVariantMap toMap(const TorrentFilesWatcher::WatchedFolderOptions &options)
    {
        const BitTorrent::AddTorrentParams &p = options.addTorrentParams;
        QVariantMap map;
        map["recursive"] = options.recursive;
        map["savePath"] = p.savePath.toString();
        map["downloadPath"] = p.downloadPath.toString();
        map["useDownloadPath"] = triToInt(p.useDownloadPath);
        map["useAutoTMM"] = triToInt(p.useAutoTMM);
        map["category"] = p.category;
        return map;
    }

    static TorrentFilesWatcher::WatchedFolderOptions fromMap(const QVariantMap &map)
    {
        TorrentFilesWatcher::WatchedFolderOptions options;
        options.recursive = map.value("recursive", false).toBool();

        BitTorrent::AddTorrentParams &p = options.addTorrentParams;
        p.savePath = Path(map.value("savePath").toString());
        p.downloadPath = Path(map.value("downloadPath").toString());
        p.useDownloadPath = intToTri(map.value("useDownloadPath", -1).toInt());
        p.useAutoTMM = intToTri(map.value("useAutoTMM", -1).toInt());
        p.category = map.value("category").toString();
        return options;
    }

    QList<Row> m_rows;
    bool m_modified = false;
};
