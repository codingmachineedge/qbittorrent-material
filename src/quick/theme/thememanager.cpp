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

#include "thememanager.h"

#include <QFile>
#include <QGuiApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStyleHints>

#include "base/logging.h"
#include "base/preferences.h"

using namespace Qt::StringLiterals;

namespace
{
    const QString kColorSchemeKey = u"Appearance/ColorScheme"_s;
    const QString kTrayIconStyleKey = u"Appearance/TrayIconStyle"_s;

    QColor parseColor(const QString &hex)
    {
        const QColor c(hex);
        return c;
    }
}

ThemeManager *ThemeManager::m_instance = nullptr;

ThemeManager::ThemeManager(QObject *parent)
    : QObject(parent)
{
    qCDebug(lcTheme) << "ThemeManager constructing";

    buildPalette();
    buildNamedIdMap();

    // Restore persisted user choices (legacy setting keys preserved verbatim).
    auto *prefs = Preferences::instance();
    if (prefs)
    {
        m_colorScheme = static_cast<ColorScheme>(
            prefs->value(kColorSchemeKey, static_cast<int>(System)).toInt());
        m_trayIconStyle = static_cast<TrayIconStyle>(
            prefs->value(kTrayIconStyleKey, static_cast<int>(Normal)).toInt());
    }
    qCInfo(lcTheme) << "ThemeManager initialized; colorScheme=" << m_colorScheme
                    << "trayIconStyle=" << m_trayIconStyle << "isDark=" << isDark();

    // Follow the OS when the scheme is `System`.
    if (const QStyleHints *hints = QGuiApplication::styleHints())
    {
        connect(hints, &QStyleHints::colorSchemeChanged, this,
            [this](Qt::ColorScheme) { onSystemColorSchemeChanged(); });
    }
}

ThemeManager *ThemeManager::create(QQmlEngine *engine, QJSEngine *jsEngine)
{
    Q_UNUSED(engine)
    Q_UNUSED(jsEngine)
    return instance();
}

ThemeManager *ThemeManager::instance()
{
    if (!m_instance)
        m_instance = new ThemeManager;
    return m_instance;
}

ThemeManager::ColorScheme ThemeManager::colorScheme() const
{
    return m_colorScheme;
}

void ThemeManager::setColorScheme(ColorScheme value)
{
    if (m_colorScheme == value)
        return;

    qCInfo(lcTheme) << "Color scheme changed" << m_colorScheme << "->" << value;
    m_colorScheme = value;

    if (auto *prefs = Preferences::instance())
    {
        prefs->setValue(kColorSchemeKey, static_cast<int>(value));
        prefs->apply();
    }
    emit themeChanged();
}

ThemeManager::TrayIconStyle ThemeManager::trayIconStyle() const
{
    return m_trayIconStyle;
}

void ThemeManager::setTrayIconStyle(TrayIconStyle value)
{
    if (m_trayIconStyle == value)
        return;

    qCInfo(lcTheme) << "Tray icon style changed" << m_trayIconStyle << "->" << value;
    m_trayIconStyle = value;

    if (auto *prefs = Preferences::instance())
    {
        prefs->setValue(kTrayIconStyleKey, static_cast<int>(value));
        prefs->apply();
    }
    emit trayIconStyleChanged();
}

bool ThemeManager::isDark() const
{
    switch (m_colorScheme)
    {
    case Light:
        return false;
    case Dark:
        return true;
    case System:
    default:
        if (const QStyleHints *hints = QGuiApplication::styleHints())
            return hints->colorScheme() == Qt::ColorScheme::Dark;
        return false;
    }
}

void ThemeManager::onSystemColorSchemeChanged()
{
    if (m_colorScheme != System)
        return; // only relevant while following the OS

    qCInfo(lcTheme) << "System color scheme changed; effective isDark=" << isDark();
    emit themeChanged();
}

