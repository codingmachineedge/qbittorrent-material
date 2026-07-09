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
import qBittorrent

/*!
    \qmltype BlockedIPsView
    \brief The "Blocked IPs" tab of the Execution Log.

    Lists peer-ban / IP-block events from a \c LogPeerModel. Each line is colored
    with the banned-peer color (\c Log.BannedPeer) supplied by the model's
    \c logLevel role, so the presentation is identical to the General log — this
    view is a thin, purpose-named wrapper over \l GeneralLogView with a tailored
    empty-state message.
*/
Item {
    id: root

    /*! The \c LogPeerModel instance to display. */
    property var model: null

    GeneralLogView {
        anchors.fill: parent
        model: root.model
        emptyText: qsTr("No IP addresses have been blocked yet")
    }

    Component.onCompleted: Log.debug("ui", "BlockedIPsView ready")
}
