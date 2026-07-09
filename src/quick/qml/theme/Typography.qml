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

pragma Singleton

import QtQuick
import qBittorrent

/*!
    \qmltype Typography
    \brief Roboto type-scale tokens (DESIGN_SYSTEM §2).

    Each token is a ready-to-assign \c font group, e.g.:
    \code
    Label { font: Typography.titleMedium; text: qsTr("Transfer") }
    Label { font: Typography.mono; text: speedText }   // tabular numerics
    \endcode

    Sizes are given in pixels (\c pixelSize) so they match the Material spec
    regardless of screen DPI scaling handled by Qt.
*/
QtObject {
    id: typography

    // The app-assets resource collection is mounted at qrc:/, so fonts under
    // resources/fonts are available at qrc:/fonts/<file>. FontLoader registers
    // each bundled face with the application and exposes its canonical family
    // name; the literal fallbacks keep the UI usable if a package is damaged.
    readonly property FontLoader robotoLoader: FontLoader {
        source: "qrc:/fonts/Roboto.ttf"
    }
    readonly property FontLoader robotoMonoLoader: FontLoader {
        source: "qrc:/fonts/RobotoMono.ttf"
    }

    //! Primary UI font family.
    readonly property string family: robotoLoader.status === FontLoader.Ready
        ? robotoLoader.name : "Roboto"
    //! Monospaced family for tabular numerics (speeds/sizes/ratios).
    readonly property string monoFamily: robotoMonoLoader.status === FontLoader.Ready
        ? robotoMonoLoader.name : "Roboto Mono"

    // ---- Display / headline / title -------------------------------------------

    //! 24/32 — dialog titles.
    readonly property font headlineSmall: Qt.font({
        family: family, pixelSize: 24, weight: Font.Normal, letterSpacing: 0
    })

    //! 22 — About header and other large titles.
    readonly property font titleLarge: Qt.font({
        family: family, pixelSize: 22, weight: Font.Normal, letterSpacing: 0
    })

    //! 16 SemiBold — card / group-box headers.
    readonly property font titleMedium: Qt.font({
        family: family, pixelSize: 16, weight: Font.DemiBold, letterSpacing: 0.15
    })

    //! 14 SemiBold — tab labels.
    readonly property font titleSmall: Qt.font({
        family: family, pixelSize: 14, weight: Font.DemiBold, letterSpacing: 0.1
    })

    // ---- Body -----------------------------------------------------------------

    //! 14 — values (General tab / Statistics value rows).
    readonly property font bodyLarge: Qt.font({
        family: family, pixelSize: 14, weight: Font.Normal, letterSpacing: 0.15
    })

    //! 13 — table cells and form labels.
    readonly property font bodyMedium: Qt.font({
        family: family, pixelSize: 13, weight: Font.Normal, letterSpacing: 0.25
    })

    // ---- Label ----------------------------------------------------------------

    //! 14 Medium — buttons.
    readonly property font labelLarge: Qt.font({
        family: family, pixelSize: 14, weight: Font.Medium, letterSpacing: 0.1
    })

    //! 11 — captions / counts.
    readonly property font labelSmall: Qt.font({
        family: family, pixelSize: 11, weight: Font.Medium, letterSpacing: 0.5
    })

    // ---- Monospaced (tabular) -------------------------------------------------

    //! Default mono, matches bodyMedium size — for numeric table cells.
    readonly property font mono: Qt.font({
        family: monoFamily, pixelSize: 13, weight: Font.Normal
    })

    //! Larger mono for prominent numeric values (e.g. statistics figures).
    readonly property font monoLarge: Qt.font({
        family: monoFamily, pixelSize: 14, weight: Font.Normal
    })

    Component.onCompleted: Log.debug("theme", "Typography singleton ready; family=" + family)
}
