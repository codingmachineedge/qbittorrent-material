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
 *
 * Derived from the original qBittorrent (GPLv2+) engine headers; the public
 * class/method names and persisted setting keys are preserved verbatim so that
 * existing configs load and downstream bridge/UI teams can code against a
 * stable contract (see docs/ENGINE_API.md, docs/CONTRACTS.md §6).
 */

#pragma once

#include <chrono>

#include <QHash>
#include <QNetworkProxy>
#include <QObject>
#include <QQueue>
#include <QSet>
#include <QtTypes>

#include "base/path.h"

class QNetworkAccessManager;
class QNetworkCookie;
class QNetworkReply;
class QSslError;
class QUrl;

/// Network subsystem: HTTP(S) downloads, proxy, port-forwarding, GeoIP, DNS,
/// reverse resolution and SMTP. All types live in the `Net` namespace and are
/// engine-owned singletons/helpers (never QML bridge types themselves — a
/// dedicated controller/model wraps them for QML per the architecture).
namespace Net
{
    /// A `host:port` pair used to serialize/rate-limit requests to the same service.
    struct ServiceID
    {
        QString hostName;
        int port = 0;

        /// Builds a ServiceID from a URL's host and (scheme-derived) port.
        static ServiceID fromURL(const QUrl &url);
    };

    std::size_t qHash(const ServiceID &serviceID, std::size_t seed = 0);
    bool operator==(const ServiceID &lhs, const ServiceID &rhs);

    /// Outcome of a single download attempt.
    enum class DownloadStatus
    {
        Success,
        RedirectedToMagnet, ///< The HTTP resource redirected to a `magnet:` URI.
        Failed
    };

    /// Fluent builder describing a single download job (URL, UA, size cap,
    /// optional save-to-file). Instances are cheap value objects.
    class DownloadRequest
    {
    public:
        DownloadRequest(const QString &url); // NOLINT(google-explicit-constructor)
        DownloadRequest(const DownloadRequest &other) = default;

        QString url() const;
        DownloadRequest &url(const QString &value);

        QString userAgent() const;
        DownloadRequest &userAgent(const QString &value);

        /// Maximum accepted payload size in bytes; `0` means unlimited.
        qint64 limit() const;
        DownloadRequest &limit(qint64 value);

        bool saveToFile() const;
        DownloadRequest &saveToFile(bool value);

        /// Destination path when `saveToFile()` is set. If empty, a temporary
        /// file is used and its path is reported in `DownloadResult::filePath`.
        Path destFileName() const;
        DownloadRequest &destFileName(const Path &value);

    private:
        QString m_url;
        QString m_userAgent;
        qint64 m_limit = 0;
        bool m_saveToFile = false;
        Path m_destFileName;
    };

    /// Result payload delivered by `DownloadHandler::finished`.
    struct DownloadResult
    {
        QString url;
        DownloadStatus status = DownloadStatus::Failed;
        QString errorString;
        QByteArray data;
        Path filePath;
        QString magnetURI;
    };

    /// Per-request handle: connect to `finished()` for the result, `cancel()` to abort.
    class DownloadHandler : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(DownloadHandler)

    public:
        using QObject::QObject;

        /// Aborts the in-flight request; `finished` is still emitted (Failed).
        virtual void cancel() = 0;

    signals:
        void finished(const DownloadResult &result);
    };

    class DownloadHandlerImpl;

    /// Singleton HTTP(S) download manager. Owns the shared `QNetworkAccessManager`,
    /// the cookie jar, and per-service sequential-request queues. Bridged to QML by
    /// a controller (never polled). Log every request/redirect/failure via `lcNet`.
    class DownloadManager final : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(DownloadManager)

    public:
        static void initInstance();
        static void freeInstance();
        static DownloadManager *instance();

        /// Starts a download; the returned handler is owned by the manager and
        /// auto-deletes after `finished`. `useProxy` routes through the configured proxy.
        DownloadHandler *download(const DownloadRequest &downloadRequest, bool useProxy);

        /// Convenience overload: start a download and connect `finished` to `slot`
        /// with `context` as the receiver (auto-disconnect on context destruction).
        template <typename Context, typename Func>
        void download(const DownloadRequest &downloadRequest, bool useProxy, Context context, Func &&slot);

        /// Serializes requests to `serviceID`, spacing them by `delay` (rate-limit).
        void registerSequentialService(const ServiceID &serviceID, std::chrono::seconds delay = std::chrono::seconds(0));

        QList<QNetworkCookie> cookiesForUrl(const QUrl &url) const;
        bool setCookiesFromUrl(const QList<QNetworkCookie> &cookieList, const QUrl &url);
        QList<QNetworkCookie> allCookies() const;
        void setAllCookies(const QList<QNetworkCookie> &cookieList);
        bool deleteCookie(const QNetworkCookie &cookie);

        /// True when `url` uses a scheme this manager can fetch (http/https).
        static bool hasSupportedScheme(const QString &url);

    private:
        class NetworkCookieJar;

        explicit DownloadManager(QObject *parent = nullptr);

        void applyProxySettings();
        void processWaitingJobs(const ServiceID &serviceID);
        void processRequest(DownloadHandlerImpl *downloadHandler);

        static DownloadManager *m_instance;
        NetworkCookieJar *m_networkCookieJar = nullptr;
        QNetworkAccessManager *m_networkManager = nullptr;
        QNetworkProxy m_proxy;

        // m_sequentialServices value is the delay between same-host requests.
        QHash<ServiceID, std::chrono::seconds> m_sequentialServices;
        QSet<ServiceID> m_busyServices;
        QHash<ServiceID, QQueue<DownloadHandlerImpl *>> m_waitingJobs;
    };

    template <typename Context, typename Func>
    void DownloadManager::download(const DownloadRequest &downloadRequest, bool useProxy, Context context, Func &&slot)
    {
        const DownloadHandler *handler = download(downloadRequest, useProxy);
        connect(handler, &DownloadHandler::finished, context, std::forward<Func>(slot));
    }
}
