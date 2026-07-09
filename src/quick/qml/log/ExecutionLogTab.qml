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

    // ---- Layout ---------------------------------------------------------------

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabBar

            Layout.fillWidth: true
            Material.elevation: Spacing.elevationToolbar

            TabButton {
                id: generalTabButton

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
            implicitHeight: 1
            color: Theme.color("outlineVariant")
        }

        StackLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: tabBar.currentIndex

            GeneralLogView {
                model: filterProxy
                emptyText: qsTr("No log messages to display")
            }

            BlockedIPsView {
                model: peerModel
            }
        }
    }

    Component.onCompleted: Log.info("ui", "Execution Log tab created")
}
