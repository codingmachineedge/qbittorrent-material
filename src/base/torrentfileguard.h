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

#include <QObject>
#include <QString>

#include "base/utils/fs/path.h"

/// Utility class to defer file deletion
class FileGuard
{
public:
    explicit FileGuard(const Path &path = {});
    ~FileGuard();

    /// Cancels or re-enables deferred file deletion
    void setAutoRemove(bool remove) noexcept;

private:
    Path m_path;
    bool m_remove = false;
};

/// Reads settings for .torrent files from preferences
/// and sets the file guard up accordingly
class TorrentFileGuard : private FileGuard
{
    Q_GADGET

public:
    explicit TorrentFileGuard(const Path &path = {});
    ~TorrentFileGuard();

    /// marks the torrent file as loaded (added) into the BitTorrent::Session
    void markAsAddedToSession();
    using FileGuard::setAutoRemove;

    enum AutoDeleteMode : int     // do not change these names: they are stored in config file
    {
        Never,
        IfAdded,
        Always
    };

    // static interface to get/set preferences
    static AutoDeleteMode autoDeleteMode();
    static void setAutoDeleteMode(AutoDeleteMode mode);

private:
    TorrentFileGuard(const Path &path, AutoDeleteMode mode);

    Q_ENUM(AutoDeleteMode)
    AutoDeleteMode m_mode;
    bool m_wasAdded = false;
};
