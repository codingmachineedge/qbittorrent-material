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

#include <QLoggingCategory>

class QString;

/**
 * @file logging.h
 * @brief Central categorized-logging definitions for the whole application.
 *
 * Every subsystem logs through one of the fixed @c QLoggingCategory objects
 * declared here. The matching @c Q_LOGGING_CATEGORY definitions live in
 * logging.cpp. Do NOT invent ad-hoc categories — pick the closest one.
 *
 * Usage (C++):
 * @code
 *   #include "base/logging.h"
 *   qCDebug(lcSession) << "Adding torrent" << id;
 *   qCInfo(lcSession)  << "Torrent added" << name;
 *   qCWarning(lcNet)   << "Proxy connect failed" << host << port;
 *   qCCritical(lcTorrent) << "I/O error:" << message;
 * @endcode
 *
 * Levels map: TRACE/DEBUG -> qCDebug, INFO -> qCInfo, WARNING -> qCWarning,
 * CRITICAL -> qCCritical. Default build enables DEBUG for every @c qbt.*
 * category; the threshold is raisable via @c QT_LOGGING_RULES.
 */

Q_DECLARE_LOGGING_CATEGORY(lcApp)      // "qbt.app"     application lifecycle
Q_DECLARE_LOGGING_CATEGORY(lcEngine)   // "qbt.engine"  libtorrent engine glue
Q_DECLARE_LOGGING_CATEGORY(lcSession)  // "qbt.session" BitTorrent::Session
Q_DECLARE_LOGGING_CATEGORY(lcTorrent)  // "qbt.torrent" per-torrent state
Q_DECLARE_LOGGING_CATEGORY(lcModel)    // "qbt.model"   bridge models
Q_DECLARE_LOGGING_CATEGORY(lcUi)       // "qbt.ui"      controllers / UI actions from C++
Q_DECLARE_LOGGING_CATEGORY(lcTheme)    // "qbt.theme"   theming
Q_DECLARE_LOGGING_CATEGORY(lcI18n)     // "qbt.i18n"    internationalization
Q_DECLARE_LOGGING_CATEGORY(lcNet)      // "qbt.net"     networking
Q_DECLARE_LOGGING_CATEGORY(lcRss)      // "qbt.rss"     RSS subsystem
Q_DECLARE_LOGGING_CATEGORY(lcSearch)   // "qbt.search"  search subsystem
Q_DECLARE_LOGGING_CATEGORY(lcLog)      // "qbt.log"     the logging subsystem itself

/**
 * @namespace Logging
 * @brief Installation / configuration of the process-wide dual-sink message handler.
 *
 * The handler formats every Qt message as
 * @c [time][thread][category][level] message, writes it to a size-rotated log
 * file under @c AppDataLocation/logs, mirrors it to stderr, and forwards INFO+
 * messages to the in-app Execution Log (::Logger).
 */
namespace Logging
{
    /// Resolve the short QML/UI tag (e.g. "session") to its QLoggingCategory.
    /// Falls back to @c lcUi for unknown tags. Never returns a dangling reference.
    const QLoggingCategory &categoryForTag(const QString &tag);

    /// Install the dual-sink qInstallMessageHandler and open the rotating log
    /// file. Also raises the default verbosity of all @c qbt.* categories to
    /// DEBUG. Safe to call exactly once, early in @c main().
    void installMessageHandler();

    /// Restore the previous message handler and flush/close the log file.
    void removeMessageHandler();

    /// Absolute path of the active rotating log file (may be empty before install).
    QString currentLogFilePath();
}
