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
import qBittorrent

/*!
    \qmltype FeedsTree
    \brief The RSS feeds tree — the sticky "All"/"Unread" nodes above the real
           feed/folder hierarchy, each row showing an icon, name and unread count.

    Backed by \c RSSFeedTreeModel. Exposes the current selection (\l currentPath
    and \l currentStickyKind) plus \l activated (double-click) and
    \l contextRequested (right-click) so the hosting \c RSSTab can drive the
    article list, dialogs and the feeds context menu.
*/
Rectangle {
    id: root

    /*! The \c RSSFeedTreeModel to display. */
    property alias model: tree.model

    /*! Engine path of the current node ("" for a sticky node or the root). */
    property string currentPath: ""

    /*! Sticky kind of the current node: 0 regular, 1 All, 2 Unread. */
    property int currentStickyKind: 1

    /*! Emitted whenever the current node changes. */
    signal selectionChanged()

    /*! Emitted on double-click; \c path is the activated node's engine path. */
    signal activated(string path)

    /*! Emitted on right-click; \c pos is in FeedsTree coordinates. */
    signal contextRequested(point pos)

    color: Theme.color("surface")
    border.width: 1
    border.color: Theme.color("outlineVariant")
    radius: Spacing.radiusCard

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 1
        spacing: 0

        Label {
            Layout.fillWidth: true
            Layout.margins: Spacing.sm
            text: qsTr("RSS feeds")
            font: Typography.titleSmall
            color: Theme.color("onSurfaceVariant")
        }

        TreeView {
            id: tree
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            boundsBehavior: Flickable.StopAtBounds

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            selectionModel: ItemSelectionModel {}

            delegate: TreeViewDelegate {
                id: del
                implicitHeight: 34
                indentation: 16

                required property int stickyKind
                required property string path
                required property bool isFeed
                required property bool isFolder
                required property bool hasError
                required property bool isLoading
                required property int unreadCount

                readonly property bool isCurrent: root.currentPath === path && root.currentStickyKind === stickyKind

                background: Rectangle {
                    color: del.isCurrent
                           ? Qt.alpha(Theme.color("primary"), 0.12)
                           : (del.hovered ? Qt.alpha(Theme.color("onSurface"), 0.08) : "transparent")
                }

                contentItem: RowLayout {
                    spacing: Spacing.xs

                    MDIcon {
                        size: 18
                        color: del.hasError ? Theme.color("error") : Theme.color("onSurfaceVariant")
                        icon: {
                            if (del.stickyKind === 1) return Icons.inbox
                            if (del.stickyKind === 2) return Icons.mail
                            if (del.isFolder) return Icons.folder
                            if (del.isLoading) return Icons.progress_activity
                            if (del.hasError) return Icons.block
                            return Icons.rss_feed
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: del.model.name + (del.unreadCount > 0 ? "  (" + del.unreadCount + ")" : "")
                        elide: Text.ElideRight
                        font: del.unreadCount > 0
                              ? Qt.font({ family: Typography.family, pixelSize: Typography.bodyMedium.pixelSize, weight: Font.DemiBold })
                              : Typography.bodyMedium
                        color: del.isCurrent ? Theme.color("primary") : Theme.color("onSurface")
                    }
                }

                TapHandler {
                    acceptedButtons: Qt.LeftButton
                    onTapped: {
                        root.currentPath = del.path
                        root.currentStickyKind = del.stickyKind
                        Log.debug("rss", "FeedsTree selected: '" + del.path + "' sticky=" + del.stickyKind)
                        root.selectionChanged()
                    }
                    onDoubleTapped: root.activated(del.path)
                }

                TapHandler {
                    acceptedButtons: Qt.RightButton
                    onTapped: (eventPoint) => {
                        root.currentPath = del.path
                        root.currentStickyKind = del.stickyKind
                        root.selectionChanged()
                        const p = del.mapToItem(root, eventPoint.position.x, eventPoint.position.y)
                        root.contextRequested(Qt.point(p.x, p.y))
                    }
                }
            }
        }
    }
}
