/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026  qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import qBittorrent

/*!
    Persistent 248 px navigation from the supplied design system. Top-level
    destinations remain fixed; when Transfers is active the lower region hosts
    the real status/category/tag/tracker filters against the shared proxy.
*/
Rectangle {
    id: root

    required property var shell
    property int currentIndex: 0
    property int transfersCount: 0
    property int rssUnread: 0
    property var transferProxy: null

    signal navigateRequested(int index)
    signal optionsRequested()

    implicitWidth: Spacing.navigationWidth
    color: Theme.color("surface")

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.color("outline")
    }

    component NavigationItem: ItemDelegate {
        id: item
        property string glyph: ""
        property string label: ""
        property int destination: -1
        property int count: -1
        readonly property bool selected: root.currentIndex === destination

        Layout.fillWidth: true
        implicitHeight: Spacing.controlHeight
        leftPadding: Spacing.md
        rightPadding: Spacing.md
        topPadding: 0
        bottomPadding: 0
        Accessible.name: label

        background: Rectangle {
            radius: Spacing.radiusControl
            color: item.selected
                ? Theme.color("primaryContainer")
                : (item.hovered || item.down ? Theme.color("surfaceWarm") : "transparent")
            border.width: item.activeFocus ? 2 : 0
            border.color: Theme.color("primary")
            Behavior on color { ColorAnimation { duration: Spacing.motionFast } }
        }

        contentItem: RowLayout {
            spacing: Spacing.md
            MDIcon {
                icon: item.glyph
                size: 18
                color: item.selected
                    ? Theme.color("primary") : Theme.color("muted")
            }
            Label {
                text: item.label
                font: item.selected ? Typography.titleSmall : Typography.bodyLarge
                color: item.selected
                    ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Label {
                visible: item.count >= 0
                text: item.count
                font: Typography.metadataMono
                color: Theme.color("muted")
            }
        }

        onClicked: root.navigateRequested(destination)
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Spacing.md
        anchors.rightMargin: Spacing.md
        anchors.topMargin: Spacing.lg
        anchors.bottomMargin: Spacing.md
        spacing: 2

        Label {
            text: qsTr("LIBRARY")
            font: Typography.navLabel
            color: Theme.color("muted")
            leftPadding: Spacing.md
            bottomPadding: Spacing.xs
        }

        NavigationItem {
            glyph: Icons.download
            label: qsTr("Transfers")
            destination: 0
            count: root.transfersCount
        }

        Label {
            text: qsTr("TOOLS")
            font: Typography.navLabel
            color: Theme.color("muted")
            leftPadding: Spacing.md
            topPadding: Spacing.xl
            bottomPadding: Spacing.xs
        }

        NavigationItem { glyph: Icons.search; label: qsTr("Search"); destination: 1 }
        NavigationItem {
            glyph: Icons.rss_feed
            label: qsTr("RSS reader")
            destination: 2
            count: root.rssUnread
        }
        NavigationItem { glyph: Icons.article; label: qsTr("Execution log"); destination: 3 }
        NavigationItem { glyph: Icons.edit; label: qsTr("Workspace"); destination: 4 }

        Rectangle {
            Layout.fillWidth: true
            Layout.topMargin: Spacing.md
            Layout.bottomMargin: Spacing.sm
            implicitHeight: 1
            color: Theme.color("outlineVariant")
        }

        Label {
            visible: root.currentIndex === 0 && root.shell.sidebarVisible
            text: qsTr("FILTERS")
            font: Typography.navLabel
            color: Theme.color("muted")
            leftPadding: Spacing.md
            bottomPadding: Spacing.xs
        }

        FilterSidebar {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.currentIndex === 0 && root.shell.sidebarVisible
            proxy: root.transferProxy
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.currentIndex !== 0 || !root.shell.sidebarVisible
        }

        NavigationItem {
            glyph: Icons.settings
            label: qsTr("Options")
            destination: -2
            onClicked: root.optionsRequested()
        }
    }
}
