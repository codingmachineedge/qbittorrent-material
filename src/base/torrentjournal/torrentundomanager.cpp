/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "torrentundomanager.h"

#include <QJsonArray>
#include <QJsonValue>

#include "base/bittorrent/addtorrentparams.h"
#include "base/bittorrent/categoryoptions.h"
#include "base/bittorrent/infohash.h"
#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/bittorrent/torrentdescriptor.h"
#include "base/git/gitrepositorystore.h"
#include "base/logging.h"
#include "base/settingsstorage.h"
#include "base/tag.h"
#include "base/utils/fs/path.h"
#include "torrentjournal.h"
#include "torrentjournalserializer.h"

using namespace Qt::Literals::StringLiterals;
using namespace TorrentJournalNS;

namespace
{
    BitTorrent::Torrent *liveTorrent(const QString &torrentId)
    {
        BitTorrent::Session *session = BitTorrent::Session::instance();
        return session ? session->getTorrent(BitTorrent::TorrentID::fromString(torrentId)) : nullptr;
    }
}

TorrentUndoManager *TorrentUndoManager::m_instance = nullptr;

class TorrentUndoManager::BusyGuard
{
public:
    explicit BusyGuard(TorrentUndoManager *manager)
        : m_manager {manager}
    {
        m_manager->setBusy(true);
    }

    ~BusyGuard()
    {
        m_manager->setBusy(false);
    }

private:
    TorrentUndoManager *m_manager = nullptr;
};

void TorrentUndoManager::initInstance()
{
    if (!m_instance)
        m_instance = new TorrentUndoManager;
}

void TorrentUndoManager::freeInstance()
{
    delete m_instance;
    m_instance = nullptr;
}

TorrentUndoManager *TorrentUndoManager::instance()
{
    return m_instance;
}

bool TorrentUndoManager::isBusy() const
{
    return m_busy;
}

bool TorrentUndoManager::isEntryUndoable(const JournalEntry &entry)
{
    if ((entry.origin == JournalOrigin::Snapshot) || entry.ops.isEmpty())
        return false;
    return std::any_of(entry.ops.cbegin(), entry.ops.cend(), [](const JournalOpRecord &op)
    {
        switch (op.kind)
        {
        case JournalOpKind::Metadata:
        case JournalOpKind::Snapshot:
        case JournalOpKind::Unknown:
            return false;
        default:
            return true;
        }
    });
}

TorrentJournalNS::JournalEntry TorrentUndoManager::lastUndoableEntry() const
{
    const TorrentJournal *journal = TorrentJournal::instance();
    if (!journal || !journal->isAvailable())
        return {};
    // The journal is linear; scanning a short window is enough to skip
    // non-undoable snapshot/metadata entries at the head.
    const QList<JournalEntry> entries = journal->history(TorrentJournal::Repo::Actions, 25);
    for (const JournalEntry &entry : entries)
    {
        if (isEntryUndoable(entry))
            return entry;
    }
    return {};
}

bool TorrentUndoManager::canUndo() const
{
    return !lastUndoableEntry().commitId.isEmpty();
}

TorrentUndoManager::UndoResult TorrentUndoManager::undoLast()
{
    const JournalEntry entry = lastUndoableEntry();
    if (entry.commitId.isEmpty())
    {
        const UndoResult result {false, tr("Nothing to undo")};
        finish(result);
        return result;
    }
    return undoEntry(entry.commitId);
}

