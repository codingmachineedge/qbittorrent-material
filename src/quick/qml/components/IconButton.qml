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
    \qmltype IconButton
    \brief Flat, round Material button wrapping an \l MDIcon with a tooltip.

    Standard \c enabled / \c checkable / \c checked semantics are inherited
    from \c ToolButton. The icon fills when the button is checked. Key
    interactions are logged through the \c Log singleton — user \c onClicked /
    \c onToggled handlers assigned at the call site remain free to run
    (logging is attached separately via \l Connections).
*/
ToolButton {
    id: root

    /*! Codepoint string for the glyph, from the \c Icons singleton. */
    property string icon: ""

    /*! Icon size in pixels. */
    property int size: 24

    /*! Already-translated tooltip text; shown on hover when non-empty. */
    property string tooltip: ""

    /*! Base icon color (overridden to a muted tone when disabled). */
    property color color: Theme.color("onSurfaceVariant")

    display: AbstractButton.IconOnly
    flat: true
    padding: Spacing.xs

    implicitWidth: Math.max(size + Spacing.sm * 2, 32)
    implicitHeight: Math.max(size + Spacing.sm * 2, 32)

    contentItem: MDIcon {
        icon: root.icon
        size: root.size
        fill: root.checked
        color: root.enabled ? (root.checked ? Theme.color("primary") : root.color)
                            : Theme.color("outline")
    }

    ToolTip.visible: hovered && root.tooltip.length > 0
    ToolTip.text: root.tooltip
    ToolTip.delay: 500

    // Attach logging without shadowing caller-supplied signal handlers.
    Connections {
        target: root
        function onClicked() {
            Log.debug("ui", "IconButton clicked: " + (root.tooltip.length ? root.tooltip : root.icon))
        }
        function onToggled() {
            if (root.checkable)
                Log.debug("ui", "IconButton toggled -> " + root.checked + " (" + root.tooltip + ")")
        }
    }
}
