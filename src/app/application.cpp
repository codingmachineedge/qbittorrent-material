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

#include "application.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QIcon>
#include <QLocalServer>
#include <QLocalSocket>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QStandardPaths>
#include <QTranslator>
#include <QVariantMap>

#include "base/logging.h"
#include "base/logger.h"
#include "base/profile.h"
#include "base/settingsstorage.h"
#include "app/appcontroller.h"
#include "app/desktopintegration.h"

// --- Optional cross-team engine headers (tolerant coupling) ------------------
// Application depends on engine/i18n headers produced by other feature teams.
// Guarding with __has_include keeps the bootstrap compiling while those files
// are still landing, and lights up the real wiring the moment they exist.
#if __has_include("base/preferences.h")
#include "base/preferences.h"
#define QBT_HAS_PREFERENCES 1
#endif

#if __has_include("base/bittorrent/session.h")
#include "base/bittorrent/session.h"
#define QBT_HAS_SESSION 1
#endif

#if __has_include("base/rss/rss_session.h")
#include "base/rss/rss_session.h"
#define QBT_HAS_RSS 1
#endif

#if __has_include("base/net/proxyconfigurationmanager.h")
#include "base/net/proxyconfigurationmanager.h"
#define QBT_HAS_PROXY_MANAGER 1
#endif

#if __has_include("base/net/downloadmanager.h")
#include "base/net/downloadmanager.h"
#define QBT_HAS_DOWNLOAD_MANAGER 1
#endif

#if __has_include("base/utils/i18n/funnytranslator.h")
#include "base/utils/i18n/funnytranslator.h"
#define QBT_HAS_FUNNY_TRANSLATOR 1
#elif __has_include("quick/i18n/funnytranslator.h")
#include "quick/i18n/funnytranslator.h"
#define QBT_HAS_FUNNY_TRANSLATOR 1
#endif

using namespace Qt::StringLiterals;

namespace
{
    // Persisted setting key for the runtime UI language (see CONTRACTS §2.3).
    const QString kLanguageKey = u"Appearance/Language"_qs;

    /// A stable, per-user single-instance identifier (so two different logins
    /// on the same host don't collide).
    QString computeInstanceId()
    {
        const QString seed = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)
            + u'|' + qEnvironmentVariable("USER", qEnvironmentVariable("USERNAME"));
        const QByteArray digest =
            QCryptographicHash::hash(seed.toUtf8(), QCryptographicHash::Sha1).toHex().left(16);
        return u"qbittorrent-material-"_qs + QString::fromLatin1(digest);
    }
}

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
    , m_launchTimeSecsSinceEpoch(QDateTime::currentSecsSinceEpoch())
{
    setApplicationName(u"qBittorrent"_qs);
    setOrganizationName(u"qBittorrent"_qs);
    setOrganizationDomain(u"qbittorrent.org"_qs);
    setApplicationVersion(QCoreApplication::applicationVersion().isEmpty()
            ? u"5.0.0"_qs : QCoreApplication::applicationVersion());
    setApplicationDisplayName(u"qBittorrent"_qs);
    setQuitOnLastWindowClosed(false);  // closing the window may just hide to tray

    qCInfo(lcApp) << "Application constructed:" << applicationName()
                  << applicationVersion() << "pid" << applicationPid();

    setupSingleInstance();
}

Application::~Application()
{
    qCDebug(lcApp) << "Application destructor";
    cleanup();

    if (m_instanceServer)
    {
        m_instanceServer->close();
        delete m_instanceServer;
        m_instanceServer = nullptr;
    }
}

Application *Application::instance()
{
    return qobject_cast<Application *>(QCoreApplication::instance());
}

