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
#include <QQmlEngine>
#include <QString>

QT_BEGIN_NAMESPACE
class QJSEngine;
QT_END_NAMESPACE

namespace Utils::I18n
{
    class FunnyTranslator;
}

/**
 * @file i18ncontroller.h
 * @brief The @c I18n QML singleton — runtime language state + live retranslate.
 *
 * QML reads/writes @c I18n.language; changing it swaps the FunnyTranslator mode,
 * calls @c QQmlEngine::retranslate() (so every live @c qsTr binding updates with
 * no restart), and persists the choice under @c Appearance/Language.
 *
 * @code
 *   ComboBox {
 *       model: [ qsTr("English"), qsTr("Cantonese"), qsTr("Bilingual") ]
 *       currentIndex: I18n.language
 *       onActivated: (i) => I18n.setLanguage(i)
 *   }
 * @endcode
 */
class I18n final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_PROPERTY(Language language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString languageName READ languageName NOTIFY languageChanged)

public:
    /// Enum values are stable and mirror FunnyTranslator::Mode / the persisted
    /// @c Appearance/Language integer.
    enum Language
    {
        English = 0,
        Cantonese = 1,
        Bilingual = 2
    };
    Q_ENUM(Language)

    /// QML singleton factory. Constructs and installs the FunnyTranslator on
    /// @c qApp, restores the persisted language, and remembers @p engine so
    /// setLanguage() can retranslate live bindings.
    static I18n *create(QQmlEngine *engine, QJSEngine *scriptEngine);

    explicit I18n(QQmlEngine *engine = nullptr, QObject *parent = nullptr);
    ~I18n() override;

    Language language() const { return m_language; }
    /// Localized (endonym) display name of the *current* language.
    QString languageName() const;

    /// Switch language: log, flip translator mode, retranslate, persist, notify.
    Q_INVOKABLE void setLanguage(Language lang);

    /// Endonym display name for an arbitrary language (used by the selector).
    Q_INVOKABLE QString displayName(Language lang) const;

    /// Programmatic translate — same result as @c qsTr for the given English.
    Q_INVOKABLE QString t(const QString &english) const;

signals:
    void languageChanged();

private:
    void applyMode(Language lang);

    QQmlEngine *m_engine = nullptr;
    Utils::I18n::FunnyTranslator *m_translator = nullptr; ///< installed on qApp
    Language m_language = English;

    static I18n *s_instance;
};
