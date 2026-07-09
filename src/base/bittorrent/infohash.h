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

#ifdef QBT_USES_LIBTORRENT2
#include <libtorrent/info_hash.hpp>
#endif

#include <QMetaType>

#include "base/digest32.h"

class QString;

using SHA1Hash = Digest32<160>;
using SHA256Hash = Digest32<256>;

Q_DECLARE_METATYPE(SHA1Hash)
Q_DECLARE_METATYPE(SHA256Hash)

namespace BitTorrent
{
    class InfoHash;

    class TorrentID : public Digest32<160>
    {
    public:
        using BaseType = Digest32<160>;
        using BaseType::BaseType;

        static TorrentID fromString(const QString &hashString);
        static TorrentID fromInfoHash(const InfoHash &infoHash);
        static TorrentID fromSHA1Hash(const SHA1Hash &hash);
        static TorrentID fromSHA256Hash(const SHA256Hash &hash);
    };

    class InfoHash
    {
    public:
#ifdef QBT_USES_LIBTORRENT2
        using WrappedType = lt::info_hash_t;
#else
        using WrappedType = lt::sha1_hash;
#endif

        InfoHash() = default;
        InfoHash(const WrappedType &nativeHash);
#ifdef QBT_USES_LIBTORRENT2
        InfoHash(const SHA1Hash &v1, const SHA256Hash &v2);
#endif

        bool isValid() const;
        bool isHybrid() const;
        SHA1Hash v1() const;
        SHA256Hash v2() const;
        TorrentID toTorrentID() const;

        QString toString() const;

        operator WrappedType() const;

    private:
        bool m_valid = false;
        WrappedType m_nativeHash;
    };

    std::size_t qHash(const TorrentID &key, std::size_t seed = 0);
    std::size_t qHash(const InfoHash &key, std::size_t seed = 0);

    bool operator==(const InfoHash &left, const InfoHash &right);
}

Q_DECLARE_METATYPE(BitTorrent::TorrentID)
// We can declare it as Q_MOVABLE_TYPE to improve performance
// since base type uses QSharedDataPointer as the only member
Q_DECLARE_TYPEINFO(BitTorrent::TorrentID, Q_MOVABLE_TYPE);
