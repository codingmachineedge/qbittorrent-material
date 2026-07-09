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
    \qmltype AddTorrentParamsForm
    \brief Reusable "tri-state defaults" add-torrent parameters editor.

    Mirrors the legacy AddTorrentParamsWidget: every option may be left at
    "Default" (→ \c std::optional nullopt), so it is reused by RSS auto-download
    rules, the category dialog and watched folders. It reads/writes an
    \c AddTorrentParams-shaped object via \l params and emits \l changed() on any
    edit. Optional bools use \l TriStateComboField; the save path uses
    \l PathComboField; share limits are edited by an embedded
    \l SharedShareLimitsForm.

    \l params is treated as a plain object whose fields follow
    \c BitTorrent::AddTorrentParams: \c useAutoTMM, \c savePath,
    \c useDownloadPath, \c downloadPath, \c category, \c tags (array),
    \c contentLayout, \c skipChecking, \c addStopped, \c stopCondition,
    \c addToQueueTop and a nested \c shareLimits object.
*/
ColumnLayout {
    id: root

    /*! Two-way: the AddTorrentParams-shaped object being edited. */
    property var params: ({})

    /*! Category names for the category combo (caller-supplied). */
    property var categories: []

    /*! Save-path history for the save/download path combos. */
    property var pathHistory: []

    /*! Emitted whenever the user edits any field. */
    signal changed()

    spacing: Spacing.md

    property bool _updating: false

    Component.onCompleted: _load()
    onParamsChanged: if (!_updating) _load()

    // ---- Optional/enum <-> control-index helpers --------------------------
    function _isNil(v) { return v === undefined || v === null }
    function _optIndex(v) { return _isNil(v) ? 0 : (v ? 2 : 1) }        // Default/No/Yes
    function _indexOpt(i) { return i === 0 ? undefined : (i === 2) }    // 0→nil,1→false,2→true
    function _enumIndex(v) { return _isNil(v) ? 0 : (v + 1) }
    function _indexEnum(i) { return i === 0 ? undefined : (i - 1) }

    function _load() {
        _updating = true
        var p = params || {}

        tmmCombo.currentIndex = _optIndex(p.useAutoTMM)
        savePathField.path = p.savePath !== undefined ? p.savePath : ""
        dlPathTri.fromOptional(p.useDownloadPath)
        downloadPathField.path = p.downloadPath !== undefined ? p.downloadPath : ""
        categoryCombo.editText = p.category !== undefined ? p.category : ""
        tagsField.text = (p.tags && p.tags.length) ? p.tags.join(", ") : ""
        contentLayoutCombo.currentIndex = _enumIndex(p.contentLayout)
        skipCheck.checked = p.skipChecking === true
        // Start torrent is the inverse of addStopped.
        startTri.fromOptional(_isNil(p.addStopped) ? undefined : !p.addStopped)
        stopConditionCombo.currentIndex = _enumIndex(p.stopCondition)
        queueTopTri.fromOptional(p.addToQueueTop)

        var sl = p.shareLimits || {}
        shareForm.ratioLimit = sl.ratioLimit !== undefined ? sl.ratioLimit : -2
        shareForm.seedingTimeLimit = sl.seedingTimeLimit !== undefined ? sl.seedingTimeLimit : -2
        shareForm.inactiveSeedingTimeLimit = sl.inactiveSeedingTimeLimit !== undefined ? sl.inactiveSeedingTimeLimit : -2
        shareForm.mode = sl.mode !== undefined ? sl.mode : -1
        shareForm.action = sl.action !== undefined ? sl.action : -1

        _updating = false
    }

    function _store() {
        if (_updating)
            return
        var p = params ? params : {}

        p.useAutoTMM = _indexOpt(tmmCombo.currentIndex)
        p.savePath = savePathField.path
        p.useDownloadPath = dlPathTri.toOptional()
        p.downloadPath = downloadPathField.path
        p.category = categoryCombo.editText
        p.tags = tagsField.text.length
                 ? tagsField.text.split(",").map((t) => t.trim()).filter((t) => t.length > 0)
                 : []
        p.contentLayout = _indexEnum(contentLayoutCombo.currentIndex)
        p.skipChecking = skipCheck.checked
        var start = startTri.toOptional()
        p.addStopped = (start === undefined) ? undefined : !start
        p.stopCondition = _indexEnum(stopConditionCombo.currentIndex)
        p.addToQueueTop = queueTopTri.toOptional()

        p.shareLimits = {
            "ratioLimit": shareForm.ratioLimit,
            "seedingTimeLimit": shareForm.seedingTimeLimit,
            "inactiveSeedingTimeLimit": shareForm.inactiveSeedingTimeLimit,
            "mode": shareForm.mode,
            "action": shareForm.action
        }

        _updating = true
        params = p
        _updating = false

        Log.debug("ui", "AddTorrentParamsForm updated (category='" + p.category +
                  "', autoTMM=" + p.useAutoTMM + ")")
        root.changed()
    }

    readonly property bool _manualTMM: tmmCombo.currentIndex === 1

    // ---- Torrent management mode ------------------------------------------
    LabeledField {
        Layout.fillWidth: true
        labelWidth: 200
        label: qsTr("Torrent Management Mode:")
        ComboBox {
            id: tmmCombo
            Layout.fillWidth: true
            model: [ qsTr("Default"), qsTr("Manual"), qsTr("Automatic") ]
            onActivated: root._store()
            ToolTip.text: qsTr("Automatic mode means that various torrent properties (e.g. save path) will be decided by the associated category")
            ToolTip.visible: hovered
        }
    }

    // ---- Save paths --------------------------------------------------------
    MaterialCard {
        Layout.fillWidth: true
        title: qsTr("Save at")

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Spacing.sm

            Label {
                visible: !root._manualTMM
                text: qsTr("Note: the current defaults are displayed for reference.")
                font: Typography.labelSmall
                color: Theme.color("onSurfaceVariant")
                font.italic: true
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
            }

            PathComboField {
                id: savePathField
                Layout.fillWidth: true
                enabled: root._manualTMM
                model: root.pathHistory
                placeholder: qsTr("Default save path")
                title: qsTr("Choose save path")
                onPathChanged: root._store()
            }

            LabeledField {
                Layout.fillWidth: true
                labelWidth: 260
                label: qsTr("Use another path for incomplete torrents:")
                TriStateComboField {
                    id: dlPathTri
                    Layout.fillWidth: true
                    onPicked: root._store()
                }
            }

            PathComboField {
                id: downloadPathField
                Layout.fillWidth: true
                enabled: dlPathTri.value === TriStateComboField.Yes
                model: root.pathHistory
                placeholder: qsTr("Incomplete-torrent path")
                title: qsTr("Choose download path")
                onPathChanged: root._store()
            }
        }
    }

    // ---- Category & tags ---------------------------------------------------
    LabeledField {
        Layout.fillWidth: true
        labelWidth: 200
        label: qsTr("Category:")
        ComboBox {
            id: categoryCombo
            Layout.fillWidth: true
            editable: true
            model: root.categories
            onEditTextChanged: root._store()
            onActivated: root._store()
        }
    }

    LabeledField {
        Layout.fillWidth: true
        labelWidth: 200
        label: qsTr("Tags:")
        TextField {
            id: tagsField
            Layout.fillWidth: true
            placeholderText: qsTr("Comma-separated tags")
            selectByMouse: true
            onEditingFinished: root._store()
        }
    }

    // ---- Misc parameters ---------------------------------------------------
    GridLayout {
        Layout.fillWidth: true
        columns: 2
        columnSpacing: Spacing.lg
        rowSpacing: Spacing.sm

        LabeledField {
            Layout.fillWidth: true
            labelWidth: 140
            label: qsTr("Content layout:")
            ComboBox {
                id: contentLayoutCombo
                Layout.fillWidth: true
                model: [
                    qsTr("Default"),
                    qsTr("Original"),
                    qsTr("Create subfolder"),
                    qsTr("Don't create subfolder")
                ]
                onActivated: root._store()
            }
        }

        LabeledField {
            Layout.fillWidth: true
            labelWidth: 140
            label: qsTr("Stop condition:")
            ComboBox {
                id: stopConditionCombo
                Layout.fillWidth: true
                model: [
                    qsTr("Default"),
                    qsTr("None"),
                    qsTr("Metadata received"),
                    qsTr("Files checked")
                ]
                onActivated: root._store()
            }
        }

        LabeledField {
            Layout.fillWidth: true
            labelWidth: 140
            label: qsTr("Start torrent:")
            TriStateComboField {
                id: startTri
                Layout.fillWidth: true
                onPicked: root._store()
            }
        }

        LabeledField {
            Layout.fillWidth: true
            labelWidth: 140
            label: qsTr("Add to top of queue:")
            TriStateComboField {
                id: queueTopTri
                Layout.fillWidth: true
                onPicked: root._store()
            }
        }

        CheckBox {
            id: skipCheck
            text: qsTr("Skip hash check")
            font: Typography.bodyMedium
            Layout.columnSpan: 2
            onToggled: root._store()
        }
    }

    // ---- Share limits ------------------------------------------------------
    MaterialCard {
        Layout.fillWidth: true
        title: qsTr("Torrent share limits")

        SharedShareLimitsForm {
            id: shareForm
            Layout.fillWidth: true
            onChanged: root._store()
        }
    }
}
