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
    UILockScreen — an opaque full-window overlay shown while the UI is locked
    (AppController.locked). It swallows all input to the app underneath and
    presents a password prompt; a correct password calls AppController.unlock().
*/
Rectangle {
    id: lockScreen

    color: Theme.color("surface")

    // Swallow every mouse / wheel event so the UI beneath cannot be interacted with.
    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onWheel: (wheel) => wheel.accepted = true
    }

    // Focus the field and clear stale input whenever the lock screen appears.
    onVisibleChanged: {
        if (visible) {
            Log.info("ui", "UI lock screen shown")
            passwordField.text = ""
            errorLabel.visible = false
            passwordField.forceActiveFocus()
        } else {
            Log.info("ui", "UI unlocked")
        }
    }

    MaterialCard {
        anchors.centerIn: parent
        width: Math.min(360, lockScreen.width - Spacing.xl * 2)
        padding: Spacing.xl

        ColumnLayout {
            width: parent.width
            spacing: Spacing.lg

            MDIcon {
                icon: Icons.lock
                size: 48
                color: Theme.color("primary")
                Layout.alignment: Qt.AlignHCenter
            }
            Label {
                text: qsTr("qBittorrent is locked")
                font: Typography.headlineSmall
                color: Theme.color("onSurface")
                Layout.alignment: Qt.AlignHCenter
            }
            Label {
                text: qsTr("Enter your password to unlock the interface.")
                font: Typography.bodyMedium
                color: Theme.color("onSurfaceVariant")
                horizontalAlignment: Text.AlignHCenter
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
            TextField {
                id: passwordField
                Layout.fillWidth: true
                echoMode: TextInput.Password
                placeholderText: qsTr("Password")
                onAccepted: lockScreen.attemptUnlock()
                onTextChanged: errorLabel.visible = false
            }
            Label {
                id: errorLabel
                visible: false
                text: qsTr("Incorrect password. Please try again.")
                font: Typography.labelSmall
                color: StateColors.error
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Unlock")
                highlighted: true
                Layout.fillWidth: true
                onClicked: lockScreen.attemptUnlock()
            }
        }
    }

    function attemptUnlock() {
        Log.info("ui", "Unlock attempt")
        if (AppController.unlock(passwordField.text)) {
            passwordField.text = ""
            errorLabel.visible = false
        } else {
            Log.warning("ui", "Unlock failed: incorrect password")
            errorLabel.visible = true
            passwordField.selectAll()
            passwordField.forceActiveFocus()
        }
    }
}
