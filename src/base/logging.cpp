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

#include "logging.h"

#include <array>

#include <QByteArray>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QString>
#include <QThread>

#include "logger.h"

// ---------------------------------------------------------------------------
// Category definitions (one per subsystem — see logging.h).
// ---------------------------------------------------------------------------
Q_LOGGING_CATEGORY(lcApp, "qbt.app")
Q_LOGGING_CATEGORY(lcEngine, "qbt.engine")
Q_LOGGING_CATEGORY(lcSession, "qbt.session")
Q_LOGGING_CATEGORY(lcTorrent, "qbt.torrent")
Q_LOGGING_CATEGORY(lcModel, "qbt.model")
Q_LOGGING_CATEGORY(lcUi, "qbt.ui")
Q_LOGGING_CATEGORY(lcTheme, "qbt.theme")
Q_LOGGING_CATEGORY(lcI18n, "qbt.i18n")
Q_LOGGING_CATEGORY(lcNet, "qbt.net")
Q_LOGGING_CATEGORY(lcRss, "qbt.rss")
Q_LOGGING_CATEGORY(lcSearch, "qbt.search")
Q_LOGGING_CATEGORY(lcLog, "qbt.log")

namespace
{
    // --- Rotating-file sink state -----------------------------------------
    constexpr qint64 MAX_LOG_FILE_SIZE = 5 * 1024 * 1024; // 5 MiB per file
    constexpr int MAX_LOG_BACKUPS = 5;                    // qbittorrent.log.1 .. .5

    QMutex g_sinkMutex;
    QFile g_logFile;
    QString g_logFilePath;
    QtMessageHandler g_previousHandler = nullptr;
    bool g_installed = false;

    /// Human-readable, fixed-width level tag for a Qt message type.
    const char *levelTag(const QtMsgType type)
    {
        switch (type)
        {
        case QtDebugMsg:    return "DEBUG";
        case QtInfoMsg:     return "INFO ";
        case QtWarningMsg:  return "WARN ";
        case QtCriticalMsg: return "CRIT ";
        case QtFatalMsg:    return "FATAL";
        }
        return "?????";
    }

    /// Map a Qt message type to the Execution-Log message class. Debug/trace
    /// messages are intentionally *not* forwarded (below INFO).
    Logger::MsgType toLoggerType(const QtMsgType type)
    {
        switch (type)
        {
        case QtInfoMsg:     return Logger::Info;
        case QtWarningMsg:  return Logger::Warning;
        case QtCriticalMsg:
        case QtFatalMsg:    return Logger::Critical;
        case QtDebugMsg:    break;
        }
        return Logger::Normal;
    }

    /// Rotate qbittorrent.log -> .1 -> .2 ... discarding the oldest backup.
    /// Caller must hold @c g_sinkMutex and the file must be closed.
    void rotateLocked()
    {
        // Drop the oldest, then shift each backup up by one.
        QFile::remove(g_logFilePath + QStringLiteral(".%1").arg(MAX_LOG_BACKUPS));
        for (int i = MAX_LOG_BACKUPS - 1; i >= 1; --i)
        {
            const QString from = g_logFilePath + QStringLiteral(".%1").arg(i);
            const QString to = g_logFilePath + QStringLiteral(".%1").arg(i + 1);
            if (QFile::exists(from))
                QFile::rename(from, to);
        }
        QFile::rename(g_logFilePath, g_logFilePath + QStringLiteral(".1"));
    }

    /// Append one already-formatted line to the rotating file, rotating first
    /// if the size cap would be exceeded. Caller must hold @c g_sinkMutex.
    void writeToFileLocked(const QByteArray &line)
    {
        if (g_logFilePath.isEmpty())
            return;

        if (!g_logFile.isOpen())
        {
            g_logFile.setFileName(g_logFilePath);
            g_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        }

        if (g_logFile.isOpen() && ((g_logFile.size() + line.size()) > MAX_LOG_FILE_SIZE))
        {
            g_logFile.close();
            rotateLocked();
            g_logFile.setFileName(g_logFilePath);
            g_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
        }

        if (g_logFile.isOpen())
        {
            g_logFile.write(line);
            g_logFile.flush();
        }
    }

