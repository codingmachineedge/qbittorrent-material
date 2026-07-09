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
    \qmltype MaterialCard
    \brief Elevated, rounded surface container (radius \c Spacing.radiusCard,
           elevation 1) with an optional \l SectionHeader.

    Content is provided as default children and stacked in a column beneath the
    optional header:
    \qml
    MaterialCard {
        title: qsTr("Transfer")
        ColumnLayout { ... }
    }
    \endqml
*/
Pane {
    id: root

    /*! Optional header title; when empty no header is drawn. */
    property string title: ""

    /*! Optional leading header icon codepoint (from \c Icons). */
    property string titleIcon: ""

    /*! Material elevation of the card surface. */
    property int elevation: 1

    /*! Card body — default children, laid out vertically. */
    default property alias content: column.data

    padding: Spacing.md
    Material.elevation: root.elevation

    background: Rectangle {
        radius: Spacing.radiusCard
        color: Theme.color("surface")
        border.width: 1
        border.color: Theme.color("outlineVariant")
    }

    contentItem: ColumnLayout {
        id: column
        spacing: Spacing.sm

        // Declared first, so default children are appended *after* it.
        SectionHeader {
            id: header
            visible: root.title.length > 0
            text: root.title
            icon: root.titleIcon
            Layout.fillWidth: true
        }
    }
}
