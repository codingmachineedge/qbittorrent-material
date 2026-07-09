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

#include "optionscontroller.h"

#include <chrono>

#include <QHostAddress>
#include <QRandomGenerator>
#include <QSet>
#include <QTime>

#include "base/bittorrent/session.h"
#include "base/bittorrent/sharelimits.h"
#include "base/bittorrent/torrent.h"
#include "base/bittorrent/torrentcontentlayout.h"
#include "base/logging.h"
#include "base/net/portforwarder.h"
#include "base/net/proxyconfigurationmanager.h"
#include "base/net/smtpencryptiontype.h"
#include "base/preferences.h"
#include "base/rss/rss_autodownloader.h"
#include "base/rss/rss_session.h"
#include "base/torrentfileguard.h"
#include "base/utils/fs/path.h"

#include "../theme/thememanager.h"

using BitTorrent::Session;

namespace
{
    /// Staged keys whose change only takes effect after an application restart.
    const QSet<QString> RESTART_KEYS = {
        QStringLiteral("style"),
        QStringLiteral("colorScheme"),
        QStringLiteral("useCustomTheme"),
        QStringLiteral("customThemePath")
    };

    QString timeToString(const QTime &time)
    {
        return time.toString(QStringLiteral("HH:mm"));
    }

    QTime stringToTime(const QString &str)
    {
        const QTime time = QTime::fromString(str, QStringLiteral("HH:mm"));
        return time.isValid() ? time : QTime(0, 0);
    }
}

OptionsController *OptionsController::create(QQmlEngine *, QJSEngine *)
{
    static auto *instance = new OptionsController;
    // Ownership stays with the app; QML must not delete the singleton.
    QQmlEngine::setObjectOwnership(instance, QQmlEngine::CppOwnership);
    return instance;
}

OptionsController::OptionsController(QObject *parent)
    : QObject(parent)
{
    qCDebug(lcUi) << "OptionsController: constructing";
    load();
}

int OptionsController::lastViewedTab() const
{
    // Stored generically; default to the Behavior tab.
    return m_values.value(QStringLiteral("__lastViewedTab"), BehaviorTab).toInt();
}

void OptionsController::setLastViewedTab(int tab)
{
    if (lastViewedTab() == tab)
        return;
    qCDebug(lcUi) << "OptionsController: last viewed tab ->" << tab;
    m_values[QStringLiteral("__lastViewedTab")] = tab;
    emit lastViewedTabChanged();
}

// ---------------------------------------------------------------------------
// Staging helpers
// ---------------------------------------------------------------------------

QVariant OptionsController::staged(const QString &key, const QVariant &def) const
{
    return m_values.value(key, def);
}

void OptionsController::stage(const QString &key, const QVariant &value)
{
    m_values[key] = value;
}

QVariant OptionsController::value(const QString &key, const QVariant &defaultValue) const
{
    return m_values.value(key, defaultValue);
}

void OptionsController::setValue(const QString &key, const QVariant &value)
{
    if (m_values.value(key) == value)
        return;

    qCDebug(lcUi) << "OptionsController: stage" << key << "=" << value;
    m_values[key] = value;
    markModified();

    if (RESTART_KEYS.contains(key))
        markRestartRequired();
}

void OptionsController::markModified()
{
    if (!m_modified)
    {
        m_modified = true;
        emit modifiedChanged();
    }
}

void OptionsController::markRestartRequired()
{
    if (!m_restartRequired)
    {
        qCInfo(lcUi) << "OptionsController: a staged change requires an application restart";
        m_restartRequired = true;
        emit restartRequiredChanged();
    }
}

int OptionsController::randomPort() const
{
    return QRandomGenerator::global()->bounded(1024, 65536);
}

// ---------------------------------------------------------------------------
// load() — engine -> staging map
// ---------------------------------------------------------------------------

void OptionsController::load()
{
    qCInfo(lcUi) << "OptionsController: loading all tabs from engine";
    m_values.clear();

    loadBehavior();
    loadDownloads();
    loadConnection();
    loadSpeed();
    loadBitTorrent();
    loadSearch();
    loadRSS();
    loadWebUI();

    if (m_modified)
    {
        m_modified = false;
        emit modifiedChanged();
    }
    if (m_restartRequired)
    {
        m_restartRequired = false;
        emit restartRequiredChanged();
    }
}

void OptionsController::reset()
{
    qCDebug(lcUi) << "OptionsController: reset() — discarding staged edits";
    load();
}

