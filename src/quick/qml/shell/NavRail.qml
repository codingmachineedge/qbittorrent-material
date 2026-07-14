/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import qBittorrent

/*!
    \qmltype NavRail
    \brief Left navigation rail for the Tonal Rail (A: 84px, labeled) and
           Card Flow (C: 64px, icon-only) styles, topped by the Add FAB.
*/
Item {
    id: root

    property var navModel: []
    property int currentTab: 0
    readonly property bool labeled: Theme.isTonalRail

    signal navRequested(int index)
    signal addRequested()

    implicitWidth: labeled ? 84 : 64

    Column {
        anchors.top: parent.top
        anchors.topMargin: 4
        anchors.horizontalCenter: parent.horizontalCenter
        spacing: root.labeled ? 8 : 10

        // Add-torrent FAB.
        AbstractButton {
            id: addButton
            anchors.horizontalCenter: parent.horizontalCenter
            width: root.labeled ? 56 : 48
            height: root.labeled ? 56 : 48
            padding: 0
            hoverEnabled: true
            activeFocusOnTab: true
            scale: down ? 0.94 : 1

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("Add torrent")

            background: Rectangle {
                radius: root.labeled ? 16 : 14
                color: Theme.color("primaryContainer")
                border.width: addButton.visualFocus ? 2 : 0
                border.color: Theme.color("primary")
            }

            contentItem: MDIcon {
                name: "add"
                size: root.labeled ? 24 : 22
                color: Theme.color("onPrimaryContainer")
            }

            Behavior on scale { NumberAnimation { duration: 150 } }
            HoverHandler { cursorShape: Qt.PointingHandCursor }
            onClicked: root.addRequested()
        }

        Item { width: 1; height: root.labeled ? 2 : 0 }

        Repeater {
            model: root.navModel

            delegate: AbstractButton {
                id: navItem
                required property var modelData
                readonly property bool active: root.currentTab === modelData.page

                anchors.horizontalCenter: parent.horizontalCenter
                width: root.labeled ? 72 : 44
                height: root.labeled ? (itemColumn.implicitHeight + 4) : 44
                padding: 0
                hoverEnabled: true
                activeFocusOnTab: true

                Accessible.role: Accessible.Button
                Accessible.name: navItem.modelData.label
                Accessible.description: navItem.active
                    ? qsTr("Current destination") : qsTr("Open destination")

                background: Item { }

                contentItem: Item {
                    Column {
                        id: itemColumn
                        anchors.centerIn: parent
                        spacing: 4

                        Rectangle {
                            anchors.horizontalCenter: parent.horizontalCenter
                            width: root.labeled ? 56 : 44
                            height: root.labeled ? 32 : 44
                            radius: root.labeled ? 16 : 22
                            color: navItem.active
                                ? Theme.color("primaryContainer")
                                : (navItem.hovered ? Theme.color("hoverStrong") : "transparent")
                            border.width: navItem.visualFocus ? 2 : 0
                            border.color: Theme.color("primary")
                            Behavior on color { ColorAnimation { duration: 250 } }

                            MDIcon {
                                anchors.centerIn: parent
                                name: navItem.modelData.icon
                                size: 22
                                fill: navItem.active
                                color: navItem.active
                                    ? Theme.color("onPrimaryContainer")
                                    : Theme.color("onSurfaceVariant")
                            }

                            Rectangle {
                                visible: (navItem.modelData.badge || 0) > 0 && !navItem.active
                                anchors.top: parent.top
                                anchors.right: parent.right
                                anchors.topMargin: -3
                                anchors.rightMargin: root.labeled ? 2 : -2
                                width: Math.max(16, navBadge.implicitWidth + 8)
                                height: 16
                                radius: 8
                                color: Theme.color("primary")
                                Text {
                                    id: navBadge
                                    anchors.centerIn: parent
                                    text: String(navItem.modelData.badge || 0)
                                    color: Theme.color("onPrimary")
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                }
                            }
                        }

                        Text {
                            visible: root.labeled
                            anchors.horizontalCenter: parent.horizontalCenter
                            text: navItem.modelData.label
                            font.family: Typography.family
                            font.pixelSize: 12
                            font.weight: Font.Medium
                            font.letterSpacing: 0.3
                            color: navItem.active
                                ? Theme.color("onSurface") : Theme.color("onSurfaceVariant")
                        }
                    }
                }

                HoverHandler { cursorShape: Qt.PointingHandCursor }
                onClicked: root.navRequested(modelData.page)
            }
        }
    }
}
