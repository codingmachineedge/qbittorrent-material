/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs as Dialogs
import qBittorrent

Item {
    id: root
    objectName: "workspaceView"
    Accessible.name: qsTr("Custom workspace")

    function createTab() {
        WorkspaceManager.createTab(qsTr("New tab"))
    }

    function closeCurrentTab() {
        if (WorkspaceManager.activeIndex >= 0)
            WorkspaceManager.closeTab(WorkspaceManager.activeIndex)
    }

    function customizeCurrentTab() {
        if (WorkspaceManager.activeIndex >= 0)
            tabSettings.openForIndex(WorkspaceManager.activeIndex)
    }

    function renameApplication() {
        renameAppDialog.text = WorkspaceManager.appDisplayName
        renameAppDialog.open()
    }

    function importWorkspace() { root.showImportWarning("json") }
    function exportWorkspace() { exportJsonDialog.open() }
    function importGitRepository() { root.showImportWarning("repository") }
    function exportGitRepository() { exportRepositoryDialog.open() }
    function openGitRepository() { WorkspaceManager.openRepository() }
    function syncWorkspace() { WorkspaceManager.syncNow() }

    function showImportWarning(kind) {
        importWarning.kind = kind
        importWarning.open()
    }

    Connections {
        target: WorkspaceManager
        function onActiveIndexChanged() {
            if (WorkspaceManager.activeIndex >= 0)
                tabList.positionViewAtIndex(WorkspaceManager.activeIndex, ListView.Contain)
        }
        function onOperationFinished(success, message, location) {
            workspaceSnackbar.show(message,
                success && location && location.toString().length > 0 ? qsTr("Open") : "",
                success && location && location.toString().length > 0
                    ? function() { Qt.openUrlExternally(location) } : null)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: workspaceHeader.implicitHeight + Spacing.md * 2
            color: Theme.color("surfaceVariant")
            border.width: 0

            RowLayout {
                id: workspaceHeader
                anchors.fill: parent
                anchors.leftMargin: Spacing.lg
                anchors.rightMargin: Spacing.sm
                anchors.topMargin: Spacing.md
                anchors.bottomMargin: Spacing.md
                spacing: Spacing.md

                Image {
                    source: "qrc:/branding/logo-mark.svg"
                    sourceSize.width: 34
                    sourceSize.height: 34
                    Layout.preferredWidth: 34
                    Layout.preferredHeight: 34
                    fillMode: Image.PreserveAspectFit
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 0
                    Label {
                        text: WorkspaceManager.appDisplayName
                        textFormat: Text.PlainText
                        elide: Text.ElideRight
                        font: Typography.titleMedium
                        color: Theme.color("onSurface")
                        Layout.fillWidth: true
                    }
                    Label {
                        objectName: "workspaceSyncStatus"
                        text: WorkspaceManager.repositoryStatus
                        textFormat: Text.PlainText
                        elide: Text.ElideMiddle
                        font: Typography.bodySmall
                        color: WorkspaceManager.dirty
                            ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                        Layout.fillWidth: true
                    }
                }

                Button {
                    id: renameButton
                    objectName: "workspaceRenameAppButton"
                    visible: root.width >= 720
                    flat: true
                    text: qsTr("Rename app")
                    icon.source: ""
                    onClicked: root.renameApplication()
                }

                IconButton {
                    objectName: "workspaceSyncButton"
                    symbol: Icons.refresh
                    tooltip: qsTr("Commit pending changes to local Git")
                    onClicked: WorkspaceManager.syncNow()
                }

                IconButton {
                    objectName: "workspaceOpenRepositoryButton"
                    symbol: Icons.folder
                    tooltip: qsTr("Open local Git repository")
                    onClicked: WorkspaceManager.openRepository()
                }

                IconButton {
                    objectName: "workspacePortabilityButton"
                    symbol: Icons.more_vert
                    tooltip: qsTr("Import and export")
                    onClicked: portabilityMenu.popup()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 48
            color: Theme.color("surface")
            border.width: 0

            RowLayout {
                anchors.fill: parent
                spacing: 0

                ListView {
                    id: tabList
                    objectName: "workspaceTabBar"
                    Accessible.name: qsTr("Workspace tabs")
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    orientation: ListView.Horizontal
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true
                    spacing: 2
                    model: WorkspaceManager
                    currentIndex: WorkspaceManager.activeIndex

                    delegate: TabButton {
                        id: tabButton
                        required property int index
                        required property string tabId
                        required property string name
                        width: Math.max(132, Math.min(230, implicitWidth))
                        height: ListView.view.height
                        leftPadding: Spacing.sm
                        rightPadding: Spacing.xs
                        checked: index === WorkspaceManager.activeIndex
                        objectName: "workspaceTab_" + tabId
                        Accessible.name: qsTr("Workspace tab %1").arg(name)
                        onClicked: WorkspaceManager.activeIndex = index

                        contentItem: RowLayout {
                            spacing: Spacing.xs
                            MDIcon {
                                icon: Icons.article
                                size: 16
                                color: tabButton.checked
                                    ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                            }
                            Label {
                                text: tabButton.name
                                textFormat: Text.PlainText
                                elide: Text.ElideRight
                                font: Typography.titleSmall
                                color: tabButton.checked
                                    ? Theme.color("primary") : Theme.color("onSurfaceVariant")
                                Layout.fillWidth: true
                            }
                            IconButton {
                                objectName: "workspaceTabClose_" + tabButton.tabId
                                Accessible.name: qsTr("Close %1").arg(tabButton.name)
                                symbol: Icons.close
                                size: 14
                                tooltip: qsTr("Close tab")
                                onClicked: WorkspaceManager.closeTab(tabButton.index)
                            }
                        }

                        TapHandler {
                            acceptedButtons: Qt.RightButton
                            onTapped: {
                                WorkspaceManager.activeIndex = tabButton.index
                                tabContextMenu.targetIndex = tabButton.index
                                tabContextMenu.popup()
                            }
                        }
                        TapHandler {
                            acceptedButtons: Qt.MiddleButton
                            onTapped: WorkspaceManager.closeTab(tabButton.index)
                        }
                        TapHandler {
                            acceptedButtons: Qt.LeftButton
                            onDoubleTapped: tabSettings.openForIndex(tabButton.index)
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: 1
                    Layout.fillHeight: true
                    color: Theme.color("outlineVariant")
                }

                IconButton {
                    id: addTabButton
                    objectName: "workspaceAddTabButton"
                    Accessible.name: qsTr("New workspace tab")
                    Layout.preferredWidth: 48
                    Layout.fillHeight: true
                    symbol: Icons.add
                    tooltip: qsTr("New tab (Ctrl+T)")
                    onClicked: root.createTab()
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.color("outlineVariant")
        }

        StackLayout {
            id: pageStack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: WorkspaceManager.activeIndex
            visible: WorkspaceManager.count > 0

            Repeater {
                model: WorkspaceManager
                delegate: Item {
                    required property int index
                    required property string tabId
                    required property string name
                    required property string content
                    required property string fontFamily
                    required property string fontStyle
                    required property real fontPointSize
                    required property bool bold
                    required property bool italic
                    required property string fontColor
                    required property string updatedAt

                    WorkspacePage {
                        anchors.fill: parent
                        tabId: parent.tabId
                        tabName: parent.name
                        tabContent: parent.content
                        fontFamily: parent.fontFamily
                        fontStyle: parent.fontStyle
                        fontPointSize: parent.fontPointSize
                        bold: parent.bold
                        italic: parent.italic
                        fontColor: parent.fontColor
                        updatedAt: parent.updatedAt
                    }
                }
            }
        }

        Pane {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: WorkspaceManager.count === 0
            background: Rectangle { color: Theme.color("surface") }

            ColumnLayout {
                anchors.centerIn: parent
                width: Math.min(460, parent.width * 0.86)
                spacing: Spacing.md
                MDIcon {
                    Layout.alignment: Qt.AlignHCenter
                    icon: Icons.article
                    size: 56
                    color: Theme.color("primary")
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Open your first page")
                    horizontalAlignment: Text.AlignHCenter
                    font: Typography.headlineSmall
                    color: Theme.color("onSurface")
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Each tab is a persistent page with its own typography and unlimited color. Every change is versioned in a private local Git repository.")
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.WordWrap
                    font: Typography.bodyMedium
                    color: Theme.color("onSurfaceVariant")
                }
                Button {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Create tab")
                    highlighted: true
                    onClicked: root.createTab()
                }
            }
        }
    }

    Menu {
        id: tabContextMenu
        objectName: "workspaceTabContextMenu"
        property int targetIndex: -1
        Material.elevation: Spacing.elevationMenu

        MenuItem {
            id: customizeTabAction
            objectName: "workspaceCustomizeTabAction"
            text: qsTr("Name & appearance…")
            onTriggered: tabSettings.openForIndex(tabContextMenu.targetIndex)
        }
        MenuItem {
            text: qsTr("Duplicate tab")
            onTriggered: WorkspaceManager.duplicateTab(tabContextMenu.targetIndex)
        }
        MenuItem {
            text: qsTr("Close other tabs")
            enabled: WorkspaceManager.count > 1
            onTriggered: WorkspaceManager.closeOtherTabs(tabContextMenu.targetIndex)
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Close tab")
            onTriggered: WorkspaceManager.closeTab(tabContextMenu.targetIndex)
        }
    }

    Menu {
        id: portabilityMenu
        Material.elevation: Spacing.elevationMenu
        MenuItem {
            objectName: "workspaceImportAction"
            text: qsTr("Import workspace JSON…")
            onTriggered: root.showImportWarning("json")
        }
        MenuItem {
            objectName: "workspaceExportAction"
            text: qsTr("Export workspace JSON…")
            onTriggered: exportJsonDialog.open()
        }
        MenuSeparator {}
        MenuItem {
            objectName: "workspaceImportRepoAction"
            text: qsTr("Import complete Git repository…")
            onTriggered: root.showImportWarning("repository")
        }
        MenuItem {
            objectName: "workspaceExportRepoAction"
            text: qsTr("Export complete Git repository…")
            onTriggered: exportRepositoryDialog.open()
        }
        MenuSeparator {}
        MenuItem {
            text: qsTr("Rename application…")
            onTriggered: root.renameApplication()
        }
        MenuItem {
            text: qsTr("Open managed repository")
            onTriggered: WorkspaceManager.openRepository()
        }
    }

    TextInputDialog {
        id: renameAppDialog
        objectName: "workspaceRenameAppDialog"
        inputObjectName: "workspaceRenameAppField"
        title: qsTr("Rename application")
        label: qsTr("Display name")
        placeholder: qsTr("qBittorrent Material")
        onAccepted: (value) => WorkspaceManager.appDisplayName = value
    }

    WorkspaceTabSettingsDialog { id: tabSettings }

    Popup {
        id: importWarning
        property string kind: "json"
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape
        parent: Overlay.overlay
        anchors.centerIn: parent
        width: Math.min(460, parent.width * 0.9)
        padding: Spacing.xl
        Material.elevation: 24
        background: Rectangle {
            radius: Spacing.radiusDialog
            color: Theme.color("surface")
        }
        contentItem: ColumnLayout {
            spacing: Spacing.md
            Label {
                text: qsTr("Replace current workspace?")
                font: Typography.headlineSmall
                color: Theme.color("onSurface")
            }
            Label {
                Layout.fillWidth: true
                text: importWarning.kind === "repository"
                    ? qsTr("Importing a repository replaces the current tabs and local history. The current workspace is committed first.")
                    : qsTr("Importing JSON replaces the current tabs and appearance settings. The current workspace remains in local Git history.")
                wrapMode: Text.WordWrap
                font: Typography.bodyMedium
                color: Theme.color("onSurfaceVariant")
            }
            DialogButtonBox {
                Layout.fillWidth: true
                background: null
                Button {
                    text: qsTr("Cancel")
                    flat: true
                    DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                    onClicked: importWarning.close()
                }
                Button {
                    text: qsTr("Continue")
                    highlighted: true
                    DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                    onClicked: {
                        var kind = importWarning.kind
                        importWarning.close()
                        if (kind === "repository")
                            importRepositoryDialog.open()
                        else
                            importJsonDialog.open()
                    }
                }
            }
        }
    }

    Dialogs.FileDialog {
        id: importJsonDialog
        title: qsTr("Import workspace JSON")
        fileMode: Dialogs.FileDialog.OpenFile
        nameFilters: [qsTr("Workspace JSON (*.json)"), qsTr("All files (*)")]
        onAccepted: WorkspaceManager.importWorkspace(selectedFile)
    }

    Dialogs.FileDialog {
        id: exportJsonDialog
        title: qsTr("Export workspace JSON")
        fileMode: Dialogs.FileDialog.SaveFile
        defaultSuffix: "json"
        selectedFile: WorkspaceManager.suggestedExportUrl("qbt-material-workspace.json")
        nameFilters: [qsTr("Workspace JSON (*.json)"), qsTr("All files (*)")]
        onAccepted: WorkspaceManager.exportWorkspace(selectedFile)
    }

    Dialogs.FolderDialog {
        id: exportRepositoryDialog
        title: qsTr("Choose where to export the complete Git repository")
        onAccepted: WorkspaceManager.exportRepository(selectedFolder)
    }

    Dialogs.FolderDialog {
        id: importRepositoryDialog
        title: qsTr("Choose an exported workspace Git repository")
        onAccepted: WorkspaceManager.importRepository(selectedFolder)
    }

    Snackbar { id: workspaceSnackbar }
}