void OptionsController::loadBehavior()
{
    auto *pref = Preferences::instance();
    auto *session = Session::instance();
    auto *theme = ThemeManager::instance();

    stage(QStringLiteral("language"), pref->getLanguageMode());
    stage(QStringLiteral("style"), pref->getStyle());
    stage(QStringLiteral("colorScheme"), static_cast<int>(theme->colorScheme()));
    stage(QStringLiteral("useCustomTheme"), pref->useCustomUITheme());
    stage(QStringLiteral("customThemePath"), pref->customUIThemePath().toString());

    stage(QStringLiteral("confirmDeletion"), pref->confirmTorrentDeletion());
    stage(QStringLiteral("alternatingRowColors"), pref->useAlternatingRowColors());
    stage(QStringLiteral("useTorrentStatesColors"), pref->useTorrentStatesColors());
    stage(QStringLiteral("progressBarFollowsTextColor"), pref->getProgressBarFollowsTextColor());
    stage(QStringLiteral("hideZeroValues"), pref->getHideZeroValues());
    stage(QStringLiteral("hideZeroComboValues"), pref->getHideZeroComboValues());
    stage(QStringLiteral("actionDblClickDl"), pref->getActionOnDblClOnTorrentDl());
    stage(QStringLiteral("actionDblClickFn"), pref->getActionOnDblClOnTorrentFn());
    stage(QStringLiteral("hideZeroStatusFilters"), pref->getHideZeroStatusFilters());
    stage(QStringLiteral("useSeparateTrackerStatusFilter"), pref->useSeparateTrackerStatusFilter());
    stage(QStringLiteral("torrentContentDrag"), pref->isTorrentContentDragEnabled());

    stage(QStringLiteral("statusbarFreeDiskSpace"), pref->isStatusbarFreeDiskSpaceDisplayed());
    stage(QStringLiteral("statusbarExternalIP"), pref->isStatusbarExternalIPDisplayed());

    // "Show splash screen" is the inverse of the stored "disabled" flag.
    stage(QStringLiteral("showSplashScreen"), !pref->isSplashScreenDisabled());
    stage(QStringLiteral("confirmOnExit"), pref->confirmOnExit());
    // "Confirm auto-exit" is the inverse of the stored "don't confirm" flag.
    stage(QStringLiteral("confirmAutoExit"), !pref->dontConfirmAutoExit());

#ifndef Q_OS_MACOS
    stage(QStringLiteral("systrayEnabled"), pref->systemTrayEnabled());
    stage(QStringLiteral("minimizeToTray"), pref->minimizeToTray());
    stage(QStringLiteral("closeToTray"), pref->closeToTray());
#endif
    stage(QStringLiteral("trayIconStyle"), static_cast<int>(theme->trayIconStyle()));

    stage(QStringLiteral("preventSuspendDownloading"), pref->preventFromSuspendWhenDownloading());
    stage(QStringLiteral("preventSuspendSeeding"), pref->preventFromSuspendWhenSeeding());

    stage(QStringLiteral("performanceWarning"), session->isPerformanceWarningEnabled());

#ifdef Q_OS_WIN
    stage(QStringLiteral("winStartup"), pref->WinStartup());
#endif
}

void OptionsController::loadDownloads()
{
    auto *pref = Preferences::instance();
    auto *session = Session::instance();

    stage(QStringLiteral("additionDialog"), pref->isAddNewTorrentDialogEnabled());
    stage(QStringLiteral("additionDialogFront"), pref->isAddNewTorrentDialogTopLevel());
    stage(QStringLiteral("contentLayout"), static_cast<int>(session->torrentContentLayout()));
    stage(QStringLiteral("addToQueueTop"), session->isAddTorrentToQueueTop());
    stage(QStringLiteral("addStopped"), session->isAddTorrentStopped());
    stage(QStringLiteral("stopCondition"), static_cast<int>(session->torrentStopCondition()));

    stage(QStringLiteral("mergeTrackers"), session->isMergeTrackersEnabled());
    stage(QStringLiteral("confirmMergeTrackers"), pref->confirmMergeTrackers());

    const TorrentFileGuard::AutoDeleteMode mode = TorrentFileGuard::autoDeleteMode();
    stage(QStringLiteral("deleteTorrentFiles"), (mode != TorrentFileGuard::Never));
    stage(QStringLiteral("deleteTorrentFilesWhenCancelled"), (mode == TorrentFileGuard::Always));

    stage(QStringLiteral("preallocateAll"), session->isPreallocationEnabled());
    stage(QStringLiteral("appendExtension"), session->isAppendExtensionEnabled());
    stage(QStringLiteral("unwantedFolder"), session->isUnwantedFolderEnabled());
    stage(QStringLiteral("recursiveDownload"), pref->isRecursiveDownloadEnabled());

    // TMM combos: index 0 == Manual, 1 == Automatic.
    stage(QStringLiteral("savingModeAutomatic"), !session->isAutoTMMDisabledByDefault());
    stage(QStringLiteral("tmmRelocateOnCategoryChanged"), !session->isDisableAutoTMMWhenCategoryChanged());
    stage(QStringLiteral("tmmRelocateOnDefaultPathChanged"), !session->isDisableAutoTMMWhenDefaultSavePathChanged());
    stage(QStringLiteral("tmmRelocateOnCategorySavePathChanged"), !session->isDisableAutoTMMWhenCategorySavePathChanged());
    stage(QStringLiteral("useCategoryPaths"), session->useCategoryPathsInManualMode());

    stage(QStringLiteral("savePath"), session->savePath().toString());
    stage(QStringLiteral("useDownloadPath"), session->isDownloadPathEnabled());
    stage(QStringLiteral("downloadPath"), session->downloadPath().toString());
    stage(QStringLiteral("exportDir"), session->torrentExportDirectory().toString());
    stage(QStringLiteral("exportDirFinished"), session->finishedTorrentExportDirectory().toString());

    stage(QStringLiteral("excludedFileNamesEnabled"), session->isExcludedFileNamesEnabled());
    stage(QStringLiteral("excludedFileNames"), session->excludedFileNames().join(QLatin1Char('\n')));

    stage(QStringLiteral("mailEnabled"), pref->isMailNotificationEnabled());
    stage(QStringLiteral("mailSender"), pref->getMailNotificationSender());
    stage(QStringLiteral("mailDest"), pref->getMailNotificationEmail());
    stage(QStringLiteral("mailSMTP"), pref->getMailNotificationSMTP());
    stage(QStringLiteral("mailSMTPEncryption"), static_cast<int>(pref->getMailNotificationSMTPEncryptionType()));
    stage(QStringLiteral("mailAuth"), pref->getMailNotificationSMTPAuth());
    stage(QStringLiteral("mailUsername"), pref->getMailNotificationSMTPUsername());
    stage(QStringLiteral("mailPassword"), pref->getMailNotificationSMTPPassword());

    stage(QStringLiteral("autoRunOnAdded"), pref->isAutoRunOnTorrentAddedEnabled());
    stage(QStringLiteral("autoRunOnAddedProgram"), pref->getAutoRunOnTorrentAddedProgram());
    stage(QStringLiteral("autoRunOnFinished"), pref->isAutoRunOnTorrentFinishedEnabled());
    stage(QStringLiteral("autoRunOnFinishedProgram"), pref->getAutoRunOnTorrentFinishedProgram());
#ifdef Q_OS_WIN
    stage(QStringLiteral("autoRunConsole"), pref->isAutoRunConsoleEnabled());
#endif
}

