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

#include "iconprovider.h"

#include <QFile>
#include <QPainter>
#include <QSvgRenderer>

#include "base/logging.h"

using namespace Qt::StringLiterals;

namespace
{
    // Candidate base directories for the flag SVGs. The qml module bundles them
    // under the module resource prefix; a filesystem path is kept as a dev
    // fallback so the provider works from a build tree too.
    const QStringList kFlagBases = {
        u":/qt/qml/qBittorrent/icons/flags/"_s,
        u":/icons/flags/"_s,
        u"qrc:/qt/qml/qBittorrent/icons/flags/"_s
    };
}

FlagImageProvider::FlagImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    qCDebug(lcTheme) << "FlagImageProvider created";
}

QString FlagImageProvider::resolveSource(const QString &id) const
{
    // Normalize: lowercase, strip any extension, then look for `<code>.svg`.
    QString code = id.trimmed().toLower();
    if (code.endsWith(u".svg"))
        code.chop(4);

    for (const QString &base : kFlagBases)
    {
        const QString candidate = base + code + u".svg";
        if (QFile::exists(candidate))
            return candidate;
    }
    return {};
}

QImage FlagImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    const QSize target = (requestedSize.width() > 0 && requestedSize.height() > 0)
        ? requestedSize
        : QSize(kDefaultWidth, kDefaultHeight);

    if (size)
        *size = target;

    const QString source = resolveSource(id);
    if (source.isEmpty())
    {
        qCWarning(lcTheme) << "Flag not found for id" << id;
        QImage placeholder(target, QImage::Format_ARGB32_Premultiplied);
        placeholder.fill(Qt::transparent);
        return placeholder;
    }

    QSvgRenderer renderer(source);
    if (!renderer.isValid())
    {
        qCWarning(lcTheme) << "Invalid flag SVG" << source;
        QImage placeholder(target, QImage::Format_ARGB32_Premultiplied);
        placeholder.fill(Qt::transparent);
        return placeholder;
    }

    QImage image(target, QImage::Format_ARGB32_Premultiplied);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    renderer.render(&painter, QRectF(0, 0, target.width(), target.height()));
    painter.end();

    qCDebug(lcTheme) << "Rendered flag" << id << "at" << target;
    return image;
}
