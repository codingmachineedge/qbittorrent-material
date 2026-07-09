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
    \qmltype HttpSourcesTab
    \brief The properties "HTTP Sources" (web seeds) tab.

    A single-column \c DataTable over \c PropertiesController.webSeedModel (a
    \c WebSeedListModel) with a context menu (Add / Remove / Copy / Edit) and the
    Add/Edit web-seed dialogs. Every mutation dispatches to the model's own
    invokables (\c addWebSeed / \c removeWebSeeds / \c editWebSeed / \c urlAt).
    Double-click edits the focused URL.
*/
Item {
    id: root

    readonly property var webSeedModel: PropertiesController.webSeedModel

    // Minimal QML-only clipboard (hidden, off-screen TextEdit).
    TextEdit {
        id: clip
        visible: false
        width: 0
        height: 0
    }
    function _copy(text) {
        clip.text = text
        clip.selectAll()
        clip.copy()
        Log.info("ui", "HttpSourcesTab copied web seed URL(s) to clipboard")
    }

    function _selectedUrls() {
        const out = []
        const rows = table.selectedRows
        for (let i = 0; i < rows.length; ++i) {
            const u = webSeedModel.urlAt(rows[i])
            if (u.length > 0)
                out.push(u)
        }
        return out
    }

    DataTable {
        id: table
        anchors.fill: parent
        anchors.margins: Spacing.sm
        model: root.webSeedModel
        persistKey: "TorrentProperties/WebSeedList"

        columns: [
            { role: "url", title: qsTr("URL"), width: 480, align: Qt.AlignLeft, visible: true, resizable: true }
        ]

        onActivated: (row) => root._openEdit(row)
        onContextRequested: (row, pos) => {
            webSeedMenu.x = pos.x
            webSeedMenu.y = pos.y
            webSeedMenu.open()
        }
    }

    function _openEdit(row) {
        if (row < 0 || !webSeedModel)
            return
        editWebSeedDialog.row = row
        editWebSeedDialog.originalUrl = webSeedModel.urlAt(row)
        editWebSeedDialog.open()
    }

    // ---- Context menu ---------------------------------------------------------
    Menu {
        id: webSeedMenu
        Material.elevation: Spacing.elevationMenu

        MenuItem {
            text: qsTr("Add web seed...")
            enabled: PropertiesController.hasTorrent
            onTriggered: {
                Log.info("ui", "HttpSourcesTab add web seed")
                addWebSeedDialog.open()
            }
        }
        MenuItem {
            text: qsTr("Remove web seed")
            visible: table.selectedRows.length > 0
            height: visible ? implicitHeight : 0
            onTriggered: {
                const urls = root._selectedUrls()
                Log.info("ui", "HttpSourcesTab remove " + urls.length + " web seed(s)")
                if (root.webSeedModel && urls.length > 0)
                    root.webSeedModel.removeWebSeeds(urls)
            }
        }

        MenuSeparator { visible: table.selectedRows.length > 0 }

        MenuItem {
            text: qsTr("Copy web seed URL")
            visible: table.selectedRows.length > 0
            height: visible ? implicitHeight : 0
            onTriggered: root._copy(root._selectedUrls().join("\n"))
        }
        MenuItem {
            text: qsTr("Edit web seed URL...")
            visible: table.selectedRows.length === 1
            height: visible ? implicitHeight : 0
            onTriggered: root._openEdit(table.selectedRows[0])
        }
    }

    // ---- Dialogs --------------------------------------------------------------
    AddWebSeedDialog {
        id: addWebSeedDialog
        onWebSeedAdded: (url) => {
            Log.info("ui", "HttpSourcesTab adding web seed " + url)
            if (root.webSeedModel)
                root.webSeedModel.addWebSeed(url)
        }
    }

    EditWebSeedDialog {
        id: editWebSeedDialog
        onWebSeedEdited: (row, oldUrl, newUrl) => {
            Log.info("ui", "HttpSourcesTab edit web seed row " + row)
            if (root.webSeedModel)
                root.webSeedModel.editWebSeed(oldUrl, newUrl)
        }
    }

    Component.onCompleted: Log.debug("ui", "HttpSourcesTab loaded")
}
