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
import qBittorrent

/*!
    \qmltype FilterPatternFormatMenu
    \brief Context menu that selects the pattern format (Wildcard / Regular
           expression) for a \l FilterTextField.

    \l regexEnabled is two-way bindable; picking an entry flips it and emits
    \l formatChanged(). Open with \c popup().
*/
Menu {
    id: root

    /*! Two-way: whether the regex format is selected. */
    property bool regexEnabled: false

    /*! Emitted when the user changes the pattern format. */
    signal formatChanged()

    modal: false

    MenuItem {
        text: qsTr("Wildcard")
        checkable: true
        checked: !root.regexEnabled
        onTriggered: {
            root.regexEnabled = false
            Log.debug("ui", "Filter pattern format -> Wildcard")
            root.formatChanged()
        }
    }

    MenuItem {
        text: qsTr("Regular expression")
        checkable: true
        checked: root.regexEnabled
        onTriggered: {
            root.regexEnabled = true
            Log.debug("ui", "Filter pattern format -> Regular expression")
            root.formatChanged()
        }
    }
}
