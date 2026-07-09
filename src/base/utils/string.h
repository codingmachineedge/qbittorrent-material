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

#include <numeric>
#include <optional>
#include <string_view>

#include <QChar>
#include <QMetaEnum>
#include <QString>
#include <QtContainerFwd>

#include "base/concepts/explicitlyconvertibleto.h"
#include "base/global.h"

namespace Utils::String
{
    QString wildcardToRegexPattern(const QString &pattern);

    template <typename T>
    T unquote(const T &str, const QString &quotes = u"\""_s)
    {
        if (str.length() < 2)
            return str;

        for (const QChar quote : quotes)
        {
            if (str.startsWith(quote) && str.endsWith(quote))
                return str.sliced(1, (str.length() - 2));
        }

        return str;
    }

    std::optional<bool> parseBool(const QString &string);
    std::optional<int> parseInt(const QString &string);
    std::optional<double> parseDouble(const QString &string);

    QStringList splitCommand(const QString &command);

    QString fromDouble(double n, int precision);
    QString fromLatin1(std::string_view string);
    QString fromLocal8Bit(std::string_view string);

    template <typename Container>
    QString joinIntoString(const Container &container, const QString &separator)
        requires ExplicitlyConvertibleTo<typename Container::value_type, QString>
    {
        auto iter = container.cbegin();
        const auto end = container.cend();
        if (iter == end)
            return {};

        const qsizetype totalLength = std::accumulate(iter, end, (separator.size() * (container.size() - 1))
            , [](const qsizetype total, const typename Container::value_type &value)
        {
            return total + QString(value).size();
        });

        QString ret;
        ret.reserve(totalLength);
        ret.append(QString(*iter));
        ++iter;

        while (iter != end)
        {
            ret.append(separator + QString(*iter));
            ++iter;
        }

        return ret;
    }

    template <typename T>
    QString fromEnum(const T &value)
        requires std::is_enum_v<T>
    {
        static_assert(std::is_same_v<int, typename std::underlying_type_t<T>>,
                      "Enumeration underlying type has to be int.");

        const auto metaEnum = QMetaEnum::fromType<T>();
        return QString::fromLatin1(metaEnum.valueToKey(static_cast<int>(value)));
    }

    template <typename T>
    T toEnum(const QString &serializedValue, const T &defaultValue)
        requires std::is_enum_v<T>
    {
        static_assert(std::is_same_v<int, typename std::underlying_type_t<T>>,
                      "Enumeration underlying type has to be int.");

        const auto metaEnum = QMetaEnum::fromType<T>();
        bool ok = false;
        const T value = static_cast<T>(metaEnum.keyToValue(serializedValue.toLatin1().constData(), &ok));
        return (ok ? value : defaultValue);
    }
}
