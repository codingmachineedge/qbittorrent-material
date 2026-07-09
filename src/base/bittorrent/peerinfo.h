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

#include <libtorrent/peer_info.hpp>

#include <QCoreApplication>

class QBitArray;

namespace BitTorrent
{
    struct PeerAddress;

    class PeerInfo
    {
        Q_DECLARE_TR_FUNCTIONS(PeerInfo)

    public:
        PeerInfo() = default;
        PeerInfo(const lt::peer_info &nativeInfo, const QBitArray &allPieces);

        bool fromDHT() const;
        bool fromPeX() const;
        bool fromLSD() const;

        bool isInteresting() const;
        bool isChocked() const;
        bool isRemoteInterested() const;
        bool isRemoteChocked() const;
        bool isSupportsExtensions() const;
        bool isLocalConnection() const;

        bool isHandshake() const;
        bool isConnecting() const;
        bool isOnParole() const;
        bool isSeed() const;

        bool optimisticUnchoke() const;
        bool isSnubbed() const;
        bool isUploadOnly() const;
        bool isEndgameMode() const;
        bool isHolepunched() const;

        bool useI2PSocket() const;
        bool useUTPSocket() const;
        bool useSSLSocket() const;

        bool isRC4Encrypted() const;
        bool isPlaintextEncrypted() const;

        PeerAddress address() const;
        QString I2PAddress() const;
        QString client() const;
        QString peerIdClient() const;
        qreal progress() const;
        int payloadUpSpeed() const;
        int payloadDownSpeed() const;
        qlonglong totalUpload() const;
        qlonglong totalDownload() const;
        QBitArray pieces() const;
        QString connectionType() const;
        qreal relevance() const;
        QString flags() const;
        QString flagsDescription() const;
        QString country() const;
        int downloadingPieceIndex() const;

    private:
        qreal calcRelevance(const QBitArray &allPieces) const;
        void determineFlags();

        lt::peer_info m_nativeInfo = {};
        qreal m_relevance = 0;
        QString m_flags;
        QString m_flagsDescription;

        mutable QString m_country;
        mutable QString m_I2PAddress;
    };
}
