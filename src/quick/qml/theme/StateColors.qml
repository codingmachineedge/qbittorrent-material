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

pragma Singleton

import QtQuick
import qBittorrent

/*!
    \qmltype StateColors
    \brief Semantic state -> color mapping used across the app.

    Backed by \c Theme (all values resolve through \c Theme.color so user
    overrides + the named-id map still apply). Three independent visual channels
    (row text, progress bar, state icon) are gated by different preferences and
    must never be coupled — this singleton only owns the semantic COLOR, callers
    decide which channel to apply it to.

    \code
    // transfer-list row text:
    color: StateColors.forState(model.status)
    // a semantic accent:
    color: StateColors.warning
    // a log line:
    color: StateColors.forLog("Warning")
    \endcode
*/
QtObject {
    id: stateColors

    // ---- Named semantic colors (DESIGN_SYSTEM extended roles) ------------------
    readonly property color success: Theme.color("success")
    readonly property color successEmphasis: Theme.color("successEmphasis")
    readonly property color warning: Theme.color("warning")
    readonly property color error: Theme.color("error")
    readonly property color info: Theme.color("info")
    readonly property color done: Theme.color("done")
    readonly property color muted: Theme.color("muted")
    readonly property color severe: Theme.color("severe")

    // ---- Transfer-list row TEXT color per TorrentState ------------------------

    /*!
        Row text color for a BitTorrent::TorrentState \a state (int value).
        Identical to \c Theme.stateColor(state); provided here so views can use a
        single "state colors" vocabulary.
    */
    function forState(state) {
        return Theme.stateColor(state);
    }

    // ---- Execution-log line colors --------------------------------------------

    /*!
        Color for an execution-log severity \a level, one of
        "TimeStamp" | "Info" | "Normal" | "Warning" | "Critical" | "BannedPeer".
    */
    function forLog(level) {
        return Theme.color("Log." + level);
    }

    // Convenience log-color properties.
    readonly property color logTimeStamp: Theme.color("Log.TimeStamp")
    readonly property color logInfo: Theme.color("Log.Info")
    readonly property color logNormal: Theme.color("Log.Normal")
    readonly property color logWarning: Theme.color("Log.Warning")
    readonly property color logCritical: Theme.color("Log.Critical")
    readonly property color logBannedPeer: Theme.color("Log.BannedPeer")

    // ---- RSS article colors ---------------------------------------------------

    /*!
        Color for an RSS article by \a read state. Unread uses primary (rendered
        SemiBold by the delegate); read uses muted (Normal weight).
    */
    function forRss(read) {
        return Theme.color(read ? "RSS.ReadArticle" : "RSS.UnreadArticle");
    }

    readonly property color rssUnread: Theme.color("RSS.UnreadArticle")
    readonly property color rssRead: Theme.color("RSS.ReadArticle")

    Component.onCompleted: Log.debug("theme", "StateColors singleton ready")
}
