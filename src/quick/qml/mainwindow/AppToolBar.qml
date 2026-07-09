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
    AppToolBar — the Material top toolbar. Buttons trigger the shared Actions
    declared in Main.qml (\c shell). A right-click opens \c ToolbarContextMenu to
    pick the text position (icons only / text only / beside / under / follow),
    persisted to \c Preferences::setToolbarTextPosition. On the far right sits the
    transfer-list filter box + "Filter by" column combo (shown on the Transfers
    tab only), then the Options and Lock buttons.
*/
ToolBar {
    id: toolBar

    /// The Main.qml root, exposing the shared Action objects + shell state.
    required property var shell

    /// Toolbar button text position: 0 icons only, 1 text only, 2 text beside,
    /// 3 text under, 4 follow system style. Mirrors Qt::ToolButtonStyle order.
    property int textPosition: 0

    Material.elevation: 0

    // Bottom divider (toolbars are elevation 0 + a divider per DESIGN_SYSTEM §3).
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 1
        color: Theme.color("outlineVariant")
    }

    // Right-click anywhere on the toolbar background -> text-position menu.
    TapHandler {
        acceptedButtons: Qt.RightButton
        onTapped: (eventPoint) => {
            Log.debug("ui", "Toolbar context menu requested")
            contextMenu.popup()
        }
    }

    ToolbarContextMenu {
        id: contextMenu
        currentPosition: toolBar.textPosition
        onPositionSelected: (pos) => toolBar.applyTextPosition(pos)
    }

    // A toolbar button that renders an MDIcon glyph and, per textPosition, a label.
    component TbButton: ToolButton {
        id: tb
        property string glyph: ""
        property var boundAction: null
        action: boundAction
        display: AbstractButton.IconOnly   // we lay out the content ourselves
        ToolTip.visible: hovered && (toolBar.textPosition === 0 || toolBar.textPosition === 4)
        ToolTip.text: boundAction ? boundAction.text.replace(/&/g, "").replace(/\.\.\.$/, "") : ""
        ToolTip.delay: 500
        contentItem: Grid {
            columns: (toolBar.textPosition === 3) ? 1 : 2
            rows: (toolBar.textPosition === 3) ? 2 : 1
            flow: (toolBar.textPosition === 3) ? Grid.TopToBottom : Grid.LeftToRight
            columnSpacing: Spacing.sm
            rowSpacing: 2
            horizontalItemAlignment: Grid.AlignHCenter
            verticalItemAlignment: Grid.AlignVCenter
            MDIcon {
                visible: toolBar.textPosition !== 1
                icon: tb.glyph
                size: 20
                opacity: tb.enabled ? 1.0 : 0.4
                color: Theme.color("onSurface")
            }
            Label {
                visible: toolBar.textPosition !== 0 && toolBar.textPosition !== 4
                text: tb.boundAction ? tb.boundAction.text.replace(/&/g, "").replace(/\.\.\.$/, "") : ""
                font: Typography.labelLarge
                color: Theme.color("onSurface")
                opacity: tb.enabled ? 1.0 : 0.4
            }
        }
    }

    component TbSeparator: ToolSeparator {}

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Spacing.sm
        anchors.rightMargin: Spacing.sm
        spacing: Spacing.xs

        TbButton { boundAction: toolBar.shell.actionOpen; glyph: Icons.note_add }
        TbButton { boundAction: toolBar.shell.actionDownloadFromURL; glyph: Icons.add_link }
        TbButton { boundAction: toolBar.shell.actionDelete; glyph: Icons.deleteIcon }
        TbSeparator {}
        TbButton { boundAction: toolBar.shell.actionStart; glyph: Icons.play_arrow }
        TbButton { boundAction: toolBar.shell.actionStop; glyph: Icons.pause }
        TbButton { boundAction: toolBar.shell.actionOpenDestinationFolder; glyph: Icons.folder_open }
        TbButton {
            boundAction: toolBar.shell.actionTopQueuePos
            glyph: Icons.vertical_align_top
            visible: toolBar.shell.queueingEnabled
        }
        TbButton {
            boundAction: toolBar.shell.actionIncreaseQueuePos
            glyph: Icons.arrow_upward
            visible: toolBar.shell.queueingEnabled
        }
        TbButton {
            boundAction: toolBar.shell.actionDecreaseQueuePos
            glyph: Icons.arrow_downward
            visible: toolBar.shell.queueingEnabled
        }
        TbButton {
            boundAction: toolBar.shell.actionBottomQueuePos
            glyph: Icons.vertical_align_bottom
            visible: toolBar.shell.queueingEnabled
        }
        TbSeparator {}
        TbButton { boundAction: toolBar.shell.actionCreateTorrent; glyph: Icons.build }

        // Flexible stretch spacer.
        Item { Layout.fillWidth: true }

        // Transfer-list filter (only on the Transfers tab, §3).
        RowLayout {
            spacing: Spacing.sm
            visible: toolBar.shell.currentTabIndex === 0

            FilterTextField {
                id: filterField
                Layout.preferredWidth: 200
                placeholder: qsTr("Filter torrents...")
                regexEnabled: false
                onTextChanged: toolBar.applyTransferFilter()
                onRegexEnabledChanged: {
                    Log.debug("ui", "Transfer filter regex -> " + regexEnabled)
                    Preferences.setRegexAsFilteringPatternForTransferList(regexEnabled)
                    Preferences.apply()
                    toolBar.applyTransferFilter()
                }
            }
            Label {
                text: qsTr("Filter by:")
                font: Typography.bodyMedium
                color: Theme.color("onSurfaceVariant")
            }
            ComboBox {
                id: filterColumnCombo
                Layout.preferredWidth: 150
                textRole: "label"
                model: [
                    { label: qsTr("Name"), column: "name" },
                    { label: qsTr("Save Path"), column: "savePath" },
                    { label: qsTr("Info Hash v1"), column: "infohashV1" },
                    { label: qsTr("Info Hash v2"), column: "infohashV2" }
                ]
                onActivated: {
                    Log.debug("ui", "Transfer filter column -> " + currentIndex)
                    toolBar.applyTransferFilter()
                }
            }
        }

        TbButton { boundAction: toolBar.shell.actionOptions; glyph: Icons.settings }
        TbButton { boundAction: toolBar.shell.actionLock; glyph: Icons.lock }
    }

    function applyTransferFilter() {
        var col = filterColumnCombo.model[filterColumnCombo.currentIndex].column
        toolBar.shell.applyTransferFilter(filterField.text, col)
    }

    function applyTextPosition(pos) {
        Log.info("ui", "Toolbar text position -> " + pos)
        toolBar.textPosition = pos
        Preferences.setToolbarTextPosition(pos)
        Preferences.apply()
    }

    // Hiding the toolbar clears the filter (matches legacy behavior).
    onVisibleChanged: {
        if (!visible && filterField.text.length > 0) {
            Log.debug("ui", "Toolbar hidden; clearing transfer filter")
            filterField.text = ""
        }
    }

    Component.onCompleted: {
        toolBar.textPosition = Preferences.getToolbarTextPosition()
        filterField.regexEnabled = Preferences.getRegexAsFilteringPatternForTransferList()
        Log.debug("ui", "AppToolBar ready; textPosition=" + toolBar.textPosition)
    }
}
