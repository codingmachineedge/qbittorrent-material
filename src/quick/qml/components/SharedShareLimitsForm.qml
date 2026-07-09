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
    \qmltype SharedShareLimitsForm
    \brief Reusable ratio / seeding-time share-limit editor.

    Mirrors \c BitTorrent::ShareLimits and the legacy TorrentShareLimitsWidget.
    Used by Torrent Options, the category dialog, Add-torrent and RSS rules.
    Sentinel conventions (from \c sharelimits.h):
    \list
      \li ratioLimit: \c -2 default, \c -1 unlimited, else the ratio value
      \li seedingTimeLimit / inactiveSeedingTimeLimit: \c -2 default,
          \c -1 unlimited, else minutes
      \li mode (ShareLimitsMode): \c -1 default, \c 0 match-any, \c 1 match-all
      \li action (ShareLimitAction): \c -1 default, \c 0 stop, \c 1 remove,
          \c 2 super-seeding, \c 3 remove-with-content
    \endlist
    All properties are two-way bindable; \l changed() fires on any user edit.
*/
ColumnLayout {
    id: root

    /*! Ratio limit sentinel/value. */
    property real ratioLimit: -2

    /*! Seeding-time limit in minutes (sentinel/value). */
    property int seedingTimeLimit: -2

    /*! Inactive seeding-time limit in minutes (sentinel/value). */
    property int inactiveSeedingTimeLimit: -2

    /*! ShareLimitsMode sentinel/value. */
    property int mode: -1

    /*! ShareLimitAction sentinel/value ("action when the limit is reached"). */
    property int action: -1

    /*! Emitted whenever the user edits any field. */
    signal changed()

    spacing: Spacing.sm

    // Guards property→control syncs from re-triggering change emission.
    property bool _updating: false

    // Mode-combo indices: 0 Default, 1 Unlimited, 2 Set to.
    function _limitIndex(v) { return v === -2 ? 0 : (v === -1 ? 1 : 2) }

    // ShareLimitAction combo order → enum value.
    readonly property var _indexToAction: [-1, 0, 1, 3, 2]
    function _actionIndex(v) {
        var i = _indexToAction.indexOf(v)
        return i < 0 ? 0 : i
    }

    // ShareLimitsMode combo order → enum value.
    readonly property var _indexToMode: [-1, 0, 1]
    function _modeIndex(v) {
        var i = _indexToMode.indexOf(v)
        return i < 0 ? 0 : i
    }

    Component.onCompleted: _syncAll()
    onRatioLimitChanged: if (!_updating) _syncRatio()
    onSeedingTimeLimitChanged: if (!_updating) _syncSeeding()
    onInactiveSeedingTimeLimitChanged: if (!_updating) _syncInactive()
    onModeChanged: if (!_updating) modeCombo.currentIndex = _modeIndex(mode)
    onActionChanged: if (!_updating) actionCombo.currentIndex = _actionIndex(action)

    function _syncAll() {
        _updating = true
        _syncRatio()
        _syncSeeding()
        _syncInactive()
        modeCombo.currentIndex = _modeIndex(mode)
        actionCombo.currentIndex = _actionIndex(action)
        _updating = false
    }

    function _syncRatio() {
        _updating = true
        ratioCombo.currentIndex = _limitIndex(ratioLimit)
        if (ratioLimit >= 0)
            ratioSpin.value = Math.round(ratioLimit * 100)
        _updating = false
    }
    function _syncSeeding() {
        _updating = true
        seedingCombo.currentIndex = _limitIndex(seedingTimeLimit)
        if (seedingTimeLimit >= 0)
            seedingSpin.value = seedingTimeLimit
        _updating = false
    }
    function _syncInactive() {
        _updating = true
        inactiveCombo.currentIndex = _limitIndex(inactiveSeedingTimeLimit)
        if (inactiveSeedingTimeLimit >= 0)
            inactiveSpin.value = inactiveSeedingTimeLimit
        _updating = false
    }

    function _emit() {
        Log.debug("ui", "ShareLimits changed: ratio=" + ratioLimit +
                  " seed=" + seedingTimeLimit + " inactive=" + inactiveSeedingTimeLimit +
                  " mode=" + mode + " action=" + action)
        root.changed()
    }

    // ---- Ratio -------------------------------------------------------------
    RowLayout {
        Layout.fillWidth: true
        spacing: Spacing.sm

        Label {
            text: qsTr("Ratio:")
            font: Typography.bodyMedium
            color: Theme.color("onSurfaceVariant")
            Layout.preferredWidth: 160
        }
        ComboBox {
            id: ratioCombo
            Layout.preferredWidth: 140
            model: [ qsTr("Default"), qsTr("Unlimited"), qsTr("Set to") ]
            onActivated: (i) => {
                root._updating = true
                root.ratioLimit = (i === 0) ? -2 : (i === 1 ? -1 : Math.max(0, ratioSpin.value / 100))
                root._updating = false
                root._emit()
            }
        }
        SpinBox {
            id: ratioSpin
            Layout.fillWidth: true
            enabled: ratioCombo.currentIndex === 2
            from: 0
            to: 100000000
            stepSize: 5   // 0.05 in hundredths
            textFromValue: (v) => (v / 100).toFixed(2)
            valueFromText: (t) => Math.round(parseFloat(t.replace(",", ".")) * 100) || 0
            onValueModified: {
                if (root._updating) return
                root._updating = true
                root.ratioLimit = value / 100
                root._updating = false
                root._emit()
            }
        }
    }

    // ---- Seeding time ------------------------------------------------------
    RowLayout {
        Layout.fillWidth: true
        spacing: Spacing.sm

        Label {
            text: qsTr("Seeding time:")
            font: Typography.bodyMedium
            color: Theme.color("onSurfaceVariant")
            Layout.preferredWidth: 160
        }
        ComboBox {
            id: seedingCombo
            Layout.preferredWidth: 140
            model: [ qsTr("Default"), qsTr("Unlimited"), qsTr("Set to") ]
            onActivated: (i) => {
                root._updating = true
                root.seedingTimeLimit = (i === 0) ? -2 : (i === 1 ? -1 : seedingSpin.value)
                root._updating = false
                root._emit()
            }
        }
        SpinBox {
            id: seedingSpin
            Layout.fillWidth: true
            enabled: seedingCombo.currentIndex === 2
            from: 0
            to: 9999999
            value: 1440
            textFromValue: (v) => v + " " + qsTr("min")
            valueFromText: (t) => parseInt(t.replace(/[^0-9]/g, "")) || 0
            onValueModified: {
                if (root._updating) return
                root._updating = true
                root.seedingTimeLimit = value
                root._updating = false
                root._emit()
            }
        }
    }

    // ---- Inactive seeding time --------------------------------------------
    RowLayout {
        Layout.fillWidth: true
        spacing: Spacing.sm

        Label {
            text: qsTr("Inactive seeding time:")
            font: Typography.bodyMedium
            color: Theme.color("onSurfaceVariant")
            Layout.preferredWidth: 160
        }
        ComboBox {
            id: inactiveCombo
            Layout.preferredWidth: 140
            model: [ qsTr("Default"), qsTr("Unlimited"), qsTr("Set to") ]
            onActivated: (i) => {
                root._updating = true
                root.inactiveSeedingTimeLimit = (i === 0) ? -2 : (i === 1 ? -1 : inactiveSpin.value)
                root._updating = false
                root._emit()
            }
        }
        SpinBox {
            id: inactiveSpin
            Layout.fillWidth: true
            enabled: inactiveCombo.currentIndex === 2
            from: 0
            to: 9999999
            value: 1440
            textFromValue: (v) => v + " " + qsTr("min")
            valueFromText: (t) => parseInt(t.replace(/[^0-9]/g, "")) || 0
            onValueModified: {
                if (root._updating) return
                root._updating = true
                root.inactiveSeedingTimeLimit = value
                root._updating = false
                root._emit()
            }
        }
    }

    // ---- Mode --------------------------------------------------------------
    RowLayout {
        Layout.fillWidth: true
        spacing: Spacing.sm

        Label {
            text: qsTr("Mode:")
            font: Typography.bodyMedium
            color: Theme.color("onSurfaceVariant")
            Layout.preferredWidth: 160
        }
        ComboBox {
            id: modeCombo
            Layout.fillWidth: true
            model: [ qsTr("Default"), qsTr("Match any limit"), qsTr("Match all the limits") ]
            onActivated: (i) => {
                root._updating = true
                root.mode = root._indexToMode[i]
                root._updating = false
                root._emit()
            }
        }
    }

    // ---- Action when reached ----------------------------------------------
    RowLayout {
        Layout.fillWidth: true
        spacing: Spacing.sm

        Label {
            text: qsTr("Action when the limit is reached:")
            font: Typography.bodyMedium
            color: Theme.color("onSurfaceVariant")
            Layout.preferredWidth: 160
            wrapMode: Text.WordWrap
        }
        ComboBox {
            id: actionCombo
            Layout.fillWidth: true
            model: [
                qsTr("Default"),
                qsTr("Stop torrent"),
                qsTr("Remove torrent"),
                qsTr("Remove torrent and its content"),
                qsTr("Enable super seeding for torrent")
            ]
            onActivated: (i) => {
                root._updating = true
                root.action = root._indexToAction[i]
                root._updating = false
                root._emit()
            }
        }
    }
}
