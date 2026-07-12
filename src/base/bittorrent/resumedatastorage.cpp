/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "resumedatastorage.h"

#include <memory>
#include <utility>

#include <libtorrent/bdecode.hpp>
#include <libtorrent/read_resume_data.hpp>
#include <libtorrent/span.hpp>
#include <libtorrent/torrent_info.hpp>
#include <libtorrent/write_resume_data.hpp>

#include <QDir>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "base/git/gitrepositorystore.h"
#include "base/logging.h"
#include "loadtorrentparams.h"
#include "sharelimits.h"
#include "torrent.h"
#include "torrentcontentlayout.h"

using namespace BitTorrent;
using namespace Qt::StringLiterals;

namespace
{
    QString metaPath(const TorrentID &id)
    {
        return u"torrents/%1.meta.json"_s.arg(id.toString());
    }

    QString resumePath(const TorrentID &id)
    {
        return u"torrents/%1.resume.dat"_s.arg(id.toString());
    }

    // The torrent's immutable metadata (info dict + piece hashes), stored once
    // as a bencoded add_torrent_params. Routine checkpoints leave it alone, so
    // per-checkpoint commits stay small and metadata still survives restarts.
    QString metadataPath(const TorrentID &id)
    {
        return u"torrents/%1.metadata.dat"_s.arg(id.toString());
    }

    QJsonObject serializeMeta(const TorrentID &id, const LoadTorrentParams &params)
    {
        QJsonObject obj;
        obj[u"id"_s] = id.toString();
        // Persisted verbatim: an empty name means "track the metadata name",
        // and turning it into a snapshot here would be indistinguishable from
        // a user rename after one restart (display falls back via
        // Torrent::name(), which reads the metadata/hint name).
        obj[u"name"_s] = params.name;
        obj[u"category"_s] = params.category;

        QJsonArray tags;
        for (const Tag &tag : params.tags)
            tags.append(tag.toString());
        obj[u"tags"_s] = tags;

        obj[u"savePath"_s] = params.savePath.toString();
        obj[u"downloadPath"_s] = params.downloadPath.toString();
        obj[u"comment"_s] = params.comment;
        obj[u"contentLayout"_s] = static_cast<int>(params.contentLayout);
        obj[u"operatingMode"_s] = static_cast<int>(params.operatingMode);
        obj[u"useAutoTMM"_s] = params.useAutoTMM;
        obj[u"firstLastPiecePriority"_s] = params.firstLastPiecePriority;
        obj[u"hasFinishedStatus"_s] = params.hasFinishedStatus;
        obj[u"stopped"_s] = params.stopped;
        obj[u"stopCondition"_s] = static_cast<int>(params.stopCondition);

        QJsonObject shareLimits;
        shareLimits[u"ratioLimit"_s] = params.shareLimits.ratioLimit;
        shareLimits[u"seedingTimeLimit"_s] = params.shareLimits.seedingTimeLimit;
        shareLimits[u"inactiveSeedingTimeLimit"_s] = params.shareLimits.inactiveSeedingTimeLimit;
        shareLimits[u"mode"_s] = static_cast<int>(params.shareLimits.mode);
        shareLimits[u"action"_s] = static_cast<int>(params.shareLimits.action);
        obj[u"shareLimits"_s] = shareLimits;

        return obj;
    }

    bool deserializeMeta(const QJsonObject &obj, LoadTorrentParams *params)
    {
        params->name = obj[u"name"_s].toString();
        params->category = obj[u"category"_s].toString();

        params->tags.clear();
        for (const QJsonValue &value : obj[u"tags"_s].toArray())
            params->tags.insert(Tag(value.toString()));

        params->savePath = Path(obj[u"savePath"_s].toString());
        params->downloadPath = Path(obj[u"downloadPath"_s].toString());
        params->comment = obj[u"comment"_s].toString();
        params->contentLayout = static_cast<TorrentContentLayout>(obj[u"contentLayout"_s].toInt());
        params->operatingMode = static_cast<TorrentOperatingMode>(obj[u"operatingMode"_s].toInt());
        params->useAutoTMM = obj[u"useAutoTMM"_s].toBool();
        params->firstLastPiecePriority = obj[u"firstLastPiecePriority"_s].toBool();
        params->hasFinishedStatus = obj[u"hasFinishedStatus"_s].toBool();
        params->stopped = obj[u"stopped"_s].toBool();
        params->stopCondition = static_cast<Torrent::StopCondition>(obj[u"stopCondition"_s].toInt());

        const QJsonObject shareLimits = obj[u"shareLimits"_s].toObject();
        params->shareLimits.ratioLimit = shareLimits[u"ratioLimit"_s].toDouble();
        params->shareLimits.seedingTimeLimit = shareLimits[u"seedingTimeLimit"_s].toInt();
        params->shareLimits.inactiveSeedingTimeLimit = shareLimits[u"inactiveSeedingTimeLimit"_s].toInt();
        params->shareLimits.mode = static_cast<ShareLimitsMode>(shareLimits[u"mode"_s].toInt());
        params->shareLimits.action = static_cast<ShareLimitAction>(shareLimits[u"action"_s].toInt());

        return true;
    }
}

