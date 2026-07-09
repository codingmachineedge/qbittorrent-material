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

#include <QAbstractListModel>
#include <QFuture>
#include <QFutureWatcher>
#include <QHash>
#include <QList>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QUrl>

#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/logging.h"

/**
 * @file webseedlistmodel.h
 * @brief The @c WebSeedListModel — flat list model backing Properties → HTTP Sources.
 *
 * One row per web seed (URL seed) of the current torrent. The list is read
 * asynchronously via @c Torrent::fetchURLSeeds() (bridged with a
 * @c QFutureWatcher) and mutated through the @ref addWebSeed / @ref removeWebSeeds
 * / @ref editWebSeed invokables, each of which re-reads the list afterwards.
 */
class WebSeedListModel final : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged FINAL)

public:
    enum Roles
    {
        UrlRole = Qt::UserRole + 1 // "url"
    };

    explicit WebSeedListModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        qCDebug(lcModel) << "WebSeedListModel constructed";
    }

    /// Bind the model to a torrent (nullptr clears it). Refreshes immediately.
    void setTorrent(BitTorrent::Torrent *torrent)
    {
        if (m_torrent == torrent)
            return;

        m_torrent = torrent;
        qCDebug(lcModel) << "WebSeedListModel torrent ->" << (torrent ? torrent->name() : QStringLiteral("<none>"));
        if (!m_torrent)
        {
            beginResetModel();
            m_urls.clear();
            endResetModel();
            emit countChanged();
        }
        else
        {
            refresh();
        }
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_urls.size());
    }

    QVariant data(const QModelIndex &index, const int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || (index.row() < 0) || (index.row() >= m_urls.size()))
            return {};

        if ((role == UrlRole) || (role == Qt::DisplayRole))
            return m_urls.at(index.row());
        return {};
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {{UrlRole, "url"}};
    }

    /// The URL at @p row (or empty when out of range).
    Q_INVOKABLE QString urlAt(const int row) const
    {
        return ((row >= 0) && (row < m_urls.size())) ? m_urls.at(row) : QString();
    }

    /// Whether @p url is already present (case-sensitive, exact match).
    Q_INVOKABLE bool contains(const QString &url) const
    {
        return m_urls.contains(url);
    }

    /// Add a single web seed URL. Returns false if the torrent is unset or the
    /// URL is already present.
    Q_INVOKABLE bool addWebSeed(const QString &url)
    {
        if (!m_torrent || url.isEmpty() || m_urls.contains(url))
        {
            qCDebug(lcModel) << "WebSeedListModel: add rejected for" << url;
            return false;
        }

        qCInfo(lcModel) << "Adding web seed:" << url;
        m_torrent->addUrlSeeds({QUrl(url)});
        refresh();
        return true;
    }

    /// Remove the given web seed URLs.
    Q_INVOKABLE void removeWebSeeds(const QStringList &urls)
    {
        if (!m_torrent || urls.isEmpty())
            return;

        qCInfo(lcModel) << "Removing" << urls.size() << "web seed(s)";
        QList<QUrl> toRemove;
        toRemove.reserve(urls.size());
        for (const QString &url : urls)
            toRemove.append(QUrl(url));
        m_torrent->removeUrlSeeds(toRemove);
        refresh();
    }

    /// Replace @p oldUrl with @p newUrl. Returns false when the new URL already
    /// exists or the torrent is unset.
    Q_INVOKABLE bool editWebSeed(const QString &oldUrl, const QString &newUrl)
    {
        if (!m_torrent || newUrl.isEmpty() || (oldUrl == newUrl) || m_urls.contains(newUrl))
        {
            qCDebug(lcModel) << "WebSeedListModel: edit rejected" << oldUrl << "->" << newUrl;
            return false;
        }

        qCInfo(lcModel) << "Editing web seed:" << oldUrl << "->" << newUrl;
        m_torrent->removeUrlSeeds({QUrl(oldUrl)});
        m_torrent->addUrlSeeds({QUrl(newUrl)});
        refresh();
        return true;
    }

    /// Re-read the URL seed list asynchronously.
    Q_INVOKABLE void refresh()
    {
        if (!m_torrent)
            return;

        const quint64 generation = ++m_generation;
        auto *watcher = new QFutureWatcher<QList<QUrl>>(this);
        connect(watcher, &QFutureWatcherBase::finished, this
                , [this, watcher, generation, torrent = QPointer<BitTorrent::Torrent>(m_torrent)]
        {
            watcher->deleteLater();
            if ((generation != m_generation) || (m_torrent != torrent) || !m_torrent)
                return;

            QStringList urls;
            const QList<QUrl> result = watcher->result();
            urls.reserve(result.size());
            for (const QUrl &url : result)
                urls.append(url.toString());

            beginResetModel();
            m_urls = std::move(urls);
            endResetModel();
            emit countChanged();
            qCDebug(lcModel) << "WebSeedListModel refreshed:" << m_urls.size() << "seed(s)";
        });
        watcher->setFuture(m_torrent->fetchURLSeeds());
    }

signals:
    void countChanged();

private:
    BitTorrent::Torrent *m_torrent = nullptr;
    QStringList m_urls;
    quint64 m_generation = 0;
};
