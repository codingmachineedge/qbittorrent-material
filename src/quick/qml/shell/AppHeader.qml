/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Layouts
import qBittorrent

/*!
    \qmltype AppHeader
    \brief The redesigned 64px header: transfers search pill (with regex
           toggle + Regex Builder hook), Split Dock nav segments, session
           speed pill, Alt-speed toggle, notification bell, History (git),
           theme toggle, Settings — plus a trailing overflow menu that keeps
           every legacy menu verb reachable.
*/
Rectangle {
    id: root

    required property var shell
    property var filterProxy: null
    property int currentTab: 0
    property int unreadNotifications: 0
    property string activePanel: ""

    signal navRequested(int index)
    signal panelRequested(string panel)
    signal regexBuilderRequested()

    height: 64
    color: "transparent"

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 20
        anchors.rightMargin: 20
        spacing: 12

        // --- Split Dock (B): nav segments live in the header -----------------
        Rectangle {
            visible: Theme.isSplitDock
            Layout.preferredHeight: 40
            Layout.preferredWidth: segmentsRow.implicitWidth + 8
            radius: 20
            color: Theme.color("surfaceVariant")

            Row {
                id: segmentsRow
                anchors.centerIn: parent
                spacing: 4

                Repeater {
                    model: root.navModel
                    delegate: Rectangle {
                        required property var modelData
                        required property int index
                        readonly property bool active: root.currentTab === modelData.page

                        height: 32
                        width: segRow.implicitWidth + 28
                        radius: 16
                        color: active ? Theme.color("primaryContainer")
                                      : (segMouse.containsMouse ? Theme.color("hoverStrong") : "transparent")

                        Row {
                            id: segRow
                            anchors.centerIn: parent
                            spacing: 6

                            MDIcon {
                                anchors.verticalCenter: parent.verticalCenter
                                name: modelData.icon
                                size: 17
                                fill: parent.parent.active
                                color: parent.parent.active ? Theme.color("onPrimaryContainer")
                                                            : Theme.color("onSurfaceVariant")
                            }
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: modelData.label
                                font.family: Typography.family
                                font.pixelSize: 13
                                font.weight: Font.DemiBold
                                color: parent.parent.active ? Theme.color("onSurface")
                                                            : Theme.color("onSurfaceVariant")
                            }
                            Rectangle {
                                visible: (modelData.badge || 0) > 0 && !parent.parent.active
                                anchors.verticalCenter: parent.verticalCenter
                                width: Math.max(16, segBadge.implicitWidth + 8)
                                height: 16
                                radius: 8
                                color: Theme.color("primary")
                                Text {
                                    id: segBadge
                                    anchors.centerIn: parent
                                    text: String(modelData.badge || 0)
                                    color: Theme.color("onPrimary")
                                    font.pixelSize: 10
                                    font.weight: Font.Bold
                                }
                            }
                        }
                        MouseArea {
                            id: segMouse
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.navRequested(modelData.page)
                        }
                    }
                }
            }
        }

        // --- Transfers filter pill (Transfers tab only) -----------------------
        Rectangle {
            id: searchPill
            visible: (root.currentTab === 0) && (root.filterProxy !== null)
            Layout.fillWidth: true
            Layout.maximumWidth: 560
            Layout.preferredHeight: 44
            radius: 22
            color: Theme.color("surfaceVariant")
            border.width: 1
            border.color: rxInvalid ? Theme.color("error") : "transparent"

            readonly property bool useRegex: root.filterProxy && root.filterProxy.useRegex
            readonly property string textFilter: root.filterProxy ? root.filterProxy.textFilter : ""
            readonly property bool rxInvalid: useRegex
                && (textFilter.length > 0) && !root.regexValid(textFilter)

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 16
                anchors.rightMargin: 6
                spacing: 8

                MDIcon { name: "search"; size: 20; color: Theme.color("onSurfaceVariant") }

                TextInput {
                    id: filterInput
                    objectName: "headerFilterInput"
                    Layout.fillWidth: true
                    text: searchPill.textFilter
                    onTextEdited: { if (root.filterProxy) root.filterProxy.textFilter = text }
                    font.family: Typography.family
                    font.pixelSize: 14
                    color: Theme.color("onSurface")
                    clip: true
                    selectByMouse: true

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        visible: filterInput.text.length === 0
                        text: searchPill.useRegex ? qsTr("Search with /regex/") : qsTr("Search torrents")
                        font: filterInput.font
                        color: Theme.color("onSurfaceVariant")
                    }
                }

                Rectangle {
                    Layout.preferredHeight: 28
                    Layout.preferredWidth: rxLabel.implicitWidth + 20
                    radius: 14
                    color: searchPill.useRegex
                        ? (searchPill.rxInvalid ? Theme.color("errorContainer") : Theme.color("primaryContainer"))
                        : "transparent"
                    Text {
                        id: rxLabel
                        anchors.centerIn: parent
                        text: ".*"
                        font.family: Typography.monoFamily
                        font.pixelSize: 12
                        font.weight: Font.Bold
                        color: searchPill.useRegex
                            ? (searchPill.rxInvalid ? Theme.color("error") : Theme.color("onPrimaryContainer"))
                            : Theme.color("onSurfaceVariant")
                    }
                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: { if (root.filterProxy) root.filterProxy.useRegex = !root.filterProxy.useRegex }
                    }
                }

                HeaderIconButton {
                    Layout.preferredWidth: 32
                    Layout.preferredHeight: 32
                    iconName: "data_object"
                    iconSize: 18
                    iconColor: Theme.color("onSurfaceVariant")
                    tooltip: qsTr("Open Regex Builder")
                    onClicked: root.regexBuilderRequested()
                }
            }
        }

        Item { Layout.fillWidth: true }

        // --- Session speeds pill ---------------------------------------------
        Rectangle {
            Layout.preferredHeight: 34
            Layout.preferredWidth: speedsRow.implicitWidth + 28
            radius: 17
            color: Theme.color("surfaceVariant")

            Row {
                id: speedsRow
                anchors.centerIn: parent
                spacing: 8

                MDIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: "arrow_downward"; size: 18; color: Theme.color("success")
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.formatSpeed(Session.downloadRate || 0)
                    font.family: Typography.monoFamily
                    font.pixelSize: 13
                    color: Theme.color("onSurface")
                }
                Rectangle {
                    anchors.verticalCenter: parent.verticalCenter
                    width: 1; height: 14; color: Theme.color("outline")
                }
                MDIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: "arrow_upward"; size: 18; color: Theme.color("primary")
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: root.formatSpeed(Session.uploadRate || 0)
                    font.family: Typography.monoFamily
                    font.pixelSize: 13
                    color: Theme.color("onSurface")
                }
            }
        }

        // --- Alternative speed limits toggle ----------------------------------
        Rectangle {
            readonly property bool altOn: SpeedLimitController.alternativeLimitsEnabled || false
            Layout.preferredHeight: 34
            Layout.preferredWidth: altRow.implicitWidth + 28
            radius: 17
            color: altOn ? Theme.color("primaryContainer") : "transparent"
            border.width: 1
            border.color: altOn ? Theme.color("primaryContainer") : Theme.color("outline")

            Behavior on color { ColorAnimation { duration: 250 } }

            Row {
                id: altRow
                anchors.centerIn: parent
                spacing: 6
                MDIcon {
                    anchors.verticalCenter: parent.verticalCenter
                    name: "speed"; size: 18
                    fill: parent.parent.altOn
                    color: parent.parent.altOn ? Theme.color("onPrimaryContainer") : Theme.color("onSurfaceVariant")
                }
                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Alt")
                    font.family: Typography.family
                    font.pixelSize: 13
                    font.weight: Font.Medium
                    color: parent.parent.altOn ? Theme.color("onPrimaryContainer") : Theme.color("onSurfaceVariant")
                }
            }
            MouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: SpeedLimitController.toggleAlternativeLimits()
            }
        }

        // --- Panel buttons -----------------------------------------------------
        HeaderIconButton {
            iconName: "notifications"
            tooltip: qsTr("Notifications")
            active: root.activePanel === "notifications"
            badgeCount: root.unreadNotifications
            onClicked: root.panelRequested("notifications")
        }
        HeaderIconButton {
            iconName: "history"
            tooltip: qsTr("History (git)")
            active: root.activePanel === "history"
            onClicked: root.panelRequested("history")
        }
        HeaderIconButton {
            iconName: Theme.isDark ? "light_mode" : "dark_mode"
            tooltip: qsTr("Toggle light / dark")
            onClicked: ThemeManager.colorScheme = Theme.isDark ? ThemeManager.Light : ThemeManager.Dark
        }
        HeaderIconButton {
            iconName: "settings"
            tooltip: qsTr("Settings")
            active: root.activePanel === "settings"
            onClicked: root.panelRequested("settings")
        }

        // --- Overflow: the full legacy menu tree stays reachable --------------
        AppMenuBar {
            shell: root.shell
        }
    }

    // Navigation model shared by the header segments (B) and the nav rails (A/C).
    property int rssUnread: 0
    readonly property var navModel: [
        { label: qsTr("Transfers"), icon: "download", page: 0, badge: 0 },
        { label: qsTr("Search"), icon: "travel_explore", page: 1, badge: 0 },
        { label: qsTr("RSS"), icon: "rss_feed", page: 2, badge: root.rssUnread },
        { label: qsTr("Log"), icon: "article", page: 3, badge: 0 },
        { label: qsTr("Notes"), icon: "edit_note", page: 4, badge: 0 }
    ]

    function formatSpeed(bytesPerSec) {
        if (bytesPerSec >= 1024 * 1024)
            return (bytesPerSec / (1024 * 1024)).toFixed(1) + " MiB/s"
        if (bytesPerSec >= 1024)
            return Math.round(bytesPerSec / 1024) + " KiB/s"
        return bytesPerSec + " B/s"
    }

    function regexValid(pattern) {
        try { new RegExp(pattern); return true } catch (e) { return false }
    }

    function focusFilter() {
        filterInput.forceActiveFocus()
    }
}
