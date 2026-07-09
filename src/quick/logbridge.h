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

#include <QObject>
#include <QQmlEngine>
#include <QString>

/**
 * @file logbridge.h
 * @brief The @c Log QML singleton — QML-side entry point into the categorized
 *        logging system.
 *
 * QML code logs every user action and important state change through this
 * singleton, e.g.
 * @code
 *   Log.info("ui", "Delete clicked for " + count + " torrent(s)")
 *   Log.debug("ui", "OptionsDialog opened")
 * @endcode
 *
 * The @p category argument is one of the short tags
 * ("app" "engine" "session" "torrent" "model" "ui" "theme" "i18n" "net" "rss"
 * "search" "log"); it is mapped to the matching @c QLoggingCategory so QML logs
 * flow through exactly the same file + Execution-Log sinks as C++ logs.
 */
// Named LogBridge in C++ (avoids clashing with the upstream-compat `namespace
// Log` in base/logger.h); exposed to QML as the singleton `Log`.
class LogBridge : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Log)
    QML_SINGLETON

public:
    /// QML singleton factory — returns the shared instance.
    static LogBridge *create(QQmlEngine *engine, QJSEngine *scriptEngine);

    explicit LogBridge(QObject *parent = nullptr);

    /// Lowest verbosity — routed to qCDebug (Qt has no distinct trace level).
    Q_INVOKABLE void trace(const QString &category, const QString &message);
    Q_INVOKABLE void debug(const QString &category, const QString &message);
    Q_INVOKABLE void info(const QString &category, const QString &message);
    Q_INVOKABLE void warning(const QString &category, const QString &message);
    Q_INVOKABLE void critical(const QString &category, const QString &message);
};