void OptionsController::loadConnection()
{
    auto *session = Session::instance();
    auto *proxyMgr = Net::ProxyConfigurationManager::instance();
    auto *pref = Preferences::instance();

    stage(QStringLiteral("btProtocol"), static_cast<int>(session->btProtocol()));
    stage(QStringLiteral("port"), session->port());
    stage(QStringLiteral("upnp"), Net::PortForwarder::instance()->isEnabled());

    // Connection-limit rows: value <= 0 means "unchecked / unlimited".
    stage(QStringLiteral("maxConnections"), session->maxConnections());
    stage(QStringLiteral("maxConnectionsPerTorrent"), session->maxConnectionsPerTorrent());
    stage(QStringLiteral("maxUploads"), session->maxUploads());
    stage(QStringLiteral("maxUploadsPerTorrent"), session->maxUploadsPerTorrent());

    const Net::ProxyConfiguration proxy = proxyMgr->proxyConfiguration();
    stage(QStringLiteral("proxyType"), static_cast<int>(proxy.type));
    stage(QStringLiteral("proxyIP"), proxy.ip);
    stage(QStringLiteral("proxyPort"), proxy.port);
    stage(QStringLiteral("proxyAuth"), proxy.authEnabled);
    stage(QStringLiteral("proxyUsername"), proxy.username);
    stage(QStringLiteral("proxyPassword"), proxy.password);
    stage(QStringLiteral("proxyHostnameLookup"), proxy.hostnameLookupEnabled);
    stage(QStringLiteral("proxyPeerConnections"), session->isProxyPeerConnectionsEnabled());
    stage(QStringLiteral("useProxyForBT"), pref->useProxyForBT());
    stage(QStringLiteral("useProxyForRSS"), pref->useProxyForRSS());
    stage(QStringLiteral("useProxyForGeneral"), pref->useProxyForGeneralPurposes());

    stage(QStringLiteral("ipFilterEnabled"), session->isIPFilteringEnabled());
    stage(QStringLiteral("ipFilterFile"), session->IPFilterFile().toString());
    stage(QStringLiteral("ipFilterTrackers"), session->isTrackerFilteringEnabled());

    stage(QStringLiteral("addTrackersEnabled"), session->isAddTrackersEnabled());
    stage(QStringLiteral("additionalTrackers"), session->additionalTrackers());
}

void OptionsController::loadSpeed()
{
    auto *session = Session::instance();
    auto *pref = Preferences::instance();

    stage(QStringLiteral("globalDownloadLimit"), session->globalDownloadSpeedLimit() / 1024);
    stage(QStringLiteral("globalUploadLimit"), session->globalUploadSpeedLimit() / 1024);
    stage(QStringLiteral("altDownloadLimit"), session->altGlobalDownloadSpeedLimit() / 1024);
    stage(QStringLiteral("altUploadLimit"), session->altGlobalUploadSpeedLimit() / 1024);
    stage(QStringLiteral("altSpeedEnabled"), session->isAltGlobalSpeedLimitEnabled());

    stage(QStringLiteral("schedulerEnabled"), session->isBandwidthSchedulerEnabled());
    stage(QStringLiteral("schedulerStart"), timeToString(pref->getSchedulerStartTime()));
    stage(QStringLiteral("schedulerEnd"), timeToString(pref->getSchedulerEndTime()));
    stage(QStringLiteral("schedulerDays"), static_cast<int>(pref->getSchedulerDays()));

    stage(QStringLiteral("limitUTPRate"), session->isUTPRateLimited());
    stage(QStringLiteral("limitTCPOverhead"), session->includeOverheadInLimits());
    // UI checkbox is "apply to LAN" == NOT ignoreLimitsOnLAN.
    stage(QStringLiteral("applyLimitsToLAN"), !session->ignoreLimitsOnLAN());
}

