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
    \qmltype GeneralLogView
    \brief A colored, auto-scrolling console list of execution-log lines.

    Renders one row per model entry: a muted monospace timestamp followed by the
    message, tinted per its \c logLevel via \c StateColors.forLog. Works for any
    log model that exposes the \c time / \c message / \c logLevel roles plus the
    \c reset() and \c rowText(row) invokables — i.e. both \c LogFilterProxy (the
    General log) and \c LogPeerModel (Blocked IPs), so \c BlockedIPsView reuses
    this type directly.

    New rows append at the bottom; the view stays pinned to the newest line
    unless the user scrolls up (auto-scroll pauses until they return to the
    bottom). Right-click (or the menu key) opens \c LogContextMenu with
    Copy / Clear.
*/
Item {
    id: root

    /*! The log model to display (a QAbstractItemModel; usually a proxy). */
    property var model: null

    /*! Message shown when the model is empty (already translated by the caller). */
    property string emptyText: qsTr("No log messages to display")

    /*! When true the view keeps itself pinned to the newest line. */
    property bool autoScroll: true

    // Copy the current row's "time - message" via the controller (QML has no
    // direct clipboard access).
    function copyCurrent() {
        if (!root.model || (listView.currentIndex < 0)) {
            Log.debug("ui", "Execution Log copy skipped: no current row")
            return
        }
        const text = root.model.rowText(listView.currentIndex)
        ExecutionLogController.copyToClipboard(text)
        Log.info("ui", "Copied Execution Log row " + listView.currentIndex + " to clipboard")
    }

    // Clear the underlying buffer (proxy forwards to its source model).
    function clearAll() {
        if (root.model && root.model.reset) {
            Log.info("ui", "Clearing Execution Log view (" + listView.count + " rows)")
            root.model.reset()
        }
    }

    ListView {
        id: listView

        anchors.fill: parent
        model: root.model
        clip: true
        focus: true
        boundsBehavior: Flickable.StopAtBounds
        reuseItems: true
        currentIndex: -1

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
        }

        // Keep pinned to the newest line while the user has not scrolled away.
        onCountChanged: {
            if (root.autoScroll)
                Qt.callLater(listView.positionViewAtEnd)
        }
        onMovementEnded: root.autoScroll = listView.atYEnd

        // Ctrl+C copies the current line, matching the desktop client.
        Keys.onPressed: (event) => {
            if ((event.key === Qt.Key_C) && (event.modifiers & Qt.ControlModifier)) {
                root.copyCurrent()
                event.accepted = true
            }
        }

        delegate: Rectangle {
            id: row

            required property int index
            required property string time
            required property string message
            required property string logLevel

            width: ListView.view.width
            implicitHeight: Math.max(line.implicitHeight + (Spacing.xs * 2), Spacing.rowHeight)

            color: ListView.isCurrentItem
                ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, Theme.selectedOpacity)
                : ((row.index % 2) === 1 ? Theme.alternateRow : "transparent")

            RowLayout {
                id: line

                anchors.fill: parent
                anchors.leftMargin: Spacing.md
                anchors.rightMargin: Spacing.md
                spacing: Spacing.sm

                Label {
                    text: row.time
                    font: Typography.mono
                    color: StateColors.logTimeStamp
                    Layout.alignment: Qt.AlignTop
                }

                Label {
                    text: "-"
                    font: Typography.mono
                    color: StateColors.muted
                    Layout.alignment: Qt.AlignTop
                }

                Label {
                    text: row.message
                    font: Typography.bodyMedium
                    color: StateColors.forLog(row.logLevel)
                    wrapMode: Text.Wrap
                    Layout.fillWidth: true
                }
            }

            MouseArea {
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton | Qt.RightButton
                onClicked: (mouse) => {
                    listView.currentIndex = row.index
                    listView.forceActiveFocus()
                    if (mouse.button === Qt.RightButton) {
                        Log.debug("ui", "Execution Log context menu requested for row " + row.index)
                        contextMenu.canCopy = true
                        contextMenu.popup()
                    }
                }
            }
        }
    }

    // Empty-state placeholder.
    Label {
        anchors.centerIn: parent
        width: parent.width - (Spacing.xl * 2)
        visible: listView.count === 0
        text: root.emptyText
        font: Typography.bodyMedium
        color: StateColors.muted
        horizontalAlignment: Text.AlignHCenter
        wrapMode: Text.WordWrap
    }

    LogContextMenu {
        id: contextMenu
        onCopyRequested: root.copyCurrent()
        onClearRequested: root.clearAll()
    }

    Component.onCompleted: Log.debug("ui", "GeneralLogView ready")
}
