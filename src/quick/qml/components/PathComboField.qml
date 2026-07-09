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
import Qt.labs.platform as Platform
import qBittorrent

/*!
    \qmltype PathComboField
    \brief Like \l PathField but the editor is an editable ComboBox of recent /
           known paths, plus a trailing OS picker button.

    \l path is two-way bindable and mirrors the combo's edit text; \l model
    supplies the history entries.
*/
RowLayout {
    id: root

    /*! Two-way: the current path (== combo edit text). */
    property string path: ""

    /*! History model (e.g. a QStringList of recent paths). */
    property alias model: combo.model

    /*! True → folder picker; false → file picker. */
    property bool pickFolder: true

    /*! Placeholder shown when empty. */
    property string placeholder: ""

    /*! Already-translated OS picker caption. */
    property string title: qsTr("Choose a location")

    spacing: Spacing.xs

    function _fromUrl(u) {
        var s = decodeURIComponent(("" + u).replace(/^file:\/\//, ""))
        if (/^\/[A-Za-z]:/.test(s))
            s = s.substring(1)
        return s
    }

    function _toUrl(p) {
        if (!p || p.length === 0)
            return ""
        return "file:///" + p.replace(/\\/g, "/").replace(/^\//, "")
    }

    ComboBox {
        id: combo
        Layout.fillWidth: true
        editable: true
        editText: root.path
        // Guard against binding loops: only push when the two differ.
        onEditTextChanged: if (editText !== root.path) root.path = editText
        onActivated: {
            root.path = currentText
            Log.debug("ui", "PathComboField selected history entry: " + root.path)
        }
    }

    IconButton {
        icon: Icons.folder_open
        size: 20
        tooltip: qsTr("Browse")
        onClicked: {
            Log.debug("ui", "PathComboField browse (" + (root.pickFolder ? "folder" : "file") + ")")
            if (root.pickFolder)
                folderDialog.open()
            else
                fileDialog.open()
        }
    }

    Platform.FolderDialog {
        id: folderDialog
        title: root.title
        folder: root._toUrl(root.path)
        onAccepted: {
            root.path = root._fromUrl(folder)
            Log.info("ui", "PathComboField folder chosen: " + root.path)
        }
    }

    Platform.FileDialog {
        id: fileDialog
        title: root.title
        folder: root._toUrl(root.path)
        onAccepted: {
            root.path = root._fromUrl(file)
            Log.info("ui", "PathComboField file chosen: " + root.path)
        }
    }
}
