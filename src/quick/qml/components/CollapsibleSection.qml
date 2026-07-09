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
import QtCore

/*!
    \qmltype CollapsibleSection
    \brief Expand / collapse container with an animated header chevron.

    The header row toggles the body's visibility (height animated). When a
    \l persistKey is supplied the expanded state is persisted across sessions.
*/
ColumnLayout {
    id: root

    /*! Already-translated header title. */
    property string title: ""

    /*! Optional leading header icon codepoint. */
    property string icon: ""

    /*! Whether the body is currently expanded. */
    property bool expanded: true

    /*! When set, \l expanded is persisted under this key. */
    property string persistKey: ""

    /*! Body content — default children. */
    default property alias content: body.data

    /*! Emitted whenever the section is toggled by the user. */
    signal toggled(bool isExpanded)

    spacing: 0

    // Persist expanded state only when a key is given, so unrelated sections
    // never collide on a shared settings group.
    Loader {
        active: root.persistKey.length > 0
        sourceComponent: Settings {
            category: "CollapsibleSection/" + root.persistKey
            property alias expandedState: root.expanded
        }
    }

    // Header row --------------------------------------------------------------
    ItemDelegate {
        id: headerRow
        Layout.fillWidth: true
        padding: Spacing.sm

        contentItem: RowLayout {
            spacing: Spacing.sm

            MDIcon {
                icon: Icons.expand_more
                size: 20
                color: Theme.color("onSurfaceVariant")
                rotation: root.expanded ? 0 : -90
                Behavior on rotation { NumberAnimation { duration: 150; easing.type: Easing.OutCubic } }
            }

            MDIcon {
                visible: root.icon.length > 0
                icon: root.icon
                size: 20
                color: Theme.color("onSurfaceVariant")
            }

            Label {
                text: root.title
                font: Typography.titleSmall
                color: Theme.color("onSurface")
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        onClicked: {
            root.expanded = !root.expanded
            Log.debug("ui", "CollapsibleSection '" + root.title + "' toggled -> " + root.expanded)
            root.toggled(root.expanded)
        }
    }

    // Body (height-animated, clipped) ----------------------------------------
    Item {
        id: bodyClip
        Layout.fillWidth: true
        clip: true
        implicitHeight: root.expanded ? body.implicitHeight : 0
        Behavior on implicitHeight { NumberAnimation { duration: 160; easing.type: Easing.OutCubic } }

        ColumnLayout {
            id: body
            width: parent.width
            spacing: Spacing.sm
        }
    }
}
