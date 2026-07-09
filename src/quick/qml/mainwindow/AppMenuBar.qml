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
    AppMenuBar — the top-level Material menu bar (File / Edit / View / Tools /
    Plugins / Help). Every item is driven by a shared \c Action declared in
    Main.qml (\c shell), so the menu, toolbar and tray menu all trigger the exact
    same verbs and stay in sync. Icons are rendered with \c MDIcon glyphs.
*/
MenuBar {
    id: menuBar

    /// The Main.qml root, exposing the shared Action objects + shell state.
    required property var shell

    // A menu item that renders a leading Material Symbols glyph next to its label.
    // (Checkable items keep the default checkmark indicator, so they omit the glyph.)
    component IconMenuItem: MenuItem {
        id: ctl
        property string glyph: ""
        implicitHeight: 40
        contentItem: RowLayout {
            spacing: Spacing.md
            MDIcon {
                visible: ctl.glyph.length > 0
                icon: ctl.glyph
                size: 18
                opacity: ctl.enabled ? 1.0 : 0.4
                color: Theme.color("onSurface")
                Layout.leftMargin: Spacing.xs
            }
            Label {
                text: ctl.text
                font: Typography.bodyMedium
                color: Theme.color("onSurface")
                opacity: ctl.enabled ? 1.0 : 0.4
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }

    // ---- File ---------------------------------------------------------------
    Menu {
        title: qsTr("&File")
        Material.elevation: 8
        onOpened: Log.debug("ui", "File menu opened")

        IconMenuItem { action: menuBar.shell.actionOpen; glyph: Icons.note_add }
        IconMenuItem { action: menuBar.shell.actionDownloadFromURL; glyph: Icons.add_link }
        MenuSeparator {}
        IconMenuItem { action: menuBar.shell.actionExit; glyph: Icons.logout }
    }

    // ---- Edit ---------------------------------------------------------------
    Menu {
        title: qsTr("&Edit")
        Material.elevation: 8
        onOpened: Log.debug("ui", "Edit menu opened")

        IconMenuItem { action: menuBar.shell.actionStart; glyph: Icons.play_arrow }
        IconMenuItem { action: menuBar.shell.actionStop; glyph: Icons.pause }
        MenuSeparator {}
        IconMenuItem { action: menuBar.shell.actionDelete; glyph: Icons.delete }

        MenuSeparator { visible: menuBar.shell.queueingEnabled }
        IconMenuItem {
            action: menuBar.shell.actionTopQueuePos
            glyph: Icons.vertical_align_top
            visible: menuBar.shell.queueingEnabled
            height: visible ? implicitHeight : 0
        }
        IconMenuItem {
            action: menuBar.shell.actionIncreaseQueuePos
            glyph: Icons.arrow_upward
            visible: menuBar.shell.queueingEnabled
            height: visible ? implicitHeight : 0
        }
        IconMenuItem {
            action: menuBar.shell.actionDecreaseQueuePos
            glyph: Icons.arrow_downward
            visible: menuBar.shell.queueingEnabled
            height: visible ? implicitHeight : 0
        }
        IconMenuItem {
            action: menuBar.shell.actionBottomQueuePos
            glyph: Icons.vertical_align_bottom
            visible: menuBar.shell.queueingEnabled
            height: visible ? implicitHeight : 0
        }

        MenuSeparator {}
        IconMenuItem {
            action: menuBar.shell.actionPauseSession
            glyph: Icons.pause_circle
            visible: !menuBar.shell.sessionPaused
            height: visible ? implicitHeight : 0
        }
        IconMenuItem {
            action: menuBar.shell.actionResumeSession
            glyph: Icons.play_circle
            visible: menuBar.shell.sessionPaused
            height: visible ? implicitHeight : 0
        }
    }

    // ---- View ---------------------------------------------------------------
    Menu {
        title: qsTr("&View")
        Material.elevation: 8
        onOpened: Log.debug("ui", "View menu opened")

        MenuItem { action: menuBar.shell.actionTopToolBar }
        MenuItem { action: menuBar.shell.actionShowStatusbar }
        MenuItem { action: menuBar.shell.actionShowFiltersSidebar }
        MenuItem { action: menuBar.shell.actionSpeedInTitleBar }
        MenuSeparator {}
        MenuItem { action: menuBar.shell.actionSearchWidget }
        MenuItem { action: menuBar.shell.actionRSSReader }

        Menu {
            title: qsTr("&Log")
            Material.elevation: 8
            MenuItem { action: menuBar.shell.actionExecutionLogs }
            MenuSeparator {}
            MenuItem { action: menuBar.shell.actionNormalMessages }
            MenuItem { action: menuBar.shell.actionInformationMessages }
            MenuItem { action: menuBar.shell.actionWarningMessages }
            MenuItem { action: menuBar.shell.actionCriticalMessages }
        }

        MenuSeparator {}
        IconMenuItem { action: menuBar.shell.actionStatistics; glyph: Icons.bar_chart }
        MenuSeparator {}
        IconMenuItem { action: menuBar.shell.actionLock; glyph: Icons.lock }
        Menu {
            title: qsTr("Lock Options")
            Material.elevation: 8
            MenuItem { action: menuBar.shell.actionSetLockPassword }
            MenuItem { action: menuBar.shell.actionClearLockPassword }
        }
    }

    // ---- Tools --------------------------------------------------------------
    Menu {
        title: qsTr("&Tools")
        Material.elevation: 8
        onOpened: Log.debug("ui", "Tools menu opened")

        IconMenuItem { action: menuBar.shell.actionCreateTorrent; glyph: Icons.build }
        MenuSeparator {}
        IconMenuItem { action: menuBar.shell.actionManageCookies; glyph: Icons.cookie }
        IconMenuItem { action: menuBar.shell.actionOptions; glyph: Icons.settings }
        MenuSeparator {}

        Menu {
            title: qsTr("On Downloads &Done")
            Material.elevation: 8
            MenuItem { action: menuBar.shell.actionAutoShutdownDisabled }
            MenuItem { action: menuBar.shell.actionAutoExit }
            MenuItem { action: menuBar.shell.actionAutoSuspend }
            MenuItem { action: menuBar.shell.actionAutoHibernate }
            MenuItem { action: menuBar.shell.actionAutoReboot }
            MenuItem { action: menuBar.shell.actionAutoShutdown }
        }
    }

    // ---- Plugins (only when ENABLE_PLUGINS; hidden otherwise) ----------------
    Menu {
        title: qsTr("Plugins")
        Material.elevation: 8
        enabled: menuBar.shell.pluginsMenuVisible
        // NOTE: Qt Quick MenuBar cannot hide a whole Menu at runtime; when the
        // build lacks plugin support we simply disable it. Dynamic per-plugin
        // items are appended here by the plugins engine (TODO(engine)).
        IconMenuItem { action: menuBar.shell.actionManagePlugins; glyph: Icons.extension }
        MenuSeparator {}
    }

    // ---- Help ---------------------------------------------------------------
    Menu {
        title: qsTr("&Help")
        Material.elevation: 8
        onOpened: Log.debug("ui", "Help menu opened")

        IconMenuItem { action: menuBar.shell.actionDocumentation; glyph: Icons.menu_book }
        IconMenuItem { action: menuBar.shell.actionCheckForUpdates; glyph: Icons.system_update }
        MenuSeparator {}
        IconMenuItem { action: menuBar.shell.actionDonateMoney; glyph: Icons.volunteer_activism }
        IconMenuItem { action: menuBar.shell.actionAbout; glyph: Icons.info }
    }
}
