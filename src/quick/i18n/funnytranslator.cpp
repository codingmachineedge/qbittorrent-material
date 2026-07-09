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

#include "funnytranslator.h"

#include <QByteArray>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLatin1String>
#include <QStringList>

#include "base/logging.h"

namespace
{
    /// Thin separator between the English and Cantonese halves in Bilingual mode.
    const QString kBilingualSeparator = QStringLiteral(" · "); // " · "

    /// Candidate resource/file locations for the catalog, tried in order. The
    /// exact resource prefix depends on how qt_add_qml_module packaged it.
    const QStringList kCatalogCandidates = {
        QStringLiteral(":/i18n/cantonese.json"),
        QStringLiteral(":/qt/qml/qBittorrent/i18n/cantonese.json"),
        QStringLiteral(":/resources/i18n/cantonese.json"),
        QStringLiteral(":/qbittorrent/i18n/cantonese.json")
    };
}

namespace Utils::I18n
{
    FunnyTranslator::FunnyTranslator(QObject *parent)
        : QTranslator(parent)
    {
        qCDebug(lcI18n) << "FunnyTranslator constructing; probing catalog locations";
        bool loaded = false;
        for (const QString &path : kCatalogCandidates)
        {
            if (QFile::exists(path) && loadCatalog(path))
            {
                loaded = true;
                break;
            }
        }
        if (!loaded)
            qCWarning(lcI18n) << "FunnyTranslator: no Cantonese catalog found; "
                                 "Cantonese/Bilingual modes will fall back to English";
    }

    void FunnyTranslator::setMode(const Mode mode)
    {
        if (m_mode == mode)
        {
            qCDebug(lcI18n) << "FunnyTranslator::setMode no-op; already in mode" << int(mode);
            return;
        }
        qCInfo(lcI18n) << "FunnyTranslator mode changed:" << int(m_mode) << "->" << int(mode);
        m_mode = mode;
    }

    bool FunnyTranslator::loadCatalog(const QString &jsonPath)
    {
        QFile file(jsonPath);
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            qCWarning(lcI18n) << "Cannot open Cantonese catalog:" << jsonPath << file.errorString();
            return false;
        }

        const QByteArray raw = file.readAll();
        file.close();

        QJsonParseError parseError {};
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        {
            qCWarning(lcI18n) << "Malformed Cantonese catalog:" << jsonPath
                              << parseError.errorString();
            return false;
        }

        m_enToYue.clear();
        const QJsonObject obj = doc.object();
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it)
        {
            if (it.value().isString())
                m_enToYue.insert(it.key(), it.value().toString());
        }

        qCInfo(lcI18n) << "Loaded Cantonese catalog:" << jsonPath
                       << "with" << m_enToYue.size() << "entries";
        return true;
    }

    QString FunnyTranslator::translate(const char *context, const char *sourceText,
                                       const char *disambiguation, int n) const
    {
        Q_UNUSED(context)
        Q_UNUSED(disambiguation)
        Q_UNUSED(n)

        if (sourceText == nullptr)
            return {};

        switch (m_mode)
        {
        case Mode::English:
            // Empty -> Qt uses the original English literal.
            return {};

        case Mode::Cantonese:
            // Cantonese if present; empty otherwise so Qt shows the English key.
            return m_enToYue.value(QString::fromUtf8(sourceText));

        case Mode::Bilingual:
        {
            const QString en = QString::fromUtf8(sourceText);
            const auto it = m_enToYue.constFind(en);
            if (it == m_enToYue.constEnd())
                return {}; // no entry -> Qt shows English alone
            return en + kBilingualSeparator + it.value();
        }
        }

        return {};
    }
}
