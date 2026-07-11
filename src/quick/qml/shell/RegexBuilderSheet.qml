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
    \brief A full Regex Builder: pattern field with a validity dot, i/g/m/s/u
           flag chips, a grouped token palette (classes, anchors, quantifiers,
           groups & lookaround, escapes), one-click presets, a live test that
           highlights matches in the real torrent names, an optional
           replace/substitution preview, a plain-English explanation, a
           persisted saved-pattern library, copy, and "Apply to search".
*/
Sheet {
    id: root
    sheetWidth: 580

    required property var filterProxy

    signal closeRequested()

    property string pattern: "(19|20)\\d{2}"
    property string replacement: ""
    property bool flagI: true
    property bool flagG: true
    property bool flagM: false
    property bool flagS: false
    property bool flagU: false
    property var library: []
    property string libName: ""

    readonly property string flagStr: (flagI ? "i" : "") + (flagG ? "g" : "")
        + (flagM ? "m" : "") + (flagS ? "s" : "") + (flagU ? "u" : "")
    readonly property bool patternValid: {
        if (pattern.length === 0) return true
        try { new RegExp(pattern, flagStr || undefined); return true } catch (e) { return false }
    }

    // Sample names come from the live transfer model (first 6 rows), with a
    // static fallback so the tester is useful on an empty session.
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
                     "Fedora-Workstation-Live-x86_64-42-1.6.iso", "archlinux-2026.07.01-x86_64.iso",
                     "kali-linux-2026.2-installer-amd64.iso"]
        return names
    }

    readonly property var testRows: {
        var rows = []
        var re = null
        if (patternValid && pattern.length > 0) {
            try { re = new RegExp(pattern, "g" + (flagI ? "i" : "") + (flagS ? "s" : "") + (flagU ? "u" : "")) }
            catch (e) { re = null }
        }
        for (var i = 0; i < sampleNames.length; ++i) {
            var name = sampleNames[i]
            rows.push(root.buildRow(name, re))
        }
        return rows
    }
    readonly property int matchCount: {
        var c = 0
        for (var i = 0; i < testRows.length; ++i) if (testRows[i].hit) c++
        return c
    }

    // Split a name into matched/unmatched segments for highlighting, and
    // compute its substitution if a replacement is set.
    function buildRow(name, re) {
        if (!re) return { hit: false, segs: [{ text: name, hit: false }], replaced: name }
        var segs = []
        var last = 0
        var hitAny = false
        var m
        re.lastIndex = 0
        var guard = 0
        while ((m = re.exec(name)) !== null && guard < 200) {
            guard++
            if (m.index > last) segs.push({ text: name.slice(last, m.index), hit: false })
            segs.push({ text: m[0] || "∅", hit: true })
            last = m.index + (m[0].length || 1)
            if (m[0].length === 0) re.lastIndex++
            hitAny = true
        }
        if (last < name.length) segs.push({ text: name.slice(last), hit: false })
        if (segs.length === 0) segs = [{ text: name, hit: false }]
        var replaced = name
        if (root.replacement.length > 0) {
            try { replaced = name.replace(new RegExp(pattern, root.flagStr || undefined), root.replacement) }
            catch (e) { replaced = name }
        }
        return { hit: hitAny, segs: segs, replaced: replaced }
    }

    function explain() {
        if (pattern.length === 0) return qsTr("Type or compose a pattern to see what it matches.")
        if (!patternValid) return qsTr("This pattern is not valid — check the highlighted brackets.")
        var parts = []
        var rules = [
            [/^\^/, qsTr("anchored to the start")],
            [/\$$/, qsTr("anchored to the end")],
            [/\(\?<[^>]+>/, qsTr("a named group")],
            [/\(\?:/, qsTr("a non-capturing group")],
            [/\(\?=/, qsTr("a positive lookahead")],
            [/\(\?!/, qsTr("a negative lookahead")],
            [/\(\?<=/, qsTr("a positive lookbehind")],
            [/\((?!\?)/, qsTr("a capture group")],
            [/\|/, qsTr("alternatives (or)")],
            [/\\d/, qsTr("digits")],
            [/\\w/, qsTr("word characters")],
            [/\\s/, qsTr("whitespace")],
            [/\\b/, qsTr("word boundaries")],
            [/\[[^\]]*-[^\]]*\]/, qsTr("a character range")],
            [/\{\d+,?\d*\}/, qsTr("a counted repetition")],
            [/\+/, qsTr("one or more")],
            [/\*/, qsTr("zero or more")],
            [/\?/, qsTr("an optional part")],
            [/\./, qsTr("any character")]
        ]
        for (var i = 0; i < rules.length; ++i)
            if (rules[i][0].test(pattern) && parts.indexOf(rules[i][1]) < 0) parts.push(rules[i][1])
        if (parts.length === 0) return qsTr("Matches the literal text '%1'.").arg(pattern)
        var trail = []
        if (flagI) trail.push(qsTr("case-insensitively"))
        if (flagG) trail.push(qsTr("every occurrence"))
        if (flagM) trail.push(qsTr("across lines"))
        return qsTr("Matches %1%2.").arg(parts.slice(0, 6).join(", "))
            .arg(trail.length ? " — " + trail.join(", ") : "")
    }

    function appendToken(tok) { root.pattern += tok }

    // --- Library persistence (survives restart via Preferences) --------------
    function loadLibrary() {
        var stored = Preferences.value("RegexBuilder/Library", "")
        if (stored && ("" + stored).length > 0) {
            try {
                var parsed = JSON.parse("" + stored)
                if (Array.isArray(parsed)) { root.library = parsed; return }
            } catch (e) { /* fall through to defaults */ }
        }
        root.library = [
            { name: qsTr("Episode"), pat: "S\\d{2}E\\d{2}" },
            { name: qsTr("Resolution"), pat: "(1080|2160)p" }
        ]
    }
    function saveLibrary() {
        Preferences.setValue("RegexBuilder/Library", JSON.stringify(root.library))
        Preferences.apply()
    }

    Component.onCompleted: root.loadLibrary()

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
                spacing: 12

                // Pattern + validity dot.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.topMargin: 8
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

                // Flags.
                RowLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    spacing: 6
                    Text {
                        text: qsTr("FLAGS")
                        font.family: Typography.family; font.pixelSize: 11; font.weight: Font.Bold
                        font.letterSpacing: 0.8
                        color: Theme.color("onSurfaceVariant")
                    }
                    Repeater {
                        model: [
                            { key: "i", label: qsTr("ignore case"), get: function() { return root.flagI }, set: function(v) { root.flagI = v } },
                            { key: "g", label: qsTr("global"), get: function() { return root.flagG }, set: function(v) { root.flagG = v } },
                            { key: "m", label: qsTr("multiline"), get: function() { return root.flagM }, set: function(v) { root.flagM = v } },
                            { key: "s", label: qsTr("dotall"), get: function() { return root.flagS }, set: function(v) { root.flagS = v } },
                            { key: "u", label: qsTr("unicode"), get: function() { return root.flagU }, set: function(v) { root.flagU = v } }
                        ]
                        delegate: Rectangle {
                            required property var modelData
                            readonly property bool on: modelData.get()
                            Layout.preferredHeight: 30
                            implicitWidth: flagRow.implicitWidth + 20
                            radius: 15
                            color: on ? Theme.color("primaryContainer") : "transparent"
                            border.width: 1
                            border.color: on ? Theme.color("primaryContainer") : Theme.color("outline")
                            Row {
                                id: flagRow
                                anchors.centerIn: parent
                                spacing: 4
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.key
                                    font.family: Typography.monoFamily; font.pixelSize: 13; font.weight: Font.Bold
                                    color: parent.parent.on ? Theme.color("onPrimaryContainer") : Theme.color("onSurfaceVariant")
                                }
                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    text: modelData.label
                                    font.family: Typography.family; font.pixelSize: 11
                                    color: parent.parent.on ? Theme.color("onPrimaryContainer") : Theme.color("onSurfaceVariant")
                                }
                            }
                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: modelData.set(!modelData.get())
                            }
                        }
                    }
                }

                // Plain-English explainer.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.preferredHeight: explainCol.implicitHeight + 20
                    radius: 10
                    color: Theme.color("surfaceVariant")
                    border.width: 1
                    border.color: Theme.color("outlineVariant")
                    Column {
                        id: explainCol
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 3
                        Text {
                            text: qsTr("EXPLANATION")
                            font.family: Typography.family; font.pixelSize: 11; font.weight: Font.Bold
                            font.letterSpacing: 0.8
                            color: Theme.color("onSurfaceVariant")
                        }
                        Text {
                            width: parent.width
                            text: root.explain()
                            wrapMode: Text.WordWrap
                            font.family: Typography.family; font.pixelSize: 13
                            color: Theme.color("onSurface")
                        }
                    }
                }

                // Token groups.
                Repeater {
                    model: [
                        { label: qsTr("Classes"), tokens: [".", "\\d", "\\w", "\\s", "\\D", "\\W", "[a-z]", "[^ ]"] },
                        { label: qsTr("Anchors"), tokens: ["^", "$", "\\b", "\\B"] },
                        { label: qsTr("Quantifiers"), tokens: ["*", "+", "?", "{2,4}", "*?"] },
                        { label: qsTr("Groups & lookaround"), tokens: ["( )", "(?: )", "(?<n> )", "|", "(?= )", "(?! )", "(?<= )"] },
                        { label: qsTr("Escapes"), tokens: ["\\.", "\\/", "\\t", "\\n"] }
                    ]
                    delegate: RowLayout {
                        required property var modelData
                        Layout.fillWidth: true
                        Layout.leftMargin: 20
                        Layout.rightMargin: 20
                        spacing: 8
                        Text {
                            Layout.preferredWidth: 96
                            Layout.alignment: Qt.AlignTop
                            topPadding: 6
                            text: modelData.label.toUpperCase()
                            font.family: Typography.family; font.pixelSize: 11; font.weight: Font.Bold
                            font.letterSpacing: 0.6
                            color: Theme.color("onSurfaceVariant")
                        }
                        Flow {
                            Layout.fillWidth: true
                            spacing: 6
                            Repeater {
                                model: modelData.tokens
                                delegate: Rectangle {
                                    required property string modelData
                                    height: 28
                                    width: tkLabel.implicitWidth + 18
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
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    spacing: 8
                    Text {
                        Layout.preferredWidth: 96
                        text: qsTr("PRESETS")
                        font.family: Typography.family; font.pixelSize: 11; font.weight: Font.Bold
                        font.letterSpacing: 0.6
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
                                { label: qsTr("ISO"), pat: "\\.iso$" },
                                { label: qsTr("Version"), pat: "\\d+\\.\\d+(\\.\\d+)?" }
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

                // Live test — highlights matches inside the real torrent names.
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
                            model: root.testRows
                            delegate: RowLayout {
                                required property var modelData
                                Layout.fillWidth: true
                                spacing: 8
                                MDIcon {
                                    name: modelData.hit ? "check_circle" : "radio_button_unchecked"
                                    size: 15
                                    color: modelData.hit ? Theme.color("success") : Theme.color("outline")
                                }
                                // Highlighted segments.
                                Flow {
                                    Layout.fillWidth: true
                                    spacing: 0
                                    Repeater {
                                        model: modelData.segs
                                        delegate: Rectangle {
                                            required property var modelData
                                            width: segText.implicitWidth + (modelData.hit ? 4 : 0)
                                            height: segText.implicitHeight + 2
                                            radius: 3
                                            color: modelData.hit ? Theme.color("primaryContainer") : "transparent"
                                            Text {
                                                id: segText
                                                anchors.centerIn: parent
                                                text: modelData.text
                                                font.family: Typography.monoFamily; font.pixelSize: 12
                                                color: modelData.hit ? Theme.color("onPrimaryContainer") : Theme.color("onSurfaceVariant")
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                // Replace / substitution.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.preferredHeight: 44
                    radius: 12
                    color: Theme.color("surfaceVariant")
                    border.width: 1
                    border.color: Theme.color("outlineVariant")
                    RowLayout {
                        anchors.fill: parent
                        anchors.leftMargin: 14
                        anchors.rightMargin: 14
                        spacing: 10
                        MDIcon { name: "find_replace"; size: 18; color: Theme.color("onSurfaceVariant") }
                        TextInput {
                            id: replaceInput
                            Layout.fillWidth: true
                            text: root.replacement
                            onTextEdited: root.replacement = text
                            font.family: Typography.monoFamily; font.pixelSize: 13
                            color: Theme.color("onSurface")
                            clip: true
                            selectByMouse: true
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                visible: replaceInput.text.length === 0
                                text: qsTr("Replace with… (use $1, $2 for captures)")
                                font: replaceInput.font
                                color: Theme.color("onSurfaceVariant")
                            }
                        }
                    }
                }
                Rectangle {
                    visible: root.replacement.length > 0
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.preferredHeight: replaceCol.implicitHeight + 20
                    radius: 10
                    color: Theme.color("surfaceContainerHigh")
                    Column {
                        id: replaceCol
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 4
                        Text {
                            text: qsTr("REPLACEMENT RESULT")
                            font.family: Typography.family; font.pixelSize: 11; font.weight: Font.Bold
                            font.letterSpacing: 0.8
                            color: Theme.color("onSurfaceVariant")
                        }
                        Repeater {
                            model: root.testRows
                            delegate: Text {
                                required property var modelData
                                width: parent.width
                                text: modelData.replaced
                                elide: Text.ElideRight
                                font.family: Typography.monoFamily; font.pixelSize: 12
                                color: Theme.color("onSurface")
                            }
                        }
                    }
                }

                // Saved library.
                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.leftMargin: 20
                    Layout.rightMargin: 20
                    Layout.bottomMargin: 12
                    spacing: 8
                    Text {
                        text: qsTr("SAVED PATTERNS")
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
                                            onClicked: {
                                                root.library = root.library.filter(function(_, j) { return j !== index })
                                                root.saveLibrary()
                                            }
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
                                    root.saveLibrary()
                                }
                            }
                        }
                    }
                }
            }
        }

        // Footer: copy + apply.
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

                Rectangle {
                    Layout.preferredHeight: 40
                    width: copyRow.implicitWidth + 28
                    radius: 20
                    color: copyMouse.containsMouse ? Theme.color("hoverStrong") : "transparent"
                    border.width: 1
                    border.color: Theme.color("outline")
                    Row {
                        id: copyRow
                        anchors.centerIn: parent
                        spacing: 6
                        MDIcon { anchors.verticalCenter: parent.verticalCenter; name: "content_copy"; size: 17; color: Theme.color("onSurfaceVariant") }
                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Copy")
                            font.family: Typography.family; font.pixelSize: 13; font.weight: Font.DemiBold
                            color: Theme.color("onSurfaceVariant")
                        }
                    }
                    MouseArea {
                        id: copyMouse
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            clipboardHelper.text = "/" + root.pattern + "/" + root.flagStr
                            clipboardHelper.selectAll()
                            clipboardHelper.copy()
                        }
                    }
                }

                Item { Layout.fillWidth: true }

                Text {
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

    // Off-screen helper that owns the system clipboard copy for the Copy button.
    TextInput {
        id: clipboardHelper
        visible: false
        width: 0; height: 0
    }
}
