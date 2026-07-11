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
    \qmltype NotificationsSheet
    \brief The notification center. It surfaces recent journaled actions (the
           design's "every action is committed to the action log" model) with
           a filter, and a footer link into the full History manager.
*/
Sheet {
    id: root
    sheetWidth: 396

    signal closeRequested()
    signal openHistoryRequested()

    JournalHistoryModel {
        id: feed
        repo: "actions"
        filterText: filterField.text
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header.
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 20
            Layout.rightMargin: 16
            Layout.bottomMargin: 10
            spacing: 8
            Text {
                text: qsTr("Notifications")
                font.family: Typography.family
                font.pixelSize: 16
                font.weight: Font.DemiBold
                color: Theme.color("onSurface")
            }
            Item { Layout.fillWidth: true }
            HeaderIconButton {
                Layout.preferredWidth: 34; Layout.preferredHeight: 34
                iconName: "close"; iconSize: 19; iconColor: Theme.color("onSurfaceVariant")
                tooltip: qsTr("Close")
                onClicked: root.closeRequested()
            }
        }

        // Filter.
        Rectangle {
            Layout.fillWidth: true
            Layout.leftMargin: 16
            Layout.rightMargin: 16
            Layout.bottomMargin: 10
            Layout.preferredHeight: 38
            radius: 19
            color: Theme.color("surfaceVariant")
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 12
                spacing: 8
                MDIcon { name: "search"; size: 17; color: Theme.color("onSurfaceVariant") }
                TextInput {
                    id: filterField
                    Layout.fillWidth: true
                    font.family: Typography.family; font.pixelSize: 13
                    color: Theme.color("onSurface")
                    clip: true
                    selectByMouse: true
                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: filterField.text.length === 0
                        text: qsTr("Filter notifications")
                        font: filterField.font
                        color: Theme.color("onSurfaceVariant")
                    }
                }
            }
        }

        // Feed.
        ListView {
            id: feedView
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.leftMargin: 12
            Layout.rightMargin: 12
            clip: true
            model: feed
            spacing: 6
            reuseItems: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }

            delegate: Rectangle {
                required property string message
                required property string timeText
                required property string origin
                required property var diffLines

                width: feedView.width
                height: cardCol.implicitHeight + 24
                radius: 16
                color: cardMouse.containsMouse ? Theme.color("hover") : "transparent"

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 12
                    anchors.leftMargin: 14
                    spacing: 12

                    Rectangle {
                        Layout.preferredWidth: 36; Layout.preferredHeight: 36
                        Layout.alignment: Qt.AlignTop
                        radius: 12
                        readonly property color tint: origin === "undo" ? Theme.color("warning")
                            : origin === "restore" ? Theme.color("info") : Theme.color("success")
                        color: Qt.alpha(tint, Theme.isDark ? 0.18 : 0.12)
                        MDIcon {
                            anchors.centerIn: parent
                            name: origin === "undo" ? "undo" : (origin === "restore" ? "settings_backup_restore" : "bolt")
                            size: 19
                            color: parent.tint
                        }
                    }

                    ColumnLayout {
                        id: cardCol
                        Layout.fillWidth: true
                        spacing: 3
                        RowLayout {
                            Layout.fillWidth: true
                            Text {
                                Layout.fillWidth: true
                                text: message
                                elide: Text.ElideRight
                                font.family: Typography.family; font.pixelSize: 14; font.weight: Font.DemiBold
                                color: Theme.color("onSurface")
                            }
                            Text {
                                text: timeText
                                font.family: Typography.family; font.pixelSize: 11
                                color: Theme.color("onSurfaceVariant")
                            }
                        }
                        Text {
                            visible: diffLines.length > 0
                            Layout.fillWidth: true
                            text: diffLines.length > 0 ? diffLines[0].to : ""
                            elide: Text.ElideRight
                            font.family: Typography.family; font.pixelSize: 13
                            color: Theme.color("onSurfaceVariant")
                        }
                    }
                }

                MouseArea {
                    id: cardMouse
                    anchors.fill: parent
                    hoverEnabled: true
                }
            }

            // Empty state.
            Column {
                visible: feedView.count === 0
                anchors.centerIn: parent
                spacing: 8
                MDIcon {
                    anchors.horizontalCenter: parent.horizontalCenter
                    name: "notifications_off"; size: 40; color: Theme.color("outline")
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: qsTr("All caught up")
                    font.family: Typography.family; font.pixelSize: 13
                    color: Theme.color("onSurfaceVariant")
                }
            }
        }

        // Footer.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 46
            color: footerMouse.containsMouse ? Theme.color("hover") : "transparent"
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.color("outlineVariant") }
            Row {
                anchors.centerIn: parent
                spacing: 8
                MDIcon { anchors.verticalCenter: parent.verticalCenter; name: "history"; size: 17; color: Theme.color("primary") }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Every action is committed — open the action log")
                    font.family: Typography.family; font.pixelSize: 13; font.weight: Font.DemiBold
                    color: Theme.color("primary")
                }
            }
            MouseArea {
                id: footerMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.openHistoryRequested()
            }
        }
    }
}
