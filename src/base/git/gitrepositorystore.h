/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QByteArray>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringList>

namespace Git
{
    struct CommitInfo
    {
        QString id;            // full hex OID
        QString shortId;       // first 8 hex characters
        QDateTime timestamp;   // committer time (UTC)
        QString summary;       // first line of the message
        QString message;       // full message including trailers
        QStringList parentIds; // full hex OIDs
    };

    enum class FileChangeStatus
    {
        Added,
        Deleted,
        Modified,
        Other
    };

    struct FileChange
    {
        FileChangeStatus status = FileChangeStatus::Other;
        QString path;    // path after the commit
        QString oldPath; // path before the commit (differs only for renames)
    };

    /**
     * Synchronous, main-thread wrapper around an in-process libgit2 repository
     * rooted at a managed directory. Generalized from the Workspace feature's
     * embedded Git engine so multiple subsystems (workspace tabs, the torrent
     * journal) can share one implementation.
     *
     * The store owns no open handles between calls — every operation opens the
     * repository, works, and frees everything, which keeps the on-disk state
     * safe against crashes at any point.
     */
    class GitRepositoryStore
    {
    public:
        GitRepositoryStore(const QString &rootPath, const QString &signatureName,
            const QString &signatureEmail);
        ~GitRepositoryStore();

        GitRepositoryStore(const GitRepositoryStore &) = delete;
        GitRepositoryStore &operator=(const GitRepositoryStore &) = delete;

        [[nodiscard]] QString rootPath() const;

        // Opens the repository, creating it (initial branch "main") when absent.
        // An existing directory whose repository is unusable is moved aside to
        // "<root>-recovery-<timestamp>" and a fresh repository is initialized;
        // recoveryPath() then names the preserved copy.
        [[nodiscard]] bool ensureRepository(QString *error = nullptr);
        [[nodiscard]] QString recoveryPath() const;

        // Stages every working-tree change (additions, modifications, deletions)
        // and commits to HEAD. When the resulting tree matches HEAD's the commit
        // is skipped and *committed is set to false (still a success).
        [[nodiscard]] bool commitAll(const QString &message, QString *commitId = nullptr,
            bool *committed = nullptr, QString *error = nullptr);

        // Empty string when the repository has no commits yet.
        [[nodiscard]] QString headCommitId(QString *error = nullptr) const;

        // Newest-first history from HEAD. maxCount <= 0 means unlimited.
        [[nodiscard]] QList<CommitInfo> log(int maxCount, QString *error = nullptr) const;

        // Resolves full or abbreviated commit ids.
        [[nodiscard]] bool commitInfo(const QString &commitId, CommitInfo *info,
            QString *error = nullptr) const;

        // Reads one file from the commit's tree. A missing file is a success
        // with *found == false; other failures return false.
        [[nodiscard]] bool readFileAtCommit(const QString &commitId, const QString &relPath,
            QByteArray *content, bool *found = nullptr, QString *error = nullptr) const;

        // Names of the immediate blob children of a subdirectory ("" = root)
        // in the commit's tree. A missing subdirectory yields an empty list.
        [[nodiscard]] bool listTreeFiles(const QString &commitId, const QString &subdir,
            QStringList *files, QString *error = nullptr) const;

        // Files changed by the commit relative to its first parent (the whole
        // tree for a root commit).
        [[nodiscard]] bool changedFilesInCommit(const QString &commitId,
            QList<FileChange> *changes, QString *error = nullptr) const;

        [[nodiscard]] static bool writeFileAtomically(const QString &path,
            const QByteArray &bytes, QString *error = nullptr);

    private:
        QString m_rootPath;
        QString m_recoveryPath;
        QByteArray m_signatureName;
        QByteArray m_signatureEmail;
    };
}
