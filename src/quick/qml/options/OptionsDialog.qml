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
    \qmltype OptionsDialog
    \brief The Material Preferences dialog.

    A NavigationRail-style left list drives a \c StackLayout of the nine option
    pages (Behavior / Downloads / Connection / Speed / BitTorrent / Search / RSS /
    WebUI / Advanced). Every control on every page is bound to the
    \c OptionsController staging layer: reads go through
    \c OptionsController.value(key, default) (made reactive by the controller's
    \c revision counter) and edits stage through \c OptionsController.setValue(),
    which flips \c OptionsController.modified. The bottom button box mirrors the
    legacy OK / Cancel / Apply semantics: OK applies + closes, Apply applies and
    stays open, Cancel discards the staged changes.

    Language and color-scheme selectors bypass staging and drive \c I18n /
    \c ThemeManager live, exactly like the legacy immediate-apply behavior.
*/
Dialog {
    id: root

    title: qsTr("Options")
    modal: true
    parent: Overlay.overlay
    anchors.centerIn: parent

    // Cap to 90% of the window; the pages scroll internally.
    width: Math.min(940, (parent ? parent.width : 940) * 0.95)
    height: Math.min(720, (parent ? parent.height : 720) * 0.95)
    padding: 0

    Material.elevation: 24
    Material.roundedScale: Material.MediumScale

    background: Rectangle {
        radius: Spacing.radiusDialog
        color: Theme.color("surface")
    }

    // The rail entries; each maps 1:1 to a StackLayout page by index (mirrors the
    // legacy Tabs enum order TAB_UI..TAB_ADVANCED).
    readonly property var pages: [
        { icon: Icons.palette,          label: qsTr("Behavior") },
        { icon: Icons.download,         label: qsTr("Downloads") },
        { icon: Icons.lan,              label: qsTr("Connection") },
        { icon: Icons.speed,            label: qsTr("Speed") },
        { icon: Icons.swap_vert,        label: qsTr("BitTorrent") },
        { icon: Icons.search,           label: qsTr("Search") },
        { icon: Icons.rss_feed,         label: qsTr("RSS") },
        { icon: Icons.language,         label: qsTr("Web UI") },
        { icon: Icons.settings_suggest, label: qsTr("Advanced") }
    ]

    function open() {
        Log.info("ui", "OptionsDialog opening; reloading staged settings")
        OptionsController.load()
        rail.currentIndex = Preferences.value("GUI/Preferences/LastViewedPage", 0)
        visible = true
    }

    // Public slot (mirrors the legacy OptionsDialog::showConnectionTab): open the
    // dialog straight on the Connection page (TAB_CONNECTION == index 2). Used by
    // the status-bar connection indicator.
    function showConnectionTab() {
        Log.info("ui", "OptionsDialog: showConnectionTab()")
        open()
        rail.currentIndex = 2
    }

    onAccepted: {
        Log.info("ui", "OptionsDialog OK — applying settings")
        OptionsController.apply()
    }
    onRejected: {
        Log.info("ui", "OptionsDialog Cancel — discarding staged settings")
        OptionsController.reset()
    }

    header: Label {
        text: root.title
        font: Typography.headlineSmall
        color: Theme.color("onSurface")
        elide: Text.ElideRight
        padding: Spacing.lg
        bottomPadding: Spacing.sm
    }

    contentItem: RowLayout {
        spacing: 0

        // ---- NavigationRail-style left list -----------------------------------
        Rectangle {
            Layout.fillHeight: true
            Layout.preferredWidth: 176
            color: Theme.color("surfaceVariant")

            // Right divider (rail elevation 0 + outline per DESIGN_SYSTEM §3).
            Rectangle {
                anchors.right: parent.right
                width: 1
                height: parent.height
                color: Theme.color("outlineVariant")
            }

            ListView {
                id: rail
                anchors.fill: parent
                anchors.margins: Spacing.sm
                spacing: Spacing.xs
                clip: true
                currentIndex: 0
                model: root.pages
                boundsBehavior: Flickable.StopAtBounds

                onCurrentIndexChanged: {
                    Log.debug("ui", "Options page -> " + currentIndex
                              + " (" + (root.pages[currentIndex] ? root.pages[currentIndex].label : "?") + ")")
                    Preferences.setValue("GUI/Preferences/LastViewedPage", currentIndex)
                }

                delegate: ItemDelegate {
                    id: railItem
                    required property int index
                    required property var modelData
                    width: ListView.view.width
                    height: Spacing.touchTarget + Spacing.xs
                    highlighted: rail.currentIndex === index
                    onClicked: rail.currentIndex = index

                    background: Rectangle {
                        radius: Spacing.radiusChip
                        color: railItem.highlighted
                               ? Theme.color("primaryContainer")
                               : (railItem.hovered
                                  ? Qt.rgba(Theme.onSurface.r, Theme.onSurface.g, Theme.onSurface.b, Theme.hoverOpacity)
                                  : "transparent")
                    }

                    contentItem: RowLayout {
                        spacing: Spacing.sm
                        MDIcon {
                            icon: railItem.modelData.icon
                            size: Spacing.iconSizeSmall
                            color: railItem.highlighted
                                   ? Theme.color("onPrimaryContainer")
                                   : Theme.color("onSurfaceVariant")
                            Layout.alignment: Qt.AlignVCenter
                        }
                        Label {
                            text: railItem.modelData.label
                            font: Typography.titleSmall
                            elide: Text.ElideRight
                            color: railItem.highlighted
                                   ? Theme.color("onPrimaryContainer")
                                   : Theme.color("onSurface")
                            Layout.fillWidth: true
                            Layout.alignment: Qt.AlignVCenter
                        }
                    }
                }
            }
        }

        // ---- Page stack --------------------------------------------------------
        StackLayout {
            id: stack
            Layout.fillWidth: true
            Layout.fillHeight: true
            currentIndex: rail.currentIndex

            BehaviorPage {}
            DownloadsPage {}
            ConnectionPage {}
            SpeedPage {}
            BitTorrentPage {}

            // Search — its settings are minimal in this build (the Python
            // executable path lives on the Advanced page); shown as an
            // informational placeholder so the rail order matches the legacy
            // Tabs enum. The full Search options screen is owned by the search
            // feature and can replace this page.
            Flickable {
                contentHeight: searchCol.implicitHeight + (2 * Spacing.lg)
                clip: true
                ColumnLayout {
                    id: searchCol
                    x: Spacing.lg
                    y: Spacing.lg
                    width: parent.width - (2 * Spacing.lg)
                    spacing: Spacing.lg
                    MaterialCard {
                        title: qsTr("Search")
                        titleIcon: Icons.search
                        Layout.fillWidth: true
                        Label {
                            text: qsTr("Search engine settings are managed from the Search tab. The Python interpreter used by search plugins can be configured on the Advanced page.")
                            font: Typography.bodyMedium
                            color: Theme.color("onSurfaceVariant")
                            wrapMode: Text.WordWrap
                            Layout.fillWidth: true
                        }
                    }
                }
                ScrollBar.vertical: ScrollBar {}
            }

            RSSPage {}
            WebUIPage {}
            AdvancedPage {}
        }
    }

    footer: DialogButtonBox {
        padding: Spacing.lg
        topPadding: Spacing.sm
        spacing: Spacing.sm

        Button {
            text: qsTr("Cancel")
            flat: true
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            onClicked: root.reject()
        }

        Button {
            text: qsTr("Apply")
            flat: true
            enabled: OptionsController.modified
            DialogButtonBox.buttonRole: DialogButtonBox.ApplyRole
            onClicked: {
                Log.info("ui", "OptionsDialog Apply")
                OptionsController.apply()
            }
        }

        Button {
            text: qsTr("OK")
            highlighted: true
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            onClicked: root.accept()
        }
    }

    Component.onCompleted: Log.debug("ui", "OptionsDialog constructed")
}
