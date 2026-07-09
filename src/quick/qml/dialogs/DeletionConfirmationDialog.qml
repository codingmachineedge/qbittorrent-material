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
    \qmltype DeletionConfirmationDialog
    \brief Material rebuild of the legacy \c DeletionConfirmationDialog ("Remove
           torrent(s)").

    Warns before removing torrents and offers an "Also remove the content files"
    checkbox plus a "remember choice" affordance that persists the default to the
    \c Preferences key \c Preferences/General/DeleteTorrentsFilesAsDefault. Set
    \l torrentsCount / \l torrentName (and optionally \l defaultDeleteFiles)
    before \c open(); on accept it emits \l confirmed() with the final choice.
*/
Dialog {
    id: root

    /*! Number of torrents being removed (drives singular/plural wording). */
    property int torrentsCount: 1

    /*! Name of the (first) torrent — shown when removing exactly one. */
    property string torrentName: ""

    /*! Initial state of the "also delete content" checkbox. */
    property bool defaultDeleteFiles: false

    /*! The Preferences key backing the remembered default. */
    readonly property string _rememberKey: "Preferences/General/DeleteTorrentsFilesAsDefault"

    /*! Emitted on accept with the final "delete content files" choice. */
    signal confirmed(bool deleteFiles)

    title: qsTr("Remove torrent(s)")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(480, (parent ? parent.width : 480) * 0.9)
    padding: Spacing.lg

    Material.elevation: 24
    Material.roundedScale: Material.MediumScale

    background: Rectangle {
        radius: Spacing.radiusDialog
        color: Theme.color("surface")
    }

    onOpened: {
        Log.debug("ui", "DeletionConfirmationDialog opened for " + torrentsCount + " torrent(s)")
        removeContent.checked = defaultDeleteFiles
                || (Preferences.value(_rememberKey, false) === true)
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
                icon: Icons.warning
                size: 28
                color: Theme.color("warning")
                Layout.alignment: Qt.AlignTop
            }

            Label {
                text: root.torrentsCount === 1
                      ? qsTr("Are you sure you want to remove '%1' from the transfer list?").arg(root.torrentName)
                      : qsTr("Are you sure you want to remove these %1 torrents from the transfer list?").arg(root.torrentsCount)
                font: Typography.bodyMedium
                color: Theme.color("onSurfaceVariant")
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        RowLayout {
            spacing: Spacing.sm
            Layout.fillWidth: true

            CheckBox {
                id: removeContent
                text: qsTr("Also remove the content files")
                font: Typography.bodyMedium
                Layout.fillWidth: true
                onToggled: Log.debug("ui", "DeletionConfirmationDialog remove-content -> " + checked)
            }

            IconButton {
                id: rememberButton
                symbol: Icons.lock
                tooltip: qsTr("Remember choice")
                // Enabled only when the choice differs from the stored default.
                enabled: removeContent.checked !== (Preferences.value(root._rememberKey, false) === true)
                onClicked: {
                    Log.info("ui", "DeletionConfirmationDialog remembering choice: " + removeContent.checked)
                    Preferences.setValue(root._rememberKey, removeContent.checked)
                }
            }
        }
    }

    footer: DialogButtonBox {
        spacing: Spacing.sm
        padding: Spacing.lg
        topPadding: Spacing.sm

        Button {
            text: qsTr("Cancel")
            flat: true
            focus: true
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: root.reject()
        }

        Button {
            text: removeContent.checked ? qsTr("Remove torrent and content") : qsTr("Remove torrent")
            highlighted: true
            Material.accent: Theme.color("error")
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: root.accept()
        }
    }

    onAccepted: {
        Log.info("ui", "DeletionConfirmationDialog accepted; deleteFiles=" + removeContent.checked)
        root.confirmed(removeContent.checked)
    }

    onRejected: Log.debug("ui", "DeletionConfirmationDialog rejected")
}
