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

/*!
    \qmltype PathField
    \brief A text field plus a trailing browse button that opens an OS
           folder / file picker (the only permitted native dialog).

    \l path is two-way bindable; \l pickFolder chooses between a folder and a
    file dialog. \l accepted() is emitted when the user commits the text field
    with Return.
*/
RowLayout {
    id: root

    /*! Two-way: the selected filesystem path. */
    property string path: ""

    /*! True → pick a directory; false → pick a file. */
    property bool pickFolder: true

    /*! Placeholder shown when the field is empty. */
    property string placeholder: ""

    /*! Already-translated caption for the OS picker dialog. */
    property string title: qsTr("Choose a location")

    /*! Emitted when the text field is accepted (Return). */
    signal accepted()

    spacing: Spacing.xs

    // Convert a file:// URL from the OS dialog into a plain local path,
    // handling both POSIX ("file:///home/x") and Windows ("file:///C:/x").
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

    TextField {
        id: field
        Layout.fillWidth: true
        text: root.path
        placeholderText: root.placeholder
        selectByMouse: true
        onTextEdited: root.path = text
        onAccepted: {
            root.path = text
            Log.debug("ui", "PathField accepted: " + root.path)
            root.accepted()
        }
    }

    IconButton {
        icon: Icons.folder_open
        size: 20
        tooltip: qsTr("Browse")
        onClicked: {
            Log.debug("ui", "PathField browse (" + (root.pickFolder ? "folder" : "file") + ")")
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
            Log.info("ui", "PathField folder chosen: " + root.path)
            root.accepted()
        }
    }

    Platform.FileDialog {
        id: fileDialog
        title: root.title
        folder: root._toUrl(root.path)
        onAccepted: {
            root.path = root._fromUrl(file)
            Log.info("ui", "PathField file chosen: " + root.path)
            root.accepted()
        }
    }
}
