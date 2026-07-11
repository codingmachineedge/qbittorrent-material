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
    \qmltype SettingsSheet
    \brief The redesigned quick-settings sheet: theme segmented control, the
           three UI-style cards (each change committed to the settings
           journal), History retention chips + open-history shortcut, and a
           link to the full Options dialog.
*/
Sheet {
    id: root
    sheetWidth: 430

    signal closeRequested()
    signal openHistoryRequested()
    signal openFullOptionsRequested()

    readonly property var styleCards: [
        { style: 0, name: qsTr("Tonal Rail"), desc: qsTr("Nav rail + chips, comfortable rows") },
        { style: 1, name: qsTr("Split Dock"), desc: qsTr("Classic sidebar, dense table, dock") },
        { style: 2, name: qsTr("Card Flow"), desc: qsTr("Cards + persistent detail panel") }
    ]
    readonly property var retentionOptions: ["30 days", "1 year", "Forever"]

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header.
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 20
            Layout.rightMargin: 16
            Layout.bottomMargin: 6
            spacing: 8
            Text {
                text: qsTr("Settings")
                font.family: Typography.family
                font.pixelSize: 16
                font.weight: Font.DemiBold
                color: Theme.color("onSurface")
            }
            Text {
                text: qsTr("every change becomes a commit")
                font.family: Typography.family
                font.pixelSize: 12
                color: Theme.color("onSurfaceVariant")
            }
            Item { Layout.fillWidth: true }
            HeaderIconButton {
                Layout.preferredWidth: 34; Layout.preferredHeight: 34
                iconName: "close"; iconSize: 19; iconColor: Theme.color("onSurfaceVariant")
                tooltip: qsTr("Close")
                onClicked: root.closeRequested()
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: settingsColumn.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }

            ColumnLayout {
                id: settingsColumn
                width: parent.width
                spacing: 18

                // --- Appearance ---
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.topMargin: 8
                    spacing: 10

                    Text {
                        text: qsTr("APPEARANCE")
                        font.family: Typography.family
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        font.letterSpacing: 1
                        color: Theme.color("primary")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: qsTr("Theme")
                            font.family: Typography.family
                            font.pixelSize: 14
                            color: Theme.color("onSurface")
                        }
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            Layout.preferredHeight: 36
                            width: themeRow.implicitWidth + 6
                            radius: 18
                            color: Theme.color("surfaceVariant")
                            Row {
                                id: themeRow
                                anchors.centerIn: parent
                                spacing: 0
                                Repeater {
                                    model: [
                                        { dark: false, icon: "light_mode", label: qsTr("Light") },
                                        { dark: true, icon: "dark_mode", label: qsTr("Dark") }
                                    ]
                                    delegate: Rectangle {
                                        required property var modelData
                                        readonly property bool active: Theme.isDark === modelData.dark
                                        height: 30
                                        width: segItemRow.implicitWidth + 24
                                        radius: 15
                                        color: active ? Theme.color("primary") : "transparent"
                                        Row {
                                            id: segItemRow
                                            anchors.centerIn: parent
                                            spacing: 5
                                            MDIcon {
                                                anchors.verticalCenter: parent.verticalCenter
                                                name: modelData.icon; size: 15
                                                color: parent.parent.active ? Theme.color("onPrimary") : Theme.color("onSurfaceVariant")
                                            }
                                            Text {
                                                anchors.verticalCenter: parent.verticalCenter
                                                text: modelData.label
                                                font.family: Typography.family; font.pixelSize: 13; font.weight: Font.DemiBold
                                                color: parent.parent.active ? Theme.color("onPrimary") : Theme.color("onSurfaceVariant")
                                            }
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: ThemeManager.colorScheme = modelData.dark ? ThemeManager.Dark : ThemeManager.Light
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Text {
                        text: qsTr("UI style")
                        font.family: Typography.family
                        font.pixelSize: 14
                        color: Theme.color("onSurface")
                    }

                    Repeater {
                        model: root.styleCards
                        delegate: Rectangle {
                            required property var modelData
                            readonly property bool active: Theme.uiStyle === modelData.style
                            Layout.fillWidth: true
                            Layout.preferredHeight: 68
                            radius: 16
                            color: active ? Qt.alpha(Theme.color("primary"), Theme.isDark ? 0.10 : 0.06)
                                          : (cardMouse.containsMouse ? Theme.color("hover") : "transparent")
                            border.width: active ? 2 : 1
                            border.color: active ? Theme.color("primary") : Theme.color("outlineVariant")

                            RowLayout {
                                anchors.fill: parent
                                anchors.leftMargin: 14
                                anchors.rightMargin: 14
                                spacing: 14

                                // Mini thumbnail.
                                Rectangle {
                                    Layout.preferredWidth: 64
                                    Layout.preferredHeight: 44
                                    radius: 10
                                    color: Theme.color("background")
                                    border.width: 1
                                    border.color: Theme.color("outlineVariant")
                                    Row {
                                        anchors.fill: parent
                                        anchors.margins: 5
                                        spacing: 3
                                        Rectangle {
                                            width: modelData.style === 1 ? 14 : (modelData.style === 2 ? 6 : 8)
                                            height: parent.height
                                            radius: 3
                                            color: Theme.color("primary")
                                        }
                                        Column {
                                            width: parent.width - (modelData.style === 1 ? 17 : (modelData.style === 2 ? 9 : 11))
                                            height: parent.height
                                            spacing: 3
                                            Rectangle { width: parent.width * 0.7; height: 5; radius: 2; color: Theme.color("primary"); opacity: 0.5 }
                                            Rectangle { width: parent.width; height: modelData.style === 0 ? parent.height - 8 : 8; radius: 3; color: Theme.color("primary"); opacity: 0.22 }
                                            Rectangle { visible: modelData.style !== 0; width: parent.width; height: 8; radius: 3; color: Theme.color("primary"); opacity: 0.22 }
                                        }
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text {
                                        text: modelData.name
                                        font.family: Typography.family
                                        font.pixelSize: 14
                                        font.weight: Font.DemiBold
                                        color: Theme.color("onSurface")
                                    }
                                    Text {
                                        Layout.fillWidth: true
                                        text: modelData.desc
                                        elide: Text.ElideRight
                                        font.family: Typography.family
                                        font.pixelSize: 12
                                        color: Theme.color("onSurfaceVariant")
                                    }
                                }

                                MDIcon {
                                    name: parent.active ? "radio_button_checked" : "radio_button_unchecked"
                                    size: 20
                                    color: parent.active ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                                }
                            }

                            MouseArea {
                                id: cardMouse
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: ThemeManager.uiStyle = modelData.style
                            }
                        }
                    }
                }

                // --- History ---
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    spacing: 8

                    Text {
                        text: qsTr("HISTORY")
                        font.family: Typography.family
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        font.letterSpacing: 1
                        color: Theme.color("primary")
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Text {
                            text: qsTr("Retention")
                            font.family: Typography.family
                            font.pixelSize: 14
                            color: Theme.color("onSurface")
                        }
                        Item { Layout.fillWidth: true }
                        Row {
                            spacing: 6
                            Repeater {
                                model: root.retentionOptions
                                delegate: Rectangle {
                                    required property string modelData
                                    readonly property bool active: JournalController.retention === modelData
                                    height: 30
                                    width: retLabel.implicitWidth + 26
                                    radius: 15
                                    color: active ? Theme.color("primaryContainer") : "transparent"
                                    border.width: 1
                                    border.color: active ? Theme.color("primaryContainer") : Theme.color("outline")
                                    Text {
                                        id: retLabel
                                        anchors.centerIn: parent
                                        text: modelData
                                        font.family: Typography.family; font.pixelSize: 13; font.weight: Font.DemiBold
                                        color: parent.active ? Theme.color("onPrimaryContainer") : Theme.color("onSurfaceVariant")
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: JournalController.retention = modelData
                                    }
                                }
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        radius: 12
                        color: openHistMouse.containsMouse ? Theme.color("surfaceContainerHigh") : Theme.color("surfaceVariant")
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 8
                            MDIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                name: "history"; size: 17; color: Theme.color("primary")
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("Open history manager")
                                font.family: Typography.family; font.pixelSize: 13; font.weight: Font.DemiBold
                                color: Theme.color("primary")
                            }
                        }
                        Text {
                            anchors.right: parent.right
                            anchors.rightMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("%1 commits").arg(JournalController.settingsCount)
                            font.family: Typography.monoFamily; font.pixelSize: 11
                            color: Theme.color("onSurfaceVariant")
                        }
                        MouseArea {
                            id: openHistMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.openHistoryRequested()
                        }
                    }
                }

                // --- Full options link ---
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.bottomMargin: 20
                    spacing: 8

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        radius: 12
                        color: fullOptMouse.containsMouse ? Theme.color("surfaceContainerHigh") : Theme.color("surfaceVariant")
                        Row {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            spacing: 8
                            MDIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                name: "tune"; size: 17; color: Theme.color("onSurfaceVariant")
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: qsTr("All qBittorrent options…")
                                font.family: Typography.family; font.pixelSize: 13; font.weight: Font.DemiBold
                                color: Theme.color("onSurface")
                            }
                        }
                        MouseArea {
                            id: fullOptMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.openFullOptionsRequested()
                        }
                    }
                }
            }
        }
    }
}
