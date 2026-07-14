/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import qBittorrent

/*!
    \qmltype FilterChips
    \brief Status filter chip row (Tonal Rail / Card Flow styles), backed by
           the StatusFilterModel counts and the shared filter proxy.
*/
Flow {
    id: root

    required property var filterProxy

    spacing: 8

    StatusFilterModel {
        id: statusModel
        hideZero: false
    }

    Repeater {
        model: statusModel

        delegate: AbstractButton {
            id: chip
            required property string label
            required property int count
            required property int value
            readonly property bool selected: root.filterProxy
                && (root.filterProxy.statusFilter === value)

            height: 32
            width: chipRow.implicitWidth + 28
            padding: 0
            hoverEnabled: true
            activeFocusOnTab: true

            Accessible.role: Accessible.Button
            Accessible.name: qsTr("%1 filter, %2 torrents").arg(chip.label).arg(chip.count)
            Accessible.description: chip.selected
                ? qsTr("Current filter") : qsTr("Apply filter")

            background: Rectangle {
                radius: 16
                color: chip.selected ? Theme.color("primaryContainer") : "transparent"
                border.width: chip.visualFocus ? 2 : 1
                border.color: chip.selected ? Theme.color("primaryContainer") : Theme.color("outline")

                Behavior on color { ColorAnimation { duration: 200 } }
            }

            contentItem: Row {
                id: chipRow
                anchors.centerIn: parent
                spacing: 6

                MDIcon {
                    visible: chip.selected
                    anchors.verticalCenter: parent.verticalCenter
                    name: "check"
                    size: 16
                    color: Theme.color("onPrimaryContainer")
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: chip.label
                    font: Typography.labelLarge
                    color: chip.selected
                        ? Theme.color("onPrimaryContainer") : Theme.color("onSurfaceVariant")
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: String(chip.count)
                    font: Typography.metadataMono
                    opacity: 0.75
                    color: chip.selected
                        ? Theme.color("onPrimaryContainer") : Theme.color("onSurfaceVariant")
                }
            }

            HoverHandler { cursorShape: Qt.PointingHandCursor }
            onClicked: {
                if (root.filterProxy)
                    root.filterProxy.statusFilter = value
            }
        }
    }
}
