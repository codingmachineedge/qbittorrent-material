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
    \qmltype SpeedSpinBox
    \brief Numeric spin box for KiB/s limits with a sentinel "unlimited" value.

    When \l value equals \l unlimitedValue (the low end of the range, e.g. 0),
    the field renders \l unlimitedText (e.g. "∞") instead of a number; any other
    value is shown with the localized number and \l suffix. Callers set \l from /
    \l to / \l unlimitedValue to match the semantics of the specific limit.
*/
SpinBox {
    id: root

    /*! Text shown for the unlimited sentinel value. */
    property string unlimitedText: qsTr("∞")

    /*! Unit suffix appended to real values (e.g. "KiB/s"). Empty = none. */
    property string suffix: qsTr("KiB/s")

    /*! The value that maps to \l unlimitedText (defaults to \l from). */
    property int unlimitedValue: from

    from: 0
    to: 1000000
    stepSize: 1
    editable: true

    textFromValue: function(value, locale) {
        if (value === root.unlimitedValue && root.unlimitedText.length > 0)
            return root.unlimitedText
        var num = Number(value).toLocaleString(locale, 'f', 0)
        return root.suffix.length > 0 ? (num + " " + root.suffix) : num
    }

    valueFromText: function(text, locale) {
        if (root.unlimitedText.length > 0 && text.indexOf(root.unlimitedText) !== -1)
            return root.unlimitedValue
        var digits = text.replace(/[^0-9\-]/g, "")
        var parsed = parseInt(digits)
        return isNaN(parsed) ? root.unlimitedValue : parsed
    }

    onValueModified: Log.debug("ui", "SpeedSpinBox value -> " + value)
}