void OptionsController::loadBitTorrent()
{
    auto *session = Session::instance();

    stage(QStringLiteral("dht"), session->isDHTEnabled());
    stage(QStringLiteral("pex"), session->isPeXEnabled());
    stage(QStringLiteral("lsd"), session->isLSDEnabled());
    stage(QStringLiteral("encryption"), session->encryption());
    stage(QStringLiteral("anonymousMode"), session->isAnonymousModeEnabled());

    stage(QStringLiteral("queueingEnabled"), session->isQueueingSystemEnabled());
    stage(QStringLiteral("maxActiveDownloads"), session->maxActiveDownloads());
    stage(QStringLiteral("maxActiveUploads"), session->maxActiveUploads());
    stage(QStringLiteral("maxActiveTorrents"), session->maxActiveTorrents());
    stage(QStringLiteral("ignoreSlowTorrents"), session->ignoreSlowTorrentsForQueueing());
    stage(QStringLiteral("slowDownloadRate"), session->downloadRateForSlowTorrents());
    stage(QStringLiteral("slowUploadRate"), session->uploadRateForSlowTorrents());
    stage(QStringLiteral("slowInactivityTimer"), session->slowTorrentsInactivityTimer());

    const BitTorrent::ShareLimits limits = session->shareLimits();
    stage(QStringLiteral("shareRatioLimit"), limits.ratioLimit);
    stage(QStringLiteral("shareSeedingTimeLimit"), limits.seedingTimeLimit);
    stage(QStringLiteral("shareInactiveSeedingTimeLimit"), limits.inactiveSeedingTimeLimit);
    stage(QStringLiteral("shareLimitAction"), static_cast<int>(limits.action));

    stage(QStringLiteral("maxActiveCheckingTorrents"), session->maxActiveCheckingTorrents());
    stage(QStringLiteral("i2pEnabled"), session->isI2PEnabled());
    stage(QStringLiteral("i2pAddress"), session->I2PAddress());
    stage(QStringLiteral("i2pPort"), session->I2PPort());
    stage(QStringLiteral("i2pMixedMode"), session->I2PMixedMode());
}

void OptionsController::loadSearch()
{
    auto *pref = Preferences::instance();

    stage(QStringLiteral("searchEnabled"), pref->isSearchEnabled());
    stage(QStringLiteral("searchHistoryLength"), pref->searchHistoryLength());
    stage(QStringLiteral("storeOpenedSearchTabs"), pref->storeOpenedSearchTabs());
    stage(QStringLiteral("storeOpenedSearchTabResults"), pref->storeOpenedSearchTabResults());
    stage(QStringLiteral("pythonExecutablePath"), pref->getPythonExecutablePath().toString());
}

void OptionsController::loadRSS()
{
    auto *rss = RSS::Session::instance();
    auto *autoDl = RSS::AutoDownloader::instance();
    auto *pref = Preferences::instance();

    stage(QStringLiteral("rssProcessingEnabled"), rss->isProcessingEnabled());
    stage(QStringLiteral("rssRefreshInterval"), rss->refreshInterval());
    stage(QStringLiteral("rssMaxArticlesPerFeed"), rss->maxArticlesPerFeed());
    stage(QStringLiteral("rssAutoDownloadEnabled"), autoDl->isProcessingEnabled());
    stage(QStringLiteral("rssDownloadRepacks"), autoDl->downloadRepacks());
    stage(QStringLiteral("rssWidgetEnabled"), pref->isRSSWidgetEnabled());
}

void OptionsController::loadWebUI()
{
    auto *pref = Preferences::instance();

    stage(QStringLiteral("webUIEnabled"), pref->isWebUIEnabled());
    stage(QStringLiteral("webUIAddress"), pref->getWebUIAddress());
    stage(QStringLiteral("webUIPort"), pref->getWebUIPort());
    stage(QStringLiteral("webUIUPnP"), pref->useUPnPForWebUIPort());
    stage(QStringLiteral("webUIServerDomains"), pref->getServerDomains());

    stage(QStringLiteral("webUIUsername"), pref->getWebUIUsername());
    stage(QStringLiteral("webUILocalAuth"), pref->isWebUILocalAuthEnabled());
    stage(QStringLiteral("webUIAuthSubnetWhitelistEnabled"), pref->isWebUIAuthSubnetWhitelistEnabled());
    stage(QStringLiteral("webUIMaxAuthFailCount"), pref->getWebUIMaxAuthFailCount());
    stage(QStringLiteral("webUIBanDuration"), static_cast<int>(pref->getWebUIBanDuration().count()));
    stage(QStringLiteral("webUISessionTimeout"), pref->getWebUISessionTimeout());

    stage(QStringLiteral("webUIClickjacking"), pref->isWebUIClickjackingProtectionEnabled());
    stage(QStringLiteral("webUICSRF"), pref->isWebUICSRFProtectionEnabled());
    stage(QStringLiteral("webUISecureCookie"), pref->isWebUISecureCookieEnabled());
    stage(QStringLiteral("webUIHostHeaderValidation"), pref->isWebUIHostHeaderValidationEnabled());

    stage(QStringLiteral("webUIHttps"), pref->isWebUIHttpsEnabled());
    stage(QStringLiteral("webUIHttpsCert"), pref->getWebUIHttpsCertificatePath().toString());
    stage(QStringLiteral("webUIHttpsKey"), pref->getWebUIHttpsKeyPath().toString());

    stage(QStringLiteral("webUIAltEnabled"), pref->isAltWebUIEnabled());
    stage(QStringLiteral("webUIRootFolder"), pref->getWebUIRootFolder().toString());

    stage(QStringLiteral("webUICustomHeadersEnabled"), pref->isWebUICustomHTTPHeadersEnabled());
    stage(QStringLiteral("webUICustomHeaders"), pref->getWebUICustomHTTPHeaders());
    stage(QStringLiteral("webUIReverseProxyEnabled"), pref->isWebUIReverseProxySupportEnabled());
    stage(QStringLiteral("webUIReverseProxyList"), pref->getWebUITrustedReverseProxiesList());

    stage(QStringLiteral("dynDNSEnabled"), pref->isDynDNSEnabled());
    stage(QStringLiteral("dynDNSService"), static_cast<int>(pref->getDynDNSService()));
    stage(QStringLiteral("dynDNSDomain"), pref->getDynDomainName());
    stage(QStringLiteral("dynDNSUsername"), pref->getDynDNSUsername());
    stage(QStringLiteral("dynDNSPassword"), pref->getDynDNSPassword());
}