// ---------------------------------------------------------------------------
// Single-instance guard
// ---------------------------------------------------------------------------
void Application::setupSingleInstance()
{
    m_instanceId = computeInstanceId();
    qCDebug(lcApp) << "Single-instance id:" << m_instanceId;

    // Probe for an existing primary by trying to connect to its local server.
    QLocalSocket probe;
    probe.connectToServer(m_instanceId);
    if (probe.waitForConnected(300))
    {
        qCInfo(lcApp) << "Detected an existing primary instance";
        probe.disconnectFromServer();
        m_isPrimaryInstance = false;
        return;
    }

    // Become the primary: (re)create the local server.
    m_instanceServer = new QLocalServer(this);
    QLocalServer::removeServer(m_instanceId);  // clear a stale socket from a crash
    if (!m_instanceServer->listen(m_instanceId))
    {
        qCWarning(lcApp) << "Failed to listen on single-instance socket:"
                         << m_instanceServer->errorString()
                         << "— continuing as primary anyway";
    }

    connect(m_instanceServer, &QLocalServer::newConnection, this, [this]
    {
        qCInfo(lcApp) << "Received activation request from a secondary instance";
        while (QLocalSocket *conn = m_instanceServer->nextPendingConnection())
        {
            conn->waitForReadyRead(200);
            const QByteArray payload = conn->readAll();
            conn->deleteLater();
            if (m_appController)
                m_appController->handleActivationRequest(QString::fromUtf8(payload));
        }
    });

    m_isPrimaryInstance = true;
    qCInfo(lcApp) << "This process is the primary instance";
}

bool Application::isPrimaryInstance() const
{
    return m_isPrimaryInstance;
}

void Application::notifyPrimaryInstance()
{
    qCDebug(lcApp) << "Notifying primary instance of our launch";
    QLocalSocket socket;
    socket.connectToServer(m_instanceId);
    if (socket.waitForConnected(500))
    {
        const QStringList args = QCoreApplication::arguments();
        const QByteArray payload = (args.size() > 1) ? args.at(1).toUtf8() : QByteArray("activate");
        socket.write(payload);
        socket.flush();
        socket.waitForBytesWritten(500);
        socket.disconnectFromServer();
        qCInfo(lcApp) << "Handoff to primary complete";
    }
    else
    {
        qCWarning(lcApp) << "Could not reach primary instance:" << socket.errorString();
    }
}

// ---------------------------------------------------------------------------
// Boot sequence
// ---------------------------------------------------------------------------
int Application::run()
{
    qCInfo(lcApp) << "Application::run() — beginning boot sequence";

    // Engine singletons must be constructed before anything (translation,
    // controllers, QML) touches Preferences/Session.
    initEngine();
    setupTranslation();

    // App-owned QML singleton instances must exist before Main.qml loads,
    // because their create() factories hand back these very objects.
    m_appController = new AppController(this);
    m_desktopIntegration = new DesktopIntegration(this);
    qCDebug(lcApp) << "Created AppController and DesktopIntegration";

    m_engine = new QQmlApplicationEngine(this);
    // Let the engine find QML modules deployed next to the executable (e.g.
    // Qt.labs.platform, copied there by windeployqt) so file-picker dialogs work.
    m_engine->addImportPath(applicationDirPath() + u"/qml"_qs);
    registerContext();
    loadMainQml();

    connect(this, &QCoreApplication::aboutToQuit, this, &Application::cleanup);

    qCInfo(lcApp) << "Entering Qt event loop";
    const int rc = QApplication::exec();
    qCInfo(lcApp) << "Qt event loop exited with code" << rc;
    return rc;
}

void Application::setupTranslation()
{
    int langValue = 0;  // 0 == English (default)
#ifdef QBT_HAS_PREFERENCES
    langValue = Preferences::instance()->value(kLanguageKey, 0).toInt();
    qCInfo(lcI18n) << "Loaded persisted language mode:" << langValue;
#else
    qCWarning(lcI18n) << "Preferences unavailable at startup; defaulting language to English";
#endif

#ifdef QBT_HAS_FUNNY_TRANSLATOR
    using Utils::I18n::FunnyTranslator;
    auto *funny = new FunnyTranslator(this);
    if (funny->loadCatalog(u":/i18n/cantonese.json"_qs))
        qCInfo(lcI18n) << "Cantonese catalog loaded";
    else
        qCWarning(lcI18n) << "Cantonese catalog missing — Cantonese/Bilingual will fall back to English";
    funny->setMode(static_cast<FunnyTranslator::Mode>(langValue));
    m_translator = funny;
    installTranslator(m_translator);
    qCInfo(lcI18n) << "FunnyTranslator installed on qApp before QML load";
#else
    Q_UNUSED(langValue)
    qCWarning(lcI18n) << "FunnyTranslator not available yet; UI will render raw English literals";
#endif
}

