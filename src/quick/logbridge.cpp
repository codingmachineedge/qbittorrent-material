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

#include "logbridge.h"

#include <QLoggingCategory>

#include "base/logging.h"

LogBridge *LogBridge::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(scriptEngine)

    auto *instance = new LogBridge;
    // The QML engine takes ownership of the returned singleton.
    QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
    qCDebug(lcLog) << "LogBridge QML singleton created";
    return instance;
}

LogBridge::LogBridge(QObject *parent)
    : QObject(parent)
{
}

void LogBridge::trace(const QString &category, const QString &message)
{
    // Qt has no trace level; route to debug so it still reaches every sink.
    const QLoggingCategory &cat = Logging::categoryForTag(category);
    QMessageLogger(nullptr, 0, nullptr, cat.categoryName()).debug(cat).noquote() << message;
}

void LogBridge::debug(const QString &category, const QString &message)
{
    const QLoggingCategory &cat = Logging::categoryForTag(category);
    QMessageLogger(nullptr, 0, nullptr, cat.categoryName()).debug(cat).noquote() << message;
}

void LogBridge::info(const QString &category, const QString &message)
{
    const QLoggingCategory &cat = Logging::categoryForTag(category);
    QMessageLogger(nullptr, 0, nullptr, cat.categoryName()).info(cat).noquote() << message;
}

void LogBridge::warning(const QString &category, const QString &message)
{
    const QLoggingCategory &cat = Logging::categoryForTag(category);
    QMessageLogger(nullptr, 0, nullptr, cat.categoryName()).warning(cat).noquote() << message;
}

void LogBridge::critical(const QString &category, const QString &message)
{
    const QLoggingCategory &cat = Logging::categoryForTag(category);
    QMessageLogger(nullptr, 0, nullptr, cat.categoryName()).critical(cat).noquote() << message;
}