// ---------------------------------------------------------------------------
// apply() — staging map -> engine
// ---------------------------------------------------------------------------

bool OptionsController::validate()
{
    // Speed: bandwidth scheduler needs distinct start/end times.
    if (staged(QStringLiteral("schedulerEnabled")).toBool())
    {
        if (staged(QStringLiteral("schedulerStart")).toString() == staged(QStringLiteral("schedulerEnd")).toString())
        {
            const QString msg = tr("The start time and the end time can't be the same.");
            qCWarning(lcUi) << "OptionsController: validation failed (Speed):" << msg;
            emit validationFailed(SpeedTab, msg);
            return false;
        }
    }

    // WebUI: a username is required when the server is enabled.
    if (staged(QStringLiteral("webUIEnabled")).toBool())
    {
        if (staged(QStringLiteral("webUIUsername")).toString().trimmed().length() < 3)
        {
            const QString msg = tr("The Web UI username must be at least 3 characters long.");
            qCWarning(lcUi) << "OptionsController: validation failed (WebUI):" << msg;
            emit validationFailed(WebUITab, msg);
            return false;
        }
    }

    return true;
}

bool OptionsController::apply()
{
    qCInfo(lcUi) << "OptionsController: apply() requested";
    if (!validate())
        return false;

    applyBehavior();
    applyDownloads();
    applyConnection();
    applySpeed();
    applyBitTorrent();
    applySearch();
    applyRSS();
    applyWebUI();

    // Flush persisted preferences to disk once.
    Preferences::instance()->apply();

    m_modified = false;
    emit modifiedChanged();
    emit applied();
    qCInfo(lcUi) << "OptionsController: settings applied";
    return true;
}

void OptionsController::applyBehavior()
{
    auto *pref = Preferences::instance();
    auto *session = Session::instance();
    auto *theme = ThemeManager::instance();

    const int newLanguage = staged(QStringLiteral("language"), 0).toInt();
    if (newLanguage != pref->getLanguageMode())
    {
        qCInfo(lcUi) << "OptionsController: language mode ->" << newLanguage;
        pref->setLanguageMode(newLanguage);
        // The live retranslate is performed by the QML layer via I18n.
        emit languageChangeRequested(newLanguage);
    }

    pref->setStyle(staged(QStringLiteral("style")).toString());
    theme->setColorScheme(static_cast<ThemeManager::ColorScheme>(staged(QStringLiteral("colorScheme")).toInt()));
    pref->setUseCustomUITheme(staged(QStringLiteral("useCustomTheme")).toBool());
    pref->setCustomUIThemePath(Path(staged(QStringLiteral("customThemePath")).toString()));

    pref->setConfirmTorrentDeletion(staged(QStringLiteral("confirmDeletion")).toBool());
    pref->setAlternatingRowColors(staged(QStringLiteral("alternatingRowColors")).toBool());
    pref->setUseTorrentStatesColors(staged(QStringLiteral("useTorrentStatesColors")).toBool());
    pref->setProgressBarFollowsTextColor(staged(QStringLiteral("progressBarFollowsTextColor")).toBool());
    pref->setHideZeroValues(staged(QStringLiteral("hideZeroValues")).toBool());
    pref->setHideZeroComboValues(staged(QStringLiteral("hideZeroComboValues")).toInt());
    pref->setActionOnDblClOnTorrentDl(staged(QStringLiteral("actionDblClickDl")).toInt());
    pref->setActionOnDblClOnTorrentFn(staged(QStringLiteral("actionDblClickFn")).toInt());
    pref->setHideZeroStatusFilters(staged(QStringLiteral("hideZeroStatusFilters")).toBool());
    pref->setUseSeparateTrackerStatusFilter(staged(QStringLiteral("useSeparateTrackerStatusFilter")).toBool());
    pref->setTorrentContentDragEnabled(staged(QStringLiteral("torrentContentDrag")).toBool());

    pref->setStatusbarFreeDiskSpaceDisplayed(staged(QStringLiteral("statusbarFreeDiskSpace")).toBool());
    pref->setStatusbarExternalIPDisplayed(staged(QStringLiteral("statusbarExternalIP")).toBool());

    pref->setSplashScreenDisabled(!staged(QStringLiteral("showSplashScreen")).toBool());
    pref->setConfirmOnExit(staged(QStringLiteral("confirmOnExit")).toBool());
    pref->setDontConfirmAutoExit(!staged(QStringLiteral("confirmAutoExit")).toBool());

#ifndef Q_OS_MACOS
    pref->setSystemTrayEnabled(staged(QStringLiteral("systrayEnabled")).toBool());
    pref->setMinimizeToTray(staged(QStringLiteral("minimizeToTray")).toBool());
    pref->setCloseToTray(staged(QStringLiteral("closeToTray")).toBool());
#endif
    theme->setTrayIconStyle(static_cast<ThemeManager::TrayIconStyle>(staged(QStringLiteral("trayIconStyle")).toInt()));

    pref->setPreventFromSuspendWhenDownloading(staged(QStringLiteral("preventSuspendDownloading")).toBool());
    pref->setPreventFromSuspendWhenSeeding(staged(QStringLiteral("preventSuspendSeeding")).toBool());

    session->setPerformanceWarningEnabled(staged(QStringLiteral("performanceWarning")).toBool());

#ifdef Q_OS_WIN
    pref->setWinStartup(staged(QStringLiteral("winStartup")).toBool());
#endif
}

