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

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QVariantMap>

/**
 * @file rsscontroller.h
 * @brief The @c RSSController QML singleton — the imperative surface for the RSS
 *        tab (feeds tree, articles list, article preview and toolbar).
 *
 * Models (RSSFeedTreeModel / RSSArticleModel) provide the observable data;
 * @c RSSController provides the verbs: adding feeds/folders, renaming, deleting,
 * moving, refreshing, marking read, plus the article actions (download torrent,
 * open news URL — with the legacy local-file safety guard) and the HTML the
 * preview pane renders. Session-wide processing toggles are surfaced as
 * notifiable properties so the "feeds disabled" banners bind live.
 *
 * All state is read from / written to @c RSS::Session and
 * @c RSS::AutoDownloader (the engine facades); this controller never caches
 * feed data — it subscribes to the engine and forwards.
 */
class RSSController final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    /// Whether the RSS session is fetching feeds (drives the tab warning banner).
    Q_PROPERTY(bool processingEnabled READ processingEnabled WRITE setProcessingEnabled NOTIFY processingEnabledChanged)
    /// Whether the auto-downloader is enabled (drives the rules-dialog banner).
    Q_PROPERTY(bool autoDownloadEnabled READ autoDownloadEnabled WRITE setAutoDownloadEnabled NOTIFY autoDownloadEnabledChanged)
    /// Aggregate unread-article count across every feed (root folder).
    Q_PROPERTY(int unreadCount READ unreadCount NOTIFY unreadCountChanged)

public:
    /// QML singleton factory — returns the shared instance.
    static RSSController *create(QQmlEngine *engine, QJSEngine *scriptEngine);

    explicit RSSController(QObject *parent = nullptr);
    ~RSSController() override;

    bool processingEnabled() const;
    void setProcessingEnabled(bool enabled);

    bool autoDownloadEnabled() const;
    void setAutoDownloadEnabled(bool enabled);

    int unreadCount() const;

    // ---- Feed / folder tree mutations (return "" on success, else an error) ---

    /// Add a subscription @p url stored under @p destFolderPath ("" = root).
    Q_INVOKABLE QString addFeed(const QString &url, const QString &destFolderPath);
    /// Create a new folder named @p name under @p destFolderPath ("" = root).
    Q_INVOKABLE QString addFolder(const QString &name, const QString &destFolderPath);
    /// Remove the item at @p path (skips the sticky nodes — path "" is ignored).
    Q_INVOKABLE QString removeItem(const QString &path);
    /// Rename the item at @p path to @p newName (kept within the same parent).
    Q_INVOKABLE QString renameItem(const QString &path, const QString &newName);
    /// Move the item at @p path into folder @p destFolderPath.
    Q_INVOKABLE QString moveItem(const QString &path, const QString &destFolderPath);
    /// Change a feed's URL (Feed options dialog).
    Q_INVOKABLE QString setFeedURL(const QString &path, const QString &url, int refreshIntervalSeconds);

    // ---- Refresh / read state ------------------------------------------------

    Q_INVOKABLE void refreshItem(const QString &path);
    Q_INVOKABLE void refreshAll();
    Q_INVOKABLE void markItemRead(const QString &path);

    // ---- Introspection helpers used by the QML views ------------------------

    /// Display name of the item at @p path (empty for unknown/sticky).
    Q_INVOKABLE QString itemName(const QString &path) const;
    /// Whether the item at @p path is a feed.
    Q_INVOKABLE bool isFeed(const QString &path) const;
    /// Whether the item at @p path is a folder.
    Q_INVOKABLE bool isFolder(const QString &path) const;
    /// A feed's URL (empty when @p path is not a feed).
    Q_INVOKABLE QString feedURL(const QString &path) const;
    /// A feed's refresh interval in seconds (0 = use the session default).
    Q_INVOKABLE int feedRefreshInterval(const QString &path) const;
    /// Newline-joined feed URLs beneath @p path (a single feed, or a folder tree).
    Q_INVOKABLE QStringList feedURLsUnder(const QString &path) const;
    /// Destination folder path for a "New subscription" given the current selection.
    Q_INVOKABLE QString newSubscriptionDestination(const QString &selectedPath) const;

    // ---- Clipboard convenience ----------------------------------------------

    /// A supported URL from the clipboard, else "https://" (New subscription prefill).
    Q_INVOKABLE QString clipboardURL() const;
    /// Copy @p text to the system clipboard.
    Q_INVOKABLE void copyToClipboard(const QString &text) const;

    // ---- Article actions -----------------------------------------------------

    /// Add a torrent from an article's torrent URL (or magnet).
    Q_INVOKABLE void downloadTorrent(const QString &torrentUrl);
    /// Open an article's news URL in the browser. Local-file URLs are blocked
    /// (security guard mirrored from the Widgets client). Returns false + emits
    /// @c articleUrlBlocked when refused.
    Q_INVOKABLE bool openArticleLink(const QString &link);

    /// Build the preview HTML for an article map (keys: title, dateText, feedName,
    /// author, description, link). @p showFeed adds the "Feed:" row (sticky views).
    Q_INVOKABLE QString renderArticleHtml(const QVariantMap &article, bool showFeed) const;

signals:
    void processingEnabledChanged();
    void autoDownloadEnabledChanged();
    void unreadCountChanged();

    /// A user-facing error occurred (shown as a snackbar / dialog by QML).
    void errorOccurred(const QString &message);
    /// An article URL was refused because it resolved to a local file.
    void articleUrlBlocked(const QString &link);

private:
    void connectSession();

    static RSSController *s_instance;
};
