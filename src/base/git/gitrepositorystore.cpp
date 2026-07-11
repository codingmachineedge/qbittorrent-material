/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "gitrepositorystore.h"

#include <memory>

#include <QDateTime>
#include <QDir>
#include <QFileInfo>
#include <QSaveFile>

#include <git2.h>

#include "base/logging.h"

namespace
{
    QString gitErrorText(const QString &fallback)
    {
        if (const git_error *error = git_error_last(); error && error->message)
            return QString::fromUtf8(error->message);
        return fallback;
    }

    void setError(QString *error, const QString &prefix)
    {
        if (error)
            *error = prefix + QStringLiteral(": ") + gitErrorText(QStringLiteral("unknown libgit2 error"));
    }

    template <typename T, void (*FreeFn)(T *)>
    struct GitDeleter
    {
        void operator()(T *handle) const
        {
            if (handle)
                FreeFn(handle);
        }
    };

    using RepositoryPtr = std::unique_ptr<git_repository, GitDeleter<git_repository, git_repository_free>>;
    using IndexPtr = std::unique_ptr<git_index, GitDeleter<git_index, git_index_free>>;
    using TreePtr = std::unique_ptr<git_tree, GitDeleter<git_tree, git_tree_free>>;
    using CommitPtr = std::unique_ptr<git_commit, GitDeleter<git_commit, git_commit_free>>;
    using SignaturePtr = std::unique_ptr<git_signature, GitDeleter<git_signature, git_signature_free>>;
    using ReferencePtr = std::unique_ptr<git_reference, GitDeleter<git_reference, git_reference_free>>;
    using RevwalkPtr = std::unique_ptr<git_revwalk, GitDeleter<git_revwalk, git_revwalk_free>>;
    using ObjectPtr = std::unique_ptr<git_object, GitDeleter<git_object, git_object_free>>;
    using BlobPtr = std::unique_ptr<git_blob, GitDeleter<git_blob, git_blob_free>>;
    using DiffPtr = std::unique_ptr<git_diff, GitDeleter<git_diff, git_diff_free>>;
    using TreeEntryPtr = std::unique_ptr<git_tree_entry, GitDeleter<git_tree_entry, git_tree_entry_free>>;

    RepositoryPtr openRepository(const QString &rootPath, QString *error)
    {
        git_repository *repository = nullptr;
        const QByteArray encoded = QDir::fromNativeSeparators(rootPath).toUtf8();
        if (git_repository_open_ext(&repository, encoded.constData(),
                GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr) != 0)
        {
            setError(error, QStringLiteral("Could not open the local Git repository"));
            return nullptr;
        }
        return RepositoryPtr {repository};
    }

    QString oidToString(const git_oid *oid)
    {
        char buffer[GIT_OID_HEXSZ + 1] {};
        git_oid_tostr(buffer, sizeof(buffer), oid);
        return QString::fromLatin1(buffer);
    }

    CommitPtr resolveCommit(git_repository *repository, const QString &commitId, QString *error)
    {
        git_object *object = nullptr;
        const QByteArray spec = commitId.toLatin1();
        if (git_revparse_single(&object, repository, spec.constData()) != 0)
        {
            setError(error, QStringLiteral("Unknown commit %1").arg(commitId));
            return nullptr;
        }
        const ObjectPtr guard {object};
        git_commit *commit = nullptr;
        if (git_object_peel(reinterpret_cast<git_object **>(&commit), object, GIT_OBJECT_COMMIT) != 0)
        {
            setError(error, QStringLiteral("%1 is not a commit").arg(commitId));
            return nullptr;
        }
        return CommitPtr {commit};
    }

    Git::CommitInfo makeCommitInfo(git_commit *commit)
    {
        Git::CommitInfo info;
        info.id = oidToString(git_commit_id(commit));
        info.shortId = info.id.left(8);
        info.timestamp = QDateTime::fromSecsSinceEpoch(git_commit_time(commit), Qt::UTC);
        info.summary = QString::fromUtf8(git_commit_summary(commit));
        info.message = QString::fromUtf8(git_commit_message(commit));
        const unsigned int parentCount = git_commit_parentcount(commit);
        info.parentIds.reserve(parentCount);
        for (unsigned int i = 0; i < parentCount; ++i)
            info.parentIds.append(oidToString(git_commit_parent_id(commit, i)));
        return info;
    }
}

namespace Git
{
    GitRepositoryStore::GitRepositoryStore(const QString &rootPath, const QString &signatureName,
        const QString &signatureEmail)
        : m_rootPath {QDir::cleanPath(rootPath)}
        , m_signatureName {signatureName.toUtf8()}
        , m_signatureEmail {signatureEmail.toUtf8()}
    {
        git_libgit2_init();
    }

