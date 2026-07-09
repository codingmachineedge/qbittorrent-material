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
    \qmltype LabeledField
    \brief A form row pairing a text label with an arbitrary editor control.

    The control may be supplied either through the \l control property or as a
    default child. With \c orientation == Qt.Horizontal (default) the label sits
    to the left; with Qt.Vertical it stacks above — useful for narrow /
    responsive layouts.

    \qml
    LabeledField { label: qsTr("Save path"); control: PathField { ... } }
    // or
    LabeledField { label: qsTr("Category"); ComboBox { Layout.fillWidth: true } }
    \endqml
*/
GridLayout {
    id: root

    /*! Already-translated label text. */
    property string label: ""

    /*! Optional explicit editor control; reparented into the field slot. */
    property Item control: null

    /*! Fixed label width for column alignment (0 = intrinsic width). */
    property int labelWidth: 0

    /*! Qt.Horizontal (label left) or Qt.Vertical (label above). */
    property int orientation: Qt.Horizontal

    /*! Editor slot — default children are placed here. */
    default property alias content: holder.data

    columns: orientation === Qt.Vertical ? 1 : 2
    columnSpacing: Spacing.md
    rowSpacing: Spacing.xs

    Label {
        text: root.label
        font: Typography.bodyMedium
        color: Theme.color("onSurfaceVariant")
        wrapMode: Text.WordWrap
        Layout.preferredWidth: root.labelWidth > 0 ? root.labelWidth : implicitWidth
        Layout.alignment: root.orientation === Qt.Vertical
                          ? Qt.AlignLeft | Qt.AlignVCenter
                          : Qt.AlignLeft | Qt.AlignVCenter
    }

    RowLayout {
        id: holder
        spacing: Spacing.sm
        Layout.fillWidth: true
    }

    onControlChanged: {
        if (control) {
            control.parent = holder
            control.Layout.fillWidth = true
        }
    }
}