TorrentUndoManager::UndoResult TorrentUndoManager::undoEntry(const QString &commitId,
    const ConflictPolicy policy)
{
    TorrentJournal *journal = TorrentJournal::instance();
    if (!journal || !journal->isAvailable())
    {
        const UndoResult result {false, tr("The journal is not available")};
        finish(result);
        return result;
    }
    if (m_busy)
    {
        const UndoResult result {false, tr("Another undo is still running")};
        finish(result);
        return result;
    }

    Git::CommitInfo info;
    QString error;
    if (!journal->store(TorrentJournal::Repo::Actions)->commitInfo(commitId, &info, &error))
    {
        const UndoResult result {false, error};
        finish(result);
        return result;
    }
    JournalEntry entry = decodeCommitMessage(info.message);
    entry.commitId = info.id;
    entry.shortId = info.shortId;
    entry.timestamp = info.timestamp;
    if (!isEntryUndoable(entry))
    {
        const UndoResult result {false, tr("This entry cannot be undone")};
        finish(result);
        return result;
    }

    const BusyGuard guard {this};
    journal->beginAnnotatedScope(JournalOrigin::Undo, u"undo: "_s + entry.summary, entry.commitId);
    UndoResult result = undoOps(entry, policy);
    journal->endAnnotatedScope();

    result.success = (result.applied > 0) || (result.conflicted == 0);
    if (result.message.isEmpty())
    {
        if (result.conflicted > 0)
        {
            result.message = tr("Undid \"%1\" (%2 changes skipped: values changed since)")
                .arg(entry.summary, QString::number(result.conflicted));
        }
        else
        {
            result.message = tr("Undid \"%1\"").arg(entry.summary);
        }
    }
    finish(result);
    return result;
}

TorrentUndoManager::UndoResult TorrentUndoManager::undoOps(const JournalEntry &entry,
    const ConflictPolicy policy)
{
    UndoResult result;
    const TorrentJournal *journal = TorrentJournal::instance();
    Git::CommitInfo info;
    if (!journal->store(TorrentJournal::Repo::Actions)->commitInfo(entry.commitId, &info))
    {
        result.message = tr("Could not read the journal entry");
        return result;
    }
    // A root commit has no parent; only Add ops can be undone against it.
    const QString parentCommitId = info.parentIds.isEmpty() ? QString() : info.parentIds.first();

    for (const JournalOpRecord &op : entry.ops)
    {
        if (!undoSingleOp(op, parentCommitId, entry.commitId, policy, &result))
            continue;
    }
    return result;
}