    GitRepositoryStore::~GitRepositoryStore()
    {
        git_libgit2_shutdown();
    }

    QString GitRepositoryStore::rootPath() const
    {
        return m_rootPath;
    }

    QString GitRepositoryStore::recoveryPath() const
    {
        return m_recoveryPath;
    }

    bool GitRepositoryStore::ensureRepository(QString *error)
    {
        const QByteArray encoded = QDir::fromNativeSeparators(m_rootPath).toUtf8();
        git_repository *repository = nullptr;
        if (git_repository_open_ext(&repository, encoded.constData(),
                GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr) == 0)
        {
            const RepositoryPtr guard {repository};
            const bool usable = !git_repository_is_bare(repository)
                && !git_repository_head_detached(repository);
            if (usable)
                return true;
        }

        // A directory whose .git is unusable (or a plain directory blocking a
        // fresh init) is moved aside so no user data is ever destroyed.
        const QFileInfo gitDirectory {QDir(m_rootPath).filePath(QStringLiteral(".git"))};
        if (gitDirectory.exists())
        {
            const QString timestamp = QDateTime::currentDateTimeUtc()
                .toString(QStringLiteral("yyyyMMdd-hhmmss"));
            const QString recovery = m_rootPath + QStringLiteral("-recovery-") + timestamp;
            if (!QDir().rename(m_rootPath, recovery))
            {
                if (error)
                {
                    *error = QStringLiteral("The repository at %1 is unusable and could not be moved aside")
                        .arg(QDir::toNativeSeparators(m_rootPath));
                }
                return false;
            }
            m_recoveryPath = recovery;
            qCWarning(lcApp) << "Git store: unusable repository moved aside to" << recovery;
        }

        git_repository_init_options options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
        options.flags = GIT_REPOSITORY_INIT_MKPATH;
        options.initial_head = "main";
        repository = nullptr;
        if ((git_repository_init_ext(&repository, encoded.constData(), &options) != 0) || !repository)
        {
            setError(error, QStringLiteral("Could not initialize the local Git repository"));
            git_repository_free(repository);
            return false;
        }
        git_repository_free(repository);
        return true;
    }

    bool GitRepositoryStore::commitAll(const QString &message, QString *commitId,
        bool *committed, QString *error)
    {
        if (committed)
            *committed = false;

        const RepositoryPtr repository = openRepository(m_rootPath, error);
        if (!repository)
            return false;

        git_index *rawIndex = nullptr;
        if (git_repository_index(&rawIndex, repository.get()) != 0)
        {
            setError(error, QStringLiteral("Could not open the Git index"));
            return false;
        }
        const IndexPtr index {rawIndex};

        // "git add -A": add_all stages new/modified paths, update_all records
        // modifications and deletions of already-tracked paths.
        char allPaths[] = "*";
        char *pathStrings[] = {allPaths};
        git_strarray pathspec {pathStrings, 1};
        if ((git_index_add_all(index.get(), &pathspec, GIT_INDEX_ADD_DEFAULT, nullptr, nullptr) != 0)
            || (git_index_update_all(index.get(), &pathspec, nullptr, nullptr) != 0)
            || (git_index_write(index.get()) != 0))
        {
            setError(error, QStringLiteral("Could not stage the working tree"));
            return false;
        }

        git_oid treeId;
        git_tree *rawTree = nullptr;
        if ((git_index_write_tree(&treeId, index.get()) != 0)
            || (git_tree_lookup(&rawTree, repository.get(), &treeId) != 0))
        {
            setError(error, QStringLiteral("Could not create the Git tree"));
            return false;
        }
        const TreePtr tree {rawTree};

        CommitPtr parentCommit;
        git_oid parentId;
        if (git_reference_name_to_id(&parentId, repository.get(), "HEAD") == 0)
        {
            git_commit *rawParent = nullptr;
            if (git_commit_lookup(&rawParent, repository.get(), &parentId) == 0)
            {
                parentCommit.reset(rawParent);
                git_tree *rawParentTree = nullptr;
                if (git_commit_tree(&rawParentTree, rawParent) == 0)
                {
                    const TreePtr parentTree {rawParentTree};
                    if (git_oid_equal(git_tree_id(parentTree.get()), &treeId))
                    {
                        if (commitId)
                            *commitId = oidToString(&parentId);
                        return true; // nothing changed — skip the empty commit
                    }
                }
            }
        }

        git_signature *rawSignature = nullptr;
        if (git_signature_now(&rawSignature, m_signatureName.constData(),
                m_signatureEmail.constData()) != 0)
        {
            setError(error, QStringLiteral("Could not create the Git signature"));
            return false;
        }
        const SignaturePtr signature {rawSignature};

        git_oid newCommitId;
        const QByteArray commitMessage = message.toUtf8();
        const git_commit *parents[] = {parentCommit.get()};
        const int result = git_commit_create(&newCommitId, repository.get(), "HEAD",
            signature.get(), signature.get(), "UTF-8", commitMessage.constData(), tree.get(),
            parentCommit ? 1 : 0, parentCommit ? parents : nullptr);
        if (result != 0)
        {
            setError(error, QStringLiteral("Could not create the commit"));
            return false;
        }
        if (commitId)
            *commitId = oidToString(&newCommitId);
        if (committed)
            *committed = true;
        return true;
    }

