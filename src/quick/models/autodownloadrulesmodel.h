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

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QQmlEngine>
#include <QString>

#include "base/logging.h"
#include "base/rss/rss_autodownloader.h"
#include "base/rss/rss_autodownloadrule.h"

/**
 * @file autodownloadrulesmodel.h
 * @brief List model of the RSS auto-download rules, sorted by name.
 *
 * Backs the "Download Rules" list in the Automated Download Rules dialog. Each
 * row is a rule with a @c name and an @c enabled flag (rendered as a checkbox in
 * QML). The model mirrors @c RSS::AutoDownloader and stays in sync via its
 * @c ruleAdded / @c ruleChanged / @c ruleRenamed / @c ruleAboutToBeRemoved
 * signals. Mutations are performed through @c RuleEditorController; this model is
 * purely a projection.
 *
 * Header-only so it registers as a @c QML_ELEMENT with no separate TU.
 */
class AutoDownloadRulesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles
    {
        NameRole = Qt::UserRole + 1, ///< "name"
        EnabledRole                  ///< "enabled"
    };

    explicit AutoDownloadRulesModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        reload();

        if (auto *autoDl = RSS::AutoDownloader::instance())
        {
            connect(autoDl, &RSS::AutoDownloader::ruleAdded, this, [this](const QString &) { reload(); });
            connect(autoDl, &RSS::AutoDownloader::ruleAboutToBeRemoved, this, [this](const QString &name) {
                Q_UNUSED(name)
                QMetaObject::invokeMethod(this, [this] { reload(); }, Qt::QueuedConnection);
            });
            connect(autoDl, &RSS::AutoDownloader::ruleRenamed, this, [this](const QString &, const QString &) { reload(); });
            connect(autoDl, &RSS::AutoDownloader::ruleChanged, this, [this](const QString &name) { touch(name); });
        }
        qCDebug(lcModel) << "AutoDownloadRulesModel constructed:" << m_names.size() << "rules";
    }

    ~AutoDownloadRulesModel() override
    {
        qCDebug(lcModel) << "AutoDownloadRulesModel destroyed";
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {{NameRole, "name"}, {EnabledRole, "enabled"}};
    }

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : int(m_names.size());
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!index.isValid() || (index.row() < 0) || (index.row() >= m_names.size()))
            return {};
        const QString name = m_names.at(index.row());
        switch (role)
        {
        case NameRole:
        case Qt::DisplayRole:
            return name;
        case EnabledRole:
        {
            auto *autoDl = RSS::AutoDownloader::instance();
            return autoDl ? autoDl->ruleByName(name).isEnabled() : false;
        }
        default:
            return {};
        }
    }

    /// Rule name at @p row (empty when out of range).
    Q_INVOKABLE QString ruleName(const int row) const
    {
        return ((row >= 0) && (row < m_names.size())) ? m_names.at(row) : QString();
    }

    /// Row index of the rule named @p name (-1 when absent).
    Q_INVOKABLE int indexOfRule(const QString &name) const
    {
        return int(m_names.indexOf(name));
    }

signals:
    void countChanged();

private:
    void touch(const QString &name)
    {
        const int row = int(m_names.indexOf(name));
        if (row < 0)
            return;
        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx, {NameRole, EnabledRole});
    }

    void reload()
    {
        beginResetModel();
        m_names.clear();
        if (auto *autoDl = RSS::AutoDownloader::instance())
        {
            const QList<RSS::AutoDownloadRule> rules = autoDl->rules();
            for (const RSS::AutoDownloadRule &rule : rules)
                m_names.append(rule.name());
        }
        m_names.sort(Qt::CaseInsensitive);
        endResetModel();
        emit countChanged();
        qCDebug(lcModel) << "AutoDownloadRulesModel reloaded:" << m_names.size() << "rules";
    }

    QStringList m_names;
};
