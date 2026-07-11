/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026  qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import qBittorrent

/*!
    Main workspace host. The old top TabBar has been replaced by the supplied
    design system's persistent 248 px grouped navigation. Workspace loaders and
    their controller lifetimes stay exactly as before.
*/
Item {
    id: central

    required property var shell
    property int transfersCount: 0
    property bool searchEnabled: false
    property bool rssEnabled: false
    property bool logEnabled: false
    property int currentIndex: 0

    readonly property int rssUnread:
        (typeof RSSController !== "undefined" && RSSController.unreadCount) || 0

    onSearchEnabledChanged: normalizeCurrent()
    onRssEnabledChanged: normalizeCurrent()
    onLogEnabledChanged: normalizeCurrent()

    function normalizeCurrent() {
        if ((currentIndex === 1 && !searchEnabled)
                || (currentIndex === 2 && !rssEnabled)
                || (currentIndex === 3 && !logEnabled)) {
            currentIndex = 0
        }
    }

    function activateTab(index) {
        if (index < 0 || index > 4)
            return
        Log.debug("ui", "Workspace destination -> " + index)
        central.currentIndex = index
    }

    function requestDestination(index) {
        if (index >= 1 && index <= 3)
            central.shell.switchToTab(index)
        else
            central.activateTab(index)
    }

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

    RowLayout {
        anchors.fill: parent
        spacing: 0

        AppNavigationSidebar {
            Layout.preferredWidth: Spacing.navigationWidth
            Layout.fillHeight: true
            shell: central.shell
            currentIndex: central.currentIndex
            transfersCount: central.transfersCount
            rssUnread: central.rssUnread
            transferProxy: transfersLoader.item ? transfersLoader.item.proxy : null
            onNavigateRequested: (index) => central.requestDestination(index)
            onOptionsRequested: central.shell.showOptions()
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            color: Theme.color("background")

            StackLayout {
                id: stack
                anchors.fill: parent
                currentIndex: central.currentIndex

                Loader {
                    id: transfersLoader
                    active: true
                    sourceComponent: TransfersTab {
                        shell: central.shell
                        sidebarVisible: false
                    }
                }

                Loader {
                    id: searchLoader
                    active: central.searchEnabled
                    sourceComponent: SearchTab {}
                    onLoaded: Log.debug("ui", "Search workspace instantiated")
                }

                Loader {
                    id: rssLoader
                    active: central.rssEnabled
                    sourceComponent: RSSTab {}
                    onLoaded: Log.debug("ui", "RSS workspace instantiated")
                }

                Loader {
                    id: logLoader
                    active: central.logEnabled
                    sourceComponent: ExecutionLogTab {}
                    onLoaded: Log.debug("ui", "Execution log workspace instantiated")
                }

                Loader {
                    id: workspaceLoader
                    active: true
                    sourceComponent: WorkspaceView {}
                    onLoaded: Log.debug("ui", "Personal workspace instantiated")
                }
            }
        }
    }

    Component.onCompleted: Log.debug("ui", "Design-system workspace shell ready")
}