ResumeDataStorage::ResumeDataStorage(const Path &dataFolderPath)
    : m_rootPath {dataFolderPath}
    , m_store {std::make_unique<Git::GitRepositoryStore>(
          dataFolderPath.data(), u"qBittorrent Material Resume Store"_s, u"resume@local.invalid"_s)}
{
    QString error;
    if (!m_store->ensureRepository(&error))
    {
        qCWarning(lcSession) << "Resume data store unavailable:" << error;
        m_available = false;
        return;
    }
    if (!m_store->recoveryPath().isEmpty())
    {
        qCWarning(lcSession) << "Resume data store recovered; previous files kept in"
                             << m_store->recoveryPath();
    }

    const Path torrentsDir = m_rootPath / Path(u"torrents"_s);
    QDir().mkpath(torrentsDir.data());

    m_available = true;
}

ResumeDataStorage::~ResumeDataStorage() = default;

bool ResumeDataStorage::store(const TorrentID &id, const LoadTorrentParams &params)
{
    if (!m_available)
        return false;

    // Routine checkpoints (saved without `save_info_dict`) deliberately omit
    // the torrent metadata. Keep the metadata in its own write-once file so
    // per-checkpoint blobs stay small and metadata still survives restarts:
    // resume.dat is always serialized WITHOUT the info dict, and
    // metadata.dat is written only when a checkpoint actually carries it.
    lt::add_torrent_params leanParams = params.ltAddTorrentParams;
    const std::shared_ptr<lt::torrent_info> ti = std::exchange(leanParams.ti, nullptr);

    std::vector<char> resumeBuf;
    std::vector<char> metadataBuf;
    try
    {
        resumeBuf = lt::write_resume_data_buf(leanParams);
        if (ti && ti->is_valid())
        {
            lt::add_torrent_params metadataParams;
            metadataParams.ti = ti;
            metadataParams.info_hashes = params.ltAddTorrentParams.info_hashes;
            metadataBuf = lt::write_resume_data_buf(metadataParams);
        }
    }
    catch (const std::exception &err)
    {
        qCWarning(lcSession) << "Failed to serialize resume data for" << params.name
                             << ':' << err.what();
        return false;
    }

    const QJsonObject meta = serializeMeta(id, params);
    const QByteArray metaBytes = QJsonDocument(meta).toJson(QJsonDocument::Compact);
    const QByteArray resumeBytes(resumeBuf.data(), static_cast<qsizetype>(resumeBuf.size()));

    QString error;
    const QDir root {m_rootPath.data()};
    const QString metaFilePath = root.filePath(metaPath(id));
    const QString resumeFilePath = root.filePath(resumePath(id));
    const QString metadataFilePath = root.filePath(metadataPath(id));
    QDir().mkpath(QFileInfo(metaFilePath).absolutePath());

    // Best-effort undo of already-written worktree files if a later write
    // fails: commitAll() stages the whole tree, so half-written pairs left
    // behind here would otherwise be swept into another torrent's commit.
    const auto restoreFromHead = [this](const QString &absolutePath, const QString &repoPath)
    {
        QString headError;
        const QString head = m_store->headCommitId(&headError);
        QByteArray oldBytes;
        bool found = false;
        if (!head.isEmpty()
                && m_store->readFileAtCommit(head, repoPath, &oldBytes, &found, &headError) && found)
        {
            QString writeError;
            if (!Git::GitRepositoryStore::writeFileAtomically(absolutePath, oldBytes, &writeError))
                qCWarning(lcSession) << "Failed to roll back" << repoPath << ':' << writeError;
        }
        else
        {
            QFile::remove(absolutePath);
        }
    };

    if (!metadataBuf.empty() && !QFileInfo::exists(metadataFilePath))
    {
        const QByteArray metadataBytes(metadataBuf.data(), static_cast<qsizetype>(metadataBuf.size()));
        if (!Git::GitRepositoryStore::writeFileAtomically(metadataFilePath, metadataBytes, &error))
        {
            qCWarning(lcSession) << "Failed to write torrent metadata for" << params.name << ':' << error;
            return false;
        }
    }

    if (!Git::GitRepositoryStore::writeFileAtomically(metaFilePath, metaBytes, &error))
    {
        qCWarning(lcSession) << "Failed to write resume metadata for" << params.name << ':' << error;
        return false;
    }
    if (!Git::GitRepositoryStore::writeFileAtomically(resumeFilePath, resumeBytes, &error))
    {
        qCWarning(lcSession) << "Failed to write resume blob for" << params.name << ':' << error;
        restoreFromHead(metaFilePath, metaPath(id));
        return false;
    }

    QString commitId;
    bool committed = false;
    if (!m_store->commitAll(u"resume: %1"_s.arg(params.name.isEmpty() ? id.toString() : params.name),
            &commitId, &committed, &error))
    {
        qCWarning(lcSession) << "Failed to commit resume data for" << params.name << ':' << error;
        return false;
    }
    return true;
}

