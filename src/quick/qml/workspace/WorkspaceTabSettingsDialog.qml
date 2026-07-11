/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

import QtQuick
import QtQuick.Controls.Material
import QtQuick.Layouts
import qBittorrent

Popup {
    id: root

    property int tabIndex: -1
    property string tabId: ""
    property color selectedColor: "#6750A4"
    property bool updatingColor: false
    property string errorText: ""

    objectName: "workspaceTabSettingsDialog"
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape
    padding: 0
    parent: Overlay.overlay
    anchors.centerIn: parent
    width: Math.min(760, (parent ? parent.width : 760) * 0.94)
    height: Math.min(690, (parent ? parent.height : 690) * 0.92)
    Material.elevation: 24

    component ColorSlider: RowLayout {
        property alias label: sliderLabel.text
        property alias value: control.value
        property alias from: control.from
        property alias to: control.to
        property alias stepSize: control.stepSize
        signal edited()
        Layout.fillWidth: true
        Label {
            id: sliderLabel
            Layout.preferredWidth: 84
            font: Typography.labelMedium
            color: Theme.color("onSurfaceVariant")
        }
        Slider {
            id: control
            Layout.fillWidth: true
            onMoved: edited()
        }
        Label {
            Layout.preferredWidth: 48
            horizontalAlignment: Text.AlignRight
            text: Math.round(control.value)
            font: Typography.labelMedium
            color: Theme.color("onSurface")
        }
    }

    background: Rectangle {
        radius: Spacing.radiusDialog
        color: Theme.color("surface")
    }

    function twoHex(value) {
        var result = Math.round(Math.max(0, Math.min(1, value)) * 255).toString(16).toUpperCase()
        return result.length < 2 ? "0" + result : result
    }

    function colorHex(colorValue) {
        return "#" + twoHex(colorValue.a) + twoHex(colorValue.r)
            + twoHex(colorValue.g) + twoHex(colorValue.b)
    }

    function applySelectedColor(colorValue) {
        updatingColor = true
        selectedColor = colorValue
        var normalizedColor = selectedColor
        hueRow.value = normalizedColor.hsvHue < 0 ? 0 : normalizedColor.hsvHue * 360
        saturationRow.value = normalizedColor.hsvSaturation * 1000
        valueRow.value = normalizedColor.hsvValue * 1000
        alphaRow.value = normalizedColor.a * 1000
        colorHexField.text = colorHex(normalizedColor)
        updatingColor = false
    }

    function updateColorFromSliders() {
        if (updatingColor)
            return
        errorText = ""
        selectedColor = Qt.hsva(hueRow.value / 360,
                                saturationRow.value / 1000,
                                valueRow.value / 1000,
                                alphaRow.value / 1000)
        colorHexField.text = colorHex(selectedColor)
    }

    function parseHex(value) {
        var text = value.trim().toUpperCase()
        if (text.length === 7 && text[0] === "#")
            text = "#FF" + text.substring(1)
        if (!/^#[0-9A-F]{8}$/.test(text))
            return null
        var alpha = parseInt(text.substring(1, 3), 16) / 255
        var red = parseInt(text.substring(3, 5), 16) / 255
        var green = parseInt(text.substring(5, 7), 16) / 255
        var blue = parseInt(text.substring(7, 9), 16) / 255
        return Qt.rgba(red, green, blue, alpha)
    }

    function openForIndex(index) {
        if (!WorkspaceManager.writable)
            return
        var tab = WorkspaceManager.tabAt(index)
        if (!tab || !tab.tabId)
            return
        tabIndex = index
        tabId = tab.tabId
        tabNameField.text = tab.name
        fontFamilyCombo.model = WorkspaceManager.fontFamilies()
        var familyIndex = fontFamilyCombo.find(tab.fontFamily)
        fontFamilyCombo.currentIndex = familyIndex
        if (familyIndex < 0)
            fontFamilyCombo.editText = tab.fontFamily
        fontStyleCombo.model = WorkspaceManager.fontStyles(tab.fontFamily)
        var styleIndex = fontStyleCombo.find(tab.fontStyle)
        fontStyleCombo.currentIndex = Math.max(0, styleIndex)
        fontSizeSpin.value = Math.round(tab.fontPointSize * 10)
        boldCheck.checked = tab.bold
        italicCheck.checked = tab.italic
        applySelectedColor(tab.fontColor)
        errorText = ""
        open()
    }

    function applyChanges() {
        var parsedColor = parseHex(colorHexField.text)
        if (parsedColor === null) {
            errorText = qsTr("Enter a color as #RRGGBB or #AARRGGBB.")
            colorHexField.forceActiveFocus()
            return
        }
        applySelectedColor(parsedColor)
        var family = fontFamilyCombo.currentText.trim()
        var style = fontStyleCombo.currentText.trim()
        var ok = WorkspaceManager.updateTab(tabId, tabNameField.text, family, style,
                                            fontSizeSpin.value / 10,
                                            boldCheck.checked, italicCheck.checked,
                                            colorHex(selectedColor))
        if (!ok) {
            errorText = qsTr("Check the name, font size, and color before applying.")
            return
        }
        close()
    }

    contentItem: ColumnLayout {
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: Spacing.xl
            Layout.rightMargin: Spacing.sm
            Layout.topMargin: Spacing.md
            Layout.bottomMargin: Spacing.md
            spacing: Spacing.md

            Rectangle {
                Layout.preferredWidth: 42
                Layout.preferredHeight: 42
                radius: 21
                color: Theme.color("primaryContainer")
                MDIcon {
                    anchors.centerIn: parent
                    icon: Icons.edit
                    size: 22
                    color: Theme.color("onPrimaryContainer")
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                spacing: 0
                Label {
                    text: qsTr("Tab name & appearance")
                    font: Typography.titleLarge
                    color: Theme.color("onSurface")
                }
                Label {
                    text: qsTr("Every setting belongs to this tab and is saved in its Git history.")
                    font: Typography.bodySmall
                    color: Theme.color("onSurfaceVariant")
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
            IconButton {
                symbol: Icons.close
                tooltip: qsTr("Close")
                onClicked: root.close()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.color("outlineVariant")
        }

        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true

            ColumnLayout {
                width: Math.max(0, root.width - Spacing.xl * 2)
                spacing: Spacing.lg

                Item { Layout.preferredHeight: Spacing.xs }

                TextField {
                    id: tabNameField
                    objectName: "workspaceTabNameField"
                    Accessible.name: qsTr("Tab name")
                    Layout.fillWidth: true
                    placeholderText: qsTr("Tab name")
                    selectByMouse: true
                    maximumLength: 120
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.width >= 620 ? 2 : 1
                    columnSpacing: Spacing.md
                    rowSpacing: Spacing.md

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Font family"); font: Typography.labelMedium }
                        ComboBox {
                            id: fontFamilyCombo
                            objectName: "workspaceFontFamilyCombo"
                            Accessible.name: qsTr("Font family")
                            Layout.fillWidth: true
                            editable: true
                            onActivated: {
                                fontStyleCombo.model = WorkspaceManager.fontStyles(currentText)
                                fontStyleCombo.currentIndex = 0
                            }
                            onAccepted: {
                                fontStyleCombo.model = WorkspaceManager.fontStyles(currentText)
                                fontStyleCombo.currentIndex = 0
                            }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Font style"); font: Typography.labelMedium }
                        ComboBox {
                            id: fontStyleCombo
                            objectName: "workspaceFontStyleCombo"
                            Accessible.name: qsTr("Font style")
                            Layout.fillWidth: true
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        Label { text: qsTr("Font size"); font: Typography.labelMedium }
                        SpinBox {
                            id: fontSizeSpin
                            objectName: "workspaceFontSizeSpinBox"
                            Accessible.name: qsTr("Font size in points")
                            Layout.fillWidth: true
                            from: 60
                            to: 1440
                            stepSize: 5
                            editable: true
                            textFromValue: function(value, locale) {
                                return Number(value / 10).toLocaleString(locale, "f", 1) + " pt"
                            }
                            valueFromText: function(text, locale) {
                                return Math.round(Number.fromLocaleString(locale,
                                    text.replace(/[^0-9.,-]/g, "")) * 10)
                            }
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        Layout.alignment: Qt.AlignBottom
                        CheckBox {
                            id: boldCheck
                            objectName: "workspaceBoldButton"
                            text: qsTr("Bold")
                            font.bold: true
                        }
                        CheckBox {
                            id: italicCheck
                            objectName: "workspaceItalicButton"
                            text: qsTr("Italic")
                            font.italic: true
                        }
                        Item { Layout.fillWidth: true }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: colorLayout.implicitHeight + Spacing.lg * 2
                    radius: Spacing.radiusCard
                    color: Theme.color("surfaceVariant")
                    border.width: 1
                    border.color: Theme.color("outlineVariant")

                    ColumnLayout {
                        id: colorLayout
                        anchors.fill: parent
                        anchors.margins: Spacing.lg
                        spacing: Spacing.sm

                        RowLayout {
                            Layout.fillWidth: true
                            Label {
                                text: qsTr("Unlimited font color")
                                font: Typography.titleSmall
                                color: Theme.color("onSurface")
                            }
                            Item { Layout.fillWidth: true }
                            Rectangle {
                                objectName: "workspaceColorButton"
                                Layout.preferredWidth: 48
                                Layout.preferredHeight: 32
                                radius: Spacing.radiusChip
                                color: root.selectedColor
                                border.width: 1
                                border.color: Theme.color("outline")
                                Accessible.name: qsTr("Selected font color")
                            }
                        }

                        ColorSlider {
                            id: hueRow
                            label: qsTr("Hue")
                            from: 0
                            to: 360
                            stepSize: 1
                            onEdited: root.updateColorFromSliders()
                        }
                        ColorSlider {
                            id: saturationRow
                            label: qsTr("Saturation")
                            from: 0
                            to: 1000
                            stepSize: 1
                            onEdited: root.updateColorFromSliders()
                        }
                        ColorSlider {
                            id: valueRow
                            label: qsTr("Value")
                            from: 0
                            to: 1000
                            stepSize: 1
                            onEdited: root.updateColorFromSliders()
                        }
                        ColorSlider {
                            id: alphaRow
                            label: qsTr("Alpha")
                            from: 0
                            to: 1000
                            stepSize: 1
                            onEdited: root.updateColorFromSliders()
                        }

                        TextField {
                            id: colorHexField
                            objectName: "workspaceColorHexField"
                            Accessible.name: qsTr("ARGB font color")
                            Layout.fillWidth: true
                            placeholderText: qsTr("#AARRGGBB or #RRGGBB")
                            maximumLength: 9
                            selectByMouse: true
                            onEditingFinished: {
                                var parsed = root.parseHex(text)
                                if (parsed === null) {
                                    root.errorText = qsTr("Enter a color as #RRGGBB or #AARRGGBB.")
                                    return
                                }
                                root.errorText = ""
                                root.applySelectedColor(parsed)
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    implicitHeight: preview.implicitHeight + Spacing.xl * 2
                    radius: Spacing.radiusCard
                    color: Theme.color("surface")
                    border.width: 1
                    border.color: Theme.color("outlineVariant")
                    Label {
                        id: preview
                        anchors.fill: parent
                        anchors.margins: Spacing.xl
                        text: qsTr("The quick brown fox · 0123456789")
                        textFormat: Text.PlainText
                        wrapMode: Text.WordWrap
                        color: root.selectedColor
                        font: WorkspaceManager.resolvedFont(fontFamilyCombo.currentText,
                            fontStyleCombo.currentText, fontSizeSpin.value / 10,
                            boldCheck.checked, italicCheck.checked)
                    }
                }

                Label {
                    text: root.errorText
                    visible: text.length > 0
                    color: Theme.color("error")
                    font: Typography.bodySmall
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }

                Item { Layout.preferredHeight: Spacing.xs }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 1
            color: Theme.color("outlineVariant")
        }

        DialogButtonBox {
            Layout.fillWidth: true
            Layout.margins: Spacing.md
            spacing: Spacing.sm
            padding: 0
            topPadding: Spacing.sm
            background: Rectangle { color: Theme.color("surface") }
            Button {
                text: qsTr("Cancel")
                flat: true
                DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
                onClicked: root.close()
            }
            Button {
                objectName: "workspaceSettingsApplyButton"
                text: qsTr("Apply")
                highlighted: true
                enabled: WorkspaceManager.writable
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: root.applyChanges()
            }
        }
    }
}
