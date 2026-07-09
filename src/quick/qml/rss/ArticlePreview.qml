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
    \qmltype ArticlePreview
    \brief Renders the selected article as rich text (header block + body).

    The HTML is composed by \c RSSController.renderArticleHtml() (which also does
    the BBCode → HTML conversion). Anchor clicks are routed the same way the
    Widgets \c HtmlBrowser did: \c .torrent / \c magnet links add a torrent,
    everything else opens the browser (with the local-file safety guard applied
    by the controller).
*/
Rectangle {
    id: root

    /*! The article map (as returned by \c RSSArticleModel.get(row)). */
    property var article: ({})

    /*! Whether to show the "Feed:" header row (sticky All/Unread views). */
    property bool showFeed: false

    readonly property string _html: (article && article.title)
        ? RSSController.renderArticleHtml(article, showFeed)
        : ""

    color: Theme.color("surface")
    border.width: 1
    border.color: Theme.color("outlineVariant")
    radius: Spacing.radiusCard

    onArticleChanged: Log.debug("rss", "ArticlePreview -> " + (article && article.title ? article.title : "(none)"))

    Flickable {
        id: flick
        anchors.fill: parent
        anchors.margins: Spacing.md
        contentWidth: width
        contentHeight: body.implicitHeight
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar { policy: ScrollBar.AsNeeded }

        Text {
            id: body
            width: flick.width
            text: root._html
            visible: root._html.length > 0
            textFormat: Text.RichText
            wrapMode: Text.WordWrap
            color: Theme.color("onSurface")
            font: Typography.bodyMedium
            onLinkActivated: (link) => {
                Log.info("rss", "Preview link activated: " + link)
                const lower = link.toLowerCase()
                if (lower.endsWith(".torrent") || lower.startsWith("magnet:"))
                    RSSController.downloadTorrent(link)
                else
                    RSSController.openArticleLink(link)
            }
        }
    }

    Label {
        anchors.centerIn: parent
        visible: root._html.length === 0
        text: qsTr("Select an article to preview")
        color: Theme.color("onSurfaceVariant")
        font: Typography.bodyMedium
    }
}