void OptionsController::applyDownloads()
{
    auto *pref = Preferences::instance();
    auto *session = Session::instance();

    pref->setAddNewTorrentDialogEnabled(staged(QStringLiteral("additionDialog")).toBool());
    pref->setAddNewTorrentDialogTopLevel(staged(QStringLiteral("additionDialogFront")).toBool());
    session->setTorrentContentLayout(static_cast<BitTorrent::TorrentContentLayout>(staged(QStringLiteral("contentLayout")).toInt()));
    session->setAddTorrentToQueueTop(staged(QStringLiteral("addToQueueTop")).toBool());
    session->setAddTorrentStopped(staged(QStringLiteral("addStopped")).toBool());
    session->setTorrentStopCondition(static_cast<BitTorrent::Torrent::StopCondition>(staged(QStringLiteral("stopCondition")).toInt()));

    session->setMergeTrackersEnabled(staged(QStringLiteral("mergeTrackers")).toBool());
    pref->setConfirmMergeTrackers(staged(QStringLiteral("confirmMergeTrackers")).toBool());

    // .torrent auto-delete tri-state: Never / IfAdded / Always.
    const bool deleteFiles = staged(QStringLiteral("deleteTorrentFiles")).toBool();
    const bool deleteWhenCancelled = staged(QStringLiteral("deleteTorrentFilesWhenCancelled")).toBool();
    const TorrentFileGuard::AutoDeleteMode mode = !deleteFiles
        ? TorrentFileGuard::Never
        : (deleteWhenCancelled ? TorrentFileGuard::Always : TorrentFileGuard::IfAdded);
    TorrentFileGuard::setAutoDeleteMode(mode);

    session->setPreallocationEnabled(staged(QStringLiteral("preallocateAll")).toBool());
    session->setAppendExtensionEnabled(staged(QStringLiteral("appendExtension")).toBool());
    session->setUnwantedFolderEnabled(staged(QStringLiteral("unwantedFolder")).toBool());
    pref->setRecursiveDownloadEnabled(staged(QStringLiteral("recursiveDownload")).toBool());

    session->setAutoTMMDisabledByDefault(!staged(QStringLiteral("savingModeAutomatic")).toBool());
    session->setDisableAutoTMMWhenCategoryChanged(!staged(QStringLiteral("tmmRelocateOnCategoryChanged")).toBool());
    session->setDisableAutoTMMWhenDefaultSavePathChanged(!staged(QStringLiteral("tmmRelocateOnDefaultPathChanged")).toBool());
    session->setDisableAutoTMMWhenCategorySavePathChanged(!staged(QStringLiteral("tmmRelocateOnCategorySavePathChanged")).toBool());
    session->setUseCategoryPathsInManualMode(staged(QStringLiteral("useCategoryPaths")).toBool());

    session->setSavePath(Path(staged(QStringLiteral("savePath")).toString()));
    session->setDownloadPathEnabled(staged(QStringLiteral("useDownloadPath")).toBool());
    session->setDownloadPath(Path(staged(QStringLiteral("downloadPath")).toString()));
    session->setTorrentExportDirectory(Path(staged(QStringLiteral("exportDir")).toString()));
    session->setFinishedTorrentExportDirectory(Path(staged(QStringLiteral("exportDirFinished")).toString()));

    session->setExcludedFileNamesEnabled(staged(QStringLiteral("excludedFileNamesEnabled")).toBool());
    session->setExcludedFileNames(staged(QStringLiteral("excludedFileNames")).toString()
            .split(QLatin1Char('\n'), Qt::SkipEmptyParts));

    pref->setMailNotificationEnabled(staged(QStringLiteral("mailEnabled")).toBool());
    pref->setMailNotificationSender(staged(QStringLiteral("mailSender")).toString());
    pref->setMailNotificationEmail(staged(QStringLiteral("mailDest")).toString());
    pref->setMailNotificationSMTP(staged(QStringLiteral("mailSMTP")).toString());
    pref->setMailNotificationSMTPEncryptionType(static_cast<Net::SMTPEncryptionType>(staged(QStringLiteral("mailSMTPEncryption")).toInt()));
    pref->setMailNotificationSMTPAuth(staged(QStringLiteral("mailAuth")).toBool());
    pref->setMailNotificationSMTPUsername(staged(QStringLiteral("mailUsername")).toString());
    pref->setMailNotificationSMTPPassword(staged(QStringLiteral("mailPassword")).toString());

    pref->setAutoRunOnTorrentAddedEnabled(staged(QStringLiteral("autoRunOnAdded")).toBool());
    pref->setAutoRunOnTorrentAddedProgram(staged(QStringLiteral("autoRunOnAddedProgram")).toString().trimmed());
    pref->setAutoRunOnTorrentFinishedEnabled(staged(QStringLiteral("autoRunOnFinished")).toBool());
    pref->setAutoRunOnTorrentFinishedProgram(staged(QStringLiteral("autoRunOnFinishedProgram")).toString().trimmed());
#ifdef Q_OS_WIN
    pref->setAutoRunConsoleEnabled(staged(QStringLiteral("autoRunConsole")).toBool());
#endif
}

