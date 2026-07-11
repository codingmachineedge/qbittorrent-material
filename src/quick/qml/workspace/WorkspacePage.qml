/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import qBittorrent

Item {
    id: root

    required property string tabId
    required property string tabName
    required property string tabContent
    required property string fontFamily
    required property string fontStyle
    required property real fontPointSize
    required property bool bold
    required property bool italic
    required property string fontColor
    required property string updatedAt

    property bool initialized: false
    objectName: "workspacePage_" + tabId

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Spacing.lg
        spacing: Spacing.md

        RowLayout {
            Layout.fillWidth: true
            spacing: Spacing.md

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2
                Label {
                    text: root.tabName
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                    font: Typography.headlineSmall
                    color: Theme.color("onSurface")
                    Layout.fillWidth: true
                }
                Label {
                    text: qsTr("%1 · %2 pt · %3%4")
                        .arg(root.fontFamily)
                        .arg(root.fontPointSize)
                        .arg(root.fontStyle)
                        .arg(root.bold ? qsTr(" · Bold") : "")
                    textFormat: Text.PlainText
                    elide: Text.ElideRight
                    font: Typography.bodySmall
                    color: Theme.color("onSurfaceVariant")
                    Layout.fillWidth: true
                }
            }

            Rectangle {
                Layout.preferredWidth: 14
                Layout.preferredHeight: 14
                radius: 7
                color: root.fontColor
                border.width: 1
                border.color: Theme.color("outline")
                Accessible.name: qsTr("Page font color")
            }

            Label {
                text: !WorkspaceManager.writable
                    ? qsTr("Read only")
                    : (WorkspaceManager.dirty ? qsTr("Saving…") : qsTr("Saved"))
                font: Typography.labelSmall
                color: !WorkspaceManager.writable
                    ? Theme.color("error")
                    : (WorkspaceManager.dirty
                        ? Theme.color("primary") : Theme.color("onSurfaceVariant"))
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Spacing.radiusDialog
            color: Theme.color("surface")
            border.width: 1
            border.color: editor.activeFocus ? Theme.color("primary") : Theme.color("outlineVariant")

            ScrollView {
                anchors.fill: parent
                anchors.margins: 1
                clip: true

                TextArea {
                    id: editor
                    objectName: "workspaceEditor_" + root.tabId
                    Accessible.name: qsTr("Page editor for %1").arg(root.tabName)
                    text: root.tabContent
                    textFormat: TextEdit.PlainText
                    readOnly: !WorkspaceManager.writable
                    wrapMode: TextEdit.Wrap
                    selectByMouse: true
                    persistentSelection: true
                    leftPadding: Spacing.xl
                    rightPadding: Spacing.xl
                    topPadding: Spacing.lg
                    bottomPadding: Spacing.lg
                    placeholderText: WorkspaceManager.writable
                        ? qsTr("Write anything on this page. Changes save automatically to local Git.")
                        : qsTr("Workspace recovery is required before this page can be edited.")
                    color: root.fontColor
                    selectionColor: Theme.color("primaryContainer")
                    selectedTextColor: Theme.color("onPrimaryContainer")
                    font: WorkspaceManager.resolvedFont(root.fontFamily, root.fontStyle,
                                                        root.fontPointSize, root.bold, root.italic)
                    background: null

                    onTextChanged: {
                        if (!root.initialized)
                            return
                        if (text.length > 4 * 1024 * 1024) {
                            var preservedCursor = Math.min(cursorPosition, 4 * 1024 * 1024)
                            WorkspaceManager.setTabContent(root.tabId, text)
                            text = text.substring(0, 4 * 1024 * 1024)
                            cursorPosition = preservedCursor
                        }
                        if (text !== root.tabContent)
                            WorkspaceManager.setTabContent(root.tabId, text)
                    }
                    Component.onCompleted: root.initialized = true
                }
            }
        }
    }
}
