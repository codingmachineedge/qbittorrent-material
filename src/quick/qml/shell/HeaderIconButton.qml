/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls.Material
import qBittorrent

/*!
    \qmltype HeaderIconButton
    \brief 40px round icon button used across the redesigned header
           (bell, history, theme toggle, settings), with an optional badge.
*/
Rectangle {
    id: root

    property string iconName: ""
    property int iconSize: 22
    property string tooltip: ""
    property bool active: false
    property int badgeCount: 0
    property color iconColor: Theme.color("onSurface")

    signal clicked()

    width: 40
    height: 40
    radius: 20
    color: root.active ? Theme.color("primaryContainer")
                       : (mouse.containsMouse ? Theme.color("hoverStrong") : "transparent")

    Behavior on color { ColorAnimation { duration: 200 } }

    MDIcon {
        anchors.centerIn: parent
        name: root.iconName
        size: root.iconSize
        color: root.active ? Theme.color("onPrimaryContainer") : root.iconColor
    }

    Rectangle {
        visible: root.badgeCount > 0
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 3
        anchors.rightMargin: 3
        width: Math.max(16, badgeLabel.implicitWidth + 8)
        height: 16
        radius: 8
        color: Theme.color("error")

        Text {
            id: badgeLabel
            anchors.centerIn: parent
            text: root.badgeCount > 99 ? "99+" : String(root.badgeCount)
            color: "#ffffff"
            font.family: Typography.family
            font.pixelSize: 10
            font.weight: Font.Bold
        }
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ToolTip.visible: mouse.containsMouse && (root.tooltip.length > 0)
    ToolTip.delay: 600
    ToolTip.text: root.tooltip
}
