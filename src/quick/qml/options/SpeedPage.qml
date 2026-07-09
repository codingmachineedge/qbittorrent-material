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
    \qmltype SpeedPage
    \brief Options → Speed page (legacy TAB_SPEED).

    Global rate limits, alternative rate limits, the alternative-limit schedule
    (a checkable card with From/To time editors + day selector) and the
    rate-limit settings toggles.
*/
Flickable {
    id: root

    readonly property int rev: OptionsController.revision

    contentHeight: layout.implicitHeight + (2 * Spacing.lg)
    clip: true
    boundsBehavior: Flickable.StopAtBounds
    ScrollBar.vertical: ScrollBar {}

    component OptCheck: CheckBox {
        property string settingKey: ""
        property bool defaultValue: false
        font: Typography.bodyMedium
        checked: (root.rev, OptionsController.value(settingKey, defaultValue))
        onToggled: {
            OptionsController.setValue(settingKey, checked)
            Log.debug("ui", "Speed: " + settingKey + " -> " + checked)
        }
    }

    // Upload/Download rate pair using SpeedSpinBox (0 == unlimited ∞).
    component RatePair: ColumnLayout {
        id: pair
        property string upKey: ""
        property string downKey: ""
        Layout.fillWidth: true
        spacing: Spacing.sm
        LabeledField {
            label: qsTr("Upload:")
            labelWidth: 140
            Layout.fillWidth: true
            SpeedSpinBox {
                to: 2000000
                unlimitedValue: 0
                value: (root.rev, OptionsController.value(pair.upKey, 0))
                onValueModified: OptionsController.setValue(pair.upKey, value)
            }
        }
        LabeledField {
            label: qsTr("Download:")
            labelWidth: 140
            Layout.fillWidth: true
            SpeedSpinBox {
                to: 2000000
                unlimitedValue: 0
                value: (root.rev, OptionsController.value(pair.downKey, 0))
                onValueModified: OptionsController.setValue(pair.downKey, value)
            }
        }
    }

    Component.onCompleted: Log.debug("ui", "SpeedPage ready")

    ColumnLayout {
        id: layout
        x: Spacing.lg
        y: Spacing.lg
        width: root.width - (2 * Spacing.lg)
        spacing: Spacing.lg

        // ==== Global Rate Limits ==============================================
        MaterialCard {
            title: qsTr("Global Rate Limits")
            titleIcon: Icons.speed
            Layout.fillWidth: true
            RatePair {
                upKey: "BitTorrent/Session/GlobalUPSpeedLimit"
                downKey: "BitTorrent/Session/GlobalDLSpeedLimit"
            }
        }

        // ==== Alternative Rate Limits =========================================
        MaterialCard {
            title: qsTr("Alternative Rate Limits")
            Layout.fillWidth: true
            RatePair {
                upKey: "BitTorrent/Session/AlternativeGlobalUPSpeedLimit"
                downKey: "BitTorrent/Session/AlternativeGlobalDLSpeedLimit"
            }

            CheckableGroupBox {
                title: qsTr("Schedule the use of alternative rate limits")
                Layout.fillWidth: true
                checked: (root.rev, OptionsController.value("BitTorrent/Session/BandwidthSchedulerEnabled", false))
                onToggled: (v) => OptionsController.setValue("BitTorrent/Session/BandwidthSchedulerEnabled", v)

                LabeledField {
                    label: qsTr("From:")
                    labelWidth: 100
                    Layout.fillWidth: true
                    // Minutes-since-midnight stored; edited as hh:mm.
                    SpinBox {
                        id: fromTime
                        from: 0; to: 1439; editable: true
                        value: (root.rev, OptionsController.value("Preferences/Scheduler/start_time", 480))
                        textFromValue: (v) => root._fmtTime(v)
                        valueFromText: (t) => root._parseTime(t)
                        onValueModified: OptionsController.setValue("Preferences/Scheduler/start_time", value)
                    }
                }
                LabeledField {
                    label: qsTr("To:")
                    labelWidth: 100
                    Layout.fillWidth: true
                    SpinBox {
                        id: toTime
                        from: 0; to: 1439; editable: true
                        value: (root.rev, OptionsController.value("Preferences/Scheduler/end_time", 1200))
                        textFromValue: (v) => root._fmtTime(v)
                        valueFromText: (t) => root._parseTime(t)
                        onValueModified: OptionsController.setValue("Preferences/Scheduler/end_time", value)
                    }
                }
                LabeledField {
                    label: qsTr("When:")
                    labelWidth: 100
                    Layout.fillWidth: true
                    ComboBox {
                        Layout.fillWidth: true
                        model: [ qsTr("Every day"), qsTr("Weekdays"), qsTr("Weekends"),
                                 qsTr("Monday"), qsTr("Tuesday"), qsTr("Wednesday"), qsTr("Thursday"),
                                 qsTr("Friday"), qsTr("Saturday"), qsTr("Sunday") ]
                        currentIndex: (root.rev, OptionsController.value("Preferences/Scheduler/days", 0))
                        onActivated: (i) => OptionsController.setValue("Preferences/Scheduler/days", i)
                    }
                }
            }
        }

        // ==== Rate Limits Settings ============================================
        MaterialCard {
            title: qsTr("Rate Limits Settings")
            Layout.fillWidth: true
            OptCheck {
                text: qsTr("Apply rate limit to µTP protocol")
                settingKey: "BitTorrent/Session/uTPRateLimited"
                defaultValue: true
            }
            OptCheck {
                text: qsTr("Apply rate limit to transport overhead")
                settingKey: "BitTorrent/Session/IncludeOverheadInLimits"
                defaultValue: false
            }
            OptCheck {
                text: qsTr("Apply rate limit to peers on LAN")
                // Human-facing key; OptionsController maps it to the inverted
                // IgnoreLimitsOnLAN session value.
                settingKey: "Speed/ApplyLimitToLAN"
                defaultValue: false
            }
        }
    }

    // hh:mm helpers for the minutes-since-midnight schedule spins.
    function _fmtTime(minutes) {
        var h = Math.floor(minutes / 60)
        var m = minutes % 60
        return (h < 10 ? "0" + h : h) + ":" + (m < 10 ? "0" + m : m)
    }
    function _parseTime(text) {
        var parts = ("" + text).split(":")
        var h = parseInt(parts[0]) || 0
        var m = parts.length > 1 ? (parseInt(parts[1]) || 0) : 0
        return Math.max(0, Math.min(1439, (h * 60) + m))
    }
}