bool TorrentUndoManager::undoSingleOp(const JournalOpRecord &op, const QString &parentCommitId,
    const QString &undoneCommitId, const ConflictPolicy policy, UndoResult *result)
{
    TorrentJournal *journal = TorrentJournal::instance();
    BitTorrent::Session *session = BitTorrent::Session::instance();

    switch (op.kind)
    {
    case JournalOpKind::Add:
    {
        // Inverse of add: remove the torrent, never touching downloaded files.
        if (BitTorrent::Torrent *torrent = liveTorrent(op.torrentId); torrent)
        {
            session->removeTorrent(torrent->id(), BitTorrent::TorrentRemoveOption::KeepContent);
            ++result->applied;
        }
        else
        {
            ++result->skipped;
        }
        return true;
    }
    case JournalOpKind::Delete:
    {
        // Inverse of delete: re-add from the state stored in the parent commit.
        if (parentCommitId.isEmpty())
        {
            ++result->skipped;
            return true;
        }
        QJsonObject doc;
        if (!journal->torrentDocAtCommit(parentCommitId, op.torrentId, &doc))
        {
            ++result->skipped;
            return true;
        }
        QByteArray blob;
        (void)journal->torrentBlobAtCommit(parentCommitId, op.torrentId, &blob);
        QString error;
        if (readdTorrent(op.torrentId, doc, blob, undoneCommitId, &error))
        {
            ++result->applied;
        }
        else
        {
            ++result->conflicted;
            if (result->message.isEmpty())
                result->message = error;
        }
        return true;
    }
    case JournalOpKind::Metadata:
    case JournalOpKind::Snapshot:
    case JournalOpKind::Unknown:
        ++result->skipped;
        return true;
    case JournalOpKind::SessionCategory:
    {
        if (op.oldValue.isEmpty() && !op.newValue.isEmpty())
        {
            (void)session->removeCategory(op.newValue); // inverse of "added"
            ++result->applied;
        }
        else if (op.newValue.isEmpty() && !op.oldValue.isEmpty())
        {
            BitTorrent::CategoryOptions options;
            if (!parentCommitId.isEmpty())
            {
                const QJsonObject sessionDoc = journal->sessionDocAtCommit(parentCommitId);
                options = BitTorrent::CategoryOptions::fromJSON(
                    sessionDoc.value(u"categories"_s).toObject().value(op.oldValue).toObject());
            }
            (void)session->addCategory(op.oldValue, options); // inverse of "removed"
            ++result->applied;
        }
        else
        {
            ++result->skipped; // option edits have no engine-level inverse here
        }
        return true;
    }
    case JournalOpKind::SessionTag:
    {
        if (op.oldValue.isEmpty() && !op.newValue.isEmpty())
            (void)session->removeTag(Tag(op.newValue));
        else if (!op.oldValue.isEmpty())
            (void)session->addTag(Tag(op.oldValue));
        ++result->applied;
        return true;
    }
    default:
        break;
    }

    // Field-level ops need the live torrent and its parent-commit document.
    BitTorrent::Torrent *torrent = liveTorrent(op.torrentId);
    if (!torrent)
    {
        ++result->skipped;
        return true;
    }
    if (parentCommitId.isEmpty())
    {
        ++result->skipped;
        return true;
    }
    QJsonObject parentDoc;
    if (!TorrentJournal::instance()->torrentDocAtCommit(parentCommitId, op.torrentId, &parentDoc))
    {
        ++result->skipped;
        return true;
    }

    // Selective-undo conflict check: only revert a field whose live value
    // still matches the entry's post-state (unless forced).
    if (policy == ConflictPolicy::Skip)
    {
        const bool drifted = [&]() -> bool
        {
            switch (op.kind)
            {
            case JournalOpKind::Name:
                return !op.newValue.isEmpty() && (torrent->name() != op.newValue);
            case JournalOpKind::Category:
                return torrent->category() != op.newValue;
            case JournalOpKind::TagAdd:
                return !torrent->hasTag(Tag(op.newValue));
            case JournalOpKind::TagRemove:
                return torrent->hasTag(Tag(op.oldValue));
            default:
                return false; // structural ops are applied via the doc diff
            }
        }();
        if (drifted)
        {
            ++result->conflicted;
            return true;
        }
    }

    switch (op.kind)
    {
    case JournalOpKind::Name:
        torrent->setName(op.oldValue);
        ++result->applied;
        return true;
    case JournalOpKind::TagAdd:
        (void)torrent->removeTag(Tag(op.newValue));
        ++result->applied;
        return true;
    case JournalOpKind::TagRemove:
        if (!session->tags().contains(Tag(op.oldValue)))
            (void)session->addTag(Tag(op.oldValue));
        (void)torrent->addTag(Tag(op.oldValue));
        ++result->applied;
        return true;
    case JournalOpKind::Category:
    {
        if (!op.oldValue.isEmpty() && !session->categories().contains(op.oldValue))
            (void)session->addCategory(op.oldValue);
        (void)torrent->setCategory(op.oldValue);
        ++result->applied;
        return true;
    }
    // Everything else (save path, saving mode, start/stop, limits, trackers,
    // file renames, generic config) is restored by diff-applying the full
    // parent document — precise and far more robust than per-op inversion.
    default:
    {
        const int changes = applyDocConfig(torrent, parentDoc);
        result->applied += changes;
        return true;
    }
    }
}

