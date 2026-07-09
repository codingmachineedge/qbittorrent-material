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
    \qmltype PieceAvailabilityBar
    \brief A thin, segmented bar showing per-piece availability.

    Material equivalent of the legacy \c PieceAvailabilityBar. A thin wrapper
    around the C++ \c PiecesBarItem driven in \c Availability mode: \l availability
    is an integer array (index = piece index) giving how many copies of each piece
    are reachable across the swarm, painted as a 0..max heat scale from
    \c surfaceVariant (absent) to \c primary (most available). All scaling and
    blending happens in the painted item; this wrapper binds data + palette and
    adds a hover tooltip.
*/
Item {
    id: root

    /*! Integer array: availability count per piece (\c QList<int>). */
    property var availability

    implicitHeight: 18
    implicitWidth: 200

    PiecesBarItem {
        id: bar
        anchors.fill: parent
        mode: PiecesBarItem.Availability

        availability: root.availability !== undefined ? root.availability : []

        pieceColor: Theme.color("primary")
        missingColor: Theme.color("surfaceVariant")
        borderColor: Theme.color("outlineVariant")
    }

    HoverHandler { id: hover }

    ToolTip {
        visible: hover.hovered && root.availability && (root.availability.length > 0)
        delay: 400
        text: {
            const n = (root.availability && root.availability.length !== undefined) ? root.availability.length : 0
            if (n <= 0 || root.width <= 0)
                return ""
            const x = hover.point.position.x
            const piece = Math.max(0, Math.min(n - 1, Math.floor(x * n / root.width)))
            const val = root.availability[piece]
            return qsTr("Piece %1: %2 copies").arg(piece + 1).arg(val)
        }
    }

    Component.onCompleted: Log.debug("ui", "PieceAvailabilityBar ready")
}