    QString GitRepositoryStore::headCommitId(QString *error) const
    {
        const RepositoryPtr repository = openRepository(m_rootPath, error);
        if (!repository)
            return {};
        git_oid headId;
        if (git_reference_name_to_id(&headId, repository.get(), "HEAD") != 0)
            return {}; // unborn branch — no commits yet
        return oidToString(&headId);
    }

    QList<CommitInfo> GitRepositoryStore::log(const int maxCount, QString *error) const
    {
        QList<CommitInfo> entries;
        const RepositoryPtr repository = openRepository(m_rootPath, error);
        if (!repository)
            return entries;

        git_revwalk *rawWalk = nullptr;
        if (git_revwalk_new(&rawWalk, repository.get()) != 0)
        {
            setError(error, QStringLiteral("Could not walk the Git history"));
            return entries;
        }
        const RevwalkPtr walk {rawWalk};
        git_revwalk_sorting(walk.get(), GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME);
        if (git_revwalk_push_head(walk.get()) != 0)
            return entries; // no commits yet

        git_oid oid;
        while (git_revwalk_next(&oid, walk.get()) == 0)
        {
            git_commit *rawCommit = nullptr;
            if (git_commit_lookup(&rawCommit, repository.get(), &oid) != 0)
                continue;
            const CommitPtr commit {rawCommit};
            entries.append(makeCommitInfo(commit.get()));
            if ((maxCount > 0) && (entries.size() >= maxCount))
                break;
        }
        return entries;
    }

    bool GitRepositoryStore::commitInfo(const QString &commitId, CommitInfo *info,
        QString *error) const
    {
        const RepositoryPtr repository = openRepository(m_rootPath, error);
        if (!repository)
            return false;
        const CommitPtr commit = resolveCommit(repository.get(), commitId, error);
        if (!commit)
            return false;
        if (info)
            *info = makeCommitInfo(commit.get());
        return true;
    }

    bool GitRepositoryStore::readFileAtCommit(const QString &commitId, const QString &relPath,
        QByteArray *content, bool *found, QString *error) const
    {
        if (found)
            *found = false;
        const RepositoryPtr repository = openRepository(m_rootPath, error);
        if (!repository)
            return false;
        const CommitPtr commit = resolveCommit(repository.get(), commitId, error);
        if (!commit)
            return false;

        git_tree *rawTree = nullptr;
        if (git_commit_tree(&rawTree, commit.get()) != 0)
        {
            setError(error, QStringLiteral("Could not read the commit tree"));
            return false;
        }
        const TreePtr tree {rawTree};

        git_tree_entry *rawEntry = nullptr;
        const QByteArray encodedPath = relPath.toUtf8();
        const int lookup = git_tree_entry_bypath(&rawEntry, tree.get(), encodedPath.constData());
        if (lookup == GIT_ENOTFOUND)
            return true; // success, file absent from that tree
        if (lookup != 0)
        {
            setError(error, QStringLiteral("Could not look up %1").arg(relPath));
            return false;
        }
        const TreeEntryPtr entry {rawEntry};
        if (git_tree_entry_type(entry.get()) != GIT_OBJECT_BLOB)
            return true; // a directory of that name — treat as absent file

        git_blob *rawBlob = nullptr;
        if (git_blob_lookup(&rawBlob, repository.get(), git_tree_entry_id(entry.get())) != 0)
        {
            setError(error, QStringLiteral("Could not read %1").arg(relPath));
            return false;
        }
        const BlobPtr blob {rawBlob};
        if (content)
        {
            *content = QByteArray(static_cast<const char *>(git_blob_rawcontent(blob.get())),
                static_cast<qsizetype>(git_blob_rawsize(blob.get())));
        }
        if (found)
            *found = true;
        return true;
    }