void Application::initEngine()
{
    qCInfo(lcEngine) << "Initializing engine singletons";

    // Order matters: Logger and Profile first, then the settings store, then
    // Preferences (reads the store), then the BitTorrent session (reads prefs).
    Logger::initInstance();
    Profile::initInstance({}, {}, false);  // default per-user profile location
    SettingsStorage::initInstance();
    qCDebug(lcEngine) << "Profile + SettingsStorage ready";

#ifdef QBT_HAS_PREFERENCES
    Preferences::initInstance();
    qCDebug(lcEngine) << "Preferences instance ready";
#else
    qCWarning(lcEngine) << "Preferences header not present at build time";
#endif

#ifdef QBT_HAS_PROXY_MANAGER
    // Network proxy manager: an app-owned singleton read by several controllers
    // (e.g. the Options "Connection" tab). It only reads settings, so it is safe
    // to bring up here, after the settings store is ready.
    Net::ProxyConfigurationManager::initInstance();
    qCDebug(lcEngine) << "Net::ProxyConfigurationManager ready";
#endif

#ifdef QBT_HAS_DOWNLOAD_MANAGER
    // The network download manager (also the cookie store used by CookiesModel /
    // URL & RSS downloads). Its constructor wires up the proxy manager, so it
    // must be created after ProxyConfigurationManager above.
    Net::DownloadManager::initInstance();
    qCDebug(lcEngine) << "Net::DownloadManager ready";
#endif

#ifdef QBT_HAS_SESSION
    // Brings up the concrete libtorrent session on its own IO thread.
    BitTorrent::Session::initInstance();
    qCInfo(lcEngine) << "BitTorrent::Session initialized";
#else
    qCWarning(lcEngine) << "BitTorrent::Session header not present at build time";
#endif

#ifdef QBT_HAS_RSS
    // The RSS session is an app-owned singleton (private ctor; Application is a
    // friend). It must exist before any RSS bridge object (e.g. RSSController)
    // is touched by QML, otherwise RSS::Session::instance() is null and the
    // controller dereferences it. Depends on Profile/SettingsStorage/Preferences
    // (already up) so it is created here, after the BitTorrent session.
    if (!RSS::Session::instance())
        new RSS::Session;
    qCInfo(lcEngine) << "RSS::Session initialized";
#endif
}

void Application::registerContext()
{
    Q_ASSERT(m_engine);

    // A tiny read-only context object with build info (About dialog, logs).
    QVariantMap appInfo;
    appInfo.insert(u"name"_qs, applicationName());
    appInfo.insert(u"displayName"_qs, applicationDisplayName());
    appInfo.insert(u"version"_qs, applicationVersion());
    appInfo.insert(u"qtVersion"_qs, QString::fromLatin1(qVersion()));
    appInfo.insert(u"launchTime"_qs, m_launchTimeSecsSinceEpoch);
    m_engine->rootContext()->setContextProperty(u"ApplicationInfo"_qs, appInfo);

    qCDebug(lcApp) << "Registered ApplicationInfo context property";
}

void Application::loadMainQml()
{
    Q_ASSERT(m_engine);

    connect(m_engine, &QQmlApplicationEngine::objectCreationFailed, this, []
    {
        qCCritical(lcApp) << "Failed to create Main.qml root object — aborting";
        QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);

    connect(m_engine, &QQmlApplicationEngine::objectCreated, this,
        [](QObject *obj, const QUrl &url)
        {
            if (obj)
                qCInfo(lcApp) << "Main QML object created:" << url.toString();
        });

    qCInfo(lcApp) << "Loading qBittorrent QML module entry point (Main)";
    m_engine->loadFromModule(u"qBittorrent"_qs, u"Main"_qs);

    if (m_engine->rootObjects().isEmpty())
        qCCritical(lcApp) << "No root QML objects were created";
}

void Application::cleanup()
{
    if (m_cleanupDone)
        return;
    m_cleanupDone = true;

    qCInfo(lcApp) << "Application cleanup started";
    emit aboutToShutDown();

    // Drop the QML engine first so bindings stop touching engine singletons.
    if (m_engine)
    {
        m_engine->deleteLater();
        m_engine = nullptr;
        qCDebug(lcApp) << "QML engine scheduled for deletion";
    }

    qCInfo(lcApp) << "Application cleanup finished";
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------
AppController *Application::appController() const
{
    return m_appController.data();
}

DesktopIntegration *Application::desktopIntegration() const
{
    return m_desktopIntegration.data();
}

QQmlApplicationEngine *Application::qmlEngine() const
{
    return m_engine;
}

qint64 Application::launchTimeSecsSinceEpoch() const
{
    return m_launchTimeSecsSinceEpoch;
}
