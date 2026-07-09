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
    \qmltype SpeedPlotView
    \brief Live multi-series speed graph for the properties Speed tab.

    Rebuild of the legacy \c SpeedWidget + \c SpeedPlotView, backed entirely by
    the C++ \c SpeedPlotModel (\c PropertiesController.speedPlotModel). The model
    owns the multi-resolution averagers (5min/1s … 24h/144s) and persistence;
    this view is pure presentation:

    \list
      \li A \c Period combo selects the time window (writes \c model.period).
      \li A "Select Graphs" menu toggles each of the ten \c GraphId series
          (\c model.setGraphEnabled) with the model's own translated names.
      \li The plot + Y axis are drawn on a \c Canvas from \c model.seriesPoints(id)
          and \c model.yScale(); a repaint is scheduled on the model's
          \c updated() signal (never polled).
    \endlist
*/
Item {
    id: root

    readonly property var model: PropertiesController.speedPlotModel

    // Period combo index == SpeedPlotModel::Period enum order.
    readonly property var _periodNames: [
        qsTr("1 Minute"), qsTr("5 Minutes"), qsTr("30 Minutes"),
        qsTr("3 Hours"), qsTr("6 Hours"), qsTr("12 Hours"), qsTr("24 Hours")
    ]

    readonly property color _upColor: Theme.color("info")
    readonly property color _downColor: Theme.color("success")

    // Number of currently-enabled series (drives the legend visibility). Bumped
    // via _recomputeEnabled() whenever the enable set changes.
    property int _enabledCount: 0
    function _recomputeEnabled() {
        let n = 0
        if (model)
            for (let i = 0; i < model.graphCount(); ++i)
                if (model.isGraphEnabled(i)) ++n
        _enabledCount = n
    }

    // Series direction / dash family derived from the stable GraphId order.
    function _isUp(id) { return (id % 2) === 0 }
    function _dashForId(id) {
        switch (Math.floor(id / 2)) {
        case 0: return []              // total
        case 1: return [6, 4]          // payload
        case 2: return [8, 4, 2, 4]    // overhead
        case 3: return [8, 4, 2, 4, 2, 4] // dht
        case 4: return [2, 4]          // tracker
        }
        return []
    }

    // Repaint whenever the model has new data / configuration.
    Connections {
        target: root.model
        function onUpdated() { plot.requestPaint(); root._recomputeEnabled() }
        function onPeriodChanged() { plot.requestPaint() }
    }

    Component.onCompleted: {
        if (model)
            periodCombo.currentIndex = model.period
        _recomputeEnabled()
        Log.debug("ui", "SpeedPlotView ready; period index " + (model ? model.period : -1))
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: Spacing.sm

        RowLayout {
            Layout.fillWidth: true
            spacing: Spacing.sm

            Label {
                text: qsTr("Period:")
                font: Typography.titleSmall
                color: Theme.color("onSurface")
            }

            ComboBox {
                id: periodCombo
                model: root._periodNames
                Layout.preferredWidth: 160
                onActivated: (i) => {
                    Log.info("ui", "SpeedPlotView period -> index " + i)
                    if (root.model)
                        root.model.period = i
                    plot.requestPaint()
                }
            }

            Item { Layout.fillWidth: true }

            Button {
                id: graphsButton
                text: qsTr("Select Graphs")
                flat: true
                onClicked: {
                    Log.debug("ui", "SpeedPlotView graphs menu opened")
                    graphsMenu.popup(graphsButton, 0, graphsButton.height)
                }

                Menu {
                    id: graphsMenu
                    Material.elevation: Spacing.elevationMenu

                    Instantiator {
                        model: root.model ? root.model.graphCount() : 0
                        delegate: MenuItem {
                            required property int index
                            text: root.model ? root.model.graphName(index) : ""
                            checkable: true
                            checked: root.model ? root.model.isGraphEnabled(index) : false
                            onTriggered: {
                                if (root.model)
                                    root.model.setGraphEnabled(index, checked)
                                Log.info("ui", "SpeedPlotView graph " + index + " -> " + checked)
                                plot.requestPaint()
                                root._recomputeEnabled()
                            }
                        }
                        onObjectAdded: (index, object) => graphsMenu.insertItem(index, object)
                        onObjectRemoved: (index, object) => graphsMenu.removeItem(object)
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: Spacing.radiusCard
            color: Theme.color("surface")
            border.width: 1
            border.color: Theme.color("outlineVariant")
            clip: true

            Canvas {
                id: plot
                anchors.fill: parent
                anchors.margins: 1

                readonly property int padLeft: 72
                readonly property int padRight: 12
                readonly property int padTop: 12
                readonly property int padBottom: 22

                onWidthChanged: requestPaint()
                onHeightChanged: requestPaint()

                onPaint: {
                    const ctx = getContext("2d")
                    ctx.reset()
                    const model = root.model
                    const W = width, H = height
                    const plotX = padLeft, plotY = padTop
                    const plotW = Math.max(1, W - padLeft - padRight)
                    const plotH = Math.max(1, H - padTop - padBottom)

                    const gridColor = Qt.alpha(Theme.color("outline"), 0.5)
                    const axisText = Theme.color("onSurfaceVariant")

                    if (!model) {
                        return
                    }

                    const scale = model.yScale()
                    const yMax = (scale && scale.max > 0) ? scale.max : 1
                    const labels = (scale && scale.labels) ? scale.labels : []

                    // ---- Grid + Y labels (top -> bottom) ----
                    ctx.strokeStyle = gridColor
                    ctx.lineWidth = 1
                    ctx.setLineDash([4, 4])
                    ctx.fillStyle = axisText
                    ctx.font = "11px Roboto"
                    ctx.textAlign = "right"
                    ctx.textBaseline = "middle"
                    for (let q = 0; q <= 4; ++q) {
                        const yy = plotY + plotH * q / 4
                        ctx.beginPath()
                        ctx.moveTo(plotX, yy)
                        ctx.lineTo(plotX + plotW, yy)
                        ctx.stroke()
                        if (q < labels.length)
                            ctx.fillText(labels[q], plotX - 6, yy)
                    }
                    // Vertical time divisions (6).
                    for (let vdiv = 0; vdiv <= 6; ++vdiv) {
                        const xx = plotX + plotW * vdiv / 6
                        ctx.beginPath()
                        ctx.moveTo(xx, plotY)
                        ctx.lineTo(xx, plotY + plotH)
                        ctx.stroke()
                    }
                    ctx.setLineDash([])

                    // ---- Series lines ----
                    const count = model.graphCount()
                    for (let id = 0; id < count; ++id) {
                        if (!model.isGraphEnabled(id))
                            continue
                        const pts = model.seriesPoints(id)
                        if (!pts || pts.length === 0)
                            continue

                        ctx.strokeStyle = root._isUp(id) ? root._upColor : root._downColor
                        ctx.lineWidth = 1.5
                        ctx.setLineDash(root._dashForId(id))
                        ctx.beginPath()
                        let started = false
                        for (let i = 0; i < pts.length; ++i) {
                            const px = plotX + plotW * Math.max(0, Math.min(1, pts[i].x))
                            const frac = Math.max(0, Math.min(1, pts[i].y / yMax))
                            const py = plotY + plotH * (1 - frac)
                            if (!started) { ctx.moveTo(px, py); started = true }
                            else ctx.lineTo(px, py)
                        }
                        if (started) ctx.stroke()
                    }
                    ctx.setLineDash([])
                }
            }

            // ---- Legend overlay (top-left) ----
            Rectangle {
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.leftMargin: plot.padLeft + Spacing.sm
                anchors.topMargin: plot.padTop + Spacing.xs
                visible: root._enabledCount > 0
                color: Qt.alpha(Theme.color("surface"), 0.65)
                radius: Spacing.radiusChip
                width: legendCol.implicitWidth + Spacing.md
                height: legendCol.implicitHeight + Spacing.sm

                ColumnLayout {
                    id: legendCol
                    anchors.centerIn: parent
                    spacing: 2

                    Repeater {
                        id: legendRepeater
                        model: root.model ? root.model.graphCount() : 0
                        delegate: RowLayout {
                            required property int index
                            visible: root._enabledCount >= 0 && root.model ? root.model.isGraphEnabled(index) : false
                            spacing: Spacing.xs

                            Rectangle {
                                width: 16
                                height: 2
                                color: root._isUp(index) ? root._upColor : root._downColor
                                Layout.alignment: Qt.AlignVCenter
                            }
                            Label {
                                text: root.model ? root.model.graphName(index) : ""
                                font: Typography.labelSmall
                                color: Theme.color("onSurface")
                            }
                        }
                    }
                }
            }
        }
    }
}
