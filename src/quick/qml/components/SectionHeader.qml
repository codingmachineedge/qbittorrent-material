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
    \qmltype SectionHeader
    \brief Card / section title row (titleMedium) with an optional leading icon
           and a right-aligned trailing slot.

    The default property collects trailing controls (buttons, counts, …) which
    are pushed to the right edge:
    \qml
    SectionHeader {
        text: qsTr("Trackers")
        icon: Icons.dns
        IconButton { icon: Icons.person_add; tooltip: qsTr("Add tracker") }
    }
    \endqml
*/
RowLayout {
    id: root

    /*! Already-translated header text. */
    property string text: ""

    /*! Optional leading icon codepoint from the \c Icons singleton. */
    property string icon: ""

    /*! Trailing, right-aligned controls (default children). */
    default property alias trailing: trailingRow.data

    spacing: Spacing.sm

    MDIcon {
        visible: root.icon.length > 0
        icon: root.icon
        size: 20
        color: Theme.color("onSurfaceVariant")
        Layout.alignment: Qt.AlignVCenter
    }

    Label {
        text: root.text
        font: Typography.titleMedium
        color: Theme.color("onSurface")
        elide: Text.ElideRight
        Layout.fillWidth: true
        Layout.alignment: Qt.AlignVCenter
    }

    Row {
        id: trailingRow
        spacing: Spacing.xs
        Layout.alignment: Qt.AlignVCenter | Qt.AlignRight
    }
}
