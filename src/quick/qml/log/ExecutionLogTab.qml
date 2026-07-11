/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026  qBittorrent-Material contributors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import qBittorrent

/*!
    \qmltype ExecutionLogTab
    \brief The Material "Execution Log" screen.

    A two-tab surface (General log | Blocked IPs) over the shared \c Logger ring
    buffers. The models are owned here so their state (and scroll position)
    survives switching between the inner tabs. The General log is filtered by
    \c ExecutionLogController.messageTypes, which mirrors the main window's
    View -> Log message-type toggles.
*/
Item {
    id: root

    readonly property bool documentationCapture: {
        const argumentsList = Qt.application.arguments
        for (let index = 0; index < argumentsList.length; ++index) {
            if (String(argumentsList[index]).startsWith("--capture-ui="))
                return true
        }
        return false
    }

    // ---- Models (owned at the tab so they persist across inner tab switches) --

    LogMessageModel {
        id: messageModel
    }

    LogFilterProxy {
        id: filterProxy
        sourceModel: messageModel
        // Follow the View menu's message-type toggles live.
        messageTypes: ExecutionLogController.messageTypes
    }

    LogPeerModel {
        id: peerModel
    }

    // Documentation builds must never publish a user's network addresses,
    // paths, tracker names, or other runtime details. Keep the real logger
    // model untouched for normal sessions and render a small deterministic
    // fixture only while --capture-ui is active.
    ListModel {
        id: documentationLogModel

        ListElement {
            time: "09:41:02"
            message: "Application session initialized"
            logLevel: "Info"
        }
        ListElement {
            time: "09:41:03"
            message: "Listening on the configured network interface"
            logLevel: "Normal"
        }
        ListElement {
            time: "09:41:04"
            message: "Transfer queue is ready"
            logLevel: "Normal"
        }
        ListElement {
            time: "09:41:05"
            message: "RSS refresh completed"
            logLevel: "Info"
        }
    }

    // ---- Layout ---------------------------------------------------------------

    Rectangle {
        anchors.fill: parent
        color: Theme.color("background")
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Spacing.pagePadding
        spacing: Spacing.lg

        // ---- Page heading -------------------------------------------------
        ColumnLayout {
            Layout.fillWidth: true
            spacing: Spacing.xs

            Label {
                Layout.fillWidth: true
                text: qsTr("Execution log")
                font: Typography.pageTitle
                color: Theme.color("onBackground")
                elide: Text.ElideRight
            }

            Label {
                Layout.fillWidth: true
                text: qsTr("Inspect application, network, and session events from the current run.")
                font: Typography.metadata
                color: Theme.color("muted")
                wrapMode: Text.WordWrap
            }
        }

        // ---- Dominant log surface -----------------------------------------
        Rectangle {
            id: logPanel
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.color("surface")
            border.width: Spacing.outlineWidth
            border.color: Theme.color("outline")
            radius: Spacing.radiusPanel
            clip: true

            Behavior on color {
                ColorAnimation {
                    duration: Spacing.motionFast
                    easing.type: Easing.BezierSpline
                    easing.bezierCurve: Spacing.easeStandard
                }
            }

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: Spacing.outlineWidth
                spacing: 0

                TabBar {
                    id: tabBar

                    Layout.fillWidth: true
                    Layout.leftMargin: Spacing.lg
                    Layout.rightMargin: Spacing.lg
                    Layout.topMargin: Spacing.lg
                    Layout.bottomMargin: Spacing.md
                    Layout.preferredHeight: Spacing.controlHeight
                    Layout.minimumHeight: Spacing.controlHeight
                    Layout.maximumHeight: Spacing.controlHeight
                    spacing: Spacing.sm
                    Material.elevation: 0
                    background: Rectangle {
                        color: "transparent"
                    }

                    TabButton {
                        id: generalTabButton
                        implicitHeight: Spacing.controlHeight

                        background: Rectangle {
                            radius: Spacing.radiusControl
                            color: generalTabButton.checked
                                ? Theme.color("primaryContainer")
                                : (generalTabButton.hovered
                                    ? Qt.alpha(Theme.color("onSurface"), Theme.hoverOpacity)
                                    : "transparent")

                            Behavior on color {
                                ColorAnimation {
                                    duration: Spacing.motionFast
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: Spacing.easeStandard
                                }
                            }
                        }

                        contentItem: RowLayout {
                            spacing: Spacing.sm

                            Item { Layout.fillWidth: true }

                            MDIcon {
                                icon: Icons.article
                                size: Spacing.iconSizeSmall
                                color: generalTabButton.checked ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Label {
                                text: qsTr("General log")
                                font: Typography.titleSmall
                                color: generalTabButton.checked ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }

                    TabButton {
                        id: blockedTabButton
                        implicitHeight: Spacing.controlHeight

                        background: Rectangle {
                            radius: Spacing.radiusControl
                            color: blockedTabButton.checked
                                ? Theme.color("primaryContainer")
                                : (blockedTabButton.hovered
                                    ? Qt.alpha(Theme.color("onSurface"), Theme.hoverOpacity)
                                    : "transparent")

                            Behavior on color {
                                ColorAnimation {
                                    duration: Spacing.motionFast
                                    easing.type: Easing.BezierSpline
                                    easing.bezierCurve: Spacing.easeStandard
                                }
                            }
                        }

                        contentItem: RowLayout {
                            spacing: Spacing.sm

                            Item { Layout.fillWidth: true }

                            MDIcon {
                                icon: Icons.block
                                size: Spacing.iconSizeSmall
                                color: blockedTabButton.checked ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Label {
                                text: qsTr("Blocked IPs")
                                font: Typography.titleSmall
                                color: blockedTabButton.checked ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                                Layout.alignment: Qt.AlignVCenter
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }

                    onCurrentIndexChanged: Log.debug("ui", "Execution Log inner tab -> " + currentIndex)
                }

                // Divider under the tab bar (toolbars carry a bottom divider).
                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: Spacing.outlineWidth
                    color: Theme.color("outlineVariant")
                }

                StackLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    currentIndex: tabBar.currentIndex

                    GeneralLogView {
                        model: root.documentationCapture ? documentationLogModel : filterProxy
                        emptyText: qsTr("No log messages to display")
                    }

                    BlockedIPsView {
                        model: peerModel
                    }
                }
            }
        }
    }

    Component.onCompleted: Log.info("ui", "Execution Log tab created")
}