QColor ThemeManager::color(const QString &id) const
{
    const bool dark = isDark();

    // 1. User config.json overrides (scheme-specific first, then generic-light).
    if (dark)
    {
        if (const auto it = m_darkOverrides.constFind(id); it != m_darkOverrides.cend())
            return it.value();
    }
    if (const auto it = m_lightOverrides.constFind(id);
        (!dark || !m_darkOverrides.contains(id)) && (it != m_lightOverrides.cend()))
    {
        return it.value();
    }

    // 2. Named-id -> role indirection (resolved recursively).
    if (const auto it = m_namedIdMap.constFind(id); it != m_namedIdMap.cend())
    {
        if (it.value() != id) // guard against a self-referential map entry
            return color(it.value());
    }

    // 3. Base Material / extended role for the active scheme.
    const QHash<QString, QColor> &palette = dark ? m_darkPalette : m_lightPalette;
    if (const auto it = palette.constFind(id); it != palette.cend())
        return it.value();

    // 4. A literal color (`#rrggbb`, `#aarrggbb`, or SVG name).
    if (QColor::isValidColorName(id))
        return QColor::fromString(id);

    // 5. Never return an invalid color.
    qCWarning(lcTheme) << "Unknown color id" << id << "- falling back to onSurface";
    return palette.value(u"onSurface"_s, dark ? QColor(0xe8, 0xea, 0xed) : QColor(0x20, 0x21, 0x24));
}

QString ThemeManager::trayIconName() const
{
    if (m_trayIconStyle == Monochrome)
        return isDark() ? u"qbittorrent-tray-light"_s : u"qbittorrent-tray-dark"_s;
    return u"qbittorrent-tray"_s;
}

bool ThemeManager::loadColorOverrides(const QString &jsonPath)
{
    qCDebug(lcTheme) << "Loading color overrides from" << jsonPath;

    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        qCDebug(lcTheme) << "No color override file at" << jsonPath;
        return false;
    }

    QJsonParseError parseError {};
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject())
    {
        qCWarning(lcTheme) << "Invalid color override JSON:" << parseError.errorString();
        return false;
    }

    const QJsonObject root = doc.object();
    const auto ingest = [](const QJsonObject &obj, QHash<QString, QColor> &target)
    {
        int count = 0;
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)
        {
            const QColor c = parseColor(it.value().toString());
            if (c.isValid())
            {
                target.insert(it.key(), c);
                ++count;
            }
            else
            {
                qCWarning(lcTheme) << "Ignoring invalid override color for" << it.key();
            }
        }
        return count;
    };

    m_lightOverrides.clear();
    m_darkOverrides.clear();
    const int light = ingest(root.value(u"colors"_s).toObject(), m_lightOverrides);
    const int dark = ingest(root.value(u"colors.dark"_s).toObject(), m_darkOverrides);

    qCInfo(lcTheme) << "Loaded color overrides:" << light << "light," << dark << "dark";
    emit themeChanged();
    return (light + dark) > 0;
}

