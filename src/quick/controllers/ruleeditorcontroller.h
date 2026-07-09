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

#include <algorithm>

#include <QDateTime>
#include <QFile>
#include <QIODevice>
#include <QList>
#include <QQmlEngine>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariantList>
#include <QVariantMap>

#include "base/bittorrent/addtorrentparams.h"
#include "base/bittorrent/sharelimits.h"
#include "base/bittorrent/torrent.h"
#include "base/bittorrent/torrentcontentlayout.h"
#include "base/exceptions.h"
#include "base/logging.h"
#include "base/path.h"
#include "base/rss/rss_article.h"
#include "base/rss/rss_autodownloader.h"
#include "base/rss/rss_autodownloadrule.h"
#include "base/rss/rss_feed.h"
#include "base/rss/rss_session.h"
#include "base/tag.h"
#include "base/tagset.h"

/**
 * @file ruleeditorcontroller.h
 * @brief The imperative + observable surface behind the Automated Download Rules
 *        dialog's rule-definition panel and the live "Matching RSS Articles"
 *        preview.
 *
 * A view creates one instance (it is a plain @c QML_ELEMENT, not a singleton).
 * @c selectRules() loads the selected rule(s) from @c RSS::AutoDownloader; the
 * per-field properties (must/mustNot/regex/episode/smart/ignore-days/priority
 * plus the embedded add-torrent parameters) are two-way bound in QML and every
 * edit commits the rule to the engine and recomputes @c matchingArticles using
 * @c AutoDownloadRule::matches() — the rule's live self-test. Rule CRUD, feed
 * selection, downloaded-episode clearing and JSON/legacy import-export are all
 * exposed as @c Q_INVOKABLE verbs.
 *
 * Header-only so it registers as a @c QML_ELEMENT with no separate TU.
 */
class RuleEditorController final : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool ruleSelected READ ruleSelected NOTIFY selectionChanged)
    Q_PROPERTY(bool multiSelected READ multiSelected NOTIFY selectionChanged)
    Q_PROPERTY(QString currentRuleName READ currentRuleName NOTIFY ruleLoaded)

    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY ruleLoaded)
    Q_PROPERTY(int priority READ priority WRITE setPriority NOTIFY ruleLoaded)
    Q_PROPERTY(bool useRegex READ useRegex WRITE setUseRegex NOTIFY ruleLoaded)
    Q_PROPERTY(bool useSmartFilter READ useSmartFilter WRITE setUseSmartFilter NOTIFY ruleLoaded)
    Q_PROPERTY(QString mustContain READ mustContain WRITE setMustContain NOTIFY ruleLoaded)
    Q_PROPERTY(QString mustNotContain READ mustNotContain WRITE setMustNotContain NOTIFY ruleLoaded)
    Q_PROPERTY(QString episodeFilter READ episodeFilter WRITE setEpisodeFilter NOTIFY ruleLoaded)
    Q_PROPERTY(int ignoreDays READ ignoreDays WRITE setIgnoreDays NOTIFY ruleLoaded)
    Q_PROPERTY(QString lastMatchText READ lastMatchText NOTIFY ruleLoaded)
    Q_PROPERTY(QVariantMap addTorrentParams READ addTorrentParams WRITE setAddTorrentParams NOTIFY ruleLoaded)

    Q_PROPERTY(QVariantList feeds READ feeds NOTIFY feedsChanged)
    Q_PROPERTY(QVariantList matchingArticles READ matchingArticles NOTIFY matchingArticlesChanged)

    Q_PROPERTY(bool autoDownloadEnabled READ autoDownloadEnabled NOTIFY autoDownloadEnabledChanged)

