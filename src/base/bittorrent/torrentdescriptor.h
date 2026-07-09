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

#include <optional>

#include <libtorrent/add_torrent_params.hpp>

#include <QtContainerFwd>
#include <QDateTime>
#include <QMetaType>
#include <QString>

#include "base/3rdparty/expected.hpp"
#include "base/utils/fs/path.h"
#include "torrentinfo.h"

class QByteArray;
class QUrl;

namespace BitTorrent
{
    class InfoHash;
    struct TrackerEntry;

    class TorrentDescriptor
    {
    public:
        TorrentDescriptor() = default;

        InfoHash infoHash() const;
        QString name() const;
        QDateTime creationDate() const;
        QString creator() const;
        QString comment() const;
        QList<TrackerEntry> trackers() const;
        QList<QUrl> urlSeeds() const;
        const std::optional<TorrentInfo> &info() const;

        void setTorrentInfo(TorrentInfo torrentInfo);

        static nonstd::expected<TorrentDescriptor, QString> load(const QByteArray &data) noexcept;
        static nonstd::expected<TorrentDescriptor, QString> loadFromFile(const Path &path) noexcept;
        static nonstd::expected<TorrentDescriptor, QString> parse(const QString &str) noexcept;
        nonstd::expected<void, QString> saveToFile(const Path &path) const;
        nonstd::expected<QByteArray, QString> saveToBuffer() const;

        const lt::add_torrent_params &ltAddTorrentParams() const;

    private:
        explicit TorrentDescriptor(lt::add_torrent_params ltAddTorrentParams);

        lt::add_torrent_params m_ltAddTorrentParams;
        std::optional<TorrentInfo> m_info;
        QDateTime m_creationDate;
        QString m_creator;
        QString m_comment;
    };
}

Q_DECLARE_METATYPE(BitTorrent::TorrentDescriptor)
