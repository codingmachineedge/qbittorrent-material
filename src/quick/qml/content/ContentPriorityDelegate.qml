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
    \qmltype ContentPriorityDelegate
    \brief The "Download Priority" cell of the content tree — a compact combo box
           mapping to \c BitTorrent::DownloadPriority.

    Values follow the engine enum: \c {Do not download} = 0 (Ignored),
    \c Normal = 1, \c High = 6, \c Maximum = 7, \c Mixed = -1. The \c Mixed entry
    is only offered while \l allowMixed is true (a folder whose children disagree);
    selecting a concrete value emits \l priorityPicked so the caller can commit it
    to the model. The label strings are wrapped in \c qsTr so they retranslate live.
*/
ComboBox {
    id: root

    /*! Current priority value (DownloadPriority int). Two-way friendly. */
    property int priority: 1

    /*! Whether to include the read-only "Mixed" entry (mixed-priority folders). */
    property bool allowMixed: false

    /*! Emitted when the user chooses a concrete priority; \c value is the enum int. */
    signal priorityPicked(int value)

    // The concrete choices, plus an optional leading "Mixed" sentinel.
    readonly property var _entries: {
        const base = [
            { "label": qsTr("Do not download", "Do not download (priority)"), "value": 0 },
            { "label": qsTr("Normal", "Normal (priority)"), "value": 1 },
            { "label": qsTr("High", "High (priority)"), "value": 6 },
            { "label": qsTr("Maximum", "Maximum (priority)"), "value": 7 }
        ];
        if (root.allowMixed || (root.priority === -1))
            return [{ "label": qsTr("Mixed", "Mixed (priorities)"), "value": -1 }].concat(base);
        return base;
    }

    function _indexForValue(value) {
        for (let i = 0; i < _entries.length; ++i) {
            if (_entries[i].value === value)
                return i;
        }
        return 0;
    }

    model: _entries.map(e => e.label)
    currentIndex: _indexForValue(priority)

    flat: true
    font: Typography.bodyMedium
    implicitHeight: 28
    topInset: 0
    bottomInset: 0

    // Keep the combo in sync when the model pushes a new priority in.
    onPriorityChanged: currentIndex = _indexForValue(priority)

    onActivated: (index) => {
        const picked = _entries[index].value;
        Log.debug("ui", "ContentPriorityDelegate: picked priority " + picked);
        if (picked !== -1)
            root.priorityPicked(picked);
        else
            currentIndex = _indexForValue(root.priority); // never commit "Mixed"
    }

    contentItem: Label {
        text: root.displayText
        font: root.font
        color: Theme.color("onSurface")
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
        leftPadding: Spacing.sm
        rightPadding: root.indicator ? root.indicator.width : Spacing.sm
    }

    background: Rectangle {
        color: root.hovered ? Qt.alpha(Theme.color("onSurface"), 0.06) : "transparent"
        radius: Spacing.radiusField
    }
}
