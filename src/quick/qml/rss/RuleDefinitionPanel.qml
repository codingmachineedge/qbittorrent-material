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
    \qmltype RuleDefinitionPanel
    \brief The editable definition of the selected auto-download rule.

    Every control is two-way bound to the \c RuleEditorController and disabled
    until exactly one rule is selected. The must / must-not / episode fields show
    an inline validation status (warning icon + tooltip). The embedded
    \c AddTorrentParamsForm carries the rule's torrent parameters, and the "Apply
    Rule to Feeds" list toggles the affected feeds — every edit refreshes the
    live matching preview.
*/
Flickable {
    id: root

    /*! The \c RuleEditorController being edited. */
    property var editor: null

    readonly property bool active: editor && editor.ruleSelected

    contentWidth: width
    contentHeight: column.implicitHeight
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AlwaysOn }

    ColumnLayout {
        id: column
        width: root.width - Spacing.md
        x: Spacing.xs
        spacing: Spacing.md
        enabled: root.active
        opacity: root.active ? 1.0 : 0.5

        // ---- Priority -----------------------------------------------------
        LabeledField {
            Layout.fillWidth: true
            labelWidth: 120
            label: qsTr("Priority:")
            SpinBox {
                Layout.fillWidth: true
                from: -2147483647
                to: 2147483647
                editable: true
                value: root.editor ? root.editor.priority : 0
                onValueModified: if (root.editor) root.editor.priority = value
            }
        }

        // ---- Regex toggle -------------------------------------------------
        CheckBox {
            text: qsTr("Use Regular Expressions")
            font: Typography.bodyMedium
            checked: root.editor ? root.editor.useRegex : false
            onToggled: if (root.editor) root.editor.useRegex = checked
        }

        // ---- Match grid ---------------------------------------------------
        GridLayout {
            Layout.fillWidth: true
            columns: 3
            columnSpacing: Spacing.sm
            rowSpacing: Spacing.sm

            // Must contain
            Label {
                text: qsTr("Must Contain:")
                font: Typography.bodyMedium
                color: Theme.color("onSurfaceVariant")
            }
            TextField {
                id: mustField
                Layout.fillWidth: true
                selectByMouse: true
                text: root.editor ? root.editor.mustContain : ""
                onTextEdited: if (root.editor) root.editor.mustContain = text
                ToolTip.text: root.editor ? root.editor.validateExpression(text, root.editor.useRegex) : ""
                ToolTip.visible: hovered && ToolTip.text.length > 0
            }
            MDIcon {
                icon: Icons.warning
                size: 18
                color: Theme.color("error")
                visible: root.editor && root.editor.validateExpression(mustField.text, root.editor.useRegex).length > 0
            }

            // Must not contain
            Label {
                text: qsTr("Must Not Contain:")
                font: Typography.bodyMedium
                color: Theme.color("onSurfaceVariant")
            }
            TextField {
                id: mustNotField
                Layout.fillWidth: true
                selectByMouse: true
                text: root.editor ? root.editor.mustNotContain : ""
                onTextEdited: if (root.editor) root.editor.mustNotContain = text
                ToolTip.text: root.editor ? root.editor.validateExpression(text, root.editor.useRegex) : ""
                ToolTip.visible: hovered && ToolTip.text.length > 0
            }
            MDIcon {
                icon: Icons.warning
                size: 18
                color: Theme.color("error")
                visible: root.editor && root.editor.validateExpression(mustNotField.text, root.editor.useRegex).length > 0
            }

            // Episode filter
            Label {
                text: qsTr("Episode Filter:")
                font: Typography.bodyMedium
                color: Theme.color("onSurfaceVariant")
            }
            TextField {
                id: episodeField
                Layout.fillWidth: true
                selectByMouse: true
                text: root.editor ? root.editor.episodeFilter : ""
                onTextEdited: if (root.editor) root.editor.episodeFilter = text
                placeholderText: qsTr("1x2;8-15;5;30-;")
                ToolTip.text: qsTr("Example: 1x2;8-15;5;30-; will match 2, 8 through 15, 5 and 30 onward episodes of season one.\nSeason number is a mandatory non-zero value.\nEpisode number is a mandatory positive value.\nFilter must end with semicolon.")
                ToolTip.visible: hovered
            }
            MDIcon {
                icon: Icons.warning
                size: 18
                color: Theme.color("error")
                visible: root.editor && root.editor.validateEpisodeFilter(episodeField.text).length > 0
            }
        }

        // ---- Smart filter -------------------------------------------------
        CheckBox {
            text: qsTr("Use Smart Episode Filter")
            font: Typography.bodyMedium
            checked: root.editor ? root.editor.useSmartFilter : false
            onToggled: if (root.editor) root.editor.useSmartFilter = checked
            ToolTip.text: qsTr("Smart Episode Filter will check the episode number to prevent downloading of duplicates.\nSupports the formats: S01E01, 1x1, 2017.12.31 and 31.12.2017 (Date formats also support - as a separator)")
            ToolTip.visible: hovered
        }

        // ---- Ignore period ------------------------------------------------
        LabeledField {
            Layout.fillWidth: true
            labelWidth: 300
            label: qsTr("Ignore Subsequent Matches for (0 to Disable)")
            SpinBox {
                Layout.fillWidth: true
                from: 0
                to: 365
                editable: true
                value: root.editor ? root.editor.ignoreDays : 0
                textFromValue: (value) => value === 0 ? qsTr("Disabled") : (value + " " + qsTr("days"))
                valueFromText: (text) => {
                    const n = parseInt(text)
                    return isNaN(n) ? 0 : n
                }
                onValueModified: if (root.editor) root.editor.ignoreDays = value
            }
        }

        Label {
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignRight
            text: root.editor ? root.editor.lastMatchText : qsTr("Last Match: Unknown")
            font: Typography.labelSmall
            color: Theme.color("onSurfaceVariant")
        }

        // ---- Torrent parameters -------------------------------------------
        MaterialCard {
            Layout.fillWidth: true
            title: qsTr("Torrent parameters")

            AddTorrentParamsForm {
                id: paramsForm
                Layout.fillWidth: true
                params: root.editor ? root.editor.addTorrentParams : ({})
                onChanged: if (root.editor) root.editor.addTorrentParams = params
            }
        }

        // ---- Apply Rule to Feeds ------------------------------------------
        Label {
            text: qsTr("Apply Rule to Feeds:")
            font: Typography.titleSmall
            color: Theme.color("onSurface")
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 160
            color: Theme.color("surfaceVariant")
            radius: Spacing.radiusField
            border.width: 1
            border.color: Theme.color("outlineVariant")

            ListView {
                id: feedsList
                anchors.fill: parent
                anchors.margins: Spacing.xs
                clip: true
                model: root.editor ? root.editor.feeds : []
                boundsBehavior: Flickable.StopAtBounds

                ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

                delegate: CheckBox {
                    required property var modelData
                    width: ListView.view.width
                    text: modelData.name
                    font: Typography.bodyMedium
                    tristate: true
                    checkState: modelData.checkState === 2
                                ? Qt.Checked
                                : (modelData.checkState === 1 ? Qt.PartiallyChecked : Qt.Unchecked)
                    onClicked: {
                        const want = checkState === Qt.Checked
                        Log.debug("rss", "RuleDef: toggle feed '" + modelData.name + "' -> " + want)
                        if (root.editor)
                            root.editor.toggleFeed(modelData.url, want)
                    }
                }
            }
        }
    }
}
