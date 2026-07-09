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

#include <QtTypes>

#include "base/utils/fs/path.h"

class QString;
class QStringView;

/*  Miscellaneous functions that can be useful */
namespace Utils::Misc
{
    // use binary prefix standards from IEC 60027-2
    // see http://en.wikipedia.org/wiki/Kilobyte
    enum class SizeUnit
    {
        Byte,       // 1024^0,
        KibiByte,   // 1024^1,
        MebiByte,   // 1024^2,
        GibiByte,   // 1024^3,
        TebiByte,   // 1024^4,
        PebiByte,   // 1024^5,
        ExbiByte    // 1024^6,
        // int64 is used for sizes and thus the next units can not be handled
        // ZebiByte,   // 1024^7,
        // YobiByte,   // 1024^8
    };

    enum class TimeResolution
    {
        Seconds,
        Minutes
    };

    QString parseHtmlLinks(const QString &rawText);

    QString osName();
    QString boostVersionString();
    QString libtorrentVersionString();
    QString opensslVersionString();
    QString zlibVersionString();

    QString unitString(SizeUnit unit, bool isSpeed = false);

    // return the best user friendly storage unit (B, KiB, MiB, GiB, TiB)
    // value must be given in bytes
    QString friendlyUnit(qint64 bytes, bool isSpeed = false, int precision = -1);
    QString friendlyUnitCompact(qint64 bytes);
    int friendlyUnitPrecision(SizeUnit unit);
    qint64 sizeInBytes(qreal size, SizeUnit unit);

    bool isPreviewable(const Path &filePath);
    bool isTorrentLink(const QString &str);

    // Take a number of seconds and return a user-friendly
    // time duration like "1d 2h 10m".
    QString userFriendlyDuration(qlonglong seconds, qlonglong maxCap = -1, TimeResolution resolution = TimeResolution::Minutes);

    QString languageToLocalizedString(QStringView localeStr);
}
