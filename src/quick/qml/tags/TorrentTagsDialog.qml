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
    \qmltype TorrentTagsDialog
    \brief Material dialog to assign tags to a torrent (or a selection).

    Mirrors the legacy \c TorrentTagsDialog: a wrapping flow of checkboxes, one
    per tag in \c {initialTags ∪ Session.tags()}, checked when the tag is in
    \l initialTags. A "New tag…" affordance opens the embedded \l AddTagDialog to
    register a brand-new (checked) tag.

    On OK the dialog emits \l tagsAccepted with the list of checked tag strings;
    the invoking view applies them to the engine (add/remove per torrent).
*/
Dialog {
    id: root

    // --- Public API --------------------------------------------------------

    /*! Tag strings that start out checked. */
    property var initialTags: []
    /*! Optional override of the known-tag universe (defaults to Session.tags()). */
    property var knownTags: []

    /*! Emitted on OK with the list of selected tag strings. */
    signal tagsAccepted(var tags)

    // --- Internal state ----------------------------------------------------

    // Array of { "tag": string, "checked": bool }; mutated in place on toggle.
    property var _tagItems: []

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

    function _knownUniverse() {
        let known = root._toArray(root.knownTags);
        if (known.length === 0)
            known = root._toArray(Session.tags());
        return known;
    }

    function _rebuild() {
        const initial = root._toArray(root.initialTags);
        const known = root._knownUniverse();
        const seen = {};
        const items = [];
        const pushTag = function (tag, checked) {
            if ((tag === undefined) || (tag === null) || (String(tag).length === 0))
                return;
            const key = String(tag);
            if (seen[key] !== undefined) {
                if (checked)
                    items[seen[key]].checked = true;
                return;
            }
            seen[key] = items.length;
            items.push({ "tag": key, "checked": checked });
        };
        for (let i = 0; i < initial.length; ++i)
            pushTag(initial[i], true);
        for (let j = 0; j < known.length; ++j)
            pushTag(known[j], initial.indexOf(known[j]) >= 0);
        root._tagItems = items;
    }

    function _hasTag(tag) {
        const key = String(tag);
        for (let i = 0; i < root._tagItems.length; ++i) {
            if (root._tagItems[i].tag === key)
                return true;
        }
        return false;
    }

    function _appendTag(tag) {
        const items = root._tagItems.slice();
        items.push({ "tag": String(tag), "checked": true });
        root._tagItems = items;
        // Register globally so the tag persists across the session.
        try {
            Session.addTag(String(tag));
        } catch (err) {
            Log.warning("ui", "TorrentTagsDialog: could not register tag '" + tag + "'");
        }
        Log.info("ui", "TorrentTagsDialog: added new tag '" + tag + "'");
    }

    function _selectedTags() {
        const selected = [];
        for (let i = 0; i < root._tagItems.length; ++i) {
            if (root._tagItems[i].checked)
                selected.push(root._tagItems[i].tag);
        }
        return selected;
    }

    // --- Dialog shell ------------------------------------------------------

    title: qsTr("Torrent Tags")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(500, (parent ? parent.width : 500) * 0.9)
    height: Math.min(implicitHeight, (parent ? parent.height : 520) * 0.9)
    padding: Spacing.lg
    standardButtons: Dialog.Ok | Dialog.Cancel
    Material.elevation: 24
    Material.background: Theme.color("surface")

    header: Pane {
        Material.elevation: 0
        padding: Spacing.lg
        RowLayout {
            width: parent.width
            spacing: Spacing.sm
            MDIcon {
                icon: Icons.sell
                size: 24
                color: Theme.color("primary")
            }
            Label {
                Layout.fillWidth: true
                text: qsTr("Torrent Tags")
                font: Typography.headlineSmall
                color: Theme.color("onSurface")
                elide: Label.ElideRight
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: Spacing.md

        ScrollView {
            id: scroll
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.minimumHeight: 120
            clip: true
            ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

            Flow {
                width: scroll.availableWidth
                spacing: Spacing.md

                Repeater {
                    model: root._tagItems
                    delegate: CheckBox {
                        required property var modelData
                        text: modelData.tag
                        checked: modelData.checked
                        onToggled: {
                            modelData.checked = checked;
                            Log.debug("ui", "TorrentTagsDialog: tag '" + modelData.tag + "' -> " + checked);
                        }
                    }
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: root._tagItems.length === 0
            text: qsTr("No tags yet. Use the button below to create one.")
            font: Typography.bodyMedium
            color: Theme.color("onSurfaceVariant")
            wrapMode: Text.WordWrap
        }

        Button {
            Layout.alignment: Qt.AlignLeft
            flat: true
            text: qsTr("New tag…")
            onClicked: {
                Log.info("ui", "TorrentTagsDialog: New tag clicked");
                addTagDialog.existingTags = root._tagItems.map(function (item) { return item.tag; });
                addTagDialog.open();
            }

            contentItem: RowLayout {
                spacing: Spacing.xs
                MDIcon {
                    icon: Icons.sell
                    size: 18
                    color: Theme.color("primary")
                }
                Label {
                    text: qsTr("New tag…")
                    font: Typography.labelLarge
                    color: Theme.color("primary")
                }
            }
        }
    }

    AddTagDialog {
        id: addTagDialog
        onTagCreated: (tag) => root._appendTag(tag)
    }

    // --- Behaviour ---------------------------------------------------------

    onAboutToShow: {
        root._rebuild();
        Log.info("ui", "TorrentTagsDialog opened with " + root._tagItems.length + " tag(s)");
    }

    onAccepted: {
        const selected = root._selectedTags();
        Log.info("ui", "TorrentTagsDialog accepted: " + selected.length + " tag(s) selected");
        root.tagsAccepted(selected);
    }

    onRejected: Log.debug("ui", "TorrentTagsDialog cancelled")
}
