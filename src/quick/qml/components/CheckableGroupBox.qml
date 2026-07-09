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
    \qmltype CheckableGroupBox
    \brief Card whose header \c Switch enables (and expands) its body.

    The body is disabled and collapsed while \l checked is false; enabling it
    reveals the content and raises the card elevation 1 → 2, matching the
    Material "checkable group box" convention.
*/
Pane {
    id: root

    /*! Already-translated header title. */
    property string title: ""

    /*! Two-way: the header switch state; also enables the body. */
    property bool checked: false

    /*! Body content — default children (disabled when \c !checked). */
    default property alias content: body.data

    /*! Emitted when the user flips the switch. */
    signal toggled(bool isChecked)

    padding: Spacing.md
    Material.elevation: root.checked ? 2 : 1

    background: Rectangle {
        radius: Spacing.radiusCard
        color: Theme.color("surface")
        border.width: 1
        border.color: Theme.color("outlineVariant")
    }

    contentItem: ColumnLayout {
        spacing: Spacing.sm

        RowLayout {
            Layout.fillWidth: true
            spacing: Spacing.sm

            Label {
                text: root.title
                font: Typography.titleMedium
                color: Theme.color("onSurface")
                elide: Text.ElideRight
                Layout.fillWidth: true
                Layout.alignment: Qt.AlignVCenter
            }

            Switch {
                id: enableSwitch
                checked: root.checked
                onClicked: {
                    root.checked = checked
                    Log.debug("ui", "CheckableGroupBox '" + root.title + "' -> " + root.checked)
                    root.toggled(root.checked)
                }
            }
        }

        // Body: collapses to zero height and disables when unchecked.
        Item {
            id: bodyClip
            Layout.fillWidth: true
            clip: true
            enabled: root.checked
            opacity: root.checked ? 1.0 : 0.0
            implicitHeight: root.checked ? body.implicitHeight : 0
            Behavior on implicitHeight { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }
            Behavior on opacity { NumberAnimation { duration: 120 } }

            ColumnLayout {
                id: body
                width: parent.width
                spacing: Spacing.sm
            }
        }
    }
}
