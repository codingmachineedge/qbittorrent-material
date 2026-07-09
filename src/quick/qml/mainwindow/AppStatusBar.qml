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
    AppStatusBar — the bottom Material status bar. Left→right it shows free disk
    space, external IP(s), DHT node count, a connection-status icon, an
    alternative-speed toggle, and the download / upload speed buttons (current
    rate, optional configured limit, and cumulative session total). Values are
    refreshed off \c Session.statsUpdated (never polled).

    It also exposes \c formatSpeed()/\c formatSize() so Main.qml can reuse the same
    formatting for the speed-in-title-bar feature.
*/
ToolBar {
    id: statusBar

    /// The Main.qml root (shell) for triggering the connection / speed actions.
    required property var shell

    // Bumped on every stats tick so every derived binding re-evaluates even if a
    // particular Session property lacks a fine-grained NOTIFY signal.
    property int tick: 0

    Material.elevation: 0
    implicitHeight: 32

    // Top divider.
    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.color("outlineVariant")
    }

    Connections {
        target: Session
        function onStatsUpdated() { statusBar.tick = statusBar.tick + 1 }
        function onFreeDiskSpaceChecked(result) { statusBar.tick = statusBar.tick + 1 }
    }

    component VSeparator: Rectangle {
        Layout.fillHeight: true
        Layout.topMargin: Spacing.xs
        Layout.bottomMargin: Spacing.xs
        implicitWidth: 1
        color: Theme.color("outlineVariant")
    }

    // A flat clickable speed indicator: MDIcon + monospace value label.
    component SpeedButton: ToolButton {
        id: sb
        property string glyph: ""
        property string value: ""
        display: AbstractButton.IconOnly
        Layout.minimumWidth: 180
        contentItem: RowLayout {
            spacing: Spacing.sm
            MDIcon { icon: sb.glyph; size: 16; color: Theme.color("onSurfaceVariant") }
            Label {
                text: sb.value
                font: Typography.mono
                color: Theme.color("onSurface")
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Spacing.md
        anchors.rightMargin: Spacing.md
        spacing: Spacing.sm

        // 1. Free disk space
        Label {
            visible: Preferences.isStatusbarFreeDiskSpaceDisplayed()
            text: qsTr("Free space: %1").arg(statusBar.freeSpaceText)
            font: Typography.bodyMedium
            color: Theme.color("onSurfaceVariant")
        }
        VSeparator { visible: Preferences.isStatusbarFreeDiskSpaceDisplayed() }

        // 2. External IP(s)
        Label {
            visible: Preferences.isStatusbarExternalIPDisplayed()
            text: statusBar.externalIPText
            font: Typography.bodyMedium
            color: Theme.color("onSurfaceVariant")
        }
        VSeparator { visible: Preferences.isStatusbarExternalIPDisplayed() }

        // 3. DHT nodes (only when DHT enabled)
        Label {
            visible: (Session.dhtEnabled || false)
            text: qsTr("DHT: %1 nodes").arg((statusBar.tick, Session.dhtNodes || 0))
            font: Typography.bodyMedium
            color: Theme.color("onSurfaceVariant")
        }
        VSeparator { visible: (Session.dhtEnabled || false) }

        // 4. Connection status
        ToolButton {
            id: connButton
            display: AbstractButton.IconOnly
            ToolTip.visible: hovered
            ToolTip.text: statusBar.connectionTooltip
            ToolTip.delay: 400
            contentItem: MDIcon {
                icon: statusBar.connectionIcon
                size: 18
                color: statusBar.connectionColor
                anchors.centerIn: parent
            }
            onClicked: {
                Log.info("ui", "Status bar: connection status clicked -> Options (Connection)")
                statusBar.shell.showOptionsConnectionTab()
            }
        }
        VSeparator {}

        // 5. Alternative speed limits toggle
        ToolButton {
            id: altSpeedButton
            display: AbstractButton.IconOnly
            checkable: true
            checked: statusBar.shell.altSpeedEnabled
            ToolTip.visible: hovered
            ToolTip.text: checked
                ? qsTr("Click to switch to regular speed limits")
                : qsTr("Click to switch to alternative speed limits")
            ToolTip.delay: 400
            contentItem: MDIcon {
                icon: Icons.speed
                size: 18
                fill: altSpeedButton.checked
                color: altSpeedButton.checked ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                anchors.centerIn: parent
            }
            onClicked: {
                Log.info("ui", "Status bar: alternative speed limits toggled")
                statusBar.shell.toggleAltSpeed()
            }
        }
        VSeparator {}

        // 6. Download speed
        SpeedButton {
            glyph: Icons.download
            value: statusBar.downloadText
            onClicked: {
                Log.info("ui", "Status bar: download speed clicked -> Global speed limits")
                statusBar.shell.showGlobalSpeedLimits()
            }
        }
        VSeparator {}

        // 7. Upload speed
        SpeedButton {
            glyph: Icons.upload
            value: statusBar.uploadText
            onClicked: {
                Log.info("ui", "Status bar: upload speed clicked -> Global speed limits")
                statusBar.shell.showGlobalSpeedLimits()
            }
        }
    }

    // -- Derived display strings (re-evaluated on every stats tick) ------------
    readonly property string freeSpaceText: (tick, formatSize(Session.freeDiskSpace || 0))

    readonly property string externalIPText: {
        void tick
        var v4 = Session.externalIPv4 || ""
        var v6 = Session.externalIPv6 || ""
        if (v4.length > 0 && v6.length > 0)
            return qsTr("External IPs: %1, %2").arg(v4).arg(v6)
        if (v4.length > 0)
            return qsTr("External IP: %1").arg(v4)
        if (v6.length > 0)
            return qsTr("External IP: %1").arg(v6)
        return qsTr("External IP: N/A")
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
    readonly property string connectionTooltip: {
        void tick
        if (!(Session.listening || false))
            return qsTr("Connection Status: Offline. This usually means that qBittorrent failed to listen on the selected port for incoming connections.")
        if (Session.hasIncomingConnections || false)
            return qsTr("Connection Status: Online")
        return qsTr("Connection status: No direct connections. This may indicate network configuration problems.")
    }

    readonly property string downloadText: {
        void tick
        var s = formatSpeed(Session.downloadRate || 0)
        var limit = SpeedLimitController.downloadLimit || 0
        if (limit > 0)
            s += " [" + formatSpeed(limit) + "]"
        s += " (" + formatSize(Session.totalDownloadSession || 0) + ")"
        return s
    }
    readonly property string uploadText: {
        void tick
        var s = formatSpeed(Session.uploadRate || 0)
        var limit = SpeedLimitController.uploadLimit || 0
        if (limit > 0)
            s += " [" + formatSpeed(limit) + "]"
        s += " (" + formatSize(Session.totalUploadSession || 0) + ")"
        return s
    }

    // -- Formatting helpers (also used by Main.qml for the title bar) ----------

    /// Human-friendly byte size, e.g. "1.4 GiB".
    function formatSize(bytes) {
        var units = ["B", "KiB", "MiB", "GiB", "TiB", "PiB"]
        var value = Math.max(0, bytes)
        var i = 0
        while (value >= 1024 && i < units.length - 1) {
            value /= 1024
            ++i
        }
        var digits = (i === 0) ? 0 : (value < 100 ? 1 : 0)
        return qsTr("%1 %2").arg(value.toFixed(digits)).arg(units[i])
    }

    /// Human-friendly transfer rate, e.g. "512 KiB/s".
    function formatSpeed(bytesPerSecond) {
        var units = ["B/s", "KiB/s", "MiB/s", "GiB/s", "TiB/s"]
        var value = Math.max(0, bytesPerSecond)
        var i = 0
        while (value >= 1024 && i < units.length - 1) {
            value /= 1024
            ++i
        }
        var digits = (i === 0) ? 0 : (value < 100 ? 1 : 0)
        return qsTr("%1 %2").arg(value.toFixed(digits)).arg(units[i])
    }

    Component.onCompleted: Log.debug("ui", "AppStatusBar ready")
}
