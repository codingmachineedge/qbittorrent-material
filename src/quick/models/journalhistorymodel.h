/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

#include <qqmlintegration.h>

#include "base/torrentjournal/torrentjournalop.h"

/**
 * @brief Newest-first list model over one journal repo ("actions" or
 *        "settings") for the History panel.
 *
 * Supports the design's commit search (plain or regex over message + sha +
 * diff text). Row 0 is the newest commit, so "restore to row i" reverts
 * exactly i newer actions.
 */
class JournalHistoryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString repo READ repo WRITE setRepo NOTIFY repoChanged)
    Q_PROPERTY(QString filterText READ filterText WRITE setFilterText NOTIFY filterChanged)
    Q_PROPERTY(bool filterRegex READ filterRegex WRITE setFilterRegex NOTIFY filterChanged)
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    enum Roles
    {
        CommitIdRole = Qt::UserRole + 1,
        ShaRole,
        MessageRole,
        TimeTextRole,
        DateKeyRole,
        DiffLinesRole,
        UndoableRole,
        CanRestoreRole,
        OriginRole,
        OpCountRole
    };

    explicit JournalHistoryModel(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString repo() const;
    void setRepo(const QString &repo);
    [[nodiscard]] QString filterText() const;
    void setFilterText(const QString &text);
    [[nodiscard]] bool filterRegex() const;
    void setFilterRegex(bool regex);
    [[nodiscard]] int count() const;

    Q_INVOKABLE void refresh();

signals:
    void repoChanged();
    void filterChanged();
    void countChanged();

private:
    void reload();
    [[nodiscard]] bool matchesFilter(const TorrentJournalNS::JournalEntry &entry) const;

    QString m_repo = QStringLiteral("actions");
    QString m_filterText;
    bool m_filterRegex = false;
    QList<TorrentJournalNS::JournalEntry> m_entries;
};
