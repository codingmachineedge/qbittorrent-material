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
    \qmltype DownloadedPiecesBar
    \brief A thin, segmented bar showing per-piece download progress.

    Material equivalent of the legacy \c DownloadedPiecesBar. A thin QML wrapper
    around the C++ \c PiecesBarItem (a \c QQuickPaintedItem) in \c Downloaded
    mode: completed pieces render in the piece color, in-progress pieces in the
    partial tone and missing pieces in \c surfaceVariant. All down/up-scaling and
    blending happens inside the painted item; this wrapper only binds the
    \c QBitArray data + the Theme palette and adds a hover tooltip.

    Palette follows DESIGN_SYSTEM's PiecesBar map: Piece → \c primary,
    PartialPiece → \c primaryContainer, MissingPiece → \c surfaceVariant,
    Border → \c outlineVariant.
*/
Item {
    id: root

    /*! Fully-downloaded pieces (\c QBitArray, from \c PropertiesController.havePieces). */
    property var pieces

    /*! Pieces currently downloading (\c QBitArray, from \c PropertiesController.downloadingPieces). */
    property var downloadingPieces

    /*! Overall completed fraction (0..1) — surfaced in the hover tooltip. */
    property real progress: 0.0

    implicitHeight: 18
    implicitWidth: 200

    PiecesBarItem {
        id: bar
        anchors.fill: parent
        mode: PiecesBarItem.Downloaded

        pieces: root.pieces
        downloadingPieces: root.downloadingPieces

        // DESIGN_SYSTEM PiecesBar palette (bound to Theme, never hard-coded).
        pieceColor: Theme.color("primary")
        partialColor: Theme.color("primaryContainer")
        missingColor: Theme.color("surfaceVariant")
        borderColor: Theme.color("outlineVariant")
    }

    HoverHandler { id: hover }

    ToolTip {
        visible: hover.hovered
        delay: 400
        text: qsTr("Progress: %1").arg(Math.round(root.progress * 100) + "%")
    }

    Component.onCompleted: Log.debug("ui", "DownloadedPiecesBar ready")
}
