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

#include "peerinfo.h"

#include <libtorrent/bitfield.hpp>
#include <libtorrent/peer_info.hpp>

#include <QBitArray>
#include <QString>

#include "base/bittorrent/ltqbitarray.h"
#include "base/net/geoipmanager.h"
#include "base/unicodestrings.h"
#include "peeraddress.h"

using namespace BitTorrent;
using namespace Qt::Literals::StringLiterals;

PeerInfo::PeerInfo(const lt::peer_info &nativeInfo, const QBitArray &allPieces)
    : m_nativeInfo {nativeInfo}
{
    determineFlags();
    m_relevance = calcRelevance(allPieces);
}

bool PeerInfo::fromDHT() const
{
    return static_cast<bool>(m_nativeInfo.source & lt::peer_info::dht);
}

bool PeerInfo::fromPeX() const
{
    return static_cast<bool>(m_nativeInfo.source & lt::peer_info::pex);
}

bool PeerInfo::fromLSD() const
{
    return static_cast<bool>(m_nativeInfo.source & lt::peer_info::lsd);
}

bool PeerInfo::isInteresting() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::interesting);
}

bool PeerInfo::isChocked() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::choked);
}

bool PeerInfo::isRemoteInterested() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::remote_interested);
}

bool PeerInfo::isRemoteChocked() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::remote_choked);
}

bool PeerInfo::isSupportsExtensions() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::supports_extensions);
}

bool PeerInfo::isLocalConnection() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::local_connection);
}

bool PeerInfo::isHandshake() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::handshake);
}

bool PeerInfo::isConnecting() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::connecting);
}

bool PeerInfo::isOnParole() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::on_parole);
}

bool PeerInfo::isSeed() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::seed);
}

bool PeerInfo::optimisticUnchoke() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::optimistic_unchoke);
}

bool PeerInfo::isSnubbed() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::snubbed);
}

bool PeerInfo::isUploadOnly() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::upload_only);
}

bool PeerInfo::isEndgameMode() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::endgame_mode);
}

bool PeerInfo::isHolepunched() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::holepunched);
}

bool PeerInfo::useI2PSocket() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::i2p_socket);
}

bool PeerInfo::useUTPSocket() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::utp_socket);
}

bool PeerInfo::useSSLSocket() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::ssl_socket);
}

bool PeerInfo::isRC4Encrypted() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::rc4_encrypted);
}

bool PeerInfo::isPlaintextEncrypted() const
{
    return static_cast<bool>(m_nativeInfo.flags & lt::peer_info::plaintext_encrypted);
}

PeerAddress PeerInfo::address() const
{
    // fixme: the fact that the address is unspecified while the socket is I2P
    // is used as a means to identify an I2P peer.
    if (useI2PSocket())
        return {};

    return {QHostAddress(QString::fromStdString(m_nativeInfo.ip.address().to_string()))
            , m_nativeInfo.ip.port()};
}

QString PeerInfo::I2PAddress() const
{
    if (m_I2PAddress.isEmpty() && useI2PSocket())
        m_I2PAddress = QString::fromStdString(m_nativeInfo.i2p_destination().to_string());
    return m_I2PAddress;
}

QString PeerInfo::client() const
{
    return QString::fromStdString(m_nativeInfo.client);
}

QString PeerInfo::peerIdClient() const
{
    // Return the peer id in a printable form
    const lt::peer_id &pid = m_nativeInfo.pid;
    return QString::fromLatin1(QByteArray::fromRawData(pid.data(), pid.size()).toHex());
}

qreal PeerInfo::progress() const
{
    return m_nativeInfo.progress;
}

int PeerInfo::payloadUpSpeed() const
{
    return m_nativeInfo.payload_up_speed;
}

int PeerInfo::payloadDownSpeed() const
{
    return m_nativeInfo.payload_down_speed;
}

qlonglong PeerInfo::totalUpload() const
{
    return m_nativeInfo.total_upload;
}

qlonglong PeerInfo::totalDownload() const
{
    return m_nativeInfo.total_download;
}

QBitArray PeerInfo::pieces() const
{
    return LT::toQBitArray(m_nativeInfo.pieces);
}

