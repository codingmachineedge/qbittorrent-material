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

/*!
    \qmltype ConfirmDialog
    \brief Material confirmation dialog with an optional destructive warning and
           a "Don't ask again" affordance.

    Call \c open(). When \l rememberKey is set, a checkbox is shown and its
    "remembered" state is persisted through \c Preferences; if the user has
    previously opted in, \c open() short-circuits and \c accepted() fires
    immediately. Handle \c onAccepted / \c onRejected as usual.
*/
Dialog {
    id: root

    /*! Body message (already translated). */
    property string text: ""

    /*! Accept button label. */
    property string acceptText: qsTr("OK")

    /*! Reject button label. */
    property string rejectText: qsTr("Cancel")

    /*! When true the dialog shows a warning icon and error-toned accept button. */
    property bool destructive: false

    /*! Preferences key for the "Don't ask again" opt-in ("" disables it). */
    property string rememberKey: ""

    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(implicitWidth, (parent ? parent.width : 600) * 0.9)
    padding: Spacing.lg

    Material.elevation: 24
    Material.roundedScale: Material.MediumScale

    background: Rectangle {
        radius: Spacing.radiusDialog
        color: Theme.color("surface")
    }

    // Short-circuit when the user previously chose "Don't ask again".
    function open() {
        if (rememberKey.length && Preferences.value(rememberKey, false) === true) {
            Log.debug("ui", "ConfirmDialog '" + title + "' auto-accepted (remembered)")
            accepted()
            return
        }
        Log.debug("ui", "ConfirmDialog opened: " + title)
        visible = true
    }

    header: Label {
        text: root.title
        font: Typography.headlineSmall
        color: Theme.color("onSurface")
        elide: Text.ElideRight
        padding: Spacing.lg
        bottomPadding: Spacing.sm
    }

    contentItem: ColumnLayout {
        spacing: Spacing.md

        RowLayout {
            spacing: Spacing.md
            Layout.fillWidth: true

            MDIcon {
                visible: root.destructive
                icon: Icons.warning
                size: 28
                color: Theme.color("error")
                Layout.alignment: Qt.AlignTop
            }

            Label {
                text: root.text
                font: Typography.bodyMedium
                color: Theme.color("onSurfaceVariant")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        CheckBox {
            id: rememberCheck
            visible: root.rememberKey.length > 0
            text: qsTr("Don't ask again")
            font: Typography.bodyMedium
        }
    }

    footer: DialogButtonBox {
        spacing: Spacing.sm
        padding: Spacing.lg
        topPadding: Spacing.sm

        Button {
            text: root.rejectText
            flat: true
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: root.reject()
        }

        Button {
            text: root.acceptText
            highlighted: true
            Material.accent: root.destructive ? Theme.color("error") : Theme.color("primary")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: root.accept()
        }
    }

    onAccepted: {
        Log.info("ui", "ConfirmDialog '" + title + "' accepted")
        if (rememberCheck.checked && rememberKey.length) {
            Preferences.setValue(rememberKey, true)
            Log.debug("ui", "ConfirmDialog remembered choice under key " + rememberKey)
        }
    }
    onRejected: Log.debug("ui", "ConfirmDialog '" + title + "' rejected")
}
