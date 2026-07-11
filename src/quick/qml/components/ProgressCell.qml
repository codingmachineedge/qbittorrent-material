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
    \qmltype ProgressCell
    \brief A Material progress bar overlaid with a centered percentage label.

    Used as a table cell delegate. \l progress is 0.0–1.0. When \l active is
    false (Error / Stopped / Unknown states) the cell renders in a disabled
    tone. \l barColor lets a caller follow the row state color per the
    ProgressBarFollowsTextColor policy; it defaults to the Material primary.
*/
Item {
    id: root

    /*! Completion fraction, 0.0–1.0. */
    property real progress: 0.0

    /*! Whether the bar is active; false → disabled look. */
    property bool active: true

    /*! Bar fill color (default: primary; caller may pass the row state color). */
    property color barColor: Theme.color("primary")

    /*! Optional label override; defaults to the computed percentage. */
    property string text: Math.round(Math.max(0, Math.min(1, progress)) * 100) + "%"

    implicitWidth: 120
    implicitHeight: 36

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: Spacing.sm
        anchors.rightMargin: Spacing.sm
        anchors.topMargin: 5
        anchors.bottomMargin: 4
        spacing: 3

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 6
            radius: 3
            color: Theme.color("outlineVariant")
            clip: true

            Rectangle {
                width: parent.width * Math.max(0, Math.min(1, root.progress))
                height: parent.height
                radius: parent.radius
                color: root.active ? root.barColor : Theme.color("outline")
                Behavior on width {
                    NumberAnimation {
                        duration: Spacing.motionBase
                        easing.type: Easing.OutCubic
                    }
                }
            }
        }

        Label {
            text: root.text
            font: Typography.metadataMono
            color: root.active ? Theme.color("muted") : Theme.color("outline")
            Layout.alignment: Qt.AlignHCenter
        }
    }
}
