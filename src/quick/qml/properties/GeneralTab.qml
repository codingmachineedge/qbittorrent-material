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
    \qmltype GeneralTab
    \brief The properties "General" tab: pieces bars plus Transfer and
           Information cards.

    Every value is a pre-formatted, translated \c PropertiesController property
    (e.g. \c downloadSpeed, \c totalSize, \c infoHashV1); the pieces bars bind to
    the controller's \c havePieces / \c downloadingPieces / \c pieceAvailability
    (and \c progressValue for the fallback fill). The Transfer / Information grids
    reflow 3 → 2 → 1 columns responsively; the body scrolls vertically and never
    horizontally.
*/
Flickable {
    id: root

    contentWidth: width
    contentHeight: column.implicitHeight + Spacing.lg * 2
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

    // Number of label/value columns for the current width.
    function pairColumns() {
        if (width > 900) return 3
        if (width > 560) return 2
        return 1
    }

    // Reusable label/value form row. `prop` is read dynamically off the
    // controller so the binding re-evaluates on generalChanged.
    Component {
        id: fieldComp
        RowLayout {
            id: fieldRow
            property string fieldLabel: ""
            property string propName: ""
            property string fieldTooltip: ""
            spacing: Spacing.sm
            Layout.fillWidth: true

            Label {
                text: fieldRow.fieldLabel
                font: Typography.bodyMedium
                color: Theme.color("onSurfaceVariant")
                Layout.alignment: Qt.AlignTop
            }
            Label {
                text: {
                    const v = PropertiesController[fieldRow.propName]
                    return (v !== undefined && v !== null) ? ("" + v) : ""
                }
                font: Typography.bodyMedium
                color: Theme.color("onSurface")
                elide: Text.ElideRight
                Layout.fillWidth: true

                ToolTip.visible: fieldRow.fieldTooltip.length > 0 && hh.hovered
                ToolTip.text: fieldRow.fieldTooltip
                ToolTip.delay: 500
                HoverHandler { id: hh }
            }
        }
    }

    ColumnLayout {
        id: column
        x: Spacing.lg
        y: Spacing.lg
        width: root.width - Spacing.lg * 2
        spacing: Spacing.lg

        // ---- Pieces bars card -------------------------------------------------
        MaterialCard {
            Layout.fillWidth: true

            GridLayout {
                Layout.fillWidth: true
                columns: 3
                columnSpacing: Spacing.md
                rowSpacing: Spacing.sm

                Label {
                    text: qsTr("Progress:")
                    font: Typography.bodyMedium
                    color: Theme.color("onSurfaceVariant")
                }
                DownloadedPiecesBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 18
                    pieces: PropertiesController.havePieces
                    downloadingPieces: PropertiesController.downloadingPieces
                    progress: PropertiesController.progressValue
                }
                Label {
                    text: PropertiesController.progress
                    font: Typography.mono
                    color: Theme.color("onSurface")
                    Layout.preferredWidth: 64
                    horizontalAlignment: Text.AlignRight
                }

                Label {
                    text: qsTr("Availability:")
                    font: Typography.bodyMedium
                    color: Theme.color("onSurfaceVariant")
                    visible: PropertiesController.showAvailability
                }
                PieceAvailabilityBar {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 18
                    visible: PropertiesController.showAvailability
                    availability: PropertiesController.pieceAvailability
                }
                Label {
                    text: PropertiesController.averageAvailability
                    font: Typography.mono
                    color: Theme.color("onSurface")
                    visible: PropertiesController.showAvailability
                    Layout.preferredWidth: 64
                    horizontalAlignment: Text.AlignRight
                }
            }
        }

        // ---- Transfer card ----------------------------------------------------
        MaterialCard {
            title: qsTr("Transfer")
            Layout.fillWidth: true

            GridLayout {
                Layout.fillWidth: true
                columns: root.pairColumns()
                columnSpacing: Spacing.xl
                rowSpacing: Spacing.sm

                Repeater {
                    model: [
                        { l: qsTr("Time Active:"),         k: "timeElapsed" },
                        { l: qsTr("ETA:"),                 k: "eta" },
                        { l: qsTr("Connections:"),         k: "connections" },
                        { l: qsTr("Downloaded:"),          k: "downloaded" },
                        { l: qsTr("Uploaded:"),            k: "uploaded" },
                        { l: qsTr("Seeds:"),               k: "seeds" },
                        { l: qsTr("Download Speed:"),      k: "downloadSpeed" },
                        { l: qsTr("Upload Speed:"),        k: "uploadSpeed" },
                        { l: qsTr("Peers:"),               k: "peers" },
                        { l: qsTr("Download Limit:"),      k: "downloadLimit" },
                        { l: qsTr("Upload Limit:"),        k: "uploadLimit" },
                        { l: qsTr("Wasted:"),              k: "wasted" },
                        { l: qsTr("Share Ratio:"),         k: "shareRatio" },
                        { l: qsTr("Reannounce In:"),       k: "reannounceIn" },
                        { l: qsTr("Last Seen Complete:"),  k: "lastSeenComplete" },
                        { l: qsTr("Popularity:"),          k: "popularity",
                          t: qsTr("Ratio / Time Active (in months), indicates how popular the torrent is") }
                    ]
                    delegate: Loader {
                        required property var modelData
                        Layout.fillWidth: true
                        sourceComponent: fieldComp
                        onLoaded: {
                            item.fieldLabel = modelData.l
                            item.propName = modelData.k
                            item.fieldTooltip = modelData.t !== undefined ? modelData.t : ""
                        }
                    }
                }
            }
        }

        // ---- Information card -------------------------------------------------
        MaterialCard {
            title: qsTr("Information")
            Layout.fillWidth: true

            ColumnLayout {
                Layout.fillWidth: true
                spacing: Spacing.sm

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.pairColumns()
                    columnSpacing: Spacing.xl
                    rowSpacing: Spacing.sm

                    Repeater {
                        model: [
                            { l: qsTr("Total Size:"),   k: "totalSize" },
                            { l: qsTr("Pieces:"),       k: "pieces" },
                            { l: qsTr("Created By:"),   k: "createdBy" },
                            { l: qsTr("Added On:"),     k: "addedOn" },
                            { l: qsTr("Completed On:"), k: "completedOn" },
                            { l: qsTr("Created On:"),   k: "createdOn" },
                            { l: qsTr("Private:"),      k: "isPrivate" }
                        ]
                        delegate: Loader {
                            required property var modelData
                            Layout.fillWidth: true
                            sourceComponent: fieldComp
                            onLoaded: {
                                item.fieldLabel = modelData.l
                                item.propName = modelData.k
                            }
                        }
                    }
                }

                MenuSeparator { Layout.fillWidth: true }

                // Selectable / wrapping fields, full width.
                RowLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.sm
                    Label {
                        text: qsTr("Info Hash v1:")
                        font: Typography.bodyMedium
                        color: Theme.color("onSurfaceVariant")
                    }
                    TextEdit {
                        text: PropertiesController.infoHashV1
                        font: Typography.mono
                        color: Theme.color("onSurface")
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.sm
                    Label {
                        text: qsTr("Info Hash v2:")
                        font: Typography.bodyMedium
                        color: Theme.color("onSurfaceVariant")
                    }
                    TextEdit {
                        text: PropertiesController.infoHashV2
                        font: Typography.mono
                        color: Theme.color("onSurface")
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.WrapAnywhere
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.sm
                    Label {
                        text: qsTr("Save Path:")
                        font: Typography.bodyMedium
                        color: Theme.color("onSurfaceVariant")
                        Layout.alignment: Qt.AlignTop
                    }
                    TextEdit {
                        text: PropertiesController.savePath
                        font: Typography.bodyMedium
                        color: Theme.color("onSurface")
                        readOnly: true
                        selectByMouse: true
                        wrapMode: TextEdit.Wrap
                        Layout.fillWidth: true
                    }
                }

                RowLayout {
                    Layout.fillWidth: true
                    spacing: Spacing.sm
                    visible: commentText.text.length > 0
                    Label {
                        text: qsTr("Comment:")
                        font: Typography.bodyMedium
                        color: Theme.color("onSurfaceVariant")
                        Layout.alignment: Qt.AlignTop
                    }
                    Text {
                        id: commentText
                        text: PropertiesController.comment
                        font: Typography.bodyMedium
                        color: Theme.color("onSurface")
                        textFormat: Text.RichText
                        wrapMode: Text.Wrap
                        onLinkActivated: (link) => {
                            Log.info("ui", "General tab comment link activated: " + link)
                            Qt.openUrlExternally(link)
                        }
                        Layout.fillWidth: true
                    }
                }
            }
        }
    }

    Component.onCompleted: Log.debug("ui", "GeneralTab loaded")
}
