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
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QVariantMap>

#include <qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

/**
 * @file optionscontroller.h
 * @brief The @c OptionsController QML singleton — bridges the whole Options dialog
 *        (all 9 tabs) to the engine.
 *
 * The Options dialog is a staged editor: every control reads its value from this
 * controller's in-memory staging map (via @ref value) and writes edits back (via
 * @ref setValue) without touching the engine. Only when the user presses OK/Apply
 * does @ref apply commit the entire staging map to
 * @c Preferences / @c BitTorrent::Session / @c Net::ProxyConfigurationManager /
 * @c Net::PortForwarder / @c ThemeManager / @c TorrentFileGuard, then call
 * @c Preferences::apply(). Cancel simply drops the staging map (@ref reset).
 *
 * Values are addressed by stable camelCase string keys (see the loaders in the
 * .cpp). The controller is intentionally engine-typed on write so setting keys and
 * enum numeric values stay identical to legacy qBittorrent.
 *
 * The Advanced tab and the watched-folders view are backed by their own dedicated
 * models (@c AdvancedSettingsModel, @c WatchedFoldersModel); the Options QML calls
 * their `apply()`/`reset()` alongside this controller's.
 *
 * Language is special: it maps to the @c I18n singleton. On @ref apply, if the
 * staged `language` differs, the controller persists it and emits
 * @ref languageChangeRequested so the QML layer calls `I18n.setLanguage(...)` for
 * a live retranslate (keeping the I18n dependency in the view layer).
 */
class OptionsController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_DISABLE_COPY_MOVE(OptionsController)

    /// True when the staging map differs from the last-loaded engine state
    /// (drives the Apply button's enabled state).
    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)
    /// True when a staged change requires an application restart to take effect.
    Q_PROPERTY(bool restartRequired READ isRestartRequired NOTIFY restartRequiredChanged)
    /// Persisted index of the last-viewed tab (`GUI/Preferences/LastViewedPage`).
    Q_PROPERTY(int lastViewedTab READ lastViewedTab WRITE setLastViewedTab NOTIFY lastViewedTabChanged)

public:
    /// Tab order — mirrors the legacy `OptionsDialog::Tabs` enum exactly.
    enum Tab
    {
        BehaviorTab = 0,
        DownloadsTab = 1,
        ConnectionTab = 2,
        SpeedTab = 3,
        BitTorrentTab = 4,
        SearchTab = 5,
        RSSTab = 6,
        WebUITab = 7,
        AdvancedTab = 8
    };
    Q_ENUM(Tab)

    /// QML singleton factory — returns the shared instance.
    static OptionsController *create(QQmlEngine *engine, QJSEngine *scriptEngine);

    explicit OptionsController(QObject *parent = nullptr);

    bool isModified() const { return m_modified; }
    bool isRestartRequired() const { return m_restartRequired; }

    int lastViewedTab() const;
    void setLastViewedTab(int tab);

    // --- QML staging API ---------------------------------------------------

    /// The staged value for @p key (or @p defaultValue if unknown).
    Q_INVOKABLE QVariant value(const QString &key, const QVariant &defaultValue = {}) const;

    /// Stage a new value for @p key. Marks the dialog modified (and flags a
    /// restart requirement for restart-only keys).
    Q_INVOKABLE void setValue(const QString &key, const QVariant &value);

    /// (Re)load the staging map for every tab from the engine, discarding edits.
    Q_INVOKABLE void load();

    /// Commit the staging map to the engine. Returns false if cross-tab validation
    /// fails (the offending tab is reported via @ref validationFailed).
    Q_INVOKABLE bool apply();

    /// Discard staged edits and reload from the engine (Cancel / Reset).
    Q_INVOKABLE void reset();

    /// Helper for the Connection tab "Random" port button.
    Q_INVOKABLE int randomPort() const;

signals:
    void modifiedChanged();
    void restartRequiredChanged();
    void lastViewedTabChanged();

    /// Emitted by @ref apply after a successful commit.
    void applied();
    /// Emitted when @ref apply persists a different language; the QML layer wires
    /// this to `I18n.setLanguage(mode)` for a live retranslate.
    void languageChangeRequested(int mode);
    /// Emitted when @ref apply cannot proceed; @p tab is the offending @ref Tab.
    void validationFailed(int tab, const QString &message);

private:
    // Per-tab loaders (engine -> staging map).
    void loadBehavior();
    void loadDownloads();
    void loadConnection();
    void loadSpeed();
    void loadBitTorrent();
    void loadSearch();
    void loadRSS();
    void loadWebUI();

    // Per-tab appliers (staging map -> engine).
    void applyBehavior();
    void applyDownloads();
    void applyConnection();
    void applySpeed();
    void applyBitTorrent();
    void applySearch();
    void applyRSS();
    void applyWebUI();

    // Cross-tab validation run before any writes (mirrors `applySettings()`).
    bool validate();

    // Staging helpers.
    QVariant staged(const QString &key, const QVariant &def = {}) const;
    void stage(const QString &key, const QVariant &value);
    void markModified();
    void markRestartRequired();

    QVariantMap m_values;   ///< the staging map (all tabs)
    bool m_modified = false;
    bool m_restartRequired = false;
};
