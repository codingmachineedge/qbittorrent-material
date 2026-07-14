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

#include "desktopintegration.h"

#include <QApplication>
#include <QIcon>
#include <QSystemTrayIcon>

#include "base/logging.h"
#include "app/application.h"
#include "quick/theme/thememanager.h"

#if __has_include("base/preferences.h")
#include "base/preferences.h"
#define QBT_HAS_PREFERENCES 1
#endif

using namespace Qt::StringLiterals;

namespace
{
    const QString kNotificationsKey = u"Preferences/General/NotificationEnabled"_qs;

    QString trayIconResource(const ThemeManager::TrayIconStyle style)
    {
        switch (style)
        {
        case ThemeManager::Normal:
            return u":/branding/logo-mark.svg"_qs;
        case ThemeManager::Monochrome:
        default:
            // Legacy profiles may retain the historic dark-monochrome value
            // (2). Treat every non-normal value as monochrome so their tray
            // artwork remains visually compatible.
            return u":/branding/logo-monochrome.svg"_qs;
        }
    }
}

DesktopIntegration::DesktopIntegration(QObject *parent)
    : QObject(parent)
{
    qCDebug(lcUi) << "DesktopIntegration constructing";

#ifdef QBT_HAS_PREFERENCES
    m_notificationsEnabled = Preferences::instance()->value(kNotificationsKey, true).toBool();
#endif

    // OptionsController commits this setting through ThemeManager on Apply.
    // Updating the native icon here keeps the actual system tray in sync with
    // the committed style without making a staged edit visible prematurely.
    connect(ThemeManager::instance(), &ThemeManager::trayIconStyleChanged,
        this, &DesktopIntegration::refreshTrayIcon);

    if (QSystemTrayIcon::isSystemTrayAvailable())
    {
        createTrayIcon();
    }
    else
    {
        qCWarning(lcUi) << "System tray is not available on this platform/session";
    }
}

DesktopIntegration::~DesktopIntegration()
{
    qCDebug(lcUi) << "DesktopIntegration destroyed";
    if (m_trayIcon)
        m_trayIcon->hide();
}

DesktopIntegration *DesktopIntegration::create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
{
    Q_UNUSED(qmlEngine)
    Q_UNUSED(jsEngine)

    Application *app = Application::instance();
    Q_ASSERT_X(app, "DesktopIntegration::create", "Application instance must exist");
    DesktopIntegration *di = app ? app->desktopIntegration() : nullptr;
    Q_ASSERT_X(di, "DesktopIntegration::create", "DesktopIntegration instance must exist");

    QQmlEngine::setObjectOwnership(di, QQmlEngine::CppOwnership);
    qCDebug(lcUi) << "DesktopIntegration singleton handed to QML";
    return di;
}

void DesktopIntegration::createTrayIcon()
{
    m_trayIcon = new QSystemTrayIcon(resolveTrayIcon(), this);
    m_trayIcon->setToolTip(m_toolTip.isEmpty() ? u"qBittorrent"_qs : m_toolTip);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this,
        [this](QSystemTrayIcon::ActivationReason reason)
        {
            qCDebug(lcUi) << "Tray activated, reason =" << reason;
            switch (reason)
            {
            case QSystemTrayIcon::Trigger:
            case QSystemTrayIcon::DoubleClick:
                emit activationRequested();
                break;
            case QSystemTrayIcon::Context:
                emit contextMenuRequested();
                break;
            default:
                break;
            }
        });

    connect(m_trayIcon, &QSystemTrayIcon::messageClicked, this, [this]
    {
        qCDebug(lcUi) << "Tray notification clicked";
        emit notificationClicked();
    });

    m_trayIcon->show();
    qCInfo(lcUi) << "System tray icon created and shown";
}

QIcon DesktopIntegration::resolveTrayIcon() const
{
    const QString resource = trayIconResource(ThemeManager::instance()->trayIconStyle());
    QIcon icon(resource);
    if (icon.isNull())
    {
        qCWarning(lcUi) << "Tray icon resource missing:" << resource
                        << "— falling back to window icon";
        icon = QApplication::windowIcon();
    }
    return icon;
}

void DesktopIntegration::refreshTrayIcon()
{
    if (!m_trayIcon)
        return;
    qCDebug(lcUi) << "Refreshing tray icon artwork";
    m_trayIcon->setIcon(resolveTrayIcon());
}

bool DesktopIntegration::isAvailable() const
{
    return (m_trayIcon != nullptr);
}

bool DesktopIntegration::isVisible() const
{
    return m_trayIcon && m_trayIcon->isVisible();
}

void DesktopIntegration::setVisible(bool visible)
{
    if (!m_trayIcon || (m_trayIcon->isVisible() == visible))
        return;
    qCInfo(lcUi) << "Tray icon visibility ->" << visible;
    m_trayIcon->setVisible(visible);
    emit visibleChanged();
}

bool DesktopIntegration::isNotificationsEnabled() const
{
    return m_notificationsEnabled;
}

void DesktopIntegration::setNotificationsEnabled(bool enabled)
{
    if (m_notificationsEnabled == enabled)
        return;
    m_notificationsEnabled = enabled;
    qCInfo(lcUi) << "Desktop notifications ->" << enabled;
#ifdef QBT_HAS_PREFERENCES
    Preferences::instance()->setValue(kNotificationsKey, enabled);
    Preferences::instance()->apply();
#endif
    emit notificationsEnabledChanged();
}

QString DesktopIntegration::toolTip() const
{
    return m_toolTip;
}

void DesktopIntegration::setToolTip(const QString &toolTip)
{
    if (m_toolTip == toolTip)
        return;
    m_toolTip = toolTip;
    if (m_trayIcon)
        m_trayIcon->setToolTip(toolTip.isEmpty() ? u"qBittorrent"_qs : toolTip);
    qCDebug(lcUi) << "Tray tooltip updated";
    emit toolTipChanged();
}

void DesktopIntegration::showNotification(const QString &title, const QString &message) const
{
    if (!m_notificationsEnabled)
    {
        qCDebug(lcUi) << "Notification suppressed (disabled):" << title;
        return;
    }
    if (!m_trayIcon)
    {
        qCWarning(lcUi) << "Cannot show notification, no tray:" << title << '-' << message;
        return;
    }
    qCInfo(lcUi) << "Showing notification:" << title;
    m_trayIcon->showMessage(title, message, QSystemTrayIcon::Information, 5000);
}
