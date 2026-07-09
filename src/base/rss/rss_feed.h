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

#include <chrono>

#include <QBasicTimer>
#include <QHash>
#include <QList>
#include <QtContainerFwd>
#include <QUuid>
#include <QVariantHash>

#include "base/path.h"
#include "rss_item.h"

class AsyncFileStorage;

namespace Net
{
    class DownloadHandler;
    struct DownloadResult;
}

namespace RSS
{
    class Article;
    class Session;

    namespace Private
    {
        class FeedSerializer;
        class Parser;
        struct ParsingResult;
    }

    /// A leaf node: a single subscribed RSS feed. Owns its `Article`s (capped at the
    /// session's max-articles-per-feed), downloads/refreshes on a per-feed interval,
    /// tracks loading/error state and the feed icon. Emits granular signals the RSS
    /// models react to (subscribe, never poll). Log fetch/parse/state via `lcRss`.
    class Feed final : public Item
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(Feed)

        friend class Session;

        Feed(Session *session, const QUuid &uid, const QString &url, const QString &path, std::chrono::seconds refreshInterval);
        ~Feed() override;

    public:
        QList<Article *> articles() const override;
        int unreadCount() const override;
        void markAsRead() override;
        void refresh() override;
        void updateFetchDelay() override;

        QUuid uid() const;
        QString url() const;
        QString title() const;
        QString lastBuildDate() const;
        bool hasError() const;
        bool isLoading() const;
        Article *articleByGUID(const QString &guid) const;
        Path iconPath() const;

        std::chrono::seconds refreshInterval() const;
        void setRefreshInterval(std::chrono::seconds refreshInterval);

        QJsonValue toJsonValue(bool withData = false) const override;

    signals:
        void iconLoaded(Feed *feed = nullptr);
        void titleChanged(Feed *feed = nullptr);
        void stateChanged(Feed *feed = nullptr);
        void urlChanged(const QString &oldURL);
        void refreshIntervalChanged(std::chrono::seconds oldRefreshInterval);

    private slots:
        void handleSessionProcessingEnabledChanged(bool enabled);
        void handleMaxArticlesPerFeedChanged(int n);
        void handleIconDownloadFinished(const Net::DownloadResult &result);
        void handleDownloadFinished(const Net::DownloadResult &result);
        void handleParsingFinished(const Private::ParsingResult &result);
        void handleArticleRead(Article *article);
        void handleArticleLoadFinished(QList<QVariantHash> articles);

    private:
        void timerEvent(QTimerEvent *event) override;
        void cleanup() override;
        void load();
        void store();
        void storeDeferred();
        bool addArticle(const QVariantHash &articleData);
        void removeOldestArticle();
        void increaseUnreadCount();
        void decreaseUnreadCount();
        void downloadIcon();
        int updateArticles(const QList<QVariantHash> &loadedArticles);
        void setURL(const QString &url);

        Session *m_session = nullptr;
        Private::Parser *m_parser = nullptr;
        Private::FeedSerializer *m_serializer = nullptr;
        const QUuid m_uid;
        QString m_url;
        std::chrono::seconds m_refreshInterval;
        QString m_title;
        QString m_lastBuildDate;
        bool m_hasError = false;
        bool m_isLoading = false;
        bool m_isInitialized = false;
        bool m_pendingRefresh = false;
        QHash<QString, Article *> m_articles;
        QList<Article *> m_articlesByDate;
        int m_unreadCount = 0;
        Path m_iconPath;
        Path m_dataFileName;
        QBasicTimer m_savingTimer;
        bool m_dirty = false;
        Net::DownloadHandler *m_downloadHandler = nullptr;
    };
}
