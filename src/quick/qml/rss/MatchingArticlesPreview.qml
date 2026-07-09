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
    \qmltype MatchingArticlesPreview
    \brief The live "Matching RSS Articles" test panel.

    Reads \c editor.matchingArticles — a list of \c {{ feedName, feedUrl, titles }}
    groups recomputed by \c RuleEditorController whenever the rule definition,
    feed selection or ignore period changes — and renders each feed as a bold
    header with its matching article titles beneath.
*/
Rectangle {
    id: root

    /*! The \c RuleEditorController providing \c matchingArticles. */
    property var editor: null

    color: Theme.color("surface")
    border.width: 1
    border.color: Theme.color("outlineVariant")
    radius: Spacing.radiusCard

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Spacing.sm
        spacing: Spacing.xs

        Label {
            Layout.fillWidth: true
            text: qsTr("Matching RSS Articles")
            font: Typography.titleMedium
            color: Theme.color("onSurface")
        }

        ListView {
            id: groups
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: root.editor ? root.editor.matchingArticles : []
            boundsBehavior: Flickable.StopAtBounds
            spacing: Spacing.xs

            ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

            delegate: ColumnLayout {
                id: groupDelegate
                width: ListView.view.width
                spacing: 0

                required property var modelData

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.xs
                    MDIcon { icon: Icons.rss_feed; size: 16; color: Theme.color("onSurfaceVariant") }
                    Label {
                        Layout.fillWidth: true
                        text: groupDelegate.modelData.feedName
                        elide: Text.ElideRight
                        font: Qt.font({ family: Typography.family, pixelSize: Typography.bodyMedium.pixelSize, weight: Font.DemiBold })
                        color: Theme.color("onSurface")
                    }
                }

                Repeater {
                    model: groupDelegate.modelData.titles
                    delegate: Label {
                        required property string modelData
                        Layout.fillWidth: true
                        leftPadding: Spacing.xl
                        text: modelData
                        elide: Text.ElideRight
                        font: Typography.bodyMedium
                        color: Theme.color("onSurfaceVariant")
                    }
                }
            }

            Label {
                anchors.centerIn: parent
                visible: groups.count === 0
                text: qsTr("No matching articles")
                color: Theme.color("onSurfaceVariant")
                font: Typography.bodyMedium
            }
        }
    }
}
