/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026  qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtCore
import qBittorrent

/*!
    Bordered torrent-properties panel. Tabs sit at the top like the supplied
    transfer surface, while re-clicking the current tab retains qBittorrent's
    useful collapse behavior.
*/
Item {
    id: root

    readonly property int collapsedHeight: Spacing.controlHeight + Spacing.sm
    property bool expanded: true
    property int currentTab: 0

    clip: true

    Rectangle {
        anchors.fill: parent
        z: -1
        radius: Spacing.radiusPanel
        color: Theme.color("surface")
        border.width: 1
        border.color: Theme.color("outline")
    }

    Settings {
        id: persist
        category: "TorrentProperties"
        property int currentTab: -1
        property bool visible: false
    }

    function _selectTab(index) {
        if (index === currentTab && expanded) {
            expanded = false
            persist.visible = false
            persist.currentTab = -1
            Log.info("ui", "Properties panel collapsed")
            return
        }
        currentTab = index
        if (!expanded) {
            expanded = true
            persist.visible = true
        }
        persist.currentTab = index
        PropertiesController.currentTab = index
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Layout.preferredHeight: root.collapsedHeight
            currentIndex: root.currentTab
            Material.elevation: 0
            background: Rectangle { color: "transparent" }

            component PropertyTab: TabButton {
                id: button
                property string tabIcon: ""
                property string tabText: ""
                implicitHeight: root.collapsedHeight
                width: implicitWidth
                leftPadding: Spacing.md
                rightPadding: Spacing.md
                contentItem: RowLayout {
                    spacing: Spacing.xs
                    MDIcon {
                        icon: button.tabIcon
                        size: 17
                        color: button.checked
                            ? Theme.color("primary") : Theme.color("muted")
                    }
                    Label {
                        text: button.tabText
                        font: Typography.titleSmall
                        color: button.checked
                            ? Theme.color("primary") : Theme.color("muted")
                    }
                }
            }

            PropertyTab { tabIcon: Icons.description; tabText: qsTr("General"); onClicked: root._selectTab(0) }
            PropertyTab { tabIcon: Icons.dns; tabText: qsTr("Trackers"); onClicked: root._selectTab(1) }
            PropertyTab { tabIcon: Icons.groups; tabText: qsTr("Peers"); onClicked: root._selectTab(2) }
            PropertyTab { tabIcon: Icons.publicIcon; tabText: qsTr("HTTP sources"); onClicked: root._selectTab(3) }
            PropertyTab { tabIcon: Icons.folder; tabText: qsTr("Content"); onClicked: root._selectTab(4) }
            PropertyTab { tabIcon: Icons.show_chart; tabText: qsTr("Speed"); onClicked: root._selectTab(5) }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 1
            color: Theme.color("outlineVariant")
            visible: root.expanded
        }

        Item {
            id: stackHost
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.expanded
            clip: true

            StackLayout {
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

            ColumnLayout {
                anchors.centerIn: parent
                spacing: Spacing.sm
                visible: !PropertiesController.hasTorrent

                MDIcon {
                    icon: Icons.description
                    size: 34
                    color: Theme.color("outline")
                    Layout.alignment: Qt.AlignHCenter
                }
                Label {
                    text: qsTr("Select a torrent to view its properties")
                    font: Typography.bodyLarge
                    color: Theme.color("muted")
                    Layout.alignment: Qt.AlignHCenter
                }
            }
        }
    }

    Connections {
        target: PropertiesController
        function onHasTorrentChanged() {
            if (root.expanded && PropertiesController.hasTorrent)
                PropertiesController.currentTab = root.currentTab
        }
    }

    Component.onCompleted: {
        expanded = persist.visible
        currentTab = persist.currentTab >= 0 ? persist.currentTab : 0
        if (expanded)
            PropertiesController.currentTab = currentTab
        Log.debug("ui", "Design-system Properties panel ready")
    }
}
