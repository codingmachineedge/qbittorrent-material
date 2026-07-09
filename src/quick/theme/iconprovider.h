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

#pragma once

#include <QImage>
#include <QQuickImageProvider>
#include <QString>

/// QML image provider for country flags: `image://flags/<iso>`.
///
/// The flag SVGs (369 of them, named `<iso2>.svg` plus a handful of regional
/// codes such as `gb-eng.svg`, `es-ct.svg`, `sh-ac.svg`) are bundled with the
/// application and reused verbatim from upstream qBittorrent — there is no
/// Material equivalent. QML uses them via
/// `Image { source: "image://flags/" + isoCode }`.
///
/// The provider is registered on the QML engine at startup:
/// `engine->addImageProvider(u"flags"_s, new FlagImageProvider);`
class FlagImageProvider final : public QQuickImageProvider
{
public:
    FlagImageProvider();

    /// Render the flag identified by `id` (an ISO code, with or without a
    /// trailing `.svg`) into an image. `requestedSize` is honored when valid;
    /// otherwise a default 4:3 flag size is used. On a miss, a transparent
    /// placeholder of the requested size is returned so the view degrades
    /// gracefully instead of showing a broken image.
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

private:
    QString resolveSource(const QString &id) const;

    static constexpr int kDefaultWidth = 20;
    static constexpr int kDefaultHeight = 15;
};