bool TorrentUndoManager::readdTorrent(const QString &torrentId, const QJsonObject &doc,
    const QByteArray &blob, const QString &annotationCommitId, QString *error)
{
    BitTorrent::Session *session = BitTorrent::Session::instance();
    TorrentJournal *journal = TorrentJournal::instance();

    nonstd::expected<BitTorrent::TorrentDescriptor, QString> descriptor =
        !blob.isEmpty() ? BitTorrent::TorrentDescriptor::load(blob)
                        : BitTorrent::TorrentDescriptor::parse(doc.value(u"magnetUri"_s).toString());
    if (!descriptor)
    {
        if (error)
            *error = tr("Could not rebuild the torrent: %1").arg(descriptor.error());
        return false;
    }

    if (session->isKnownTorrent(descriptor->infoHash()))
    {
        // Already present — downgrade to reconciling its configuration.
        if (BitTorrent::Torrent *torrent = liveTorrent(torrentId))
            (void)applyDocConfig(torrent, doc);
        return true;
    }

    // The add confirmation arrives via an alert AFTER any undo scope closed;
    // the expectation keeps the resulting journal entry annotated as undo work.
    journal->expectAnnotatedOps(torrentId, JournalOrigin::Undo, annotationCommitId);

    const BitTorrent::AddTorrentParams params = buildAddTorrentParams(doc);
    if (!session->addTorrent(descriptor.value(), params))
    {
        if (error)
            *error = tr("The engine rejected re-adding the torrent");
        return false;
    }

    // Post-add fixups that AddTorrentParams cannot express run once the
    // torrent materializes.
    const bool superSeeding = doc.value(u"superSeeding"_s).toBool();
    const QJsonArray trackers = doc.value(u"trackers"_s).toArray();
    const QJsonArray urlSeeds = doc.value(u"urlSeeds"_s).toArray();
    auto *connection = new QMetaObject::Connection;
    *connection = connect(session, &BitTorrent::Session::torrentAdded, this,
        [torrentId, superSeeding, trackers, urlSeeds, connection](BitTorrent::Torrent *torrent)
    {
        if (torrent->id().toString() != torrentId)
            return;
        QObject::disconnect(*connection);
        delete connection;

        if (superSeeding)
            torrent->setSuperSeeding(true);
        QList<BitTorrent::TrackerEntry> entries;
        for (const QJsonValue &value : trackers)
        {
            const QJsonObject obj = value.toObject();
            entries.append({.url = obj.value(u"url"_s).toString(),
                .tier = obj.value(u"tier"_s).toInt()});
        }
        if (!entries.isEmpty())
            torrent->addTrackers(entries);
        QList<QUrl> seeds;
        for (const QJsonValue &value : urlSeeds)
            seeds.append(QUrl(value.toString()));
        if (!seeds.isEmpty())
            torrent->addUrlSeeds(seeds);
    });
    return true;
}

