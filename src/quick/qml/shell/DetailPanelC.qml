/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import qBittorrent

/*!
    \qmltype DetailPanelC
    \brief The Card Flow persistent right detail panel: focused torrent
           header, big progress readout, action buttons, and details grid.
*/
Item {
    id: root

    required property var page

    signal deleteRequested()

    Rectangle {
        anchors.left: parent.left
        width: 1
        height: parent.height
        color: Theme.color("outlineVariant")
    }

    // No selection state.
    Column {
        anchors.centerIn: parent
        visible: !PropertiesController.hasTorrent
        spacing: 10
        MDIcon {
            anchors.horizontalCenter: parent.horizontalCenter
            name: "touch_app"; size: 44; color: Theme.color("outline")
        }
        Text {
            anchors.horizontalCenter: parent.horizontalCenter
            text: qsTr("Select a torrent")
            font.family: Typography.family
            font.pixelSize: 13
            color: Theme.color("onSurfaceVariant")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 19
        anchors.rightMargin: 18
        anchors.topMargin: 16
        anchors.bottomMargin: 16
        spacing: 14
        visible: PropertiesController.hasTorrent

        // Header.
        RowLayout {
            Layout.fillWidth: true
            spacing: 12

            Rectangle {
                readonly property color stColor: Theme.stateColor(root.page.currentState)
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                radius: 14
                color: Qt.alpha(stColor, Theme.isDark ? 0.16 : 0.12)
                MDIcon {
                    anchors.centerIn: parent
                    name: root.page.stateIcon(root.page.currentState)
                    size: 22
                    color: parent.stColor
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Text {
                    Layout.fillWidth: true
                    text: PropertiesController.name || ""
                    wrapMode: Text.WrapAnywhere
                    maximumLineCount: 2
                    elide: Text.ElideRight
                    font.family: Typography.family
                    font.pixelSize: 14
                    font.weight: Font.DemiBold
                    color: Theme.color("onSurface")
                }
            }
        }

        // Big progress readout.
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: root.page.progressLabel(PropertiesController.progressValue || 0)
                    font.family: Typography.monoFamily
                    font.pixelSize: 34
                    font.weight: Font.DemiBold
                    color: Theme.color("onSurface")
                }
                Item { Layout.fillWidth: true }
                Text {
                    Layout.alignment: Qt.AlignBottom
                    bottomPadding: 6
                    text: qsTr("of %1").arg(PropertiesController.totalSize || "—")
                    font.family: Typography.family
                    font.pixelSize: 12
                    color: Theme.color("onSurfaceVariant")
                }
            }
            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 8
                radius: 4
                color: Theme.color("surfaceContainerHigh")
                Rectangle {
                    width: parent.width * Math.min(1, PropertiesController.progressValue || 0)
                    height: parent.height
                    radius: 4
                    color: root.page.barColor(root.page.currentState)
                    Behavior on width { NumberAnimation { duration: 400 } }
                }
            }
        }

        // Action buttons.
        RowLayout {
            Layout.fillWidth: true
            spacing: 6
            Repeater {
                model: [
                    { icon: "play_arrow", primary: true, tip: qsTr("Start"), act: function() { TransferController.start() } },
                    { icon: "pause", primary: false, tip: qsTr("Stop"), act: function() { TransferController.stop() } },
                    { icon: "fact_check", primary: false, tip: qsTr("Force recheck"), act: function() { TransferController.forceRecheck() } },
                    { icon: "delete", primary: false, danger: true, tip: qsTr("Remove"), act: function() { root.deleteRequested() } }
                ]
                delegate: Rectangle {
                    required property var modelData
                    Layout.fillWidth: true
                    Layout.preferredHeight: 38
                    radius: 12
                    color: modelData.primary
                        ? Theme.color("primaryContainer")
                        : (btnMouse.containsMouse
                            ? (modelData.danger ? Theme.color("errorContainer") : Theme.color("surfaceContainerHigh"))
                            : Theme.color("surfaceVariant"))
                    MDIcon {
                        anchors.centerIn: parent
                        name: modelData.icon
                        size: 20
                        color: modelData.primary ? Theme.color("onPrimaryContainer")
                            : (modelData.danger ? Theme.color("error") : Theme.color("onSurface"))
                    }
                    MouseArea {
                        id: btnMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: modelData.act()
                    }
                }
            }
        }

        // Details.
        TorrentDetailsGrid {
            Layout.fillWidth: true
            Layout.fillHeight: true
            columns: 1
        }
    }
}
