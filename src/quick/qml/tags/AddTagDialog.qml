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
    \qmltype AddTagDialog
    \brief Inline one-line input dialog to create a new tag.

    Mirrors the legacy \c TorrentTagsDialog::addNewTag() prompt. Validates the
    entered name inline (a tag cannot be empty, cannot contain a comma, and must
    not already exist) and, on OK, emits \l tagCreated with the trimmed tag
    string. Reused by \l TorrentTagsDialog and any other tag-entry surface.
*/
Dialog {
    id: root

    // --- Public API --------------------------------------------------------

    /*! Tag strings that already exist (rejected as duplicates). */
    property var existingTags: []

    /*! Emitted on OK with the accepted, trimmed tag string. */
    signal tagCreated(string tag)

    // --- Internal state ----------------------------------------------------

    property string _error: ""

    function _toArray(value) {
        if ((value === undefined) || (value === null))
            return [];
        if (Array.isArray(value))
            return value;
        const out = [];
        try {
            for (let i = 0; i < value.length; ++i)
                out.push(value[i]);
        } catch (err) {
            // Not iterable — ignore.
        }
        return out;
    }

    function _revalidate() {
        const tag = tagField.text.trim();
        if (tag.length === 0) {
            root._error = "";
        } else if (tag.indexOf(",") >= 0) {
            root._error = qsTr("Tag name '%1' is invalid.").arg(tag);
        } else if (root._toArray(root.existingTags).indexOf(tag) >= 0) {
            root._error = qsTr("Tag name already exists.");
        } else {
            root._error = "";
        }

        const okButton = root.standardButton(Dialog.Ok);
        if (okButton)
            okButton.enabled = (tag.length > 0) && (root._error.length === 0);
    }

    // --- Dialog shell ------------------------------------------------------

    title: qsTr("Add tag")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(420, (parent ? parent.width : 420) * 0.9)
    padding: Spacing.lg
    standardButtons: Dialog.Ok | Dialog.Cancel
    Material.elevation: 24
    Material.background: Theme.color("surface")

    contentItem: ColumnLayout {
        spacing: Spacing.sm

        Label {
            Layout.fillWidth: true
            text: qsTr("Tag:")
            font: Typography.bodyMedium
            color: Theme.color("onSurfaceVariant")
        }

        TextField {
            id: tagField
            Layout.fillWidth: true
            focus: true
            placeholderText: qsTr("Tag name")
            onTextEdited: root._revalidate()
            onAccepted: {
                const okButton = root.standardButton(Dialog.Ok);
                if (okButton && okButton.enabled)
                    root.accept();
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root._error.length > 0
            text: root._error
            font: Typography.labelSmall
            color: Theme.color("error")
            wrapMode: Text.WordWrap
        }
    }

    // --- Behaviour ---------------------------------------------------------

    onAboutToShow: {
        tagField.clear();
        root._error = "";
        const okButton = root.standardButton(Dialog.Ok);
        if (okButton)
            okButton.enabled = false;
        Log.debug("ui", "AddTagDialog opened");
    }

    onOpened: tagField.forceActiveFocus()

    onAccepted: {
        const tag = tagField.text.trim();
        Log.info("ui", "AddTagDialog accepted: '" + tag + "'");
        root.tagCreated(tag);
    }

    onRejected: Log.debug("ui", "AddTagDialog cancelled")
}
