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
    \qmltype SearchNoPluginsPage
    \brief The empty state shown when no search plugins are installed.

    Mirrors the legacy \c emptyPage: a centered explanation plus a shortcut to
    install plugins.
*/
Item {
    id: root

    /*! Emitted when the user clicks the install shortcut. */
    signal installRequested()

    Component.onCompleted: Log.debug("search", "SearchNoPluginsPage shown")

    ColumnLayout {
        anchors.centerIn: parent
        width: Math.min(parent.width - Spacing.xl * 2, 480)
        spacing: Spacing.lg

        MDIcon {
            icon: Icons.extension
            size: 64
            color: Theme.color("onSurfaceVariant")
            Layout.alignment: Qt.AlignHCenter
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font: Typography.titleMedium
            color: Theme.color("onSurface")
            text: qsTr("There aren't any search plugins installed.")
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            font: Typography.bodyMedium
            color: Theme.color("onSurfaceVariant")
            text: qsTr("Click the \"Search plugins…\" button at the bottom right of the window to install some.")
        }

        Button {
            Layout.alignment: Qt.AlignHCenter
            highlighted: true
            text: qsTr("Install search plugins")
            onClicked: {
                Log.info("search", "Install plugins requested from empty page")
                root.installRequested()
            }
        }
    }
}
