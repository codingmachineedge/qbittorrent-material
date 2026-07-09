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

/*!
    \qmltype FilterTextField
    \brief Search / filter input with a leading search glyph, a clear
           affordance and a pattern-format (regex) toggle.

    \l text and \l regexEnabled are two-way bindable. The regex toggle is driven
    from the embedded \l FilterPatternFormatMenu opened via the trailing button.
*/
TextField {
    id: root

    /*! Placeholder shown when empty. */
    property string placeholder: qsTr("Filter…")

    /*! Two-way: whether regex matching is enabled (vs. wildcard). */
    property bool regexEnabled: false

    placeholderText: placeholder
    selectByMouse: true

    leftPadding: searchIcon.width + Spacing.md
    rightPadding: trailing.width + Spacing.sm

    onTextChanged: Log.trace("ui", "FilterTextField text -> '" + text + "'")

    MDIcon {
        id: searchIcon
        icon: Icons.search
        size: 18
        color: Theme.color("onSurfaceVariant")
        anchors.left: parent.left
        anchors.leftMargin: Spacing.sm
        anchors.verticalCenter: parent.verticalCenter
    }

    Row {
        id: trailing
        spacing: 0
        anchors.right: parent.right
        anchors.rightMargin: Spacing.xs
        anchors.verticalCenter: parent.verticalCenter

        IconButton {
            icon: Icons.close
            size: 16
            visible: root.text.length > 0
            tooltip: qsTr("Clear")
            onClicked: {
                Log.debug("ui", "FilterTextField cleared")
                root.clear()
            }
        }

        IconButton {
            icon: Icons.settings
            size: 16
            checkable: false
            checked: root.regexEnabled
            tooltip: qsTr("Pattern format")
            onClicked: formatMenu.popup()
        }
    }

    FilterPatternFormatMenu {
        id: formatMenu
        regexEnabled: root.regexEnabled
        onFormatChanged: root.regexEnabled = regexEnabled
    }
}