void OptionsController::applyConnection()
{
    auto *session = Session::instance();
    auto *proxyMgr = Net::ProxyConfigurationManager::instance();
    auto *pref = Preferences::instance();

    session->setBTProtocol(static_cast<BitTorrent::BTProtocol>(staged(QStringLiteral("btProtocol")).toInt()));
    session->setPort(staged(QStringLiteral("port")).toInt());
    Net::PortForwarder::instance()->setEnabled(staged(QStringLiteral("upnp")).toBool());

    session->setMaxConnections(staged(QStringLiteral("maxConnections")).toInt());
    session->setMaxConnectionsPerTorrent(staged(QStringLiteral("maxConnectionsPerTorrent")).toInt());
    session->setMaxUploads(staged(QStringLiteral("maxUploads")).toInt());
    session->setMaxUploadsPerTorrent(staged(QStringLiteral("maxUploadsPerTorrent")).toInt());

    Net::ProxyConfiguration proxy;
    proxy.type = static_cast<Net::ProxyType>(staged(QStringLiteral("proxyType")).toInt());
    proxy.ip = staged(QStringLiteral("proxyIP")).toString();
    proxy.port = static_cast<ushort>(staged(QStringLiteral("proxyPort")).toUInt());
    proxy.authEnabled = staged(QStringLiteral("proxyAuth")).toBool();
    proxy.username = staged(QStringLiteral("proxyUsername")).toString();
    proxy.password = staged(QStringLiteral("proxyPassword")).toString();
    proxy.hostnameLookupEnabled = staged(QStringLiteral("proxyHostnameLookup")).toBool();
    proxyMgr->setProxyConfiguration(proxy);

    session->setProxyPeerConnectionsEnabled(staged(QStringLiteral("proxyPeerConnections")).toBool());
    pref->setUseProxyForBT(staged(QStringLiteral("useProxyForBT")).toBool());
    pref->setUseProxyForRSS(staged(QStringLiteral("useProxyForRSS")).toBool());
    pref->setUseProxyForGeneralPurposes(staged(QStringLiteral("useProxyForGeneral")).toBool());

    session->setIPFilteringEnabled(staged(QStringLiteral("ipFilterEnabled")).toBool());
    session->setIPFilterFile(Path(staged(QStringLiteral("ipFilterFile")).toString()));
    session->setTrackerFilteringEnabled(staged(QStringLiteral("ipFilterTrackers")).toBool());

    session->setAddTrackersEnabled(staged(QStringLiteral("addTrackersEnabled")).toBool());
    session->setAdditionalTrackers(staged(QStringLiteral("additionalTrackers")).toString());
}

void OptionsController::applySpeed()
{
    auto *session = Session::instance();
    auto *pref = Preferences::instance();

    session->setGlobalDownloadSpeedLimit(staged(QStringLiteral("globalDownloadLimit")).toInt() * 1024);
    session->setGlobalUploadSpeedLimit(staged(QStringLiteral("globalUploadLimit")).toInt() * 1024);
    session->setAltGlobalDownloadSpeedLimit(staged(QStringLiteral("altDownloadLimit")).toInt() * 1024);
    session->setAltGlobalUploadSpeedLimit(staged(QStringLiteral("altUploadLimit")).toInt() * 1024);
    session->setAltGlobalSpeedLimitEnabled(staged(QStringLiteral("altSpeedEnabled")).toBool());

    session->setBandwidthSchedulerEnabled(staged(QStringLiteral("schedulerEnabled")).toBool());
    pref->setSchedulerStartTime(stringToTime(staged(QStringLiteral("schedulerStart")).toString()));
    pref->setSchedulerEndTime(stringToTime(staged(QStringLiteral("schedulerEnd")).toString()));
    pref->setSchedulerDays(static_cast<Scheduler::Days>(staged(QStringLiteral("schedulerDays")).toInt()));

    session->setUTPRateLimited(staged(QStringLiteral("limitUTPRate")).toBool());
    session->setIncludeOverheadInLimits(staged(QStringLiteral("limitTCPOverhead")).toBool());
    session->setIgnoreLimitsOnLAN(!staged(QStringLiteral("applyLimitsToLAN")).toBool());
}

void OptionsController::applyBitTorrent()
{
    auto *session = Session::instance();

    session->setDHTEnabled(staged(QStringLiteral("dht")).toBool());
    session->setPeXEnabled(staged(QStringLiteral("pex")).toBool());
    session->setLSDEnabled(staged(QStringLiteral("lsd")).toBool());
    session->setEncryption(staged(QStringLiteral("encryption")).toInt());
    session->setAnonymousModeEnabled(staged(QStringLiteral("anonymousMode")).toBool());

    session->setQueueingSystemEnabled(staged(QStringLiteral("queueingEnabled")).toBool());
    session->setMaxActiveDownloads(staged(QStringLiteral("maxActiveDownloads")).toInt());
    session->setMaxActiveUploads(staged(QStringLiteral("maxActiveUploads")).toInt());
    session->setMaxActiveTorrents(staged(QStringLiteral("maxActiveTorrents")).toInt());
    session->setIgnoreSlowTorrentsForQueueing(staged(QStringLiteral("ignoreSlowTorrents")).toBool());
    session->setDownloadRateForSlowTorrents(staged(QStringLiteral("slowDownloadRate")).toInt());
    session->setUploadRateForSlowTorrents(staged(QStringLiteral("slowUploadRate")).toInt());
    session->setSlowTorrentsInactivityTimer(staged(QStringLiteral("slowInactivityTimer")).toInt());

    BitTorrent::ShareLimits limits = session->shareLimits();
    limits.ratioLimit = staged(QStringLiteral("shareRatioLimit")).toReal();
    limits.seedingTimeLimit = staged(QStringLiteral("shareSeedingTimeLimit")).toInt();
    limits.inactiveSeedingTimeLimit = staged(QStringLiteral("shareInactiveSeedingTimeLimit")).toInt();
    limits.action = static_cast<BitTorrent::ShareLimitAction>(staged(QStringLiteral("shareLimitAction")).toInt());
    session->setShareLimits(limits);

    session->setMaxActiveCheckingTorrents(staged(QStringLiteral("maxActiveCheckingTorrents")).toInt());
    session->setI2PEnabled(staged(QStringLiteral("i2pEnabled")).toBool());
    session->setI2PAddress(staged(QStringLiteral("i2pAddress")).toString());
    session->setI2PPort(staged(QStringLiteral("i2pPort")).toInt());
    session->setI2PMixedMode(staged(QStringLiteral("i2pMixedMode")).toBool());
}

