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
    \qmltype SpeedTab
    \brief The properties "Speed" tab.

    Hosts the live \c SpeedPlotView when the speed graph is enabled
    (\c SpeedWidget/Enabled); otherwise shows the disabled placeholder, matching
    the legacy \c PropertiesWidget::configure() swap.
*/
Item {
    id: root

    readonly property bool graphEnabled: Preferences.value("SpeedWidget/Enabled", true) === true

    SpeedPlotView {
        anchors.fill: parent
        anchors.margins: Spacing.sm
        visible: root.graphEnabled
    }

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Spacing.sm
        visible: !root.graphEnabled
        width: Math.min(parent.width - Spacing.xl * 2, 360)

        Label {
            text: qsTr("Speed graphs are disabled")
            font: Typography.titleMedium
            color: Theme.color("onSurface")
            horizontalAlignment: Text.AlignHCenter
            Layout.fillWidth: true
        }
        Label {
            text: qsTr("You can enable it in Advanced Options")
            font: Typography.bodyMedium
            color: Theme.color("onSurfaceVariant")
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
        }
    }

    Component.onCompleted: Log.debug("ui", "SpeedTab loaded; graphEnabled=" + graphEnabled)
}
