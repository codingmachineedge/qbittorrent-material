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
    \qmltype Spacing
    \brief Numeric spacing / radius / elevation tokens (DESIGN_SYSTEM §3).

    Use these instead of magic numbers, e.g.:
    \code
    ColumnLayout { spacing: Spacing.md }
    Rectangle { radius: Spacing.radiusCard }
    \endcode
*/
QtObject {
    id: spacing

    // ---- Spacing scale (px) ----------------------------------------------------
    // Canonical names (CONTRACTS §1.3) plus the descriptive aliases used in the
    // DESIGN_SYSTEM (xs4/sm8/...) — both resolve to the same value.
    readonly property int xs: 4
    readonly property int sm: 8
    readonly property int md: 12
    readonly property int lg: 16
    readonly property int xl: 24
    readonly property int xxl: 32

    readonly property int xs4: xs
    readonly property int sm8: sm
    readonly property int md12: md
    readonly property int lg16: lg
    readonly property int xl24: xl
    readonly property int xxl32: xxl

    // ---- Corner radii ----------------------------------------------------------
    readonly property int radiusCard: 12
    readonly property int radiusDialog: 16
    readonly property int radiusChip: 8
    readonly property int radiusField: 4

    // ---- Elevation tokens (Material `elevation`) ------------------------------
    readonly property int elevationPage: 0
    readonly property int elevationRail: 0
    readonly property int elevationCard: 1
    readonly property int elevationCardExpanded: 2
    readonly property int elevationToolbar: 0
    readonly property int elevationMenu: 8
    readonly property int elevationSnackbar: 6
    readonly property int elevationDialog: 24
    readonly property int elevationFab: 6

    // ---- Common component metrics ---------------------------------------------
    //! Standard touch target / row height for list & toolbar controls.
    readonly property int touchTarget: 40
    //! Default table row height.
    readonly property int rowHeight: 36
    //! Standard icon size.
    readonly property int iconSize: 24
    //! Compact icon size (dense toolbars / cells).
    readonly property int iconSizeSmall: 20

    Component.onCompleted: Log.debug("theme", "Spacing singleton ready")
}