void OptionsController::applySearch()
{
    auto *pref = Preferences::instance();

    pref->setSearchEnabled(staged(QStringLiteral("searchEnabled")).toBool());
    pref->setSearchHistoryLength(staged(QStringLiteral("searchHistoryLength")).toInt());
    pref->setStoreOpenedSearchTabs(staged(QStringLiteral("storeOpenedSearchTabs")).toBool());
    pref->setStoreOpenedSearchTabResults(staged(QStringLiteral("storeOpenedSearchTabResults")).toBool());
    pref->setPythonExecutablePath(Path(staged(QStringLiteral("pythonExecutablePath")).toString()));
}

void OptionsController::applyRSS()
{
    auto *rss = RSS::Session::instance();
    auto *autoDl = RSS::AutoDownloader::instance();
    auto *pref = Preferences::instance();

    rss->setProcessingEnabled(staged(QStringLiteral("rssProcessingEnabled")).toBool());
    rss->setRefreshInterval(staged(QStringLiteral("rssRefreshInterval")).toInt());
    rss->setMaxArticlesPerFeed(staged(QStringLiteral("rssMaxArticlesPerFeed")).toInt());
    autoDl->setProcessingEnabled(staged(QStringLiteral("rssAutoDownloadEnabled")).toBool());
    autoDl->setDownloadRepacks(staged(QStringLiteral("rssDownloadRepacks")).toBool());
    pref->setRSSWidgetVisible(staged(QStringLiteral("rssWidgetEnabled")).toBool());
}

void OptionsController::applyWebUI()
{
    auto *pref = Preferences::instance();

    pref->setWebUIEnabled(staged(QStringLiteral("webUIEnabled")).toBool());
    pref->setWebUIAddress(staged(QStringLiteral("webUIAddress")).toString());
    pref->setWebUIPort(static_cast<quint16>(staged(QStringLiteral("webUIPort")).toUInt()));
    pref->setUPnPForWebUIPort(staged(QStringLiteral("webUIUPnP")).toBool());
    pref->setServerDomains(staged(QStringLiteral("webUIServerDomains")).toString());

    pref->setWebUIUsername(staged(QStringLiteral("webUIUsername")).toString());
    // The password is only rewritten when the user actually typed a new one.
    const QString newPassword = staged(QStringLiteral("webUIPassword")).toString();
    if (!newPassword.isEmpty())
        pref->setWebUIPassword(newPassword.toUtf8());
    pref->setWebUILocalAuthEnabled(staged(QStringLiteral("webUILocalAuth")).toBool());
    pref->setWebUIAuthSubnetWhitelistEnabled(staged(QStringLiteral("webUIAuthSubnetWhitelistEnabled")).toBool());
    pref->setWebUIMaxAuthFailCount(staged(QStringLiteral("webUIMaxAuthFailCount")).toInt());
    pref->setWebUIBanDuration(std::chrono::seconds(staged(QStringLiteral("webUIBanDuration")).toInt()));
    pref->setWebUISessionTimeout(staged(QStringLiteral("webUISessionTimeout")).toInt());

    pref->setWebUIClickjackingProtectionEnabled(staged(QStringLiteral("webUIClickjacking")).toBool());
    pref->setWebUICSRFProtectionEnabled(staged(QStringLiteral("webUICSRF")).toBool());
    pref->setWebUISecureCookieEnabled(staged(QStringLiteral("webUISecureCookie")).toBool());
    pref->setWebUIHostHeaderValidationEnabled(staged(QStringLiteral("webUIHostHeaderValidation")).toBool());

    pref->setWebUIHttpsEnabled(staged(QStringLiteral("webUIHttps")).toBool());
    pref->setWebUIHttpsCertificatePath(Path(staged(QStringLiteral("webUIHttpsCert")).toString()));
    pref->setWebUIHttpsKeyPath(Path(staged(QStringLiteral("webUIHttpsKey")).toString()));

    pref->setAltWebUIEnabled(staged(QStringLiteral("webUIAltEnabled")).toBool());
    pref->setWebUIRootFolder(Path(staged(QStringLiteral("webUIRootFolder")).toString()));

    pref->setWebUICustomHTTPHeadersEnabled(staged(QStringLiteral("webUICustomHeadersEnabled")).toBool());
    pref->setWebUICustomHTTPHeaders(staged(QStringLiteral("webUICustomHeaders")).toString());
    pref->setWebUIReverseProxySupportEnabled(staged(QStringLiteral("webUIReverseProxyEnabled")).toBool());
    pref->setWebUITrustedReverseProxiesList(staged(QStringLiteral("webUIReverseProxyList")).toString());

    pref->setDynDNSEnabled(staged(QStringLiteral("dynDNSEnabled")).toBool());
    pref->setDynDNSService(static_cast<DNS::Service>(staged(QStringLiteral("dynDNSService")).toInt()));
    pref->setDynDomainName(staged(QStringLiteral("dynDNSDomain")).toString());
    pref->setDynDNSUsername(staged(QStringLiteral("dynDNSUsername")).toString());
    pref->setDynDNSPassword(staged(QStringLiteral("dynDNSPassword")).toString());
}
