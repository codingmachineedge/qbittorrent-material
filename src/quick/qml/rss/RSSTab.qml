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

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtCore

/*!
    \qmltype RSSTab
    \brief The RSS reader tab: a warning banner, toolbar and a three-pane split
           (Feeds tree | Articles list | Article preview).

    Owns the RSS bridge models (\c RSSFeedTreeModel and \c RSSArticleModel) and
    coordinates the panes: selecting a feed repopulates the article list;
    navigating between articles marks the previous one read (mirroring the
    Widgets client) and renders the current one in the preview.
*/
Item {
    id: root

    RSSFeedTreeModel { id: feedModel }
    RSSArticleModel { id: articleModel }

    // Whether the current feed selection is a sticky "Unread" node.
    readonly property bool unreadView: feedsTree.currentStickyKind === 2
    // Whether the current selection is a sticky (All/Unread) node.
    readonly property bool stickyView: feedsTree.currentStickyKind !== 0

    Component.onCompleted: Log.debug("rss", "RSSTab opened")

    function _selectFeed(path, stickyKind) {
        Log.info("rss", "Feed selected: '" + path + "' stickyKind=" + stickyKind)
        articleModel.setFeed(path, stickyKind === 2)
        articlePreview.article = ({})
        articlesList.currentRow = -1
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- Warning banner (fetching disabled) ---------------------------
        Rectangle {
            Layout.fillWidth: true
            visible: !RSSController.processingEnabled
            implicitHeight: visible ? warnLabel.implicitHeight + Spacing.md : 0
            color: Qt.alpha(Theme.color("warning"), 0.15)

            Label {
                id: warnLabel
                anchors.fill: parent
                anchors.margins: Spacing.sm
                text: qsTr("Fetching of RSS feeds is disabled now! You can enable it in application settings.")
                color: Theme.color("warning")
                font: Typography.bodyMedium
                font.italic: true
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
            }
        }

        // ---- Toolbar ------------------------------------------------------
        RSSToolbar {
            id: toolbar
            Layout.fillWidth: true
            selectedPath: feedsTree.currentPath
            onNewSubscription: feedDialog.openForAdd(RSSController.newSubscriptionDestination(feedsTree.currentPath))
            onMarkItemsRead: RSSController.markItemRead(feedsTree.currentPath)
            onUpdateAll: RSSController.refreshAll()
            onOpenDownloader: rulesDialog.open()
            onFilterChanged: (text) => articleModel.setFilter(text)
        }

        // ---- Three-pane split ---------------------------------------------
        SplitView {
            id: sideSplit
            Layout.fillWidth: true
            Layout.fillHeight: true
            orientation: Qt.Horizontal

            Settings {
                id: sidePersist
                category: "RSSWidget/SideSplit"
                property alias feedsWidth: feedsPane.implicitWidth
            }

            // Feeds tree pane.
            Item {
                id: feedsPane
                SplitView.preferredWidth: 240
                SplitView.minimumWidth: 150

                FeedsTree {
                    id: feedsTree
                    anchors.fill: parent
                    model: feedModel
                    onSelectionChanged: root._selectFeed(currentPath, currentStickyKind)
                    onActivated: (path) => {
                        // Double-click a feed → rename (legacy behavior).
                        if (RSSController.isFeed(path))
                            renameDialog.openFor(path)
                    }
                    onContextRequested: (pos) => {
                        feedsMenu.updateState()
                        feedsMenu.popup(feedsTree, pos)
                    }
                }
            }

            // Articles + preview pane.
            ColumnLayout {
                SplitView.fillWidth: true
                spacing: 0

                Label {
                    Layout.fillWidth: true
                    Layout.margins: Spacing.sm
                    text: qsTr("Torrents: (double-click to download)")
                    font: Typography.titleSmall
                    color: Theme.color("onSurfaceVariant")
                }

                SplitView {
                    id: mainSplit
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: Qt.Horizontal

                    Settings {
                        category: "RSSWidget/MainSplit"
                        property alias articlesWidth: articlesPane.implicitWidth
                    }

                    Item {
                        id: articlesPane
                        SplitView.preferredWidth: 320
                        SplitView.minimumWidth: 180

                        ArticlesList {
                            id: articlesList
                            anchors.fill: parent
                            model: articleModel
                            stickyView: root.stickyView
                            onCurrentArticleChanged: (row) => {
                                articlePreview.showFeed = root.stickyView
                                articlePreview.article = row >= 0 ? articleModel.get(row) : ({})
                            }
                            onArticleActivated: (row) => {
                                const a = articleModel.get(row)
                                articleModel.markRead(row)
                                RSSController.downloadTorrent(a.torrentUrl)
                            }
                            onContextRequested: (pos) => {
                                const a = articleModel.get(articlesList.currentRow)
                                articlesMenu.hasTorrent = a.torrentUrl && a.torrentUrl.length > 0
                                articlesMenu.hasLink = a.link && a.link.length > 0
                                if (articlesMenu.hasTorrent || articlesMenu.hasLink)
                                    articlesMenu.popup(articlesList, pos)
                            }
                        }
                    }

                    ArticlePreview {
                        id: articlePreview
                        SplitView.fillWidth: true
                        SplitView.minimumWidth: 180
                    }
                }
            }
        }
    }

    // ---- Context menus ----------------------------------------------------

    FeedsContextMenu {
        id: feedsMenu

        function updateState() {
            const path = feedsTree.currentPath
            currentPath = path
            isSticky = feedsTree.currentStickyKind !== 0
            isFeed = !isSticky && RSSController.isFeed(path)
            isFolder = !isSticky && RSSController.isFolder(path)
            hasSelection = feedsTree.currentStickyKind !== 0 || path.length > 0 || RSSController.isFolder(path)
        }

        onRequestUpdate: RSSController.refreshItem(feedsTree.currentPath)
        onRequestMarkRead: RSSController.markItemRead(feedsTree.currentPath)
        onRequestRename: renameDialog.openFor(feedsTree.currentPath)
        onRequestEditFeed: feedDialog.openForEdit(feedsTree.currentPath)
        onRequestDelete: deleteFeedConfirm.open()
        onRequestNewFolder: newFolderDialog.open()
        onRequestNewSubscription: feedDialog.openForAdd(RSSController.newSubscriptionDestination(feedsTree.currentPath))
        onRequestCopyUrl: RSSController.copyToClipboard(RSSController.feedURLsUnder(feedsTree.currentPath).join("\n"))
        onRequestUpdateAll: RSSController.refreshAll()
    }

    ArticlesContextMenu {
        id: articlesMenu
        onRequestDownload: {
            const a = articleModel.get(articlesList.currentRow)
            articleModel.markRead(articlesList.currentRow)
            RSSController.downloadTorrent(a.torrentUrl)
        }
        onRequestOpenUrl: {
            const a = articleModel.get(articlesList.currentRow)
            articleModel.markRead(articlesList.currentRow)
            RSSController.openArticleLink(a.link)
        }
    }

    // ---- Dialogs ----------------------------------------------------------

    RSSFeedDialog {
        id: feedDialog
        onSubmitted: (url, intervalSeconds, editMode, path) => {
            let err = editMode
                ? RSSController.setFeedURL(path, url, intervalSeconds)
                : RSSController.addFeed(url, feedDialog.destFolder)
            if (err && err.length > 0)
                Snackbar.show(err)
        }
    }

    TextInputDialog {
        id: renameDialog
        property string path: ""
        title: qsTr("Rename")
        label: qsTr("New feed name:")
        function openFor(p) {
            path = p
            text = RSSController.itemName(p)
            open()
        }
        onAccepted: (newName) => {
            const err = RSSController.renameItem(path, newName)
            if (err && err.length > 0)
                Snackbar.show(qsTr("Rename failed") + ": " + err)
        }
    }

    TextInputDialog {
        id: newFolderDialog
        title: qsTr("New folder")
        label: qsTr("Folder name:")
        text: qsTr("New folder")
        onAccepted: (name) => {
            const dest = RSSController.newSubscriptionDestination(feedsTree.currentPath)
            const err = RSSController.addFolder(name, dest)
            if (err && err.length > 0)
                Snackbar.show(err)
        }
    }

    ConfirmDialog {
        id: deleteFeedConfirm
        title: qsTr("Deletion confirmation")
        text: qsTr("Are you sure you want to delete the selected RSS feeds?")
        destructive: true
        onAccepted: {
            const err = RSSController.removeItem(feedsTree.currentPath)
            if (err && err.length > 0)
                Snackbar.show(err)
        }
    }

    // ---- Automated Download Rules dialog ----------------------------------

    AutomatedDownloadRulesDialog {
        id: rulesDialog
    }

    Connections {
        target: RSSController
        function onErrorOccurred(message) { Snackbar.show(message) }
        function onArticleUrlBlocked(link) {
            Snackbar.show(qsTr("Blocked opening a local-file article URL."))
        }
    }
}
