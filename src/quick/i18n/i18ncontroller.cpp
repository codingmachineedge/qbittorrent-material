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

#include "i18ncontroller.h"

#include <QCoreApplication>

#include "base/logging.h"
#include "base/preferences.h"
#include "funnytranslator.h"

using Utils::I18n::FunnyTranslator;

namespace
{
    /// Persisted-settings key for the chosen language (integer enum value).
    const QString kLanguageKey = QStringLiteral("Appearance/Language");

    FunnyTranslator::Mode toMode(const I18n::Language lang)
    {
        switch (lang)
        {
        case I18n::Cantonese: return FunnyTranslator::Mode::Cantonese;
        case I18n::Bilingual: return FunnyTranslator::Mode::Bilingual;
        case I18n::English:   break;
        }
        return FunnyTranslator::Mode::English;
    }

    I18n::Language clampLanguage(const int raw)
    {
        if ((raw >= I18n::English) && (raw <= I18n::Bilingual))
            return static_cast<I18n::Language>(raw);
        return I18n::English;
    }
}

I18n *I18n::s_instance = nullptr;

I18n *I18n::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine)

    if (s_instance != nullptr)
    {
        qCDebug(lcI18n) << "I18n::create returning existing instance";
        return s_instance;
    }

    s_instance = new I18n(engine);
    QJSEngine::setObjectOwnership(s_instance, QJSEngine::CppOwnership);
    return s_instance;
}

I18n::I18n(QQmlEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
    qCInfo(lcI18n) << "I18n controller initializing";

    m_translator = new FunnyTranslator(this);

    // Restore the persisted language (default English).
    if (Preferences::instance() != nullptr)
        m_language = clampLanguage(Preferences::instance()->value(kLanguageKey, int(English)).toInt());

    m_translator->setMode(toMode(m_language));
    QCoreApplication::installTranslator(m_translator);

    qCInfo(lcI18n) << "I18n ready; language:" << displayName(m_language)
                   << "catalog entries:" << m_translator->entryCount();
}

I18n::~I18n()
{
    if (m_translator != nullptr)
        QCoreApplication::removeTranslator(m_translator);
    if (s_instance == this)
        s_instance = nullptr;
}

QString I18n::languageName() const
{
    return displayName(m_language);
}

QString I18n::displayName(const Language lang) const
{
    // Endonyms — always shown in their own script so all three are legible
    // regardless of the currently active language.
    switch (lang)
    {
    case Cantonese: return QString::fromUtf8("廣東話");
    case Bilingual: return QString::fromUtf8("English · 廣東話");
    case English:   break;
    }
    return QStringLiteral("English");
}

void I18n::setLanguage(const Language lang)
{
    if (m_language == lang)
    {
        qCDebug(lcI18n) << "setLanguage no-op; already" << displayName(lang);
        return;
    }

    qCInfo(lcI18n) << "Language change requested:" << displayName(m_language)
                   << "->" << displayName(lang);

    m_language = lang;
    applyMode(lang);

    // Persist as the integer enum value under Appearance/Language.
    if (Preferences::instance() != nullptr)
    {
        Preferences::instance()->setValue(kLanguageKey, int(lang));
        qCDebug(lcI18n) << "Persisted" << kLanguageKey << "=" << int(lang);
    }
    else
    {
        qCWarning(lcI18n) << "Preferences unavailable; language not persisted";
    }

    emit languageChanged();
    qCInfo(lcI18n) << "Language applied and live bindings retranslated:" << displayName(lang);
}

void I18n::applyMode(const Language lang)
{
    if (m_translator == nullptr)
    {
        qCWarning(lcI18n) << "applyMode called without a translator";
        return;
    }

    m_translator->setMode(toMode(lang));

    // Re-evaluate every live qsTr binding without an application restart.
    if (m_engine != nullptr)
        m_engine->retranslate();
    else
        qCWarning(lcI18n) << "No QQmlEngine stored; cannot retranslate live bindings";
}

QString I18n::t(const QString &english) const
{
    if (m_translator != nullptr)
    {
        const QString translated =
            m_translator->translate(nullptr, english.toUtf8().constData(), nullptr, -1);
        if (!translated.isEmpty())
            return translated;
    }
    return english;
}
