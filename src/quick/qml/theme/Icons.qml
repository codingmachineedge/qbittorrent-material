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
    \qmltype Icons
    \brief Legacy icon-id -> Material Symbols Outlined codepoint map.

    Each property is the single glyph string for one Material Symbols Outlined
    icon (a \c \\uXXXX Private Use Area codepoint from MaterialSymbolsOutlined.ttf).
    Render ONLY through \c MDIcon (never a raw Text), e.g.:
    \code
    MDIcon { icon: Icons.play_arrow }
    MDIcon { icon: Icons.deleteIcon; color: StateColors.error; size: 20 }
    \endcode

    Where a glyph's exact codepoint could not be verified against the shipped
    font it is flagged with a comment — verify against the bundled TTF if a tofu
    box renders.
*/
QtObject {
    id: icons

    // ---- Main toolbar / transfer-list actions ---------------------------------
    readonly property string note_add: ""             // add torrent
    readonly property string add_link: ""             // add torrent link
    readonly property string deleteIcon: ""               // delete
    readonly property string play_arrow: ""           // start / resume
    readonly property string pause: ""                // stop
    readonly property string bolt: ""                 // force start
    readonly property string play_circle: ""          // resume session
    readonly property string pause_circle: ""         // pause session
    readonly property string vertical_align_top: ""   // queue: move top
    readonly property string arrow_upward: ""         // queue: move up
    readonly property string arrow_downward: ""       // queue: move down
    readonly property string vertical_align_bottom: "" // queue: move bottom
    readonly property string folder_open: ""          // open containing folder
    readonly property string build: ""                // create torrent
    readonly property string settings: ""             // options
    readonly property string lock: ""                 // lock / password
    readonly property string speed: ""                // speed limits / alt-speed
    readonly property string bar_chart: ""            // statistics
    readonly property string info: ""                 // about
    readonly property string menu_book: ""            // documentation
    readonly property string volunteer_activism: ""   // donate
    readonly property string logout: ""               // exit
    readonly property string cookie: ""               // cookies
    readonly property string extension: ""            // plugins
    readonly property string system_update: ""        // check for updates
    readonly property string article: ""              // execution log
    readonly property string drive_file_move: ""      // set location / moving
    readonly property string fact_check: ""           // force recheck
    readonly property string campaign: ""             // force reannounce
    readonly property string preview: ""              // preview file
    readonly property string tune: ""                 // torrent options
    readonly property string category: ""             // category
    readonly property string sell: ""                 // tags
    readonly property string low_priority: ""         // queue
    readonly property string content_copy: ""         // copy
    readonly property string link: ""                 // magnet link
    readonly property string tag: ""                  // hash
    readonly property string save_alt: ""             // export .torrent
    readonly property string edit: ""                 // rename / edit

    // ---- Status bar -----------------------------------------------------------
    readonly property string cloud_done: ""           // connected
    readonly property string cloud_off: ""            // disconnected / trackerless
    readonly property string shield: ""               // firewalled / security-high
    readonly property string download: ""             // download
    readonly property string upload: ""               // upload

    // ---- Status filter --------------------------------------------------------
    readonly property string apps: ""                 // all
    readonly property string trending_up: ""          // active
    readonly property string trending_down: ""        // inactive
    readonly property string hourglass_empty: ""      // stalled
    readonly property string check_circle: ""         // completed
    readonly property string error: ""                // error

    // ---- Side filters ---------------------------------------------------------
    readonly property string dns: ""                  // trackers
    readonly property string warning: ""              // tracker warning / shared warning

    // ---- Options tabs ---------------------------------------------------------
    readonly property string palette: ""              // Behavior
    readonly property string lan: ""                  // Connection
    readonly property string swap_vert: ""            // BitTorrent
    readonly property string search: ""               // Search
    readonly property string rss_feed: ""             // RSS
    readonly property string language: ""             // WebUI
    readonly property string settings_suggest: ""     // Advanced

    // ---- Properties tabs ------------------------------------------------------
    readonly property string description: ""          // General
    readonly property string groups: ""               // Peers
    readonly property string publicIcon: ""               // HTTP sources
    readonly property string folder: ""               // Content
    readonly property string show_chart: ""           // Speed

    // ---- RSS ------------------------------------------------------------------
    readonly property string mark_email_read: ""      // read article (verify codepoint)
    readonly property string mail: ""                 // unread article
    readonly property string inbox: ""                // inbox
    readonly property string create_new_folder: ""    // new folder
    readonly property string progress_activity: ""    // loading spinner (verify codepoint)
    readonly property string block: ""                // error / ip-blocked
    readonly property string done_all: ""             // mark read
    readonly property string refresh: ""              // refresh
    readonly property string open_in_new: ""          // open url / open description

    // ---- Search / Peers -------------------------------------------------------
    readonly property string person_add: ""           // add peer
    readonly property string person_remove: ""        // ban peer

    // ---- Shared ---------------------------------------------------------------
    readonly property string gpp_bad: ""              // security-low
    readonly property string insert_drive_file: ""    // file

    // ---- Misc small utilities (used across screens) ---------------------------
    readonly property string add: ""                  // generic add
    readonly property string remove: ""               // generic remove
    readonly property string close: ""                // close / clear
    readonly property string check: ""                // checkmark
    readonly property string more_vert: ""            // overflow menu
    readonly property string expand_more: ""          // collapse chevron (expanded)
    readonly property string chevron_right: ""        // collapse chevron (collapsed)
    readonly property string arrow_drop_down: ""      // combo indicator
    readonly property string filter_list: ""          // filter
    readonly property string visibility: ""           // show
    readonly property string visibility_off: ""       // hide

    /*!
        Look up a codepoint by its string id (e.g. from a model role). Returns
        the glyph, or an empty string (renders nothing) with a warning if the id
        is unknown. Prefer the direct property (Icons.play_arrow) when the id is
        known at author time.
    */
    function forId(id) {
        const glyph = icons[id];
        if (glyph === undefined) {
            Log.warning("theme", "Unknown icon id: " + id);
            return "";
        }
        return glyph;
    }

    Component.onCompleted: Log.debug("theme", "Icons singleton ready")
}