int TorrentUndoManager::applyDocConfig(BitTorrent::Torrent *torrent, const QJsonObject &doc)
{
    using BitTorrent::Session;
    Session *session = Session::instance();
    const BitTorrent::AddTorrentParams params =
        BitTorrent::parseAddTorrentParams(doc.value(u"addParams"_s).toObject());
    int changes = 0;

    if (!params.name.isEmpty() && (torrent->name() != params.name))
    {
        torrent->setName(params.name);
        ++changes;
    }

    if (torrent->category() != params.category)
    {
        if (!params.category.isEmpty() && !session->categories().contains(params.category))
            (void)session->addCategory(params.category);
        (void)torrent->setCategory(params.category);
        ++changes;
    }

    const TagSet currentTags = torrent->tags();
    for (const Tag &tag : params.tags)
    {
        if (!currentTags.contains(tag))
        {
            (void)torrent->addTag(tag);
            ++changes;
        }
    }
    for (const Tag &tag : currentTags)
    {
        if (!params.tags.contains(tag))
        {
            (void)torrent->removeTag(tag);
            ++changes;
        }
    }

    const bool useAutoTMM = params.useAutoTMM.value_or(false);
    if (torrent->isAutoTMMEnabled() != useAutoTMM)
    {
        torrent->setAutoTMMEnabled(useAutoTMM);
        ++changes;
    }
    if (!useAutoTMM)
    {
        if (!params.savePath.isEmpty() && (torrent->savePath() != params.savePath))
        {
            torrent->setSavePath(params.savePath);
            ++changes;
        }
        const Path downloadPath = params.useDownloadPath.value_or(false)
            ? params.downloadPath : Path();
        if (torrent->downloadPath() != downloadPath)
        {
            torrent->setDownloadPath(downloadPath);
            ++changes;
        }
    }

    if (torrent->isSequentialDownload() != params.sequential)
    {
        torrent->setSequentialDownload(params.sequential);
        ++changes;
    }
    if (torrent->hasFirstLastPiecePriority() != params.firstLastPiecePriority)
    {
        torrent->setFirstLastPiecePriority(params.firstLastPiecePriority);
        ++changes;
    }
    if (torrent->uploadLimit() != params.uploadLimit)
    {
        torrent->setUploadLimit(params.uploadLimit);
        ++changes;
    }
    if (torrent->downloadLimit() != params.downloadLimit)
    {
        torrent->setDownloadLimit(params.downloadLimit);
        ++changes;
    }
    if (torrent->shareLimits() != params.shareLimits)
    {
        torrent->setShareLimits(params.shareLimits);
        ++changes;
    }
    if (params.stopCondition && (torrent->stopCondition() != *params.stopCondition))
    {
        torrent->setStopCondition(*params.stopCondition);
        ++changes;
    }

    const bool superSeeding = doc.value(u"superSeeding"_s).toBool();
    if (torrent->superSeeding() != superSeeding)
    {
        torrent->setSuperSeeding(superSeeding);
        ++changes;
    }

    const bool shouldBeStopped = params.addStopped.value_or(false);
    if (torrent->isStopped() != shouldBeStopped)
    {
        if (shouldBeStopped)
            torrent->stop();
        else
            torrent->start(params.addForced ? BitTorrent::TorrentOperatingMode::Forced
                                            : BitTorrent::TorrentOperatingMode::AutoManaged);
        ++changes;
    }

    // Trackers: replace when the stored set differs from the live one.
    const QJsonArray trackersArray = doc.value(u"trackers"_s).toArray();
    QList<BitTorrent::TrackerEntry> storedTrackers;
    QSet<QString> storedUrls;
    for (const QJsonValue &value : trackersArray)
    {
        const QJsonObject obj = value.toObject();
        storedTrackers.append({.url = obj.value(u"url"_s).toString(),
            .tier = obj.value(u"tier"_s).toInt()});
        storedUrls.insert(obj.value(u"url"_s).toString());
    }
    QSet<QString> liveUrls;
    const QList<BitTorrent::TrackerEntryStatus> liveTrackers = torrent->trackers();
    for (const BitTorrent::TrackerEntryStatus &tracker : liveTrackers)
        liveUrls.insert(tracker.url);
    if (storedUrls != liveUrls)
    {
        torrent->replaceTrackers(storedTrackers);
        ++changes;
    }

    // File renames: apply per-index differences when the layouts match.
    if (torrent->hasMetadata())
    {
        const QJsonArray storedPaths = doc.value(u"filePaths"_s).toArray();
        const PathList livePaths = torrent->filePaths();
        if (storedPaths.size() == livePaths.size())
        {
            for (int i = 0; i < livePaths.size(); ++i)
            {
                const Path storedPath {storedPaths.at(i).toString()};
                if (!storedPath.isEmpty() && (livePaths.at(i) != storedPath))
                {
                    torrent->renameFile(i, storedPath);
                    ++changes;
                }
            }
        }

        const QJsonArray storedPriorities = doc.value(u"filePriorities"_s).toArray();
        const QList<BitTorrent::DownloadPriority> livePriorities = torrent->filePriorities();
        if (storedPriorities.size() == livePriorities.size())
        {
            QList<BitTorrent::DownloadPriority> priorities;
            bool differs = false;
            for (int i = 0; i < storedPriorities.size(); ++i)
            {
                const auto priority =
                    static_cast<BitTorrent::DownloadPriority>(storedPriorities.at(i).toInt(1));
                priorities.append(priority);
                differs = differs || (priority != livePriorities.at(i));
            }
            if (differs)
            {
                torrent->prioritizeFiles(priorities);
                ++changes;
            }
        }
    }

    return changes;
}