    /// The installed handler: format, mirror to stderr, persist to file, and
    /// forward INFO+ to the in-app Execution Log.
    void messageHandler(const QtMsgType type, const QMessageLogContext &context, const QString &message)
    {
        const QString timestamp = QDateTime::currentDateTime().toString(QStringLiteral("yyyy-MM-dd HH:mm:ss.zzz"));
        const QString category = QString::fromUtf8((context.category != nullptr) ? context.category : "default");
        const auto threadId = reinterpret_cast<quintptr>(QThread::currentThreadId());

        const QString formatted = QStringLiteral("[%1][T%2][%3][%4] %5")
                                      .arg(timestamp)
                                      .arg(threadId, 0, 16)
                                      .arg(category, QString::fromLatin1(levelTag(type)), message);

        const QByteArray line = (formatted + QLatin1Char('\n')).toUtf8();

        {
            const QMutexLocker locker(&g_sinkMutex);
            // Sink 1: console (developer visibility).
            fputs(line.constData(), stderr);
            fflush(stderr);
            // Sink 2: rotating file.
            writeToFileLocked(line);
        }

        // Sink 3: in-app Execution Log (INFO and above only).
        if ((type != QtDebugMsg) && (Logger::instance() != nullptr))
            Logger::instance()->addMessage(QStringLiteral("[%1] %2").arg(category, message), toLoggerType(type));

        if (type == QtFatalMsg)
            abort();
    }
}

namespace Logging
{
    const QLoggingCategory &categoryForTag(const QString &tag)
    {
        // Short QML/UI tag -> category. Kept in sync with logbridge.cpp.
        if (tag == QLatin1String("app"))     return lcApp();
        if (tag == QLatin1String("engine"))  return lcEngine();
        if (tag == QLatin1String("session")) return lcSession();
        if (tag == QLatin1String("torrent")) return lcTorrent();
        if (tag == QLatin1String("model"))   return lcModel();
        if (tag == QLatin1String("ui"))      return lcUi();
        if (tag == QLatin1String("theme"))   return lcTheme();
        if (tag == QLatin1String("i18n"))    return lcI18n();
        if (tag == QLatin1String("net"))     return lcNet();
        if (tag == QLatin1String("rss"))     return lcRss();
        if (tag == QLatin1String("search"))  return lcSearch();
        if (tag == QLatin1String("log"))     return lcLog();
        return lcUi();
    }

    void installMessageHandler()
    {
        QMutexLocker locker(&g_sinkMutex);
        if (g_installed)
            return;

        // Resolve <AppDataLocation>/logs/qbittorrent.log and ensure the dir.
        const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        QString logsDir;
        if (baseDir.isEmpty())
            logsDir = QDir::tempPath() + QStringLiteral("/qbittorrent-logs");
        else
            logsDir = baseDir + QStringLiteral("/logs");
        QDir().mkpath(logsDir);
        g_logFilePath = logsDir + QStringLiteral("/qbittorrent.log");

        // Default verbosity: DEBUG for every qbt.* category (overridable via
        // QT_LOGGING_RULES which is applied after this call by Qt).
        QLoggingCategory::setFilterRules(QStringLiteral("qbt.*.debug=true"));

        g_previousHandler = qInstallMessageHandler(&messageHandler);
        g_installed = true;

        // First line — deliberately via the handler so it hits every sink.
        locker.unlock();
        qCInfo(lcLog) << "Message handler installed; log file:" << g_logFilePath
                      << "maxSize:" << MAX_LOG_FILE_SIZE << "backups:" << MAX_LOG_BACKUPS;
    }

    void removeMessageHandler()
    {
        const QMutexLocker locker(&g_sinkMutex);
        if (!g_installed)
            return;

        qInstallMessageHandler(g_previousHandler);
        g_previousHandler = nullptr;
        g_installed = false;

        if (g_logFile.isOpen())
        {
            g_logFile.flush();
            g_logFile.close();
        }
    }

    QString currentLogFilePath()
    {
        const QMutexLocker locker(&g_sinkMutex);
        return g_logFilePath;
    }
}
