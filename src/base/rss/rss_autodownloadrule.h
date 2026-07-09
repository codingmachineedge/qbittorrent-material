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

#include <optional>

#include <QSharedDataPointer>
#include <QVariant>

#include "base/bittorrent/addtorrentparams.h"
#include "base/global.h"
#include "base/pathfwd.h"

class QDateTime;
class QJsonObject;
class QRegularExpression;

namespace RSS
{
    struct AutoDownloadRuleData;

    /// Value type describing one RSS auto-download rule: match conditions
    /// (must/must-not contain, wildcard or regex, smart episode filter, ignore-days),
    /// the feeds it applies to, and the `AddTorrentParams` applied to accepted
    /// articles. Copy-on-write (`QSharedDataPointer`). Used live for the rule-editor
    /// matching preview via `matches()`.
    class AutoDownloadRule
    {
    public:
        explicit AutoDownloadRule(const QString &name = {});
        AutoDownloadRule(const AutoDownloadRule &other);
        ~AutoDownloadRule();

        AutoDownloadRule &operator=(const AutoDownloadRule &other);

        QString name() const;
        void setName(const QString &name);

        bool isEnabled() const;
        void setEnabled(bool enable);

        int priority() const;
        void setPriority(int value);

        QString mustContain() const;
        void setMustContain(const QString &tokens);
        QString mustNotContain() const;
        void setMustNotContain(const QString &tokens);
        QStringList feedURLs() const;
        void setFeedURLs(const QStringList &urls);
        int ignoreDays() const;
        void setIgnoreDays(int d);
        QDateTime lastMatch() const;
        void setLastMatch(const QDateTime &lastMatch);
        bool useRegex() const;
        void setUseRegex(bool enabled);
        bool useSmartFilter() const;
        void setUseSmartFilter(bool enabled);
        QString episodeFilter() const;
        void setEpisodeFilter(const QString &e);

        QStringList previouslyMatchedEpisodes() const;
        void setPreviouslyMatchedEpisodes(const QStringList &previouslyMatchedEpisodes);

        BitTorrent::AddTorrentParams addTorrentParams() const;
        void setAddTorrentParams(BitTorrent::AddTorrentParams addTorrentParams);

        /// Pure predicate: does `articleData` satisfy this rule's conditions?
        /// Side-effect-free, used for the live editor preview.
        bool matches(const QVariantHash &articleData) const;
        /// Like `matches()` but also records the episode as previously matched and
        /// updates `lastMatch` (mutating), used by the auto-downloader.
        bool accepts(const QVariantHash &articleData);

        friend bool operator==(const AutoDownloadRule &left, const AutoDownloadRule &right);

        QJsonObject toJsonObject() const;
        static AutoDownloadRule fromJsonObject(const QJsonObject &jsonObj, const QString &name = {});

        QVariantHash toLegacyDict() const;
        static AutoDownloadRule fromLegacyDict(const QVariantHash &dict);

    private:
        bool matchesMustContainExpression(const QString &articleTitle) const;
        bool matchesMustNotContainExpression(const QString &articleTitle) const;
        bool matchesEpisodeFilterExpression(const QString &articleTitle) const;
        bool matchesSmartEpisodeFilter(const QString &articleTitle) const;
        bool matchesExpression(const QString &articleTitle, const QString &expression) const;
        QRegularExpression cachedRegex(const QString &expression, bool isRegex = true) const;

        QSharedDataPointer<AutoDownloadRuleData> m_dataPtr;
    };
}
