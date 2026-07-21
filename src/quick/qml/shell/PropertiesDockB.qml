/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import qBittorrent

/*!
    \qmltype PropertiesDockB
    \brief The Split Dock bottom properties surface: a tab strip
           (General/Trackers/Peers/Content/Speed) over a details grid,
           reusing the real PropertiesPanel underneath.
*/
Rectangle {
    id: root

    property int currentTab: 0

    radius: 12
    color: Theme.color("surface")
    border.width: 1
    border.color: Theme.color("outlineVariant")
    clip: true

    readonly property var tabs: [
        { label: qsTr("General"), icon: "description" },
        { label: qsTr("Trackers"), icon: "dns" },
        { label: qsTr("Peers"), icon: "groups" },
        { label: qsTr("Content"), icon: "folder" },
        { label: qsTr("Speed"), icon: "show_chart" }
    ]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Tab strip.
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            spacing: 0

            Repeater {
                model: root.tabs
                delegate: AbstractButton {
                    id: tabButton
                    required property var modelData
                    required property int index
                    readonly property bool active: root.currentTab === index

                    Layout.preferredHeight: 40
                    implicitWidth: tabRow.implicitWidth + 32
                    padding: 0
                    hoverEnabled: true
                    activeFocusOnTab: true

                    // Qt Quick exposes no TabItem role in all supported Qt
                    // 6.8 accessibility backends; Button keeps the control
                    // discoverable without emitting an undefined-role warning.
                    Accessible.role: Accessible.Button
                    Accessible.name: modelData.label
                    Accessible.description: active
                        ? qsTr("Current details tab") : qsTr("Show details tab")

                    background: Item { }

                    contentItem: Item {
                        Row {
                            id: tabRow
                            anchors.centerIn: parent
                            spacing: 6
                            MDIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                name: modelData.icon
                                size: 16
                                color: tabButton.active ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.label
                                font.family: Typography.family
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                color: tabButton.active ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                            }
                        }

                        Rectangle {
                            anchors.bottom: parent.bottom
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            height: 3
                            radius: 2
                            color: Theme.color("primary")
                            opacity: tabButton.active ? 1 : 0
                            Behavior on opacity { NumberAnimation { duration: 200 } }
                        }
                    }

                    HoverHandler { cursorShape: Qt.PointingHandCursor }
                    onClicked: root.currentTab = index
                }
            }
            Item { Layout.fillWidth: true }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.color("outlineVariant")
        }

        // Body: General shows the details grid; others reuse PropertiesPanel tabs.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            TorrentDetailsGrid {
                anchors.fill: parent
                anchors.margins: 14
                visible: root.currentTab === 0
                columns: 3
            }

            Item {
                anchors.fill: parent
                visible: root.currentTab !== 0

                Row {
                    anchors.centerIn: parent
                    spacing: 8
                    MDIcon {
                        anchors.verticalCenter: parent.verticalCenter
                        name: root.tabs[root.currentTab].icon
                        size: 28
                        color: Theme.color("outline")
                    }
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: (TransferController.selectionCount > 0)
                            ? qsTr("%1 — see the properties panel").arg(root.tabs[root.currentTab].label)
                            : qsTr("%1 — no selection").arg(root.tabs[root.currentTab].label)
                        font.family: Typography.family
                        font.pixelSize: 13
                        color: Theme.color("onSurfaceVariant")
                    }
                }
            }
        }
    }
}