void ThemeManager::buildPalette()
{
    // ---- Base Material roles -------------------------------------------------
    // Light values are the canonical qBittorrent Material design-system tokens.
    // Dark values are their Google Material dark counterparts: neutral chrome,
    // a single blue action accent, and the Google dark status palette.
    const auto put = [this](const QString &id, const QString &light, const QString &dark)
    {
        m_lightPalette.insert(id, parseColor(light));
        m_darkPalette.insert(id, parseColor(dark));
    };

    // Primary family.
    put(u"primary"_s, u"#1a73e8"_s, u"#8ab4f8"_s);
    put(u"onPrimary"_s, u"#ffffff"_s, u"#202124"_s);
    put(u"primaryContainer"_s, u"#e8f0fe"_s, u"#394457"_s);
    put(u"onPrimaryContainer"_s, u"#1a73e8"_s, u"#aecbfa"_s);
    // Oklab mixes from the source contract: accent + 8%/14% black.
    put(u"primaryHover"_s, u"#1666d0"_s, u"#9bbdf8"_s);
    put(u"primaryPressed"_s, u"#135dbe"_s, u"#aecbfa"_s);
    put(u"primaryEmphasis"_s, u"#135dbe"_s, u"#aecbfa"_s);

    // The system has one decorative accent. Secondary stays neutral and
    // tertiary carries the semantic success family rather than adding purple.
    put(u"secondary"_s, u"#3c4043"_s, u"#bdc1c6"_s);
    put(u"onSecondary"_s, u"#ffffff"_s, u"#202124"_s);
    put(u"secondaryContainer"_s, u"#f8fafd"_s, u"#303134"_s);
    put(u"onSecondaryContainer"_s, u"#3c4043"_s, u"#bdc1c6"_s);
    put(u"tertiary"_s, u"#188038"_s, u"#81c995"_s);
    put(u"onTertiary"_s, u"#ffffff"_s, u"#202124"_s);
    put(u"tertiaryContainer"_s, u"#e6f4ea"_s, u"#1e3a2a"_s);
    put(u"onTertiaryContainer"_s, u"#188038"_s, u"#81c995"_s);

    // Surfaces / text.
    put(u"surface"_s, u"#ffffff"_s, u"#292a2d"_s);
    put(u"surfaceVariant"_s, u"#f8fafd"_s, u"#303134"_s);
    put(u"onSurface"_s, u"#202124"_s, u"#e8eaed"_s);
    put(u"onSurfaceVariant"_s, u"#5f6368"_s, u"#9aa0a6"_s);
    put(u"secondaryText"_s, u"#3c4043"_s, u"#bdc1c6"_s);
    put(u"background"_s, u"#f8fafd"_s, u"#202124"_s);
    put(u"onBackground"_s, u"#202124"_s, u"#e8eaed"_s);

    // Lines.
    put(u"outline"_s, u"#dadce0"_s, u"#5f6368"_s);
    put(u"outlineVariant"_s, u"#edf0f2"_s, u"#3c4043"_s);
    put(u"focusRing"_s, u"#3d1a73e8"_s, u"#528ab4f8"_s);
    put(u"scrim"_s, u"#59202124"_s, u"#99000000"_s);

    // Error.
    put(u"error"_s, u"#d93025"_s, u"#f28b82"_s);
    put(u"onError"_s, u"#ffffff"_s, u"#202124"_s);
    put(u"errorContainer"_s, u"#fce8e6"_s, u"#4a2525"_s);
    put(u"onErrorContainer"_s, u"#d93025"_s, u"#f28b82"_s);

    // ---- qBittorrent extended roles. ----
    put(u"success"_s, u"#188038"_s, u"#81c995"_s);
    put(u"onSuccess"_s, u"#ffffff"_s, u"#202124"_s);
    put(u"successContainer"_s, u"#e6f4ea"_s, u"#1e3a2a"_s);
    put(u"onSuccessContainer"_s, u"#188038"_s, u"#81c995"_s);

    put(u"successEmphasis"_s, u"#188038"_s, u"#81c995"_s);
    put(u"onSuccessEmphasis"_s, u"#ffffff"_s, u"#202124"_s);

    put(u"warning"_s, u"#f9ab00"_s, u"#fdd663"_s);
    put(u"onWarning"_s, u"#202124"_s, u"#202124"_s);
    put(u"warningContainer"_s, u"#fef7e0"_s, u"#4a3f1d"_s);
    put(u"onWarningContainer"_s, u"#b06000"_s, u"#fdd663"_s);

    // Completed/done is success in this system; purple is intentionally absent.
    put(u"done"_s, u"#188038"_s, u"#81c995"_s);
    put(u"onDone"_s, u"#ffffff"_s, u"#202124"_s);
    put(u"doneContainer"_s, u"#e6f4ea"_s, u"#1e3a2a"_s);
    put(u"onDoneContainer"_s, u"#188038"_s, u"#81c995"_s);

    put(u"info"_s, u"#1a73e8"_s, u"#8ab4f8"_s); // == primary
    put(u"onInfo"_s, u"#ffffff"_s, u"#202124"_s);

    put(u"muted"_s, u"#5f6368"_s, u"#9aa0a6"_s); // == onSurfaceVariant

    put(u"severe"_s, u"#f9ab00"_s, u"#fdd663"_s); // == warning
    put(u"onSevere"_s, u"#202124"_s, u"#202124"_s);

    qCDebug(lcTheme) << "Palette built:" << m_lightPalette.size() << "roles";
}

