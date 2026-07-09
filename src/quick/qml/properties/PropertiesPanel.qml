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
    \qmltype PropertiesPanel
    \brief The bottom torrent-properties pane: a stacked set of tabs above a
           tab bar (General / Trackers / Peers / HTTP Sources / Content / Speed).

    Rebuild of the legacy \c PropertiesWidget + \c PropTabBar. The content stack
    sits on top and the tab bar below; clicking the already-active tab collapses
    the pane (leaving just the tab bar), matching the legacy slide behavior.
    The current tab and expanded state persist under \c TorrentProperties/*.
    Switching tabs (or expanding) writes \c PropertiesController.currentTab so the
    controller refreshes only that tab's dynamic data.
*/
Item {
    id: root

    /*! Collapsed height = just the tab bar. */
    readonly property int collapsedHeight: tabBar.height

    /*! Whether the content stack is shown (vs. collapsed to the tab bar). */
    property bool expanded: true

    /*! Index of the active tab (0..5). */
    property int currentTab: 0

    Settings {
        id: persist
        category: "TorrentProperties"
        property int currentTab: -1   // -1 = start collapsed (legacy default)
        property bool visible: false
    }

    Component.onCompleted: {
        expanded = persist.visible
        currentTab = persist.currentTab >= 0 ? persist.currentTab : 0
        if (expanded)
            PropertiesController.currentTab = currentTab
        Log.debug("ui", "PropertiesPanel ready; expanded=" + expanded + " tab=" + currentTab)
    }

    function _selectTab(index) {
        if (index === currentTab && expanded) {
            // Reclick on the active tab → collapse.
            expanded = false
            persist.visible = false
            persist.currentTab = -1
            Log.info("ui", "PropertiesPanel collapsed")
            return
        }
        currentTab = index
        if (!expanded) {
            expanded = true
            persist.visible = true
        }
        persist.currentTab = index
        Log.info("ui", "PropertiesPanel tab -> " + index)
        PropertiesController.currentTab = index
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // ---- Content stack ----------------------------------------------------
        Item {
            id: stackHost
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.expanded
            clip: true

            StackLayout {
                id: stack
                anchors.fill: parent
                currentIndex: root.currentTab
                visible: PropertiesController.hasTorrent

                GeneralTab {}
                TrackersTab {}
                PeersTab {}
                HttpSourcesTab {}
                ContentTab {}
                SpeedTab {}
            }

            // Placeholder shown when no torrent is selected.
            ColumnLayout {
                anchors.centerIn: parent
                spacing: Spacing.sm
                visible: !PropertiesController.hasTorrent

                MDIcon {
                    icon: Icons.description
                    size: 40
                    color: Theme.color("outline")
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: qsTr("Select a torrent to view its properties")
                    font: Typography.bodyMedium
                    color: Theme.color("onSurfaceVariant")
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }

        // Divider between stack and tab bar.
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.color("outlineVariant")
            visible: root.expanded
        }

        // ---- Tab bar (bottom) -------------------------------------------------
        TabBar {
            id: tabBar
            Layout.fillWidth: true
            currentIndex: root.currentTab
            Material.elevation: 0

            component PropTab: TabButton {
                id: tb
                property string tabIcon: ""
                property string tabText: ""
                width: implicitWidth
                contentItem: RowLayout {
                    spacing: Spacing.xs
                    MDIcon {
                        icon: tb.tabIcon
                        size: 18
                        color: tb.checked ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                        Layout.alignment: Qt.AlignVCenter
                    }
                    Label {
                        text: tb.tabText
                        font: Typography.titleSmall
                        color: tb.checked ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                        Layout.alignment: Qt.AlignVCenter
                    }
                }
            }

            PropTab { tabIcon: Icons.description; tabText: qsTr("General");      onClicked: root._selectTab(0) }
            PropTab { tabIcon: Icons.dns;         tabText: qsTr("Trackers");     onClicked: root._selectTab(1) }
            PropTab { tabIcon: Icons.groups;      tabText: qsTr("Peers");        onClicked: root._selectTab(2) }
            PropTab { tabIcon: Icons.publicIcon;      tabText: qsTr("HTTP Sources"); onClicked: root._selectTab(3) }
            PropTab { tabIcon: Icons.folder;      tabText: qsTr("Content");      onClicked: root._selectTab(4) }
            PropTab { tabIcon: Icons.show_chart;  tabText: qsTr("Speed");        onClicked: root._selectTab(5) }
        }
    }

    // Refresh the active tab's dynamic data when the current torrent changes.
    Connections {
        target: PropertiesController
        function onHasTorrentChanged() {
            if (root.expanded && PropertiesController.hasTorrent)
                PropertiesController.currentTab = root.currentTab
        }
    }
}
