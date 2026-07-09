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
import qBittorrent

/*!
    \qmltype MDIcon
    \brief Renders a single Material Symbols Outlined glyph as text.

    The glyph is selected by \l icon — a codepoint string obtained from the
    \c Icons singleton (e.g. \c {Icons.play_arrow}). The bundled variable font
    is loaded once via a \l FontLoader; the FILL / weight / optical-size /
    grade axes are driven through \c font.variableAxes so a single font file
    covers every visual variant.

    Never render a raw glyph with a plain Text — always go through MDIcon so
    the font, sizing and color policy stay in one place.
*/
// Root is an Item (not Text): a Text element's implicitWidth/implicitHeight are
// read-only, so we give the icon a fixed square footprint on the Item and center
// the glyph Text inside it.
Item {
    id: root

    /*! Codepoint string to render, taken from the \c Icons singleton. */
    property string icon: ""

    /*! Rendered size in device-independent pixels. */
    property int size: 24

    /*! Sets the Material Symbols \c FILL axis (filled vs. outlined glyph). */
    property bool fill: false

    /*! Sets the Material Symbols \c wght (weight) axis. 100–700; 400 = regular. */
    property int weight: 400

    /*! Sets the Material Symbols \c GRAD (grade) axis. */
    property int grade: 0

    /*! Glyph color (defaults to the theme's onSurface). */
    property alias color: label.color

    /*!
        Location of the bundled Material Symbols font. Overridable for tests /
        alternate packaging; defaults to the app-assets resource path produced
        by \c qt_add_resources.
    */
    property string fontSource: "qrc:/fonts/MaterialSymbolsOutlined.ttf"

    // Fixed square footprint so icons align on a grid regardless of glyph metrics.
    implicitWidth: size
    implicitHeight: size

    Text {
        id: label
        anchors.centerIn: parent

        text: root.icon
        color: Theme.color("onSurface")

        font.family: symbolsLoader.status === FontLoader.Ready ? symbolsLoader.name : "Material Symbols Outlined"
        font.pixelSize: root.size
        // Variable-font axes (Qt 6.7+): one file, every FILL/weight/optical-size variant.
        font.variableAxes: ({
            "FILL": root.fill ? 1 : 0,
            "wght": root.weight,
            "GRAD": root.grade,
            "opsz": root.size
        })

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        textFormat: Text.PlainText
        renderType: Text.QtRendering
    }

    FontLoader {
        id: symbolsLoader
        source: root.fontSource
    }
}
