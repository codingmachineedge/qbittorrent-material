/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "torrentjournalserializer.h"

#include <QJsonArray>
#include <QJsonValue>

#include "base/bittorrent/addtorrentparams.h"
#include "base/bittorrent/categoryoptions.h"
#include "base/bittorrent/downloadpriority.h"
#include "base/bittorrent/infohash.h"
#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/bittorrent/trackerentrystatus.h"
#include "base/tag.h"
#include "base/utils/fs/path.h"

using namespace Qt::Literals::StringLiterals;

namespace TorrentJournalNS
{
    QJsonObject serializeTorrent(const BitTorrent::Torrent *torrent)
    {
        Q_ASSERT(torrent);

        // The addParams sub-object is exactly the established
        // serializeAddTorrentParams() format so parseAddTorrentParams() can
        // consume it verbatim when re-adding.
        BitTorrent::AddTorrentParams addParams;
        addParams.name = torrent->name();
        addParams.category = torrent->category();
        addParams.tags = torrent->tags();
        addParams.savePath = torrent->savePath();
        addParams.downloadPath = torrent->downloadPath();
        addParams.useDownloadPath = !torrent->downloadPath().isEmpty();
        addParams.sequential = torrent->isSequentialDownload();
        addParams.firstLastPiecePriority = torrent->hasFirstLastPiecePriority();
        addParams.addForced = torrent->isForced();
        addParams.addStopped = torrent->isStopped();
        addParams.stopCondition = torrent->stopCondition();
        addParams.useAutoTMM = torrent->isAutoTMMEnabled();
        addParams.uploadLimit = torrent->uploadLimit();
        addParams.downloadLimit = torrent->downloadLimit();
        addParams.shareLimits = torrent->shareLimits();
        addParams.sslParameters = torrent->getSSLParameters();

        QJsonArray filePaths;
        QJsonArray filePriorities;
        if (torrent->hasMetadata())
        {
            for (const Path &path : torrent->filePaths())
                filePaths.append(path.data());
            for (const BitTorrent::DownloadPriority priority : torrent->filePriorities())
                filePriorities.append(static_cast<int>(priority));
        }

        QJsonArray trackers;
        for (const BitTorrent::TrackerEntryStatus &tracker : torrent->trackers())
        {
            trackers.append(QJsonObject {
                {u"url"_s, tracker.url},
                {u"tier"_s, tracker.tier}
            });
        }

        QJsonArray urlSeeds;
        for (const QUrl &urlSeed : torrent->urlSeeds())
            urlSeeds.append(urlSeed.toString());

        const BitTorrent::InfoHash infoHash = torrent->infoHash();
        return QJsonObject {
            {u"schemaVersion"_s, TorrentDocSchemaVersion},
            {u"id"_s, torrent->id().toString()},
            {u"infoHashV1"_s, infoHash.v1().isValid() ? infoHash.v1().toString() : QString()},
            {u"infoHashV2"_s, infoHash.v2().isValid() ? infoHash.v2().toString() : QString()},
            {u"magnetUri"_s, torrent->createMagnetURI()},
            {u"hasMetadata"_s, torrent->hasMetadata()},
            {u"name"_s, torrent->name()},
            {u"addedTime"_s, torrent->addedTime().toUTC().toString(Qt::ISODate)},
            {u"superSeeding"_s, torrent->superSeeding()},
            {u"addParams"_s, BitTorrent::serializeAddTorrentParams(addParams)},
            {u"filePaths"_s, filePaths},
            {u"filePriorities"_s, filePriorities},
            {u"trackers"_s, trackers},
            {u"urlSeeds"_s, urlSeeds}
        };
    }

    QJsonObject serializeSession(const BitTorrent::Session *session)
    {
        Q_ASSERT(session);

        QJsonObject categories;
        QStringList categoryNames = session->categories();
        categoryNames.sort();
        for (const QString &name : std::as_const(categoryNames))
            categories[name] = session->categoryOptions(name).toJSON();

        QJsonArray tags;
        QStringList tagNames;
        for (const Tag &tag : session->tags())
            tagNames.append(tag.toString());
        tagNames.sort();
        for (const QString &tag : std::as_const(tagNames))
            tags.append(tag);

        return QJsonObject {
            {u"schemaVersion"_s, TorrentDocSchemaVersion},
            {u"categories"_s, categories},
            {u"tags"_s, tags}
        };
    }

    BitTorrent::AddTorrentParams buildAddTorrentParams(const QJsonObject &torrentDoc)
    {
        BitTorrent::AddTorrentParams params =
            BitTorrent::parseAddTorrentParams(torrentDoc.value(u"addParams"_s).toObject());

        // File renames and priorities live outside the addParams object
        // (serializeAddTorrentParams does not carry them).
        const QJsonArray filePaths = torrentDoc.value(u"filePaths"_s).toArray();
        for (const QJsonValue &value : filePaths)
            params.filePaths.append(Path(value.toString()));

        const QJsonArray filePriorities = torrentDoc.value(u"filePriorities"_s).toArray();
        for (const QJsonValue &value : filePriorities)
            params.filePriorities.append(static_cast<BitTorrent::DownloadPriority>(value.toInt(1)));

        // Let libtorrent recheck any files still on disk instead of trusting them.
        params.skipChecking = false;
        return params;
    }
}
