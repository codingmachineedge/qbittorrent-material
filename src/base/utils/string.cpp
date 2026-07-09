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

#include "string.h"

#include <cmath>

#include <QList>
#include <QLocale>
#include <QRegularExpression>
#include <QStringList>

#include "base/logging.h"

/// Convert a double to a locale-formatted string with a fixed precision.
///
/// @note This intentionally truncates (floors) rather than rounds so that a
/// value like 0.999 * 100 with precision 1 renders as "99.9" and never rounds
/// up to "100.0" — matching the historical behaviour of the transfer list.
QString Utils::String::fromDouble(const double n, const int precision)
{
    /* HACK because QString rounds up. Eg QString::number(0.999*100.0, 'f', 1) == 99.9
    ** but QString::number(0.9999*100.0, 'f' ,1) == 100.0 The problem manifests when
    ** the number has more digits after the decimal than we want AND the digit after
    ** our 'wanted' is >= 5. In this case our last digit gets rounded up. So for each
    ** precision we add an extra 0 behind 1 in the below algorithm. */

    const double prec = std::pow(10.0, precision);
    return QLocale::system().toString(std::floor(n * prec) / prec, 'f', precision);
}

QString Utils::String::fromLatin1(const std::string_view string)
{
    return QString::fromLatin1(string.data(), string.size());
}

QString Utils::String::fromLocal8Bit(const std::string_view string)
{
    return QString::fromLocal8Bit(string.data(), string.size());
}

QString Utils::String::wildcardToRegexPattern(const QString &pattern)
{
#if (QT_VERSION >= QT_VERSION_CHECK(6, 6, 1))
    return QRegularExpression::wildcardToRegularExpression(pattern
            , (QRegularExpression::UnanchoredWildcardConversion | QRegularExpression::NonPathWildcardConversion));
#else
    return QRegularExpression::wildcardToRegularExpression(pattern, QRegularExpression::UnanchoredWildcardConversion);
#endif
}

/// Split a command line into arguments, honouring double-quoted spans. The
/// quote characters themselves are preserved in the returned tokens (matching
/// legacy behaviour used when persisting/executing external programs).
QStringList Utils::String::splitCommand(const QString &command)
{
    QStringList ret;
    ret.reserve(32);

    bool inQuotes = false;
    QString tmp;
    for (const QChar c : command)
    {
        if (c == u' ')
        {
            if (!inQuotes)
            {
                if (!tmp.isEmpty())
                {
                    ret.append(tmp);
                    tmp.clear();
                }

                continue;
            }
        }
        else if (c == u'"')
        {
            inQuotes = !inQuotes;
        }

        tmp.append(c);
    }

    if (!tmp.isEmpty())
        ret.append(tmp);

    if (inQuotes)
        qCWarning(lcApp) << "Utils::String::splitCommand: unbalanced quotes in command string";

    return ret;
}

std::optional<bool> Utils::String::parseBool(const QString &string)
{
    if (string.compare(u"true", Qt::CaseInsensitive) == 0)
        return true;
    if (string.compare(u"false", Qt::CaseInsensitive) == 0)
        return false;

    return std::nullopt;
}

std::optional<int> Utils::String::parseInt(const QString &string)
{
    bool ok = false;
    const int result = string.toInt(&ok);
    if (ok)
        return result;

    return std::nullopt;
}

std::optional<double> Utils::String::parseDouble(const QString &string)
{
    bool ok = false;
    const double result = string.toDouble(&ok);
    if (ok)
        return result;

    return std::nullopt;
}
