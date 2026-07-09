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

#include <QClipboard>
#include <QGuiApplication>
#include <QObject>
#include <QQmlEngine>
#include <QString>

#include <qqmlintegration.h>

#include "base/logger.h"
#include "base/logging.h"
#include "base/settingsstorage.h"

/**
 * @file executionlogcontroller.h
 * @brief The @c ExecutionLogController QML singleton — state + persistence for
 *        the Material "Execution Log" screen.
 *
 * This controller is the QML-facing owner of the four message-type toggles that
 * the main window's @e View menu exposes (Normal / Info / Warning / Critical),
 * mirroring the legacy qBittorrent behavior. It does @b not own the log data —
 * that lives in ::Logger and is surfaced through @c LogMessageModel /
 * @c LogPeerModel / @c LogFilterProxy. The @c LogFilterProxy binds its
 * @c messageTypes to this controller so the visible severities follow the View
 * menu live.
 *
 * Persistence uses the legacy setting keys verbatim so existing configs load:
 *   - @c GUI/Log/Enabled  (bool)  — whether the Execution Log tab is shown.
 *   - @c GUI/Log/Types    (int)   — the enabled @c Logger::MsgType flags.
 *
 * The controller is header-only: every method is defined inline so no
 * translation unit is required (AUTOMOC + qmltyperegistrar pick the type up from
 * this header via the module's @c SOURCES glob).
 */
class ExecutionLogController final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    /// Whether the Execution Log tab is shown (View -> Log -> Show).
    Q_PROPERTY(bool enabled READ isEnabled WRITE setEnabled NOTIFY enabledChanged)
    /// The raw enabled-message-type flags (a @c Logger::MsgTypes value as int).
    Q_PROPERTY(int messageTypes READ messageTypes NOTIFY messageTypesChanged)
    /// Convenience one-per-severity toggles (all notify @c messageTypesChanged).
    Q_PROPERTY(bool showNormal READ showNormal WRITE setShowNormal NOTIFY messageTypesChanged)
    Q_PROPERTY(bool showInfo READ showInfo WRITE setShowInfo NOTIFY messageTypesChanged)
    Q_PROPERTY(bool showWarning READ showWarning WRITE setShowWarning NOTIFY messageTypesChanged)
    Q_PROPERTY(bool showCritical READ showCritical WRITE setShowCritical NOTIFY messageTypesChanged)

public:
    /// Setting keys (legacy-compatible).
    static constexpr auto kEnabledKey = "GUI/Log/Enabled";
    static constexpr auto kTypesKey = "GUI/Log/Types";

    /// QML singleton factory — returns the shared, app-owned instance.
    static ExecutionLogController *create(QQmlEngine *engine, QJSEngine *scriptEngine)
    {
        Q_UNUSED(engine)
        Q_UNUSED(scriptEngine)

        if (s_instance == nullptr)
            s_instance = new ExecutionLogController;
        // The controller lives for the whole app lifetime; keep C++ ownership so
        // the QML garbage collector never touches it.
        QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
        return s_instance;
    }

    /// The one shared instance (also usable from pure C++). May be null before
    /// the QML engine first references the type.
    static ExecutionLogController *instance()
    {
        return s_instance;
    }

    explicit ExecutionLogController(QObject *parent = nullptr)
        : QObject(parent)
    {
        if (auto *storage = SettingsStorage::instance())
        {
            m_enabled = storage->loadValue<bool>(QString::fromLatin1(kEnabledKey), false);
            m_messageTypes = storage->loadValue<int>(QString::fromLatin1(kTypesKey)
                , static_cast<int>(Logger::All));
        }
        qCDebug(lcUi) << "ExecutionLogController created; enabled=" << m_enabled
                      << "messageTypes=" << m_messageTypes;
    }

    ~ExecutionLogController() override
    {
        qCDebug(lcUi) << "ExecutionLogController destroyed";
    }

    // ---- enabled --------------------------------------------------------------

    [[nodiscard]] bool isEnabled() const
    {
        return m_enabled;
    }

    void setEnabled(const bool value)
    {
        if (m_enabled == value)
            return;

        m_enabled = value;
        qCInfo(lcUi) << "Execution Log tab visibility changed ->" << value;
        if (auto *storage = SettingsStorage::instance())
            storage->storeValue<bool>(QString::fromLatin1(kEnabledKey), value);
        emit enabledChanged();
    }

    // ---- message-type filter --------------------------------------------------

    [[nodiscard]] int messageTypes() const
    {
        return m_messageTypes;
    }

    [[nodiscard]] bool showNormal() const
    {
        return flags().testFlag(Logger::Normal);
    }

    [[nodiscard]] bool showInfo() const
    {
        return flags().testFlag(Logger::Info);
    }

    [[nodiscard]] bool showWarning() const
    {
        return flags().testFlag(Logger::Warning);
    }

    [[nodiscard]] bool showCritical() const
    {
        return flags().testFlag(Logger::Critical);
    }

    void setShowNormal(const bool on)
    {
        setMessageType(Logger::Normal, on);
    }

    void setShowInfo(const bool on)
    {
        setMessageType(Logger::Info, on);
    }

    void setShowWarning(const bool on)
    {
        setMessageType(Logger::Warning, on);
    }

    void setShowCritical(const bool on)
    {
        setMessageType(Logger::Critical, on);
    }

    /// Enable/disable a single @p type (a @c Logger::MsgType value) in the filter.
    Q_INVOKABLE void setMessageType(const int type, const bool on)
    {
        Logger::MsgTypes current = flags();
        current.setFlag(static_cast<Logger::MsgType>(type), on);
        applyMessageTypes(current.toInt());
    }

    /// Replace the whole filter mask at once.
    Q_INVOKABLE void setMessageTypes(const int types)
    {
        applyMessageTypes(types);
    }

    // ---- clipboard helper -----------------------------------------------------

    /// Copy @p text (already assembled by the view) to the system clipboard.
    /// QML has no direct clipboard access, so log views route "Copy" here.
    Q_INVOKABLE void copyToClipboard(const QString &text) const
    {
        if (QClipboard *clipboard = QGuiApplication::clipboard())
        {
            clipboard->setText(text);
            qCDebug(lcUi) << "Copied" << text.size() << "chars from the Execution Log to the clipboard";
        }
        else
        {
            qCWarning(lcUi) << "Execution Log copy failed: no clipboard available";
        }
    }

signals:
    void enabledChanged();
    void messageTypesChanged();

private:
    [[nodiscard]] Logger::MsgTypes flags() const
    {
        return Logger::MsgTypes::fromInt(m_messageTypes);
    }

    void applyMessageTypes(const int types)
    {
        if (m_messageTypes == types)
            return;

        m_messageTypes = types;
        qCInfo(lcUi) << "Execution Log message-type filter changed ->" << types;
        if (auto *storage = SettingsStorage::instance())
            storage->storeValue<int>(QString::fromLatin1(kTypesKey), types);
        emit messageTypesChanged();
    }

    inline static ExecutionLogController *s_instance = nullptr;

    bool m_enabled = false;
    int m_messageTypes = static_cast<int>(Logger::All);
};
