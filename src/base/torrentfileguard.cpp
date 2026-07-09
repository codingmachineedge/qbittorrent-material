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

#include "torrentfileguard.h"

#include "base/global.h"
#include "base/logging.h"
#include "base/settingvalue.h"
#include "base/utils/fs.h"
#include "base/utils/fs/path.h"

namespace
{
    /// Handy accessor for the persisted "what to do with the added .torrent
    /// file" preference. Key preserved verbatim from legacy qBittorrent.
    SettingValue<TorrentFileGuard::AutoDeleteMode> autoDeleteModeSetting()
    {
        return SettingValue<TorrentFileGuard::AutoDeleteMode> {u"Core/AutoDeleteAddedTorrentFile"_s};
    }
}

FileGuard::FileGuard(const Path &path)
    : m_path {path}
    , m_remove {true}
{
    qCDebug(lcTorrent).noquote() << "FileGuard: armed for deferred deletion of" << m_path.toString();
}

void FileGuard::setAutoRemove(const bool remove) noexcept
{
    qCDebug(lcTorrent).noquote() << "FileGuard: setAutoRemove(" << remove << ") for" << m_path.toString();
    m_remove = remove;
}

FileGuard::~FileGuard()
{
    if (m_remove && !m_path.isEmpty())
    {
        qCInfo(lcTorrent).noquote() << "FileGuard: removing file" << m_path.toString();
        Utils::Fs::removeFile(m_path); // removeFile() checks for file existence
    }
    else
    {
        qCDebug(lcTorrent).noquote() << "FileGuard: keeping file" << m_path.toString();
    }
}

TorrentFileGuard::TorrentFileGuard(const Path &path, const TorrentFileGuard::AutoDeleteMode mode)
    : FileGuard {mode != Never ? path : Path()}
    , m_mode {mode}
{
    qCDebug(lcTorrent).noquote() << "TorrentFileGuard: created for" << path.toString()
                                 << "mode=" << static_cast<int>(mode);
}

TorrentFileGuard::TorrentFileGuard(const Path &path)
    : TorrentFileGuard {path, autoDeleteMode()}
{
}

TorrentFileGuard::~TorrentFileGuard()
{
    // Cancel deferred deletion unless the file was actually added, or the user
    // asked to always delete regardless of the outcome.
    if (!m_wasAdded && (m_mode != Always))
    {
        qCDebug(lcTorrent) << "TorrentFileGuard: torrent not added and mode != Always; cancelling deletion";
        setAutoRemove(false);
    }
}

void TorrentFileGuard::markAsAddedToSession()
{
    qCDebug(lcTorrent) << "TorrentFileGuard: torrent marked as added to session";
    m_wasAdded = true;
}

TorrentFileGuard::AutoDeleteMode TorrentFileGuard::autoDeleteMode()
{
    return autoDeleteModeSetting().get(AutoDeleteMode::Never);
}

void TorrentFileGuard::setAutoDeleteMode(const TorrentFileGuard::AutoDeleteMode mode)
{
    qCInfo(lcTorrent) << "TorrentFileGuard: setting auto-delete mode to" << static_cast<int>(mode);
    autoDeleteModeSetting() = mode;
}
