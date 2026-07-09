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
    \qmltype TriStateComboField
    \brief Three-way Default / Yes / No selector mapping to an optional bool.

    The \l value uses the component's own enum, referenced by type name:
    \c TriStateComboField.Default (use the global / inherited setting),
    \c TriStateComboField.Yes, \c TriStateComboField.No. The C++ side maps these
    to \c std::optional<bool> (Default → nullopt, Yes → true, No → false).

    Two convenience helpers bridge to/from an optional bool for controllers:
    \l fromOptional() and \l toOptional().
*/
ComboBox {
    id: root

    /*! Tri-state token; indices align with the model below. */
    enum State { Default = 0, Yes = 1, No = 2 }

    /*! Two-way: current tri-state value (one of the \c State enum values). */
    property int value: TriStateComboField.Default

    /*! Emitted when the user picks a new value (also fires the property NOTIFY). */
    signal picked(int value)

    model: [ qsTr("Default"), qsTr("Yes"), qsTr("No") ]
    currentIndex: value

    onActivated: (index) => {
        value = index
        Log.debug("ui", "TriStateComboField -> " + model[index])
        root.picked(index)
    }

    // Keep currentIndex in sync when value is set programmatically.
    onValueChanged: if (currentIndex !== value) currentIndex = value

    /*! Map an optional bool (null/undefined | true | false) to a \c State. */
    function fromOptional(opt) {
        if (opt === undefined || opt === null)
            value = TriStateComboField.Default
        else
            value = opt ? TriStateComboField.Yes : TriStateComboField.No
    }

    /*! Map the current \c State back to an optional bool (undefined for Default). */
    function toOptional() {
        if (value === TriStateComboField.Yes)
            return true
        if (value === TriStateComboField.No)
            return false
        return undefined
    }
}
