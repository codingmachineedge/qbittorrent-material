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

#include <atomic>

#include <QObject>
#include <QRunnable>
#include <QStringList>

#include "base/utils/fs/path.h"

namespace BitTorrent
{
#ifdef QBT_USES_LIBTORRENT2
    enum class TorrentFormat
    {
        V1,
        V2,
        Hybrid
    };
#endif

    struct TorrentCreatorParams
    {
        bool ignoreDotfiles = true;
        bool isPrivate = false;
#ifdef QBT_USES_LIBTORRENT2
        TorrentFormat torrentFormat = TorrentFormat::Hybrid;
#else
        bool isAlignmentOptimized = false;
        int paddedFileSizeLimit = 0;
#endif
        int pieceSize = 0;
        Path sourcePath;
        Path torrentFilePath;
        QString comment;
        QString source;
        QStringList trackers;
        QStringList urlSeeds;
    };

    struct TorrentCreatorResult
    {
        Path torrentFilePath;
        Path savePath;
        int pieceSize;
    };

    class TorrentCreator final : public QObject, public QRunnable
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(TorrentCreator)

    public:
        explicit TorrentCreator(const TorrentCreatorParams &params, QObject *parent = nullptr);

        const TorrentCreatorParams &params() const;
        bool isInterruptionRequested() const;

        void run() override;

#ifdef QBT_USES_LIBTORRENT2
        static int calculateTotalPieces(const Path &inputPath, int pieceSize, bool ignoreDotfiles, TorrentFormat torrentFormat);
#else
        static int calculateTotalPieces(const Path &inputPath, int pieceSize, bool ignoreDotfiles, bool isAlignmentOptimized, int paddedFileSizeLimit);
#endif

    public slots:
        void requestInterruption();

    signals:
        void started();
        void creationFailure(const QString &msg);
        void creationSuccess(const TorrentCreatorResult &result);
        void progressUpdated(int progress);

    private:
        void sendProgressSignal(int currentPieceIdx, int totalPieces);
        void checkInterruptionRequested() const;

        TorrentCreatorParams m_params;
        std::atomic_bool m_interruptionRequested;
    };
}
