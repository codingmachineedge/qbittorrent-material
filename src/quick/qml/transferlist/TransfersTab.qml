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
import QtCore

/*!
    \qmltype TransfersTab
    \brief The top-level "Transfers" tab.

    Layout: an outer horizontal \c SplitView with the \l FilterSidebar on the
    left and, on the right, an inner vertical \c SplitView stacking the
    \l TransferListView over the \c PropertiesPanel (owned by the properties
    feature). Both the list and the sidebar share ONE
    \c TorrentFilterProxyModel (created here, per CONTRACTS §7.1): the list uses
    it as its table model, while the sidebar filter panels drive its
    status/category/tag/tracker criteria. Table selection flows to
    \c TransferController.selectedIds, which the bridge wires to
    \c PropertiesController — so the properties panel updates with no polling.

    Split sizes persist across sessions; the sidebar's visibility follows the
    \c GUI/FiltersSidebarVisible preference.
*/
Item {
    id: root

    /*! Whether the left filter sidebar is shown (toolbar toggle target). */
    property bool sidebarVisible: true

    /*!
        The one shared sort/filter proxy over the \c TransferListModel singleton.
        Exposed so sibling shell components (e.g. the toolbar's filter field) can
        reach the same instance if needed.
    */
    property alias proxy: transferProxy

    function _refreshPrefs() {
        sidebarVisible = Preferences.value("GUI/FiltersSidebarVisible", true) === true;
    }

    Connections {
        target: Preferences
        function onChanged() { root._refreshPrefs(); }
    }

    // The shared proxy: one per view (CONTRACTS §7.1).
    TorrentFilterProxyModel {
        id: transferProxy
        sourceModel: TransferListModel
    }

    // Persisted split geometry.
    Settings {
        id: layoutState
        category: "TransfersTab"
        property string outerState: ""
        property string innerState: ""
    }

    SplitView {
        id: outerSplit
        anchors.fill: parent
        orientation: Qt.Horizontal

        handle: Rectangle {
            implicitWidth: 4
            color: SplitHandle.pressed ? Theme.color("primary")
                 : (SplitHandle.hovered ? Qt.alpha(Theme.color("primary"), 0.4)
                                        : Theme.color("outlineVariant"))
        }

        FilterSidebar {
            id: sidebar
            proxy: transferProxy
            visible: root.sidebarVisible
            SplitView.preferredWidth: 240
            SplitView.minimumWidth: 160
            SplitView.maximumWidth: 480
        }

        SplitView {
            id: innerSplit
            orientation: Qt.Vertical
            SplitView.fillWidth: true

            handle: Rectangle {
                implicitHeight: 4
                color: SplitHandle.pressed ? Theme.color("primary")
                     : (SplitHandle.hovered ? Qt.alpha(Theme.color("primary"), 0.4)
                                            : Theme.color("outlineVariant"))
            }

            TransferListView {
                id: transferList
                proxy: transferProxy
                SplitView.fillHeight: true
                SplitView.minimumHeight: 120
            }

            // Owned by the properties feature; present in the same QML module.
            PropertiesPanel {
                id: properties
                SplitView.preferredHeight: 240
                SplitView.minimumHeight: 0
            }

            Component.onCompleted: {
                if (layoutState.innerState.length > 0)
                    innerSplit.restoreState(layoutState.innerState);
            }
            Component.onDestruction: layoutState.innerState = innerSplit.saveState()
        }

        Component.onCompleted: {
            if (layoutState.outerState.length > 0)
                outerSplit.restoreState(layoutState.outerState);
        }
        Component.onDestruction: layoutState.outerState = outerSplit.saveState()
    }

    Component.onCompleted: {
        _refreshPrefs();
        Log.info("ui", "TransfersTab ready");
    }
}