    bool GitRepositoryStore::listTreeFiles(const QString &commitId, const QString &subdir,
        QStringList *files, QString *error) const
    {
        if (files)
            files->clear();
        const RepositoryPtr repository = openRepository(m_rootPath, error);
        if (!repository)
            return false;
        const CommitPtr commit = resolveCommit(repository.get(), commitId, error);
        if (!commit)
            return false;

        git_tree *rawRoot = nullptr;
        if (git_commit_tree(&rawRoot, commit.get()) != 0)
        {
            setError(error, QStringLiteral("Could not read the commit tree"));
            return false;
        }
        TreePtr tree {rawRoot};

        if (!subdir.isEmpty())
        {
            git_tree_entry *rawEntry = nullptr;
            const QByteArray encodedPath = subdir.toUtf8();
            const int lookup = git_tree_entry_bypath(&rawEntry, tree.get(), encodedPath.constData());
            if (lookup == GIT_ENOTFOUND)
                return true; // no such directory in that commit — empty list
            if (lookup != 0)
            {
                setError(error, QStringLiteral("Could not look up %1").arg(subdir));
                return false;
            }
            const TreeEntryPtr entry {rawEntry};
            if (git_tree_entry_type(entry.get()) != GIT_OBJECT_TREE)
                return true;
            git_tree *rawSubtree = nullptr;
            if (git_tree_lookup(&rawSubtree, repository.get(), git_tree_entry_id(entry.get())) != 0)
            {
                setError(error, QStringLiteral("Could not open %1").arg(subdir));
                return false;
            }
            tree.reset(rawSubtree);
        }

        const size_t count = git_tree_entrycount(tree.get());
        for (size_t i = 0; i < count; ++i)
        {
            const git_tree_entry *entry = git_tree_entry_byindex(tree.get(), i);
            if (entry && (git_tree_entry_type(entry) == GIT_OBJECT_BLOB) && files)
                files->append(QString::fromUtf8(git_tree_entry_name(entry)));
        }
        return true;
    }

    bool GitRepositoryStore::changedFilesInCommit(const QString &commitId,
        QList<FileChange> *changes, QString *error) const
    {
        if (changes)
            changes->clear();
        const RepositoryPtr repository = openRepository(m_rootPath, error);
        if (!repository)
            return false;
        const CommitPtr commit = resolveCommit(repository.get(), commitId, error);
        if (!commit)
            return false;

        git_tree *rawTree = nullptr;
        if (git_commit_tree(&rawTree, commit.get()) != 0)
        {
            setError(error, QStringLiteral("Could not read the commit tree"));
            return false;
        }
        const TreePtr tree {rawTree};

        TreePtr parentTree;
        if (git_commit_parentcount(commit.get()) > 0)
        {
            git_commit *rawParent = nullptr;
            if (git_commit_parent(&rawParent, commit.get(), 0) != 0)
            {
                setError(error, QStringLiteral("Could not read the parent commit"));
                return false;
            }
            const CommitPtr parent {rawParent};
            git_tree *rawParentTree = nullptr;
            if (git_commit_tree(&rawParentTree, parent.get()) != 0)
            {
                setError(error, QStringLiteral("Could not read the parent tree"));
                return false;
            }
            parentTree.reset(rawParentTree);
        }

        git_diff *rawDiff = nullptr;
        if (git_diff_tree_to_tree(&rawDiff, repository.get(), parentTree.get(), tree.get(),
                nullptr) != 0)
        {
            setError(error, QStringLiteral("Could not diff the commit"));
            return false;
        }
        const DiffPtr diff {rawDiff};

        const size_t deltaCount = git_diff_num_deltas(diff.get());
        for (size_t i = 0; i < deltaCount; ++i)
        {
            const git_diff_delta *delta = git_diff_get_delta(diff.get(), i);
            if (!delta)
                continue;
            FileChange change;
            change.path = QString::fromUtf8(delta->new_file.path);
            change.oldPath = QString::fromUtf8(delta->old_file.path);
            switch (delta->status)
            {
            case GIT_DELTA_ADDED:
                change.status = FileChangeStatus::Added;
                break;
            case GIT_DELTA_DELETED:
                change.status = FileChangeStatus::Deleted;
                change.path = change.oldPath;
                break;
            case GIT_DELTA_MODIFIED:
            case GIT_DELTA_RENAMED:
            case GIT_DELTA_COPIED:
                change.status = FileChangeStatus::Modified;
                break;
            default:
                change.status = FileChangeStatus::Other;
                break;
            }
            if (changes)
                changes->append(change);
        }
        return true;
    }

    bool GitRepositoryStore::writeFileAtomically(const QString &path, const QByteArray &bytes,
        QString *error)
    {
        if (!QDir().mkpath(QFileInfo(path).absolutePath()))
        {
            if (error)
                *error = QStringLiteral("Could not create the directory for %1").arg(path);
            return false;
        }
        QSaveFile file {path};
        if (!file.open(QIODevice::WriteOnly))
        {
            if (error)
                *error = file.errorString();
            return false;
        }
        if ((file.write(bytes) != bytes.size()) || !file.commit())
        {
            if (error)
                *error = file.errorString();
            return false;
        }
        return true;
    }
}