public:
    explicit RuleEditorController(QObject *parent = nullptr)
        : QObject(parent)
    {
        if (auto *autoDl = RSS::AutoDownloader::instance())
        {
            connect(autoDl, &RSS::AutoDownloader::processingStateChanged, this,
                    [this] { emit autoDownloadEnabledChanged(); });
        }
        qCDebug(lcUi) << "RuleEditorController constructed";
    }

    ~RuleEditorController() override
    {
        qCDebug(lcUi) << "RuleEditorController destroyed";
    }

    // ---- Selection -----------------------------------------------------------

    bool ruleSelected() const { return m_selected.size() == 1; }
    bool multiSelected() const { return m_selected.size() > 1; }
    QString currentRuleName() const { return ruleSelected() ? m_selected.first() : QString(); }

    /// Load a single rule for editing.
    Q_INVOKABLE void selectRule(const QString &name)
    {
        selectRules(name.isEmpty() ? QStringList() : QStringList {name});
    }

    /// Load a set of selected rules (only 1 is editable in the definition panel).
    Q_INVOKABLE void selectRules(const QStringList &names)
    {
        qCDebug(lcUi) << "RuleEditor: selecting rules" << names;
        m_selected = names;

        auto *autoDl = RSS::AutoDownloader::instance();
        if (autoDl && (names.size() == 1) && autoDl->hasRule(names.first()))
            m_currentRule = autoDl->ruleByName(names.first());
        else
            m_currentRule = RSS::AutoDownloadRule();

        emit selectionChanged();
        emit ruleLoaded();
        emit feedsChanged();
        emit matchingArticlesChanged();
    }

    Q_INVOKABLE void clearSelection() { selectRules({}); }

    // ---- Field accessors (edit the loaded rule) ------------------------------

    bool enabled() const { return m_currentRule.isEnabled(); }
    void setEnabled(const bool v) { if (v != enabled()) { m_currentRule.setEnabled(v); commit(); } }

    int priority() const { return m_currentRule.priority(); }
    void setPriority(const int v) { if (v != priority()) { m_currentRule.setPriority(v); commit(); } }

    bool useRegex() const { return m_currentRule.useRegex(); }
    void setUseRegex(const bool v) { if (v != useRegex()) { m_currentRule.setUseRegex(v); commit(); } }

    bool useSmartFilter() const { return m_currentRule.useSmartFilter(); }
    void setUseSmartFilter(const bool v) { if (v != useSmartFilter()) { m_currentRule.setUseSmartFilter(v); commit(); } }

    QString mustContain() const { return m_currentRule.mustContain(); }
    void setMustContain(const QString &v) { if (v != mustContain()) { m_currentRule.setMustContain(v); commit(); } }

    QString mustNotContain() const { return m_currentRule.mustNotContain(); }
    void setMustNotContain(const QString &v) { if (v != mustNotContain()) { m_currentRule.setMustNotContain(v); commit(); } }

    QString episodeFilter() const { return m_currentRule.episodeFilter(); }
    void setEpisodeFilter(const QString &v) { if (v != episodeFilter()) { m_currentRule.setEpisodeFilter(v); commit(); } }

    int ignoreDays() const { return m_currentRule.ignoreDays(); }
    void setIgnoreDays(const int v) { if (v != ignoreDays()) { m_currentRule.setIgnoreDays(v); commit(); } }

    QString lastMatchText() const
    {
        const QDateTime lm = m_currentRule.lastMatch();
        if (!lm.isValid())
            return tr("Last Match: Unknown");
        return tr("Last Match: %1 days ago").arg(lm.daysTo(QDateTime::currentDateTime()));
    }

    QVariantMap addTorrentParams() const { return paramsToMap(m_currentRule.addTorrentParams()); }
    void setAddTorrentParams(const QVariantMap &m)
    {
        m_currentRule.setAddTorrentParams(mapToParams(m));
        commit();
    }

    // ---- Apply-rule-to-feeds -------------------------------------------------

    QVariantList feeds() const
    {
        QVariantList out;
        QList<RSS::Feed *> feeds = RSS::Session::instance()->feeds();
        std::sort(feeds.begin(), feeds.end(), [](RSS::Feed *a, RSS::Feed *b) {
            return a->name().compare(b->name(), Qt::CaseInsensitive) < 0;
        });

        auto *autoDl = RSS::AutoDownloader::instance();
        for (RSS::Feed *feed : feeds)
        {
            int checked = 0;
            for (const QString &ruleName : m_selected)
            {
                const RSS::AutoDownloadRule rule = (autoDl && autoDl->hasRule(ruleName))
                    ? autoDl->ruleByName(ruleName) : RSS::AutoDownloadRule();
                if (rule.feedURLs().contains(feed->url()))
                    ++checked;
            }
            int state = 0; // Qt.Unchecked
            if (!m_selected.isEmpty())
                state = (checked == m_selected.size()) ? 2 : (checked > 0 ? 1 : 0);

            QVariantMap fm;
            fm.insert(QStringLiteral("name"), feed->name());
            fm.insert(QStringLiteral("url"), feed->url());
            fm.insert(QStringLiteral("checkState"), state);
            out.append(fm);
        }
        return out;
    }

    /// Add/remove @p url from every selected rule's affected-feed list.
    Q_INVOKABLE void toggleFeed(const QString &url, const bool checked)
    {
        auto *autoDl = RSS::AutoDownloader::instance();
        if (!autoDl)
            return;
        qCDebug(lcUi) << "RuleEditor: toggle feed" << url << "->" << checked
                      << "for" << m_selected.size() << "rule(s)";

        for (const QString &ruleName : m_selected)
        {
            RSS::AutoDownloadRule rule = (ruleName == currentRuleName())
                ? m_currentRule
                : (autoDl->hasRule(ruleName) ? autoDl->ruleByName(ruleName) : RSS::AutoDownloadRule(ruleName));
            QStringList urls = rule.feedURLs();
            const bool has = urls.contains(url);
            if (checked && !has)
                urls.append(url);
            else if (!checked && has)
                urls.removeAll(url);
            rule.setFeedURLs(urls);

            if (ruleName == currentRuleName())
                m_currentRule = rule;
            autoDl->setRule(rule);
        }
        emit feedsChanged();
        emit matchingArticlesChanged();
    }

    // ---- Live matching preview ----------------------------------------------

    QVariantList matchingArticles() const
    {
        QVariantList groups;
        auto *session = RSS::Session::instance();
        auto *autoDl = RSS::AutoDownloader::instance();
        if (!autoDl)
            return groups;

        for (const QString &ruleName : m_selected)
        {
            const RSS::AutoDownloadRule rule = (ruleName == currentRuleName())
                ? m_currentRule
                : (autoDl->hasRule(ruleName) ? autoDl->ruleByName(ruleName) : RSS::AutoDownloadRule());

            for (const QString &url : rule.feedURLs())
            {
                RSS::Feed *feed = session->feedByURL(url);
                if (!feed)
                    continue;

                QStringList titles;
                for (RSS::Article *article : feed->articles())
                {
                    if (rule.matches(article->data()))
                    {
                        const QString title = article->title();
                        if (!titles.contains(title))
                            titles.append(title);
                    }
                }
                if (titles.isEmpty())
                    continue;

                titles.sort(Qt::CaseInsensitive);
                QVariantMap group;
                group.insert(QStringLiteral("feedName"), feed->name());
                group.insert(QStringLiteral("feedUrl"), url);
                group.insert(QStringLiteral("titles"), QVariant(titles));
                groups.append(group);
            }
        }
        return groups;
    }

    // ---- Validation (field status icons / tooltips) --------------------------

    /// Validate a must/mustNot expression; returns "" when valid, else a message.
    Q_INVOKABLE QString validateExpression(const QString &text, const bool regex) const
    {
        if (text.isEmpty())
            return {};
        const QStringList tokens = regex ? QStringList {text} : text.split(u'|');
        for (const QString &token : tokens)
        {
            const QString pattern = regex
                ? token
                : QRegularExpression::wildcardToRegularExpression(token.trimmed());
            const QRegularExpression re(pattern, QRegularExpression::CaseInsensitiveOption);
            if (!re.isValid())
                return tr("Position %1: %2").arg(re.patternErrorOffset()).arg(re.errorString());
        }
        return {};
    }

    /// Validate the episode filter grammar; returns "" when valid.
    Q_INVOKABLE QString validateEpisodeFilter(const QString &text) const
    {
        if (text.isEmpty())
            return {};
        static const QRegularExpression epRe(
            QStringLiteral("^(^\\d{1,4}x(\\d{1,4}(-(\\d{1,4})?)?;){1,}){1,1}"),
            QRegularExpression::CaseInsensitiveOption);
        return epRe.match(text).hasMatch() ? QString() : tr("Invalid episode filter.");
    }

    // ---- Rule CRUD (return "" on success, else an error message) -------------

    Q_INVOKABLE bool hasRule(const QString &name) const
    {
        auto *autoDl = RSS::AutoDownloader::instance();
        return autoDl && autoDl->hasRule(name);
    }

    Q_INVOKABLE QString addRule(const QString &name)
    {
        const QString trimmed = name.trimmed();
        if (trimmed.isEmpty())
            return tr("Please type the name of the new download rule.");
        if (hasRule(trimmed))
            return tr("A rule with this name already exists, please choose another name.");
        qCInfo(lcUi) << "RuleEditor: adding rule" << trimmed;
        RSS::AutoDownloader::instance()->setRule(RSS::AutoDownloadRule(trimmed));
        return {};
    }

    Q_INVOKABLE void removeRules(const QStringList &names)
    {
        auto *autoDl = RSS::AutoDownloader::instance();
        if (!autoDl)
            return;
        qCInfo(lcUi) << "RuleEditor: removing rules" << names;
        for (const QString &name : names)
            autoDl->removeRule(name);
    }

    Q_INVOKABLE QString renameRule(const QString &oldName, const QString &newName)
    {
        const QString trimmed = newName.trimmed();
        if (trimmed.isEmpty())
            return tr("Please type the new rule name");
        if (hasRule(trimmed))
            return tr("A rule with this name already exists, please choose another name.");
        qCInfo(lcUi) << "RuleEditor: renaming rule" << oldName << "->" << trimmed;
        if (!RSS::AutoDownloader::instance()->renameRule(oldName, trimmed))
            return tr("Rename failed");
        return {};
    }

    Q_INVOKABLE QString cloneRule(const QString &srcName, const QString &cloneName)
    {
        const QString trimmed = cloneName.trimmed();
        if (trimmed.isEmpty())
            return tr("Please type the name for the clone of the download rule.");
        if (hasRule(trimmed))
            return tr("A rule with this name already exists, please choose another name.");
        qCInfo(lcUi) << "RuleEditor: cloning rule" << srcName << "->" << trimmed;
        if (!RSS::AutoDownloader::instance()->cloneRule(srcName, trimmed))
            return tr("Clone failed");
        return {};
    }

    Q_INVOKABLE void setRuleEnabled(const QString &name, const bool value)
    {
        auto *autoDl = RSS::AutoDownloader::instance();
        if (!autoDl || !autoDl->hasRule(name))
            return;
        qCInfo(lcUi) << "RuleEditor: set rule" << name << "enabled ->" << value;
        RSS::AutoDownloadRule rule = autoDl->ruleByName(name);
        rule.setEnabled(value);
        autoDl->setRule(rule);
        if (name == currentRuleName())
        {
            m_currentRule = rule;
            emit ruleLoaded();
        }
    }

    Q_INVOKABLE void clearDownloadedEpisodes(const QStringList &names)
    {
        auto *autoDl = RSS::AutoDownloader::instance();
        if (!autoDl)
            return;
        qCInfo(lcUi) << "RuleEditor: clearing downloaded episodes for" << names;
        for (const QString &name : names)
        {
            if (!autoDl->hasRule(name))
                continue;
            RSS::AutoDownloadRule rule = autoDl->ruleByName(name);
            rule.setPreviouslyMatchedEpisodes({});
            if (name == currentRuleName())
                m_currentRule = rule;
            autoDl->setRule(rule);
        }
        emit matchingArticlesChanged();
    }

    // ---- Import / export -----------------------------------------------------

    Q_INVOKABLE QString exportRules(const QString &filePath, const bool legacy)
    {
        auto *autoDl = RSS::AutoDownloader::instance();
        if (!autoDl)
            return tr("Auto-downloader unavailable.");
        if (autoDl->rules().isEmpty())
            return tr("The list is empty, there is nothing to export.");

        const auto fmt = legacy
            ? RSS::AutoDownloader::RulesFileFormat::Legacy
            : RSS::AutoDownloader::RulesFileFormat::JSON;
        const QByteArray data = autoDl->exportRules(fmt);

        QFile file(localPath(filePath));
        if (!file.open(QIODevice::WriteOnly))
            return tr("Failed to create the destination file. Reason: %1").arg(file.errorString());
        file.write(data);
        file.close();
        qCInfo(lcUi) << "RuleEditor: exported rules to" << file.fileName();
        return {};
    }

    Q_INVOKABLE QString importRules(const QString &filePath)
    {
        auto *autoDl = RSS::AutoDownloader::instance();
        if (!autoDl)
            return tr("Auto-downloader unavailable.");

        const QString local = localPath(filePath);
        QFile file(local);
        if (!file.open(QIODevice::ReadOnly))
            return tr("Failed to read the file. %1").arg(file.errorString());
        if (file.size() > (10 * 1024 * 1024))
        {
            file.close();
            return tr("Failed to read the file. %1").arg(tr("The file is too large."));
        }
        const QByteArray data = file.readAll();
        file.close();

        const auto fmt = local.endsWith(QStringLiteral(".rssrules"))
            ? RSS::AutoDownloader::RulesFileFormat::Legacy
            : RSS::AutoDownloader::RulesFileFormat::JSON;
        try
        {
            autoDl->importRules(data, fmt);
        }
        catch (const RSS::ParsingError &error)
        {
            return tr("Failed to import the selected rules file. Reason: %1").arg(error.message());
        }
        qCInfo(lcUi) << "RuleEditor: imported rules from" << local;
        return {};
    }

    // ---- Auto-download processing toggle (banner) ----------------------------

    bool autoDownloadEnabled() const
    {
        auto *autoDl = RSS::AutoDownloader::instance();
        return autoDl && autoDl->isProcessingEnabled();
    }

