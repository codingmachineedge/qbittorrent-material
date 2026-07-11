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

#include <type_traits>

#include <QObject>
#include <QReadWriteLock>
#include <QTimer>
#include <QVariant>
#include <QVariantHash>

#include "base/concepts/stringable.h"
#include "utils/string.h"

template <typename T>
concept IsQFlags = std::same_as<T, QFlags<typename T::enum_type>>;

// There are 2 ways for class `T` provide serialization support into `SettingsStorage`:
// 1. If the `T` state is intended for users to edit (via a text editor), then
//    `T` should satisfy `Stringable` concept
// 2. Otherwise, use `Q_DECLARE_METATYPE(T)` and let `QMetaType` handle the serialization
class SettingsStorage final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(SettingsStorage)

    SettingsStorage();
    ~SettingsStorage();

public:
    static void initInstance();
    static void freeInstance();
    static SettingsStorage *instance();

    template <typename T>
    T loadValue(const QString &key, const T &defaultValue = {}) const
    {
        if constexpr (std::same_as<T, QVariant>)
        {
            // fast path for loading QVariant
            return loadValueImpl(key, defaultValue);
        }
        else if constexpr (Stringable<T>)
        {
            const QString value = loadValue(key, defaultValue.toString());
            return T {value};
        }
        else if constexpr (std::is_enum_v<T>)
        {
            const auto value = loadValue<QString>(key);
            return Utils::String::toEnum(value, defaultValue);
        }
        else if constexpr (IsQFlags<T>)
        {
            const typename T::Int value = loadValue(key, static_cast<typename T::Int>(defaultValue));
            return T {value};
        }
        else
        {
            const QVariant value = loadValueImpl(key);
            // check if retrieved value is convertible to T
            return value.template canConvert<T>() ? value.template value<T>() : defaultValue;
        }
    }

    template <typename T>
    void storeValue(const QString &key, const T &value)
    {
        if constexpr (std::same_as<T, QVariant>)
            storeValueImpl(key, value);
        else if constexpr (Stringable<T>)
            storeValueImpl(key, value.toString());
        else if constexpr (std::is_enum_v<T>)
            storeValueImpl(key, Utils::String::fromEnum(value));
        else if constexpr (IsQFlags<T>)
            storeValueImpl(key, static_cast<typename T::Int>(value));
        else
            storeValueImpl(key, QVariant::fromValue(value));
    }

    void removeValue(const QString &key);
    bool hasKey(const QString &key) const;
    bool isEmpty() const;
    QStringList allKeys() const;

signals:
    // Emitted (possibly from a non-main thread) after a key's stored value
    // actually changed. A removed key reports an invalid newValue.
    void valueChanged(const QString &key, const QVariant &oldValue, const QVariant &newValue);

public slots:
    bool save();

private:
    QVariant loadValueImpl(const QString &key, const QVariant &defaultValue = {}) const;
    void storeValueImpl(const QString &key, const QVariant &value);
    void readNativeSettings();
    bool writeNativeSettings() const;

    static SettingsStorage *m_instance;

    const QString m_nativeSettingsName;
    bool m_dirty = false;
    QVariantHash m_data;
    QTimer m_timer;
    mutable QReadWriteLock m_lock;
};
