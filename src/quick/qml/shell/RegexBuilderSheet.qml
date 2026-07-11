/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import qBittorrent

/*!
    \qmltype RegexBuilderSheet
    \brief The design's Regex Builder: a /pattern/flags field with a validity
           dot, i/g/m flag chips, token + preset chips, a live test against
           the current torrent names, a saved-pattern library, and
           "Apply to search" which drives the transfers filter proxy.
*/
Sheet {
    id: root
    sheetWidth: 560

    required property var filterProxy

    signal closeRequested()

    property string pattern: "(19|20)\\d{2}"
    property bool flagI: true
    property bool flagG: true
    property bool flagM: false
    property var library: [
        { name: qsTr("Episode"), pat: "S\\d{2}E\\d{2}" },
        { name: qsTr("Resolution"), pat: "(1080|2160)p" }
    ]
    property string libName: ""

    readonly property string flagStr: (flagI ? "i" : "") + (flagG ? "g" : "") + (flagM ? "m" : "")
    readonly property bool patternValid: {
        if (pattern.length === 0) return true
        try { new RegExp(pattern, flagStr || undefined); return true } catch (e) { return false }
    }

    // Sample names come from the live transfer model (first 6 rows).
    readonly property var sampleNames: {
        var names = []
        var model = TransferListModel
        var n = Math.min(6, model.count || 0)
        for (var i = 0; i < n; ++i) {
            var idx = model.index(i, 0)
            var nm = model.data(idx, TransferListModel.NameRole)
            if (nm) names.push(String(nm))
        }
        if (names.length === 0)
            names = ["ubuntu-24.04.2-desktop-amd64.iso", "debian-13.1.0-amd64-DVD-1.iso",
                     "Fedora-Workstation-Live-x86_64-42-1.6.iso", "archlinux-2026.07.01-x86_64.iso"]
        return names
    }

    function appendToken(tok) { root.pattern += tok }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header.
        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 16
            Layout.leftMargin: 20
            Layout.rightMargin: 16
            Layout.bottomMargin: 8
            spacing: 8
            MDIcon { name: "data_object"; size: 20; color: Theme.color("primary") }
            Text {
                text: qsTr("Regex Builder")
                font.family: Typography.family
                font.pixelSize: 16
                font.weight: Font.DemiBold
                color: Theme.color("onSurface")
            }
            Item { Layout.fillWidth: true }
            HeaderIconButton {
                Layout.preferredWidth: 34; Layout.preferredHeight: 34
                iconName: "close"; iconSize: 19; iconColor: Theme.color("onSurfaceVariant")
                tooltip: qsTr("Close")
                onClicked: root.closeRequested()
            }
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: bodyColumn.implicitHeight
            clip: true
            boundsBehavior: Flickable.StopAtBounds
            ScrollBar.vertical: ScrollBar { }

            ColumnLayout {
                id: bodyColumn
                width: parent.width
                spacing: 14

                // Pattern + flags.
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.topMargin: 8
                    spacing: 8

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 44
                        radius: 12
                        color: Theme.color("surfaceVariant")
                        border.width: 1
                        border.color: (root.pattern.length === 0)
                            ? Theme.color("outlineVariant")
                            : (root.patternValid ? Theme.color("outlineVariant") : Theme.color("error"))

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 14
                            anchors.rightMargin: 14
                            spacing: 10
                            Rectangle {
                                Layout.preferredWidth: 8; Layout.preferredHeight: 8; radius: 4
                                color: (root.pattern.length === 0) ? Theme.color("outline")
                                    : (root.patternValid ? Theme.color("success") : Theme.color("error"))
                            }
                            Text { text: "/"; font.family: Typography.monoFamily; font.pixelSize: 14; color: Theme.color("onSurfaceVariant") }
                            TextInput {
                                id: patternInput
                                Layout.fillWidth: true
                                text: root.pattern
                                onTextEdited: root.pattern = text
                                font.family: Typography.monoFamily
                                font.pixelSize: 14
                                color: Theme.color("onSurface")
                                clip: true
                                selectByMouse: true
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: patternInput.text.length === 0
                                    text: qsTr("pattern")
                                    font: patternInput.font
                                    color: Theme.color("onSurfaceVariant")
                                }
                            }
                            Text {
                                text: "/" + root.flagStr
                                font.family: Typography.monoFamily; font.pixelSize: 14
                                color: Theme.color("onSurfaceVariant")
                            }
                        }
                    }

                    Repeater {
                        model: [
                            { key: "i", tip: qsTr("ignore case") },
                            { key: "g", tip: qsTr("global") },
                            { key: "m", tip: qsTr("multiline") }
                        ]
                        delegate: Rectangle {
                            required property var modelData
                            readonly property bool on: (modelData.key === "i" && root.flagI)
                                || (modelData.key === "g" && root.flagG)
                                || (modelData.key === "m" && root.flagM)
                            Layout.preferredWidth: 34; Layout.preferredHeight: 34
                            radius: 10
                            color: on ? Theme.color("primaryContainer") : "transparent"
                            border.width: 1
                            border.color: on ? Theme.color("primaryContainer") : Theme.color("outline")
                            Text {
                                anchors.centerIn: parent
                                text: modelData.key
                                font.family: Typography.monoFamily; font.pixelSize: 13; font.weight: Font.Bold
                                color: parent.on ? Theme.color("onPrimaryContainer") : Theme.color("onSurfaceVariant")
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (modelData.key === "i") root.flagI = !root.flagI
                                    else if (modelData.key === "g") root.flagG = !root.flagG
                                    else root.flagM = !root.flagM
                                }
                            }
                        }
                    }
                }

                // Token chip groups.
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    spacing: 8
                    Repeater {
                        model: [
                            { label: qsTr("Anchors"), tokens: ["^", "$", "\\b"] },
                            { label: qsTr("Classes"), tokens: [".", "\\d", "\\w", "\\s", "[a-z]", "[^ ]"] },
                            { label: qsTr("Quantifiers"), tokens: ["*", "+", "?", "{2,4}"] },
                            { label: qsTr("Groups"), tokens: ["( )", "(?: )", "|", "\\."] }
                        ]
                        delegate: RowLayout {
                            required property var modelData
                            Layout.fillWidth: true
                            spacing: 8
                            Text {
                                Layout.preferredWidth: 84
                                text: modelData.label.toUpperCase()
                                font.family: Typography.family; font.pixelSize: 11; font.weight: Font.Bold
                                font.letterSpacing: 0.8
                                color: Theme.color("onSurfaceVariant")
                            }
                            Flow {
                                Layout.fillWidth: true
                                spacing: 8
                                Repeater {
                                    model: modelData.tokens
                                    delegate: Rectangle {
                                        required property string modelData
                                        height: 28
                                        width: tkLabel.implicitWidth + 20
                                        radius: 8
                                        color: Theme.color("surfaceVariant")
                                        border.width: 1
                                        border.color: Theme.color("outlineVariant")
                                        Text {
                                            id: tkLabel
                                            anchors.centerIn: parent
                                            text: modelData
                                            font.family: Typography.monoFamily; font.pixelSize: 12
                                            color: Theme.color("onSurface")
                                        }
                                        MouseArea {
                                            anchors.fill: parent
                                            cursorShape: Qt.PointingHandCursor
                                            onClicked: root.appendToken(modelData.replace(/ /g, ""))
                                        }
                                    }
                                }
                            }
                        }
                    }

                    // Presets.
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 84
                            text: qsTr("PRESETS")
                            font.family: Typography.family; font.pixelSize: 11; font.weight: Font.Bold
                            font.letterSpacing: 0.8
                            color: Theme.color("onSurfaceVariant")
                        }
                        Flow {
                            Layout.fillWidth: true
                            spacing: 8
                            Repeater {
                                model: [
                                    { label: qsTr("Episode"), pat: "S\\d{2}E\\d{2}" },
                                    { label: qsTr("Resolution"), pat: "(1080|2160)p" },
                                    { label: qsTr("Year"), pat: "(19|20)\\d{2}" },
                                    { label: qsTr("Arch"), pat: "(amd64|x86_64|arm64)" },
                                    { label: qsTr("ISO"), pat: "\\.iso$" }
                                ]
                                delegate: Rectangle {
                                    required property var modelData
                                    height: 28
                                    width: presetLabel.implicitWidth + 22
                                    radius: 14
                                    color: Theme.color("primaryContainer")
                                    Text {
                                        id: presetLabel
                                        anchors.centerIn: parent
                                        text: modelData.label
                                        font.family: Typography.family; font.pixelSize: 12; font.weight: Font.DemiBold
                                        color: Theme.color("onPrimaryContainer")
                                    }
                                    MouseArea {
                                        anchors.fill: parent
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: root.pattern = modelData.pat
                                    }
                                }
                            }
                        }
                    }
                }

                // Live test.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.preferredHeight: testColumn.implicitHeight + 24
                    radius: 14
                    color: Theme.color("surfaceVariant")
                    ColumnLayout {
                        id: testColumn
                        anchors.fill: parent
                        anchors.margins: 12
                        spacing: 8
                        RowLayout {
                            spacing: 8
                            Text {
                                text: qsTr("LIVE TEST")
                                font.family: Typography.family; font.pixelSize: 11; font.weight: Font.Bold
                                font.letterSpacing: 0.8
                                color: Theme.color("onSurfaceVariant")
                            }
                            Text {
                                text: root.pattern.length === 0 ? qsTr("type or compose a pattern")
                                    : (root.patternValid ? qsTr("%1 of %2 sample names match").arg(root.matchCount).arg(root.sampleNames.length)
                                    : qsTr("invalid pattern"))
                                font.family: Typography.family; font.pixelSize: 12; font.weight: Font.DemiBold
                                color: root.pattern.length === 0 ? Theme.color("onSurfaceVariant")
                                    : (root.patternValid ? Theme.color("success") : Theme.color("error"))
                            }
                        }
                        Repeater {
                            model: root.sampleNames
                            delegate: RowLayout {
                                required property string modelData
                                readonly property bool hit: {
                                    if (!root.patternValid || root.pattern.length === 0) return false
                                    try { return new RegExp(root.pattern, root.flagI ? "i" : "").test(modelData) }
                                    catch (e) { return false }
                                }
                                spacing: 8
                                MDIcon {
                                    name: hit ? "check_circle" : "radio_button_unchecked"
                                    size: 15
                                    color: hit ? Theme.color("success") : Theme.color("outline")
                                }
                                Text {
                                    Layout.fillWidth: true
                                    text: modelData
                                    elide: Text.ElideRight
                                    font.family: Typography.monoFamily; font.pixelSize: 12
                                    color: hit ? Theme.color("onSurface") : Theme.color("onSurfaceVariant")
                                }
                            }
                        }
                    }
                }

                // Library.
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.bottomMargin: 12
                    spacing: 8
                    Text {
                        text: qsTr("LIBRARY")
                        font.family: Typography.family; font.pixelSize: 11; font.weight: Font.Bold
                        font.letterSpacing: 0.8
                        color: Theme.color("onSurfaceVariant")
                    }
                    Flow {
                        Layout.fillWidth: true
                        spacing: 6
                        Repeater {
                            model: root.library
                            delegate: Rectangle {
                                required property var modelData
                                required property int index
                                height: 30
                                width: libRow.implicitWidth + 8
                                radius: 15
                                color: "transparent"
                                border.width: 1
                                border.color: Theme.color("outline")
                                Row {
                                    id: libRow
                                    anchors.centerIn: parent
                                    spacing: 6
                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        leftPadding: 6
                                        text: modelData.name
                                        font.family: Typography.family; font.pixelSize: 12; font.weight: Font.DemiBold
                                        color: Theme.color("onSurface")
                                        MouseArea { anchors.fill: parent; cursorShape: Qt.PointingHandCursor; onClicked: root.pattern = modelData.pat }
                                    }
                                    Rectangle {
                                        anchors.verticalCenter: parent.verticalCenter
                                        width: 22; height: 22; radius: 11
                                        color: rmMouse.containsMouse ? Theme.color("hoverStrong") : "transparent"
                                        MDIcon { anchors.centerIn: parent; name: "close"; size: 14; color: Theme.color("onSurfaceVariant") }
                                        MouseArea {
                                            id: rmMouse
                                            anchors.fill: parent; hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                                            onClicked: root.library = root.library.filter(function(_, j) { return j !== index })
                                        }
                                    }
                                }
                            }
                        }
                        Rectangle {
                            height: 30; width: 150; radius: 15
                            color: "transparent"
                            border.width: 1; border.color: Theme.color("outlineVariant")
                            TextInput {
                                id: libNameInput
                                anchors.fill: parent
                                anchors.leftMargin: 12
                                anchors.rightMargin: 12
                                verticalAlignment: TextInput.AlignVCenter
                                text: root.libName
                                onTextEdited: root.libName = text
                                font.family: Typography.family; font.pixelSize: 12
                                color: Theme.color("onSurface")
                                clip: true
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    visible: libNameInput.text.length === 0
                                    text: qsTr("name this pattern")
                                    font: libNameInput.font
                                    color: Theme.color("onSurfaceVariant")
                                }
                            }
                        }
                        Rectangle {
                            height: 30; width: saveLabel.implicitWidth + 26; radius: 15
                            color: "transparent"
                            border.width: 1; border.color: Theme.color("outline")
                            Text {
                                id: saveLabel
                                anchors.centerIn: parent
                                text: qsTr("Save")
                                font.family: Typography.family; font.pixelSize: 12; font.weight: Font.DemiBold
                                color: Theme.color("primary")
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    if (root.pattern.length === 0) return
                                    var name = root.libName.length > 0 ? root.libName : qsTr("pattern %1").arg(root.library.length + 1)
                                    root.library = root.library.concat([{ name: name, pat: root.pattern }])
                                    root.libName = ""
                                }
                            }
                        }
                    }
                }
            }
        }

        // Footer: apply.
        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 68
            color: "transparent"
            Rectangle { anchors.top: parent.top; width: parent.width; height: 1; color: Theme.color("outlineVariant") }
            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 20
                anchors.rightMargin: 20
                spacing: 10
                Text {
                    Layout.fillWidth: true
                    text: qsTr("Applies to the transfers search")
                    font.family: Typography.family; font.pixelSize: 12
                    color: Theme.color("onSurfaceVariant")
                }
                Rectangle {
                    Layout.preferredHeight: 40
                    width: applyRow.implicitWidth + 40
                    radius: 20
                    color: Theme.color("primary")
                    opacity: root.patternValid ? 1 : 0.5
                    Row {
                        id: applyRow
                        anchors.centerIn: parent
                        spacing: 6
                        MDIcon { anchors.verticalCenter: parent.verticalCenter; name: "check"; size: 18; color: Theme.color("onPrimary") }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Apply to search")
                            font.family: Typography.family; font.pixelSize: 14; font.weight: Font.DemiBold
                            color: Theme.color("onPrimary")
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        enabled: root.patternValid
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (root.filterProxy) {
                                root.filterProxy.useRegex = true
                                root.filterProxy.textFilter = root.pattern
                            }
                            root.closeRequested()
                        }
                    }
                }
            }
        }
    }

    // Live-test match count.
    readonly property int matchCount: {
        if (!patternValid || pattern.length === 0) return 0
        var re
        try { re = new RegExp(pattern, flagI ? "i" : "") } catch (e) { return 0 }
        var c = 0
        for (var i = 0; i < sampleNames.length; ++i)
            if (re.test(sampleNames[i])) c++
        return c
    }
}
