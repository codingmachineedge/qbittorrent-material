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
    \qmltype CategoryDialog
    \brief Material dialog to add or edit a torrent category.

    Mirrors the legacy \c TorrentCategoryDialog. Handles both entry points:
    \list
      \li \c "add" — editable name, seeded from \l parentCategory for a new
          subcategory; validates the name and rejects duplicates.
      \li \c "edit" — name is read-only; controls are pre-filled from
          \l initialOptions.
    \endlist

    Placeholders for the save/incomplete paths are read live from the
    \c Session singleton. On OK the dialog emits \l categoryAccepted with the
    category name and a plain options object; the invoking view persists it via
    \c Session.addCategory() / \c Session.setCategoryOptions().

    \l initialOptions keys (edit mode, all optional): \c savePath (string),
    \c useDownloadPath (0=Default,1=Yes,2=No), \c downloadPath (string),
    \c ratioLimit (real), \c seedingTimeLimit (int),
    \c inactiveSeedingTimeLimit (int), \c shareLimitAction (int).
*/
Dialog {
    id: root

    // --- Public API --------------------------------------------------------

    /*! Either \c "add" (default) or \c "edit". */
    property string mode: "add"
    /*! Initial / edited category name. */
    property string categoryName: ""
    /*! Parent category for a new subcategory (add mode). */
    property string parentCategory: ""
    /*! Pre-fill options for edit mode (see type docs). */
    property var initialOptions: ({})

    /*! Emitted on OK with the category name and a plain options object. */
    signal categoryAccepted(string name, var options)

    readonly property bool editMode: mode === "edit"

    // --- Internal state ----------------------------------------------------

    property string _nameError: ""
    property string _savePathPlaceholder: ""
    property string _downloadPathPlaceholder: ""

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

    function _validName(name) {
        // Mirror BitTorrent::Session::isValidCategoryName().
        if (name.length === 0)
            return true; // empty is "not yet valid" but not an error message
        if (name.indexOf("\\") >= 0)
            return false;
        if (name.startsWith("/") || name.endsWith("/"))
            return false;
        if (name.indexOf("//") >= 0)
            return false;
        return true;
    }

    function _revalidate() {
        const name = nameField.text;
        if (name.length === 0) {
            root._nameError = "";
        } else if (!root._validName(name)) {
            root._nameError = qsTr("Category name cannot contain '\\'.\nCategory name cannot start/end with '/'.\nCategory name cannot contain '//' sequence.");
        } else if (!root.editMode && (root._toArray(Session.categories()).indexOf(name) >= 0)) {
            root._nameError = qsTr("Category with the given name already exists.\nPlease choose a different name and try again.");
        } else {
            root._nameError = "";
        }

        const okButton = root.standardButton(Dialog.Ok);
        if (okButton)
            okButton.enabled = (name.length > 0) && (root._nameError.length === 0);

        root._refreshPlaceholders();
    }

    function _refreshPlaceholders() {
        const name = nameField.text;
        try {
            root._savePathPlaceholder = String(Session.categorySavePath(name));
        } catch (err) {
            root._savePathPlaceholder = "";
        }
        try {
            root._downloadPathPlaceholder = String(Session.categoryDownloadPath(name));
        } catch (err) {
            root._downloadPathPlaceholder = "";
        }
    }

    function _buildOptions() {
        let useDownloadPath = 0;
        if (useDownloadCombo.value === TriStateComboField.Yes)
            useDownloadPath = 1;
        else if (useDownloadCombo.value === TriStateComboField.No)
            useDownloadPath = 2;

        return {
            "savePath": savePathField.path,
            "useDownloadPath": useDownloadPath,
            "downloadPath": downloadPathField.path,
            "ratioLimit": shareLimitsForm.ratioLimit,
            "seedingTimeLimit": shareLimitsForm.seedingTimeLimit,
            "inactiveSeedingTimeLimit": shareLimitsForm.inactiveSeedingTimeLimit,
            "shareLimitMode": shareLimitsForm.mode,
            "shareLimitAction": shareLimitsForm.action
        };
    }

    // --- Dialog shell ------------------------------------------------------

    title: qsTr("Torrent Category Properties")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(520, (parent ? parent.width : 520) * 0.9)
    height: Math.min(implicitHeight, (parent ? parent.height : 640) * 0.9)
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
                icon: Icons.category
                size: 24
                color: Theme.color("primary")
            }
            Label {
                Layout.fillWidth: true
                text: root.editMode ? qsTr("Edit Category") : qsTr("New Category")
                font: Typography.headlineSmall
                color: Theme.color("onSurface")
                elide: Label.ElideRight
            }
        }
    }

    contentItem: ScrollView {
        id: scroll
        clip: true
        ScrollBar.horizontal.policy: ScrollBar.AlwaysOff

        ColumnLayout {
            width: scroll.availableWidth
            spacing: Spacing.md

            LabeledField {
                Layout.fillWidth: true
                label: qsTr("Name:")

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.xs

                    TextField {
                        id: nameField
                        Layout.fillWidth: true
                        enabled: !root.editMode
                        placeholderText: qsTr("Category name")
                        onTextEdited: {
                            Log.debug("ui", "CategoryDialog: name edited -> " + text);
                            root._revalidate();
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: root._nameError.length > 0
                        text: root._nameError
                        font: Typography.labelSmall
                        color: Theme.color("error")
                        wrapMode: Text.WordWrap
                    }
                }
            }

            LabeledField {
                Layout.fillWidth: true
                label: qsTr("Save path:")
                PathField {
                    id: savePathField
                    Layout.fillWidth: true
                    pickFolder: true
                    title: qsTr("Choose save path")
                    placeholder: root._savePathPlaceholder
                    onAccepted: root._refreshPlaceholders()
                }
            }

            MaterialCard {
                Layout.fillWidth: true
                title: qsTr("Save path for incomplete torrents")

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.sm

                    LabeledField {
                        Layout.fillWidth: true
                        label: qsTr("Use another path for incomplete torrents:")
                        TriStateComboField {
                            id: useDownloadCombo
                            Layout.fillWidth: true
                            value: TriStateComboField.Default
                            onValueChanged: {
                                Log.debug("ui", "CategoryDialog: useDownloadPath -> " + value);
                                root._refreshPlaceholders();
                            }
                        }
                    }

                    LabeledField {
                        Layout.fillWidth: true
                        label: qsTr("Path:")
                        enabled: useDownloadCombo.value === TriStateComboField.Yes
                        PathField {
                            id: downloadPathField
                            Layout.fillWidth: true
                            pickFolder: true
                            title: qsTr("Choose download path")
                            placeholder: root._downloadPathPlaceholder
                        }
                    }
                }
            }

            MaterialCard {
                Layout.fillWidth: true
                title: qsTr("Torrent share limits")

                SharedShareLimitsForm {
                    id: shareLimitsForm
                    Layout.fillWidth: true
                    onChanged: Log.debug("ui", "CategoryDialog: share limits changed")
                }
            }
        }
    }

    // --- Behaviour ---------------------------------------------------------

    onAboutToShow: {
        nameField.text = root.categoryName;

        const opts = root.initialOptions || ({});
        savePathField.path = (opts.savePath !== undefined) ? opts.savePath : "";
        const useDownloadPath = (opts.useDownloadPath !== undefined) ? opts.useDownloadPath : 0;
        useDownloadCombo.value = (useDownloadPath === 1) ? TriStateComboField.Yes
                : ((useDownloadPath === 2) ? TriStateComboField.No : TriStateComboField.Default);
        downloadPathField.path = (opts.downloadPath !== undefined) ? opts.downloadPath : "";

        if (opts.ratioLimit !== undefined)
            shareLimitsForm.ratioLimit = opts.ratioLimit;
        if (opts.seedingTimeLimit !== undefined)
            shareLimitsForm.seedingTimeLimit = opts.seedingTimeLimit;
        if (opts.inactiveSeedingTimeLimit !== undefined)
            shareLimitsForm.inactiveSeedingTimeLimit = opts.inactiveSeedingTimeLimit;
        if (opts.shareLimitMode !== undefined)
            shareLimitsForm.mode = opts.shareLimitMode;
        if (opts.shareLimitAction !== undefined)
            shareLimitsForm.action = opts.shareLimitAction;

        root._revalidate();

        // Select the subcategory portion after the last '/'.
        const slash = root.categoryName.lastIndexOf("/");
        if (!root.editMode && (slash >= 0))
            nameField.select(slash + 1, root.categoryName.length);

        Log.info("ui", "CategoryDialog opened (" + root.mode + ") for '" + root.categoryName + "'");
    }

    onAccepted: {
        const name = nameField.text;
        const options = root._buildOptions();
        Log.info("ui", "CategoryDialog accepted: '" + name + "'");
        root.categoryAccepted(name, options);
    }

    onRejected: Log.debug("ui", "CategoryDialog cancelled")
}
