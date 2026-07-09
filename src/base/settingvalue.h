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

#include <QString>

#include "settingsstorage.h"

// This is a thin/handy wrapper over `SettingsStorage`. Use it when store/load value
// rarely occurs, otherwise use `CachedSettingValue`.
template <typename T>
class SettingValue
{
public:
    explicit SettingValue(const QString &keyName)
        : m_keyName {keyName}
    {
    }

    T get(const T &defaultValue = {}) const
    {
        return SettingsStorage::instance()->loadValue(m_keyName, defaultValue);
    }

    operator T() const
    {
        return get();
    }

    SettingValue<T> &operator=(const T &value)
    {
        SettingsStorage::instance()->storeValue(m_keyName, value);
        return *this;
    }

private:
    const QString m_keyName;
};

template <typename T>
class CachedSettingValue
{
public:
    explicit CachedSettingValue(const QString &keyName, const T &defaultValue = {})
        : m_setting {keyName}
        , m_cache {m_setting.get(defaultValue)}
    {
    }

    // The signature of the ProxyFunc should be equivalent to the following:
    // T proxyFunc(const T &a);
    template <typename ProxyFunc>
    explicit CachedSettingValue(const QString &keyName, const T &defaultValue, ProxyFunc &&proxyFunc)
        : m_setting {keyName}
        , m_cache {proxyFunc(m_setting.get(defaultValue))}
    {
    }

    T get() const
    {
        return m_cache;
    }

    operator T() const
    {
        return get();
    }

    CachedSettingValue<T> &operator=(const T &value)
    {
        if (m_cache == value)
            return *this;

        m_setting = value;
        m_cache = value;
        return *this;
    }

private:
    SettingValue<T> m_setting;
    T m_cache;
};
