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

#include <libtorrent/torrent_info.hpp>

#include <QtContainerFwd>
#include <QCoreApplication>
#include <QList>

#include "base/indexrange.h"
#include "base/utils/fs/path.h"

class QByteArray;
class QDateTime;
class QString;
class QUrl;

namespace BitTorrent
{
    class InfoHash;
    struct TrackerEntry;

    class TorrentInfo
    {
    public:
        TorrentInfo() = default;
        TorrentInfo(const TorrentInfo &other) = default;

        explicit TorrentInfo(const lt::torrent_info &nativeInfo);

        TorrentInfo &operator=(const TorrentInfo &other);

        bool isValid() const;
        InfoHash infoHash() const;
        QString name() const;
        bool isPrivate() const;
        qlonglong totalSize() const;
        int filesCount() const;
        int pieceLength() const;
        int pieceLength(int index) const;
        int piecesCount() const;
        Path filePath(int index) const;
        PathList filePaths() const;
        qlonglong fileSize(int index) const;
        qlonglong fileOffset(int index) const;
        PathList filesForPiece(int pieceIndex) const;
        QList<int> fileIndicesForPiece(int pieceIndex) const;
        QList<QByteArray> pieceHashes() const;

        using PieceRange = IndexRange<int>;
        // returns pair of the first and the last pieces into which
        // the given file extends (maybe partially).
        PieceRange filePieces(const Path &filePath) const;
        PieceRange filePieces(int fileIndex) const;

        QByteArray rawData() const;

        bool matchesInfoHash(const InfoHash &otherInfoHash) const;

        std::shared_ptr<lt::torrent_info> nativeInfo() const;
        QList<lt::file_index_t> nativeIndexes() const;

    private:
        // returns file index or -1 if fileName is not found
        int fileIndex(const Path &filePath) const;

        std::shared_ptr<const lt::torrent_info> m_nativeInfo;

        // internal indexes of files (payload only, excluding any .pad files)
        // by which they are addressed in libtorrent
        QList<lt::file_index_t> m_nativeIndexes;
    };
}

Q_DECLARE_METATYPE(BitTorrent::TorrentInfo)
