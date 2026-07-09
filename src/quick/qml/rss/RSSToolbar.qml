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
    \qmltype RSSToolbar
    \brief The RSS tab toolbar: New subscription, Mark items read, Update all, a
           title filter field, and the RSS Downloader button.
*/
ToolBar {
    id: root

    /*! The currently-selected feed-tree path (for enabling state). */
    property string selectedPath: ""

    signal newSubscription()
    signal markItemsRead()
    signal updateAll()
    signal openDownloader()
    signal filterChanged(string text)

    Material.elevation: 0

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Spacing.sm
        anchors.rightMargin: Spacing.sm
        spacing: Spacing.xs

        ToolButton {
            text: qsTr("New subscription")
            icon.source: ""
            display: AbstractButton.TextBesideIcon
            contentItem: RowLayout {
                spacing: Spacing.xs
                MDIcon { icon: Icons.add; size: 18; color: Theme.color("onSurface") }
                Label { text: qsTr("New subscription"); font: Typography.labelLarge; color: Theme.color("onSurface") }
            }
            onClicked: {
                Log.info("rss", "Toolbar: New subscription clicked")
                root.newSubscription()
            }
        }

        ToolButton {
            contentItem: RowLayout {
                spacing: Spacing.xs
                MDIcon { icon: Icons.done_all; size: 18; color: Theme.color("onSurface") }
                Label { text: qsTr("Mark items read"); font: Typography.labelLarge; color: Theme.color("onSurface") }
            }
            onClicked: {
                Log.info("rss", "Toolbar: Mark items read clicked")
                root.markItemsRead()
            }
        }

        ToolButton {
            contentItem: RowLayout {
                spacing: Spacing.xs
                MDIcon { icon: Icons.refresh; size: 18; color: Theme.color("onSurface") }
                Label { text: qsTr("Update all"); font: Typography.labelLarge; color: Theme.color("onSurface") }
            }
            ToolTip.text: qsTr("Refresh RSS streams")
            ToolTip.visible: hovered
            onClicked: {
                Log.info("rss", "Toolbar: Update all clicked")
                root.updateAll()
            }
        }

        Item { Layout.fillWidth: true }

        FilterTextField {
            id: filterField
            Layout.preferredWidth: 200
            Layout.maximumWidth: 200
            placeholder: qsTr("Filter feed items...")
            onTextChanged: root.filterChanged(text)
        }

        ToolButton {
            contentItem: RowLayout {
                spacing: Spacing.xs
                MDIcon { icon: Icons.download; size: 18; color: Theme.color("onSurface") }
                Label { text: qsTr("RSS Downloader..."); font: Typography.labelLarge; color: Theme.color("onSurface") }
            }
            onClicked: {
                Log.info("rss", "Toolbar: RSS Downloader clicked")
                root.openDownloader()
            }
        }
    }
}
