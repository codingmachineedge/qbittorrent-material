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
    Persistent 32 px operational footer. Values use compact monospace text and
    every colored state is paired with a literal label for accessible status.
*/
ToolBar {
    id: statusBar

    required property var shell
    property int tick: 0

    implicitHeight: Spacing.statusBarHeight
    Material.elevation: 0

    background: Rectangle {
        color: Theme.color("surface")
        Rectangle {
            anchors.top: parent.top
            width: parent.width
            height: 1
            color: Theme.color("outline")
        }
    }

    Connections {
        target: Session
        function onStatsUpdated() { statusBar.tick = statusBar.tick + 1 }
        function onFreeDiskSpaceChecked(result) { statusBar.tick = statusBar.tick + 1 }
    }

    Connections {
        target: Preferences
        function onChanged() { statusBar.tick = statusBar.tick + 1 }
    }

    component FooterDivider: Rectangle {
        Layout.preferredWidth: 1
        Layout.preferredHeight: 14
        color: Theme.color("outlineVariant")
    }

    component FooterLabel: Label {
        font: Typography.metadataMono
        color: Theme.color("muted")
        verticalAlignment: Text.AlignVCenter
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Spacing.xl
        anchors.rightMargin: Spacing.xl
        spacing: Spacing.md

        FooterLabel {
            text: (Session.dhtEnabled || false)
                ? qsTr("DHT: %1 nodes").arg((statusBar.tick, Session.dhtNodes || 0))
                : qsTr("DHT: Off")
        }
        FooterDivider {}

        ToolButton {
            id: connectionButton
            flat: true
            implicitHeight: 28
            padding: 0
            ToolTip.visible: hovered
            ToolTip.text: statusBar.connectionTooltip
            ToolTip.delay: 400
            onClicked: statusBar.shell.showOptionsConnectionTab()

            contentItem: RowLayout {
                spacing: 5
                MDIcon {
                    icon: statusBar.connectionIcon
                    size: 15
                    color: statusBar.connectionColor
                }
                FooterLabel {
                    text: qsTr("Connection: %1").arg(statusBar.connectionLabel)
                    color: Theme.color("onSurface")
                }
            }
        }
        FooterDivider {}

        FooterLabel {
            visible: (statusBar.tick, Preferences.isStatusbarFreeDiskSpaceDisplayed())
            text: qsTr("Disk: %1 free").arg(statusBar.freeSpaceText)
        }

        FooterLabel {
            visible: !statusBar.shell.captureMode
                && statusBar.width >= 1280
                && (statusBar.tick, Preferences.isStatusbarExternalIPDisplayed())
            text: statusBar.externalIPText
            elide: Text.ElideRight
            Layout.maximumWidth: 260
        }

        Item { Layout.fillWidth: true }

        ToolButton {
            id: alternativeSpeedButton
            flat: true
            checkable: true
            checked: statusBar.shell.altSpeedEnabled
            implicitWidth: 28
            implicitHeight: 28
            display: AbstractButton.IconOnly
            ToolTip.visible: hovered
            ToolTip.text: checked
                ? qsTr("Alternative speed limits enabled")
                : qsTr("Regular speed limits enabled")
            ToolTip.delay: 400
            contentItem: MDIcon {
                icon: Icons.speed
                size: 16
                fill: alternativeSpeedButton.checked
                color: alternativeSpeedButton.checked
                    ? Theme.color("primary") : Theme.color("muted")
                anchors.centerIn: parent
            }
            onClicked: statusBar.shell.toggleAltSpeed()
        }

        FooterDivider {}

        ToolButton {
            flat: true
            implicitHeight: 28
            padding: 0
            onClicked: statusBar.shell.showGlobalSpeedLimits()
            contentItem: RowLayout {
                spacing: 5
                MDIcon { icon: Icons.download; size: 15; color: Theme.color("muted") }
                FooterLabel { text: statusBar.downloadText; color: Theme.color("onSurface") }
            }
        }

        FooterDivider {}

        ToolButton {
            flat: true
            implicitHeight: 28
            padding: 0
            onClicked: statusBar.shell.showGlobalSpeedLimits()
            contentItem: RowLayout {
                spacing: 5
                MDIcon { icon: Icons.upload; size: 15; color: Theme.color("muted") }
                FooterLabel { text: statusBar.uploadText; color: Theme.color("onSurface") }
            }
        }
    }

    readonly property string freeSpaceText: (tick, formatSize(Session.freeDiskSpace || 0))

    readonly property string externalIPText: {
        void tick
        var v4 = Session.externalIPv4 || ""
        var v6 = Session.externalIPv6 || ""
        if (v4.length > 0 && v6.length > 0)
            return qsTr("IPs: %1, %2").arg(v4).arg(v6)
        if (v4.length > 0)
            return qsTr("IP: %1").arg(v4)
        if (v6.length > 0)
            return qsTr("IP: %1").arg(v6)
        return qsTr("IP: N/A")
    }

    readonly property string connectionIcon: {
        void tick
        if (!(Session.listening || false))
            return Icons.cloud_off
        if (Session.hasIncomingConnections || false)
            return Icons.cloud_done
        return Icons.shield
    }

    readonly property color connectionColor: {
        void tick
        if (!(Session.listening || false))
            return StateColors.error
        if (Session.hasIncomingConnections || false)
            return StateColors.success
        return StateColors.warning
    }

    readonly property string connectionLabel: {
        void tick
        if (!(Session.listening || false))
            return qsTr("Offline")
        if (Session.hasIncomingConnections || false)
            return qsTr("Online")
        return qsTr("No direct peers")
    }

    readonly property string connectionTooltip: {
        void tick
        if (!(Session.listening || false))
            return qsTr("Offline. qBittorrent could not listen on the configured incoming port.")
        if (Session.hasIncomingConnections || false)
            return qsTr("Online with incoming peer connections")
        return qsTr("Online, but no direct incoming peer connection has been observed")
    }

    readonly property string downloadText: {
        void tick
        var s = formatSpeed(Session.downloadRate || 0)
        var limit = SpeedLimitController.downloadLimit || 0
        if (limit > 0)
            s += " [" + formatSpeed(limit * 1024) + "]"
        return s
    }

    readonly property string uploadText: {
        void tick
        var s = formatSpeed(Session.uploadRate || 0)
        var limit = SpeedLimitController.uploadLimit || 0
        if (limit > 0)
            s += " [" + formatSpeed(limit * 1024) + "]"
        return s
    }

    function formatSize(bytes) {
        var units = ["B", "KiB", "MiB", "GiB", "TiB", "PiB"]
        var value = Math.max(0, bytes)
        var i = 0
        while (value >= 1024 && i < units.length - 1) {
            value /= 1024
            ++i
        }
        var digits = i === 0 ? 0 : (value < 100 ? 1 : 0)
        return qsTr("%1 %2").arg(value.toFixed(digits)).arg(units[i])
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

    Component.onCompleted: Log.debug("ui", "Design-system status footer ready")
}