void ThemeManager::buildNamedIdMap()
{
    // Canonical design-token aliases. Resolution remains recursive so a user
    // override on either the semantic name (surfaceWarm) or its Material role
    // (primaryContainer) is honored without duplicating palette state.
    m_namedIdMap.insert(u"canvas"_s, u"background"_s);
    m_namedIdMap.insert(u"surfaceWarm"_s, u"primaryContainer"_s);
    m_namedIdMap.insert(u"selectedSurface"_s, u"primaryContainer"_s);
    m_namedIdMap.insert(u"foreground"_s, u"onSurface"_s);
    m_namedIdMap.insert(u"foregroundSecondary"_s, u"secondaryText"_s);
    m_namedIdMap.insert(u"border"_s, u"outline"_s);
    m_namedIdMap.insert(u"borderSoft"_s, u"outlineVariant"_s);
    m_namedIdMap.insert(u"accent"_s, u"primary"_s);
    m_namedIdMap.insert(u"accentOn"_s, u"onPrimary"_s);
    m_namedIdMap.insert(u"danger"_s, u"error"_s);
    m_namedIdMap.insert(u"onDanger"_s, u"onError"_s);
    m_namedIdMap.insert(u"dangerContainer"_s, u"errorContainer"_s);

    // Transfer-list row TEXT color per TorrentState (DESIGN_SYSTEM §1).
    m_namedIdMap.insert(u"ForcedDownloading"_s, u"success"_s);
    m_namedIdMap.insert(u"Downloading"_s, u"success"_s);
    m_namedIdMap.insert(u"ForcedDownloadingMetadata"_s, u"success"_s);
    m_namedIdMap.insert(u"DownloadingMetadata"_s, u"success"_s);
    m_namedIdMap.insert(u"StalledDownloading"_s, u"successEmphasis"_s);
    m_namedIdMap.insert(u"ForcedUploading"_s, u"primary"_s);
    m_namedIdMap.insert(u"Uploading"_s, u"primary"_s);
    m_namedIdMap.insert(u"StalledUploading"_s, u"primaryEmphasis"_s);
    m_namedIdMap.insert(u"CheckingResumeData"_s, u"success"_s);
    m_namedIdMap.insert(u"QueuedDownloading"_s, u"warning"_s);
    m_namedIdMap.insert(u"QueuedUploading"_s, u"warning"_s);
    m_namedIdMap.insert(u"CheckingUploading"_s, u"success"_s);
    m_namedIdMap.insert(u"CheckingDownloading"_s, u"success"_s);
    m_namedIdMap.insert(u"StoppedDownloading"_s, u"muted"_s);
    m_namedIdMap.insert(u"StoppedUploading"_s, u"done"_s);
    m_namedIdMap.insert(u"Moving"_s, u"success"_s);
    m_namedIdMap.insert(u"MissingFiles"_s, u"error"_s);
    m_namedIdMap.insert(u"Error"_s, u"error"_s);
    m_namedIdMap.insert(u"Unknown"_s, u"muted"_s);

    // Execution-log line colors.
    m_namedIdMap.insert(u"Log.TimeStamp"_s, u"muted"_s);
    m_namedIdMap.insert(u"Log.Info"_s, u"info"_s);
    m_namedIdMap.insert(u"Log.Normal"_s, u"onSurface"_s);
    m_namedIdMap.insert(u"Log.Warning"_s, u"severe"_s);
    m_namedIdMap.insert(u"Log.Critical"_s, u"error"_s);
    m_namedIdMap.insert(u"Log.BannedPeer"_s, u"error"_s);

    // RSS article colors.
    m_namedIdMap.insert(u"RSS.UnreadArticle"_s, u"primary"_s);
    m_namedIdMap.insert(u"RSS.ReadArticle"_s, u"muted"_s);

    // Pieces bar.
    m_namedIdMap.insert(u"PiecesBar.Piece"_s, u"primary"_s);
    m_namedIdMap.insert(u"PiecesBar.PartialPiece"_s, u"primaryContainer"_s);
    m_namedIdMap.insert(u"PiecesBar.MissingPiece"_s, u"surfaceVariant"_s);
    m_namedIdMap.insert(u"PiecesBar.Border"_s, u"outlineVariant"_s);

    qCDebug(lcTheme) << "Named-id map built:" << m_namedIdMap.size() << "entries";
}
