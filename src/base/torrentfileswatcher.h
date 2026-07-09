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

#include <QHash>

#include "base/bittorrent/addtorrentparams.h"
#include "base/bittorrent/torrentdescriptor.h"
#include "base/utils/fs/path.h"
#include "base/utils/thread.h"

/*
 * Watches the configured directories for new .torrent files in order
 * to add torrents to BitTorrent session. Supports Network File System
 * watching (NFS, CIFS) on Linux and Mac OS.
 */
class TorrentFilesWatcher final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(TorrentFilesWatcher)

public:
    struct WatchedFolderOptions
    {
        BitTorrent::AddTorrentParams addTorrentParams;
        bool recursive = false;
    };

    static void initInstance();
    static void freeInstance();
    static TorrentFilesWatcher *instance();

    QHash<Path, WatchedFolderOptions> folders() const;
    void setWatchedFolder(const Path &path, const WatchedFolderOptions &options);
    void removeWatchedFolder(const Path &path);

signals:
    void watchedFolderSet(const Path &path, const WatchedFolderOptions &options);
    void watchedFolderRemoved(const Path &path);

private slots:
    void onTorrentFound(const BitTorrent::TorrentDescriptor &torrentDescr, const BitTorrent::AddTorrentParams &addTorrentParams);

private:
    explicit TorrentFilesWatcher(QObject *parent = nullptr);

    void load();
    void loadLegacy();
    void store() const;

    void doSetWatchedFolder(const Path &path, const WatchedFolderOptions &options);

    static TorrentFilesWatcher *m_instance;

    QHash<Path, WatchedFolderOptions> m_watchedFolders;

    Utils::Thread::UniquePtr m_ioThread;

    class Worker;
    Worker *m_asyncWorker = nullptr;
};
