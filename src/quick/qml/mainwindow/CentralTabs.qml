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
    CentralTabs — the central Material tab area: a \c TabBar over a \c StackLayout
    holding Transfers / Search / RSS / Execution Log / Workspace. Optional tabs
    are lazily created via \c Loader when their View action is enabled. Workspace
    is always available and owns a second, browser-style set of persistent pages.
*/
Item {
    id: central

    /// Main.qml root (shell).
    required property var shell
    /// Live torrent count for the "Transfers (N)" title.
    property int transfersCount: 0
    /// Which optional tabs are enabled.
    property bool searchEnabled: false
    property bool rssEnabled: false
    property bool logEnabled: false

    /// The selected tab index (0 Transfers, 1 Search, 2 RSS, 3 Log, 4 Workspace).
    property int currentIndex: 0

    // RSS unread badge (guarded — the RSS controller may not be present yet).
    readonly property int rssUnread: (typeof RSSController !== "undefined" && RSSController.unreadCount) || 0

    readonly property int visibleTabCount:
        2 + (searchEnabled ? 1 : 0) + (rssEnabled ? 1 : 0) + (logEnabled ? 1 : 0)

    // If the active tab gets disabled, fall back to Transfers.
    onSearchEnabledChanged: normalizeCurrent()
    onRssEnabledChanged: normalizeCurrent()
    onLogEnabledChanged: normalizeCurrent()

    function normalizeCurrent() {
        if ((currentIndex === 1 && !searchEnabled)
                || (currentIndex === 2 && !rssEnabled)
                || (currentIndex === 3 && !logEnabled)) {
            Log.debug("ui", "Active tab was disabled; reverting to Transfers")
            currentIndex = 0
        }
    }

    /// Switch to @p index (called by the Alt+1..4 shortcuts / View actions).
    function activateTab(index) {
        Log.debug("ui", "CentralTabs.activateTab(" + index + ")")
        central.currentIndex = index
    }

    /// Ctrl+F / Ctrl+E: bounce focus between the transfer filter and the
    /// Content-tab file filter (delegated to the Transfers tab when present).
    function toggleFilterFocus() {
        if (currentIndex === 0 && transfersLoader.item && transfersLoader.item.toggleFilterFocus)
            transfersLoader.item.toggleFilterFocus()
    }

    function workspaceItem() {
        central.currentIndex = 4
        return workspaceLoader.item
    }

    function newWorkspaceTab() {
        var item = workspaceItem()
        if (item) item.createTab()
    }

    function closeWorkspaceTab() {
        var item = workspaceItem()
        if (item) item.closeCurrentTab()
    }

    function customizeWorkspaceTab() {
        var item = workspaceItem()
        if (item) item.customizeCurrentTab()
    }

    function renameWorkspaceApplication() {
        var item = workspaceItem()
        if (item) item.renameApplication()
    }

    function importWorkspace() {
        var item = workspaceItem()
        if (item) item.importWorkspace()
    }

    function exportWorkspace() {
        var item = workspaceItem()
        if (item) item.exportWorkspace()
    }

    function importWorkspaceRepository() {
        var item = workspaceItem()
        if (item) item.importGitRepository()
    }

    function exportWorkspaceRepository() {
        var item = workspaceItem()
        if (item) item.exportGitRepository()
    }

    function openWorkspaceRepository() {
        var item = workspaceItem()
        if (item) item.openGitRepository()
    }

    function syncWorkspace() {
        var item = workspaceItem()
        if (item) item.syncWorkspace()
    }

    // A tab button with a leading MDIcon and an optional trailing count.
    component AppTabButton: TabButton {
        id: tabBtn
        property string glyph: ""
        property string label: ""
        property int count: -1
        implicitHeight: 44
        contentItem: RowLayout {
            spacing: Spacing.sm
            Item { Layout.fillWidth: true }
            MDIcon {
                icon: tabBtn.glyph
                size: 18
                color: tabBtn.checked ? Theme.color("primary") : Theme.color("onSurfaceVariant")
            }
            Label {
                text: tabBtn.count >= 0 ? qsTr("%1 (%2)").arg(tabBtn.label).arg(tabBtn.count) : tabBtn.label
                font: Typography.titleSmall
                color: tabBtn.checked ? Theme.color("primary") : Theme.color("onSurfaceVariant")
            }
            Item { Layout.fillWidth: true }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        TabBar {
            id: tabBar
            Layout.fillWidth: true
            Material.elevation: 0
            visible: central.visibleTabCount > 1
            // Keep the TabBar and the StackLayout in sync with central.currentIndex.
            currentIndex: central.currentIndex
            onCurrentIndexChanged: {
                if (currentIndex !== central.currentIndex) {
                    Log.debug("ui", "Tab changed -> " + currentIndex)
                    central.currentIndex = currentIndex
                }
            }

            AppTabButton {
                glyph: Icons.download
                label: qsTr("Transfers")
                count: central.transfersCount
            }
            AppTabButton {
                glyph: Icons.search
                label: qsTr("Search")
                visible: central.searchEnabled
                width: visible ? implicitWidth : 0
            }
            AppTabButton {
                glyph: Icons.rss_feed
                label: qsTr("RSS")
                count: central.rssUnread
                visible: central.rssEnabled
                width: visible ? implicitWidth : 0
            }
            AppTabButton {
                glyph: Icons.article
                label: qsTr("Execution Log")
                visible: central.logEnabled
                width: visible ? implicitWidth : 0
            }
            AppTabButton {
                objectName: "workspaceTopLevelTab"
                Accessible.name: qsTr("Workspace")
                glyph: Icons.edit
                label: qsTr("Workspace")
            }
        }

        StackLayout {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: central.currentIndex

            // 0 — Transfers (always present)
            Loader {
                id: transfersLoader
                active: true
                sourceComponent: TransfersTab {
                    shell: central.shell
                    sidebarVisible: central.shell.sidebarVisible
                }
            }

            // 1 — Search (lazy)
            Loader {
                id: searchLoader
                active: central.searchEnabled
                sourceComponent: SearchTab {}
                onLoaded: Log.debug("ui", "Search tab instantiated")
            }

            // 2 — RSS (lazy)
            Loader {
                id: rssLoader
                active: central.rssEnabled
                sourceComponent: RSSTab {}
                onLoaded: Log.debug("ui", "RSS tab instantiated")
            }

            // 3 — Execution Log (lazy)
            Loader {
                id: logLoader
                active: central.logEnabled
                sourceComponent: ExecutionLogTab {}
                onLoaded: Log.debug("ui", "Execution Log tab instantiated")
            }

            // 4 — Persistent browser-style workspace
            Loader {
                id: workspaceLoader
                active: true
                sourceComponent: WorkspaceView {}
                onLoaded: Log.debug("ui", "Workspace tab instantiated")
            }
        }
    }

    Component.onCompleted: Log.debug("ui", "CentralTabs ready; visibleTabCount=" + visibleTabCount)
}
