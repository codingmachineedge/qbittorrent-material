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

#include <QHash>
#include <QString>
#include <QTranslator>

namespace Utils::I18n
{
    /**
     * @brief A non-.ts custom translator implementing the three funny modes.
     *
     * Translation is driven by a flat English->Cantonese JSON catalog
     * (@c :/i18n/cantonese.json) instead of Qt @c .qm files. The English
     * literal passed to @c qsTr()/tr() *is* the lookup key.
     *
     * Modes (see translate()):
     *  - English   : returns {} so Qt falls back to the original English literal.
     *  - Cantonese : returns the colloquial 港式口語 entry (or {} -> English).
     *  - Bilingual : composes @c "English · Cantonese" at runtime when an entry
     *                exists (no third catalog); otherwise {} -> English.
     */
    class FunnyTranslator final : public QTranslator
    {
        Q_OBJECT

    public:
        enum class Mode
        {
            English = 0,
            Cantonese = 1,
            Bilingual = 2
        };
        Q_ENUM(Mode)

        explicit FunnyTranslator(QObject *parent = nullptr);

        /// Switch active mode. The caller is responsible for triggering a
        /// QQmlEngine::retranslate() so live bindings re-evaluate.
        void setMode(Mode mode);
        Mode mode() const { return m_mode; }

        /// (Re)load the English->Cantonese catalog from a JSON file/resource.
        /// Returns true on success. Multiple candidate paths are tried by the
        /// constructor; call this only to point at a custom catalog.
        bool loadCatalog(const QString &jsonPath);

        /// Number of loaded catalog entries (diagnostics/logging).
        int entryCount() const { return m_enToYue.size(); }

        /// MUST be false so Qt actually calls translate() for every string.
        bool isEmpty() const override { return false; }

        /// Qt calls this for every qsTr()/tr(). @p sourceText is the English key.
        QString translate(const char *context, const char *sourceText,
                          const char *disambiguation = nullptr, int n = -1) const override;

    private:
        QHash<QString, QString> m_enToYue; ///< English literal -> Cantonese
        Mode m_mode = Mode::English;
    };
}
