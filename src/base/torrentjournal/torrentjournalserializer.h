/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QJsonObject>

namespace BitTorrent
{
    class Session;
    class Torrent;
    struct AddTorrentParams;
}

namespace TorrentJournalNS
{
    inline constexpr int TorrentDocSchemaVersion = 1;

    // Canonical, configuration-only document for one torrent. Contains no
    // progress/speed/peer data, so serializing an unchanged torrent yields a
    // byte-identical document (QJsonObject key order is deterministic).
    [[nodiscard]] QJsonObject serializeTorrent(const BitTorrent::Torrent *torrent);

    // Session-level journaled state: categories (with options) and tags.
    [[nodiscard]] QJsonObject serializeSession(const BitTorrent::Session *session);

    // Rebuilds AddTorrentParams from a serialized torrent document, ready to
    // re-add the torrent with its settings, file renames, and priorities.
    [[nodiscard]] BitTorrent::AddTorrentParams buildAddTorrentParams(const QJsonObject &torrentDoc);
}