QString PeerInfo::connectionType() const
{
    if (m_nativeInfo.flags & lt::peer_info::utp_socket)
        return u"μTP"_s;

    return (m_nativeInfo.connection_type == lt::peer_info::standard_bittorrent)
            ? u"BT"_s : u"Web"_s;
}

qreal PeerInfo::calcRelevance(const QBitArray &allPieces) const
{
    const QBitArray peerPieces = pieces();

    int localMissing = 0;
    int remoteHaves = 0;
    for (int i = 0; i < allPieces.size(); ++i)
    {
        if (!allPieces[i])
        {
            ++localMissing;
            if (peerPieces[i])
                ++remoteHaves;
        }
    }

    if (localMissing == 0)
        return 0;

    return static_cast<qreal>(remoteHaves) / localMissing;
}

qreal PeerInfo::relevance() const
{
    return m_relevance;
}

void PeerInfo::determineFlags()
{
    const auto updateFlags = [this](const QChar specifier, const QString &description)
    {
        m_flags += specifier;
        m_flags += u' ';
        m_flagsDescription += u"%1 = %2\n"_s.arg(specifier, description);
    };

    if (isInteresting())
    {
        if (isRemoteChocked())
        {
            // d = Your client wants to download, but peer doesn't want to send (interested and choked)
            updateFlags(u'd', tr("Interested (local) and choked (peer)"));
        }
        else
        {
            // D = Currently downloading (interested and not choked)
            updateFlags(u'D', tr("Interested (local) and unchoked (peer)"));
        }
    }

    if (isRemoteInterested())
    {
        if (isChocked())
        {
            // u = Peer wants your client to upload, but your client doesn't want to (interested and choked)
            updateFlags(u'u', tr("Interested (peer) and choked (local)"));
        }
        else
        {
            // U = Currently uploading (interested and not choked)
            updateFlags(u'U', tr("Interested (peer) and unchoked (local)"));
        }
    }

    // K = Peer is unchoking your client, but your client is not interested
    if (!isRemoteChocked() && !isInteresting())
        updateFlags(u'K', tr("Not interested (local) and unchoked (peer)"));

    // ? = Your client unchoked the peer but the peer is not interested
    if (!isChocked() && !isRemoteInterested())
        updateFlags(u'?', tr("Not interested (peer) and unchoked (local)"));

    // O = Optimistic unchoke
    if (optimisticUnchoke())
        updateFlags(u'O', tr("Optimistic unchoke"));

    // S = Peer is snubbed
    if (isSnubbed())
        updateFlags(u'S', tr("Peer snubbed"));

    // I = Peer is an incoming connection
    if (!isLocalConnection())
        updateFlags(u'I', tr("Incoming connection"));

    // H = Peer was obtained through DHT
    if (fromDHT())
        updateFlags(u'H', tr("Peer from DHT"));

    // X = Peer was included in peerlists obtained through Peer Exchange (PEX)
    if (fromPeX())
        updateFlags(u'X', tr("Peer from PEX"));

    // L = Peer is local
    if (fromLSD())
        updateFlags(u'L', tr("Peer from LSD"));

    // E = Peer is using Protocol Encryption (all traffic)
    if (isRC4Encrypted())
        updateFlags(u'E', tr("Encrypted traffic"));

    // e = Peer is using Protocol Encryption (handshake)
    if (isPlaintextEncrypted())
        updateFlags(u'e', tr("Encrypted handshake"));

    // P = Peer is using uTP
    if (useUTPSocket())
        updateFlags(u'P', u"%1 %2"_s.arg(QChar(0x03BC), u"TP"_s)); // μTP
}

QString PeerInfo::flags() const
{
    return m_flags;
}

QString PeerInfo::flagsDescription() const
{
    return m_flagsDescription;
}

QString PeerInfo::country() const
{
    if (m_country.isEmpty())
    {
        const auto *geoIPManager = Net::GeoIPManager::instance();
        m_country = Net::GeoIPManager::CountryName(
                geoIPManager ? geoIPManager->lookup(address().ip) : QString());
    }
    return m_country;
}

int PeerInfo::downloadingPieceIndex() const
{
    return static_cast<int>(m_nativeInfo.downloading_piece_index);
}