signals:
    void selectionChanged();
    void ruleLoaded();
    void feedsChanged();
    void matchingArticlesChanged();
    void autoDownloadEnabledChanged();

private:
    /// Commit the in-memory rule to the engine and refresh the live preview.
    void commit()
    {
        if (!ruleSelected())
            return;
        RSS::AutoDownloader::instance()->setRule(m_currentRule);
        emit matchingArticlesChanged();
    }

    static QString localPath(const QString &pathOrUrl)
    {
        const QUrl url(pathOrUrl);
        return url.isLocalFile() ? url.toLocalFile() : pathOrUrl;
    }

    // ---- AddTorrentParams <-> QVariantMap (shape expected by the QML form) ---

    static QVariantMap paramsToMap(const BitTorrent::AddTorrentParams &p)
    {
        const auto optBool = [](const std::optional<bool> &o) -> QVariant {
            return o.has_value() ? QVariant(*o) : QVariant();
        };

        QVariantMap m;
        m.insert(QStringLiteral("useAutoTMM"), optBool(p.useAutoTMM));
        m.insert(QStringLiteral("savePath"), p.savePath.toString());
        m.insert(QStringLiteral("useDownloadPath"), optBool(p.useDownloadPath));
        m.insert(QStringLiteral("downloadPath"), p.downloadPath.toString());
        m.insert(QStringLiteral("category"), p.category);

        QVariantList tags;
        for (const Tag &tag : p.tags)
            tags.append(tag.toString());
        m.insert(QStringLiteral("tags"), tags);

        m.insert(QStringLiteral("contentLayout"),
                 p.contentLayout.has_value() ? QVariant(int(*p.contentLayout)) : QVariant());
        m.insert(QStringLiteral("skipChecking"), p.skipChecking);
        m.insert(QStringLiteral("addStopped"), optBool(p.addStopped));
        m.insert(QStringLiteral("stopCondition"),
                 p.stopCondition.has_value() ? QVariant(int(*p.stopCondition)) : QVariant());
        m.insert(QStringLiteral("addToQueueTop"), optBool(p.addToQueueTop));

        QVariantMap sl;
        sl.insert(QStringLiteral("ratioLimit"), p.shareLimits.ratioLimit);
        sl.insert(QStringLiteral("seedingTimeLimit"), p.shareLimits.seedingTimeLimit);
        sl.insert(QStringLiteral("inactiveSeedingTimeLimit"), p.shareLimits.inactiveSeedingTimeLimit);
        sl.insert(QStringLiteral("mode"), int(p.shareLimits.mode));
        sl.insert(QStringLiteral("action"), int(p.shareLimits.action));
        m.insert(QStringLiteral("shareLimits"), sl);

        return m;
    }

    BitTorrent::AddTorrentParams mapToParams(const QVariantMap &m) const
    {
        const auto toOpt = [](const QVariant &v) -> std::optional<bool> {
            if (!v.isValid() || v.isNull())
                return std::nullopt;
            return v.toBool();
        };

        // Preserve any params the form does not surface.
        BitTorrent::AddTorrentParams p = m_currentRule.addTorrentParams();

        p.useAutoTMM = toOpt(m.value(QStringLiteral("useAutoTMM")));
        p.savePath = Path(m.value(QStringLiteral("savePath")).toString());
        p.useDownloadPath = toOpt(m.value(QStringLiteral("useDownloadPath")));
        p.downloadPath = Path(m.value(QStringLiteral("downloadPath")).toString());
        p.category = m.value(QStringLiteral("category")).toString();

        TagSet tags;
        const QVariantList tagList = m.value(QStringLiteral("tags")).toList();
        for (const QVariant &tv : tagList)
        {
            const QString s = tv.toString().trimmed();
            if (!s.isEmpty())
                tags.insert(Tag(s));
        }
        p.tags = tags;

        const QVariant cl = m.value(QStringLiteral("contentLayout"));
        p.contentLayout = (cl.isValid() && !cl.isNull())
            ? std::optional<BitTorrent::TorrentContentLayout>(BitTorrent::TorrentContentLayout(cl.toInt()))
            : std::nullopt;
        p.skipChecking = m.value(QStringLiteral("skipChecking")).toBool();
        p.addStopped = toOpt(m.value(QStringLiteral("addStopped")));

        const QVariant sc = m.value(QStringLiteral("stopCondition"));
        p.stopCondition = (sc.isValid() && !sc.isNull())
            ? std::optional<BitTorrent::Torrent::StopCondition>(BitTorrent::Torrent::StopCondition(sc.toInt()))
            : std::nullopt;
        p.addToQueueTop = toOpt(m.value(QStringLiteral("addToQueueTop")));

        const QVariantMap sl = m.value(QStringLiteral("shareLimits")).toMap();
        p.shareLimits.ratioLimit = sl.value(QStringLiteral("ratioLimit"), -2).toReal();
        p.shareLimits.seedingTimeLimit = sl.value(QStringLiteral("seedingTimeLimit"), -2).toInt();
        p.shareLimits.inactiveSeedingTimeLimit = sl.value(QStringLiteral("inactiveSeedingTimeLimit"), -2).toInt();
        p.shareLimits.mode = BitTorrent::ShareLimitsMode(sl.value(QStringLiteral("mode"), -1).toInt());
        p.shareLimits.action = BitTorrent::ShareLimitAction(sl.value(QStringLiteral("action"), -1).toInt());

        return p;
    }

    QStringList m_selected;
    RSS::AutoDownloadRule m_currentRule;
};
