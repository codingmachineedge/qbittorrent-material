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

#include "torrentcontenthandler.h"

#include "base/logging.h"
#include "base/utils/fs/path.h"

using namespace BitTorrent;

void TorrentContentHandler::renameFile(const Path &oldPath, const Path &newPath)
{
    if (!hasMetadata())
    {
        qCWarning(lcTorrent) << "Cannot rename file: torrent has no metadata";
        return;
    }

    for (int i = 0; i < filesCount(); ++i)
    {
        if (filePath(i) == oldPath)
        {
            qCDebug(lcTorrent) << "Renaming file by path" << oldPath.data() << "->" << newPath.data();
            renameFile(i, newPath);
            return;
        }
    }

    qCWarning(lcTorrent) << "Cannot rename file: no file matches" << oldPath.data();
}

void TorrentContentHandler::renameFolder(const Path &oldFolderPath, const Path &newFolderPath)
{
    if (!hasMetadata())
    {
        qCWarning(lcTorrent) << "Cannot rename folder: torrent has no metadata";
        return;
    }

    qCDebug(lcTorrent) << "Renaming folder" << oldFolderPath.data() << "->" << newFolderPath.data();
    doRenameFolder(oldFolderPath, newFolderPath);
}
