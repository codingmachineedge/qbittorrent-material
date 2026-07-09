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
import qBittorrent

/*!
    \qmltype Snackbar
    \brief Transient bottom notification (Material elevation 6) with an optional
           action button and a small queue.

    A single instance lives in \c Main.qml; teams call \c {show(text)} (or
    \c {show(text, actionText, callback)}) to surface a message — e.g. when a
    background task finishes while its tab is unfocused. Messages queue and are
    shown one at a time.
*/
Popup {
    id: root

    /*! Default auto-dismiss timeout in milliseconds. */
    property int defaultTimeout: 4000

    /*! Bottom margin from the parent's bottom edge. */
    property int bottomOffset: Spacing.xl

    // Pending messages: { text, actionText, callback, timeout }.
    property var _queue: []
    property string _text: ""
    property string _actionText: ""
    property var _callback: null

    /*!
        Show a message. \a text is already translated. Optional \a actionText
        (translated) renders an action button that invokes \a callback.
    */
    function show(text, actionText, callback) {
        _queue.push({
            "text": text,
            "actionText": actionText !== undefined ? actionText : "",
            "callback": callback !== undefined ? callback : null,
            "timeout": defaultTimeout
        })
        Log.info("ui", "Snackbar queued: " + text)
        if (!visible)
            _next()
    }

    function _next() {
        if (_queue.length === 0)
            return
        var msg = _queue.shift()
        _text = msg.text
        _actionText = msg.actionText
        _callback = msg.callback
        dismissTimer.interval = msg.timeout
        open()
        dismissTimer.restart()
    }

    modal: false
    closePolicy: Popup.NoAutoClose
    padding: 0
    Material.elevation: 6

    parent: Overlay.overlay
    x: parent ? (parent.width - width) / 2 : 0
    y: parent ? (parent.height - height - bottomOffset) : 0
    width: Math.min(560, (parent ? parent.width : 560) - Spacing.xl * 2)

    enter: Transition { NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 150 } }
    exit: Transition { NumberAnimation { property: "opacity"; from: 1; to: 0; duration: 150 } }

    onClosed: {
        // Chain to the next queued message, if any.
        if (_queue.length > 0)
            Qt.callLater(_next)
    }

    Timer {
        id: dismissTimer
        onTriggered: {
            Log.trace("ui", "Snackbar auto-dismissed")
            root.close()
        }
    }

    background: Rectangle {
        radius: Spacing.radiusChip
        color: Theme.color("onSurface")
    }

    contentItem: Row {
        spacing: Spacing.md
        leftPadding: Spacing.lg
        rightPadding: Spacing.sm
        topPadding: Spacing.sm
        bottomPadding: Spacing.sm

        Label {
            text: root._text
            font: Typography.bodyMedium
            color: Theme.color("surface")
            wrapMode: Text.WordWrap
            verticalAlignment: Text.AlignVCenter
            width: Math.min(implicitWidth, root.width - actionButton.width - Spacing.lg - Spacing.md * 2)
            anchors.verticalCenter: parent.verticalCenter
        }

        Button {
            id: actionButton
            visible: root._actionText.length > 0
            text: root._actionText
            flat: true
            Material.foreground: Theme.color("primary")
            anchors.verticalCenter: parent.verticalCenter
            onClicked: {
                Log.info("ui", "Snackbar action invoked: " + root._actionText)
                if (root._callback)
                    root._callback()
                root.close()
            }
        }
    }
}
