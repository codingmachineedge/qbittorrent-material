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

/*!
    \qmltype ArticlesList
    \brief The flat list of articles for the selected feed/folder/sticky node.

    Read/unread is styled per DESIGN_SYSTEM (unread → primary + SemiBold, read →
    muted). Navigating away from an article marks the *previous* one read
    (mirroring the Widgets client); double-clicking downloads its torrent.
*/
Rectangle {
    id: root

    /*! The \c RSSArticleModel to display. */
    property alias model: list.model

    /*! Whether a sticky (All/Unread) node is current (affects preview header). */
    property bool stickyView: false

    /*! Currently-focused article row (-1 = none). */
    property int currentRow: -1

    /*! Emitted when the focused article changes (\c row is the new row). */
    signal currentArticleChanged(int row)

    /*! Emitted on double-click (download torrent). */
    signal articleActivated(int row)

    /*! Emitted on right-click; \c pos is in ArticlesList coordinates. */
    signal contextRequested(point pos)

    color: Theme.color("surface")
    border.width: 1
    border.color: Theme.color("outlineVariant")
    radius: Spacing.radiusCard

    property int _previousRow: -1

    onCurrentRowChanged: {
        // Leaving an article marks the previous one read (legacy behavior).
        if (_previousRow >= 0 && _previousRow !== currentRow)
            list.model.markRead(_previousRow)
        _previousRow = currentRow
        currentArticleChanged(currentRow)
    }

    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: 1
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        delegate: ItemDelegate {
            id: del
            width: ListView.view.width
            implicitHeight: 30

            required property int index
            required property string title
            required property bool isRead
            required property bool hasTorrent

            highlighted: root.currentRow === index

            background: Rectangle {
                color: del.highlighted
                       ? Qt.alpha(Theme.color("primary"), 0.12)
                       : (del.hovered ? Qt.alpha(Theme.color("onSurface"), 0.08) : "transparent")
            }

            contentItem: RowLayout {
                spacing: Spacing.xs

                MDIcon {
                    icon: del.isRead ? Icons.mark_email_read : Icons.mail
                    size: 16
                    color: del.isRead ? Theme.color("onSurfaceVariant") : Theme.color("primary")
                }

                Label {
                    Layout.fillWidth: true
                    text: del.title
                    elide: Text.ElideRight
                    color: del.isRead ? Theme.color("onSurfaceVariant") : Theme.color("primary")
                    font: del.isRead
                          ? Typography.bodyMedium
                          : Qt.font({ family: Typography.family, pixelSize: Typography.bodyMedium.pixelSize, weight: Font.DemiBold })
                }

                MDIcon {
                    visible: del.hasTorrent
                    icon: Icons.download
                    size: 14
                    color: Theme.color("onSurfaceVariant")
                }
            }

            onClicked: root.currentRow = index
            onDoubleClicked: {
                root.currentRow = index
                Log.info("rss", "Article activated (download): row " + index)
                root.articleActivated(index)
            }

            TapHandler {
                acceptedButtons: Qt.RightButton
                onTapped: (eventPoint) => {
                    root.currentRow = del.index
                    const p = del.mapToItem(root, eventPoint.position.x, eventPoint.position.y)
                    root.contextRequested(Qt.point(p.x, p.y))
                }
            }
        }

        // Empty-state hint.
        Label {
            anchors.centerIn: parent
            visible: list.count === 0
            text: qsTr("No articles")
            color: Theme.color("onSurfaceVariant")
            font: Typography.bodyMedium
        }
    }
}
