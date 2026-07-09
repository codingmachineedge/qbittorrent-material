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

#include "base/utils/fs/path.h"
#include "downloadpriority.h"

template <typename T> class QFuture;

namespace BitTorrent
{
    class TorrentContentHandler : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(TorrentContentHandler)

    public:
        using QObject::QObject;

        virtual bool hasMetadata() const = 0;
        virtual int filesCount() const = 0;
        virtual Path filePath(int index) const = 0;
        virtual qlonglong fileSize(int index) const = 0;
        virtual Path actualStorageLocation() const = 0;
        virtual Path actualFilePath(int fileIndex) const = 0;
        virtual QList<DownloadPriority> filePriorities() const = 0;
        virtual QList<qreal> filesProgress() const = 0;
        /**
         * @brief fraction of file pieces that are available at least from one peer
         *
         * This is not the same as torrrent availability, it is just a fraction of pieces
         * that can be downloaded right now. It varies between 0 to 1.
         */
        virtual QFuture<QList<qreal>> fetchAvailableFileFractions() const = 0;

        virtual void renameFile(int index, const Path &newPath) = 0;
        virtual void prioritizeFiles(const QList<DownloadPriority> &priorities) = 0;
        virtual void flushCache() const = 0;

        void renameFile(const Path &oldPath, const Path &newPath);
        void renameFolder(const Path &oldFolderPath, const Path &newFolderPath);

    signals:
        void metadataReceived();
        void fileRenamed(int index, const Path &oldFilePath);
        void folderRenamed(const Path &newFolderPath, const Path &oldFolderPath, const QHash<int, Path> &renamedFiles);
        void folderRenamingFailed(const Path &newFolderPath, const Path &oldFolderPath
                , const QHash<int, Path> &renamedFiles, const QList<int> &failedFileIndexes);

    protected:
        virtual void doRenameFolder(const Path &oldFolderPath, const Path &newFolderPath) = 0;
    };
}