void TorrentUndoManager::reconcileSessionDoc(const QJsonObject &targetSessionDoc)
{
    BitTorrent::Session *session = BitTorrent::Session::instance();
    const QJsonObject categories = targetSessionDoc.value(u"categories"_s).toObject();
    const QStringList liveCategories = session->categories();
    for (auto it = categories.constBegin(); it != categories.constEnd(); ++it)
    {
        if (!liveCategories.contains(it.key()))
            (void)session->addCategory(it.key(), BitTorrent::CategoryOptions::fromJSON(it->toObject()));
    }
    for (const QString &name : liveCategories)
    {
        if (!categories.contains(name))
            (void)session->removeCategory(name);
    }

    QSet<QString> targetTags;
    const QJsonArray tags = targetSessionDoc.value(u"tags"_s).toArray();
    for (const QJsonValue &value : tags)
        targetTags.insert(value.toString());
    const TagSet liveTags = session->tags();
    for (const QString &tag : std::as_const(targetTags))
    {
        if (!liveTags.contains(Tag(tag)))
            (void)session->addTag(Tag(tag));
    }
    for (const Tag &tag : liveTags)
    {
        if (!targetTags.contains(tag.toString()))
            (void)session->removeTag(tag);
    }
}

TorrentUndoManager::UndoResult TorrentUndoManager::restoreToCommit(const QString &commitId)
{
    TorrentJournal *journal = TorrentJournal::instance();
    BitTorrent::Session *session = BitTorrent::Session::instance();
    if (!journal || !journal->isAvailable() || !session)
    {
        const UndoResult result {false, tr("The journal is not available")};
        finish(result);
        return result;
    }
    if (m_busy)
    {
        const UndoResult result {false, tr("Another undo is still running")};
        finish(result);
        return result;
    }

    Git::CommitInfo info;
    QString error;
    if (!journal->store(TorrentJournal::Repo::Actions)->commitInfo(commitId, &info, &error))
    {
        const UndoResult result {false, error};
        finish(result);
        return result;
    }

    const BusyGuard guard {this};
    UndoResult result;
    const QStringList targetIds = journal->torrentIdsAtCommit(info.id);
    const QSet<QString> targetIdSet {targetIds.cbegin(), targetIds.cend()};

    journal->beginAnnotatedScope(JournalOrigin::Restore,
        u"restore: state as of %1"_s.arg(info.timestamp.toLocalTime()
            .toString(u"yyyy-MM-dd hh:mm"_s)), {}, info.id);

    int readded = 0, removed = 0, reconfigured = 0;
    for (const QString &torrentId : targetIds)
    {
        QJsonObject doc;
        if (!journal->torrentDocAtCommit(info.id, torrentId, &doc))
            continue;
        if (BitTorrent::Torrent *torrent = liveTorrent(torrentId))
        {
            const int changes = applyDocConfig(torrent, doc);
            if (changes > 0)
                ++reconfigured;
            result.applied += changes;
        }
        else
        {
            QByteArray blob;
            (void)journal->torrentBlobAtCommit(info.id, torrentId, &blob);
            QString readdError;
            if (readdTorrent(torrentId, doc, blob, info.id, &readdError))
            {
                ++readded;
                ++result.applied;
            }
            else
            {
                ++result.conflicted;
                if (result.message.isEmpty())
                    result.message = readdError;
            }
        }
    }

    const QList<BitTorrent::Torrent *> liveTorrents = session->torrents();
    for (const BitTorrent::Torrent *torrent : liveTorrents)
    {
        if (!targetIdSet.contains(torrent->id().toString()))
        {
            // Restores never touch downloaded data.
            (void)session->removeTorrent(torrent->id(),
                BitTorrent::TorrentRemoveOption::KeepContent);
            ++removed;
            ++result.applied;
        }
    }

    reconcileSessionDoc(journal->sessionDocAtCommit(info.id));
    journal->endAnnotatedScope();

    result.success = true;
    if (result.message.isEmpty())
    {
        result.message = tr("Restored state from %1 (%2 re-added, %3 removed, %4 reconfigured)")
            .arg(info.shortId, QString::number(readded), QString::number(removed),
                QString::number(reconfigured));
    }
    finish(result);
    return result;
}

