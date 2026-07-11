/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026  qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QObject>
#include <QString>
#include <QVariant>

#include <qqmlintegration.h>

class Preferences;
class QQmlEngine;
class QJSEngine;

/**
 * QML facade over the app-owned ::Preferences singleton.
 *
 * The engine preference object deliberately remains part of qbt_base. This
 * bridge gives QML one registered `Preferences` singleton with the generic
 * key/value API and the typed shell helpers used by the native interface.
 * Persisted keys and defaults continue to come from ::Preferences itself.
 */
class PreferencesController final : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Preferences)
    QML_SINGLETON
    Q_DISABLE_COPY_MOVE(PreferencesController)

public:
    static PreferencesController *create(QQmlEngine *, QJSEngine *);

    // Generic settings access used by feature-specific view state.
    Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = {}) const;
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);
    Q_INVOKABLE void apply();

    // General shell visibility and behavior.
    Q_INVOKABLE bool confirmOnExit() const;
    Q_INVOKABLE void setConfirmOnExit(bool confirm);
    Q_INVOKABLE bool isToolbarDisplayed() const;
    Q_INVOKABLE void setToolbarDisplayed(bool displayed);
    Q_INVOKABLE bool isStatusbarDisplayed() const;
    Q_INVOKABLE void setStatusbarDisplayed(bool displayed);
    Q_INVOKABLE bool isStatusbarFreeDiskSpaceDisplayed() const;
    Q_INVOKABLE void setStatusbarFreeDiskSpaceDisplayed(bool displayed);
    Q_INVOKABLE bool isStatusbarExternalIPDisplayed() const;
    Q_INVOKABLE void setStatusbarExternalIPDisplayed(bool displayed);
    Q_INVOKABLE bool isFiltersSidebarVisible() const;
    Q_INVOKABLE void setFiltersSidebarVisible(bool visible);
    Q_INVOKABLE bool speedInTitleBar() const;
    Q_INVOKABLE void showSpeedInTitleBar(bool show);

    // Optional native workspaces.
    Q_INVOKABLE bool isSearchEnabled() const;
    Q_INVOKABLE void setSearchEnabled(bool enabled);
    Q_INVOKABLE bool isRSSWidgetEnabled() const;
    Q_INVOKABLE void setRSSWidgetVisible(bool enabled);

    // Session-completion actions.
    Q_INVOKABLE bool shutdownqBTWhenDownloadsComplete() const;
    Q_INVOKABLE void setShutdownqBTWhenDownloadsComplete(bool enabled);
    Q_INVOKABLE bool suspendWhenDownloadsComplete() const;
    Q_INVOKABLE void setSuspendWhenDownloadsComplete(bool enabled);
    Q_INVOKABLE bool hibernateWhenDownloadsComplete() const;
    Q_INVOKABLE void setHibernateWhenDownloadsComplete(bool enabled);
    Q_INVOKABLE bool rebootWhenDownloadsComplete() const;
    Q_INVOKABLE void setRebootWhenDownloadsComplete(bool enabled);
    Q_INVOKABLE bool shutdownWhenDownloadsComplete() const;
    Q_INVOKABLE void setShutdownWhenDownloadsComplete(bool enabled);

signals:
    /// Forwarded from ::Preferences after apply() flushes changed settings.
    void changed();

private:
    explicit PreferencesController(QObject *parent = nullptr);

    Preferences *m_preferences = nullptr;
};
