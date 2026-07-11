/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026  qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import qBittorrent

/*!
    The persistent 64 px application bar from the qBittorrent Material design
    system. Product identity, the complete application menu, high-frequency
    transfer actions, live throughput, settings, and lock state stay in stable
    positions while workspaces change below it.
*/
ToolBar {
    id: toolBar

    required property var shell

    implicitHeight: Spacing.topBarHeight
    Material.elevation: 0

    background: Rectangle {
        color: Theme.color("surface")
        Rectangle {
            anchors.bottom: parent.bottom
            width: parent.width
            height: 1
            color: Theme.color("outline")
        }
    }

    component ToolbarButton: ToolButton {
        id: control
        property string glyph: ""
        property var boundAction: null
        action: boundAction
        display: AbstractButton.IconOnly
        flat: true
        implicitWidth: Spacing.controlHeight
        implicitHeight: Spacing.controlHeight
        Accessible.name: boundAction
            ? String(boundAction.text).replace(/&/g, "").replace(/\.\.\.$/, "")
            : ""

        contentItem: MDIcon {
            icon: control.glyph
            size: 20
            fill: control.checked
            color: control.enabled
                ? (control.checked ? Theme.color("primary") : Theme.color("onSurfaceVariant"))
                : Theme.color("outline")
            anchors.centerIn: parent
        }

        background: Rectangle {
            radius: height / 2
            color: control.down || control.hovered
                ? Theme.color("primaryContainer") : "transparent"
            border.width: control.activeFocus ? 2 : 0
            border.color: Theme.color("primary")
            Behavior on color {
                ColorAnimation { duration: Spacing.motionFast }
            }
        }

        ToolTip.visible: hovered
        ToolTip.text: Accessible.name
        ToolTip.delay: 500
    }

    component BarDivider: Rectangle {
        Layout.preferredWidth: 1
        Layout.preferredHeight: 28
        color: Theme.color("outlineVariant")
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Spacing.xl
        anchors.rightMargin: Spacing.xl
        spacing: Spacing.sm

        RowLayout {
            spacing: 4
            Layout.rightMargin: Spacing.sm

            Label {
                readonly property bool canonicalName:
                    WorkspaceManager.appDisplayName === "qBittorrent Material"
                text: canonicalName ? "qBittorrent" : WorkspaceManager.appDisplayName
                font: Typography.brand
                color: Theme.color("onSurface")
                elide: Text.ElideRight
                Layout.maximumWidth: 260
            }
            Label {
                visible: WorkspaceManager.appDisplayName === "qBittorrent Material"
                text: "Material"
                font: Typography.brand
                color: Theme.color("primary")
            }
        }

        AppMenuBar {
            shell: toolBar.shell
        }

        BarDivider {}

        ToolbarButton { boundAction: toolBar.shell.actionOpen; glyph: Icons.note_add }
        ToolbarButton { boundAction: toolBar.shell.actionDownloadFromURL; glyph: Icons.add_link }
        ToolbarButton { boundAction: toolBar.shell.actionStart; glyph: Icons.play_arrow }
        ToolbarButton { boundAction: toolBar.shell.actionStop; glyph: Icons.pause }
        ToolbarButton { boundAction: toolBar.shell.actionDelete; glyph: Icons.deleteIcon }
        ToolbarButton { boundAction: toolBar.shell.actionOpenDestinationFolder; glyph: Icons.folder_open }

        Item { Layout.fillWidth: true }

        RowLayout {
            spacing: Spacing.lg
            Layout.rightMargin: Spacing.sm

            RowLayout {
                spacing: 5
                MDIcon {
                    icon: Icons.download
                    size: 17
                    color: Theme.color("onSurfaceVariant")
                }
                Label {
                    text: toolBar.formatSpeed(Session.downloadRate || 0)
                    font: Typography.mono
                    color: Theme.color("muted")
                }
            }

            RowLayout {
                spacing: 5
                MDIcon {
                    icon: Icons.upload
                    size: 17
                    color: Theme.color("onSurfaceVariant")
                }
                Label {
                    text: toolBar.formatSpeed(Session.uploadRate || 0)
                    font: Typography.mono
                    color: Theme.color("muted")
                }
            }
        }

        BarDivider {}
        ToolbarButton { boundAction: toolBar.shell.actionOptions; glyph: Icons.settings }
        ToolbarButton { boundAction: toolBar.shell.actionLock; glyph: Icons.lock }
    }

    function formatSpeed(bytesPerSecond) {
        var units = ["B/s", "KiB/s", "MiB/s", "GiB/s", "TiB/s"]
        var value = Math.max(0, bytesPerSecond)
        var i = 0
        while (value >= 1024 && i < units.length - 1) {
            value /= 1024
            ++i
        }
        var digits = i === 0 ? 0 : (value < 100 ? 1 : 0)
        return qsTr("%1 %2").arg(value.toFixed(digits)).arg(units[i])
    }

    Component.onCompleted: Log.debug("ui", "Design-system application bar ready")
}