TorrentUndoManager::UndoResult TorrentUndoManager::restoreMissingTorrents()
{
    TorrentJournal *journal = TorrentJournal::instance();
    if (!journal || !journal->isAvailable())
    {
        const UndoResult result {false, tr("The journal is not available")};
        finish(result);
        return result;
    }

    const BusyGuard guard {this};
    UndoResult result;
    const QStringList orphanIds = journal->journaledOnlyTorrentIds();
    for (const QString &torrentId : orphanIds)
    {
        const QJsonObject doc = journal->journaledTorrentDoc(torrentId);
        if (doc.isEmpty())
        {
            ++result.skipped;
            continue;
        }
        const QByteArray blob = journal->journaledTorrentBlob(torrentId);
        QString error;
        if (readdTorrent(torrentId, doc, blob, {}, &error))
        {
            ++result.applied;
        }
        else
        {
            ++result.conflicted;
            if (result.message.isEmpty())
                result.message = error;
        }
    }

    result.success = (result.conflicted == 0);
    if (result.message.isEmpty())
    {
        result.message = (result.applied > 0)
            ? tr("Restoring %1 journaled torrents").arg(result.applied)
            : tr("No journaled torrents to restore");
    }
    finish(result);
    return result;
}

TorrentUndoManager::UndoResult TorrentUndoManager::undoSettingsEntry(const QString &commitId)
{
    TorrentJournal *journal = TorrentJournal::instance();
    if (!journal || !journal->isAvailable())
    {
        const UndoResult result {false, tr("The journal is not available")};
        finish(result);
        return result;
    }

    Git::CommitInfo info;
    QString error;
    if (!journal->store(TorrentJournal::Repo::Settings)->commitInfo(commitId, &info, &error))
    {
        const UndoResult result {false, error};
        finish(result);
        return result;
    }
    const JournalEntry entry = decodeCommitMessage(info.message);
    if (entry.ops.isEmpty() || info.parentIds.isEmpty())
    {
        const UndoResult result {false, tr("This entry cannot be undone")};
        finish(result);
        return result;
    }

    const BusyGuard guard {this};
    const QJsonObject parentDoc = journal->settingsDocAtCommit(info.parentIds.first());

    UndoResult result;
    for (const JournalOpRecord &op : entry.ops)
    {
        const QString key = op.torrentName; // settings ops carry the key here
        if (key.isEmpty())
        {
            ++result.skipped;
            continue;
        }
        if (parentDoc.contains(key))
            SettingsStorage::instance()->storeValue(key, parentDoc.value(key).toVariant());
        else
            SettingsStorage::instance()->removeValue(key);
        ++result.applied;
    }

    result.success = true;
    result.message = tr("Reverted \"%1\"").arg(entry.summary);
    finish(result);
    return result;
}

void TorrentUndoManager::setBusy(const bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void TorrentUndoManager::finish(const UndoResult &result)
{
    if (!result.message.isEmpty())
        qCInfo(lcApp) << "Undo manager:" << (result.success ? "OK" : "FAILED") << result.message;
    emit operationCompleted(result.success, result.message);
}
