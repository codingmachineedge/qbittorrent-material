/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import qBittorrent

/*!
    \qmltype TorrentDetailsGrid
    \brief A label→value grid of the focused torrent's properties, bound to
           PropertiesController (the same engine bridge the properties tabs
           use). Reused by the B dock and the C detail panel.
*/
Item {
    id: root

    property int columns: 3

    readonly property var fields: [
        { l: qsTr("Size"), v: PropertiesController.totalSize },
        { l: qsTr("Progress"), v: PropertiesController.progress },
        { l: qsTr("Status"), v: PropertiesController.eta },
        { l: qsTr("Ratio"), v: PropertiesController.shareRatio },
        { l: qsTr("Down"), v: PropertiesController.downloadSpeed },
        { l: qsTr("Up"), v: PropertiesController.uploadSpeed },
        { l: qsTr("Seeds"), v: PropertiesController.seeds },
        { l: qsTr("Peers"), v: PropertiesController.peers },
        { l: qsTr("Downloaded"), v: PropertiesController.downloaded },
        { l: qsTr("Uploaded"), v: PropertiesController.uploaded },
        { l: qsTr("Added"), v: PropertiesController.addedOn },
        { l: qsTr("Save path"), v: PropertiesController.savePath }
    ]

    GridLayout {
        anchors.fill: parent
        visible: PropertiesController.hasTorrent
        columns: root.columns
        columnSpacing: 28
        rowSpacing: 8

        Repeater {
            model: root.fields
            delegate: RowLayout {
                required property var modelData
                Layout.fillWidth: true
                spacing: 8

                Text {
                    text: modelData.l
                    font.family: Typography.family
                    font.pixelSize: 13
                    color: Theme.color("onSurfaceVariant")
                }
                Item { Layout.fillWidth: true }
                Text {
                    Layout.maximumWidth: 160
                    text: modelData.v && ("" + modelData.v).length > 0 ? modelData.v : "—"
                    elide: Text.ElideRight
                    horizontalAlignment: Text.AlignRight
                    font.family: Typography.monoFamily
                    font.pixelSize: 13
                    color: Theme.color("onSurface")
                }
            }
        }
    }

    // Empty state when nothing is selected.
    Row {
        anchors.centerIn: parent
        visible: !PropertiesController.hasTorrent
        spacing: 8
        MDIcon {
            anchors.verticalCenter: parent.verticalCenter
            name: "info"; size: 20; color: Theme.color("outline")
        }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Select a torrent to see its details")
            font.family: Typography.family
            font.pixelSize: 13
            color: Theme.color("onSurfaceVariant")
        }
    }
}