bool ResumeDataStorage::remove(const TorrentID &id)
{
    if (!m_available)
        return false;

    // A file that is absent is fine; one that exists but cannot be deleted
    // (e.g. transiently held open by a scanner) is not -- the entry would
    // stay in HEAD and the torrent would resurrect on the next startup.
    const auto removeFile = [](const QString &path)
    {
        return !QFileInfo::exists(path) || QFile::remove(path);
    };

    const QDir root {m_rootPath.data()};
    bool allRemoved = true;
    for (const QString &path : {metaPath(id), resumePath(id), metadataPath(id)})
    {
        if (!removeFile(root.filePath(path)))
        {
            qCWarning(lcSession) << "Failed to delete resume store file" << path;
            allRemoved = false;
        }
    }

    QString error;
    QString commitId;
    bool committed = false;
    if (!m_store->commitAll(u"forget: %1"_s.arg(id.toString()), &commitId, &committed, &error))
    {
        qCWarning(lcSession) << "Failed to commit resume data removal for" << id.toString() << ':' << error;
        return false;
    }
    return allRemoved;
}

QList<TorrentID> ResumeDataStorage::torrentIds() const
{
    QList<TorrentID> ids;
    if (!m_available)
        return ids;

    QString error;
    const QString head = m_store->headCommitId(&error);
    if (head.isEmpty())
        return ids;

    QStringList files;
    if (!m_store->listTreeFiles(head, u"torrents"_s, &files, &error))
    {
        qCWarning(lcSession) << "Failed to list stored torrents:" << error;
        return ids;
    }

    const QString suffix = u".meta.json"_s;
    for (const QString &file : files)
    {
        if (!file.endsWith(suffix))
            continue;
        const QString idString = file.left(file.size() - suffix.size());
        const TorrentID id = TorrentID::fromString(idString);
        if (!id.isValid())
            continue;
        ids.append(id);
    }
    return ids;
}

bool ResumeDataStorage::load(const TorrentID &id, LoadTorrentParams *params) const
{
    if (!m_available)
        return false;

    QString error;
    const QString head = m_store->headCommitId(&error);
    if (head.isEmpty())
        return false;

    QByteArray metaBytes;
    bool metaFound = false;
    if (!m_store->readFileAtCommit(head, metaPath(id), &metaBytes, &metaFound, &error) || !metaFound)
        return false;

    QByteArray resumeBytes;
    bool resumeFound = false;
    if (!m_store->readFileAtCommit(head, resumePath(id), &resumeBytes, &resumeFound, &error) || !resumeFound)
        return false;

    const QJsonDocument metaDoc = QJsonDocument::fromJson(metaBytes);
    if (!metaDoc.isObject())
        return false;

    *params = {};
    if (!deserializeMeta(metaDoc.object(), params))
        return false;

    try
    {
        const lt::bdecode_node node = lt::bdecode(
            lt::span<char const>(resumeBytes.constData(), resumeBytes.size()));
        params->ltAddTorrentParams = lt::read_resume_data(node);
    }
    catch (const std::exception &err)
    {
        qCWarning(lcSession) << "Failed to parse resume blob for" << id.toString() << ':' << err.what();
        return false;
    }

    // Routine checkpoints are stored without the info dict; the torrent's
    // immutable metadata lives in its own write-once blob (see store()).
    if (!params->ltAddTorrentParams.ti)
    {
        QByteArray metadataBytes;
        bool metadataFound = false;
        if (m_store->readFileAtCommit(head, metadataPath(id), &metadataBytes, &metadataFound, &error)
                && metadataFound)
        {
            try
            {
                const lt::bdecode_node node = lt::bdecode(
                    lt::span<char const>(metadataBytes.constData(), metadataBytes.size()));
                params->ltAddTorrentParams.ti = lt::read_resume_data(node).ti;
            }
            catch (const std::exception &err)
            {
                // Not fatal: the torrent still restores magnet-style and
                // re-fetches its metadata from the swarm.
                qCWarning(lcSession) << "Failed to parse stored metadata for"
                                     << id.toString() << ':' << err.what();
            }
        }
    }

    return true;
}
