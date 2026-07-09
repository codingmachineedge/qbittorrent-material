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

#include <QAbstractListModel>
#include <QByteArray>
#include <QDateTime>
#include <QHash>
#include <QList>
#include <QNetworkCookie>
#include <QQmlEngine>
#include <QString>
#include <QVariant>

#include "base/logging.h"
#include "base/net/downloadmanager.h"

/**
 * @file cookiesmodel.h
 * @brief List model backing the Material @c CookiesDialog ("Manage Cookies").
 *
 * Wraps @c QList<QNetworkCookie> as a flat, one-role-per-column list model that
 * feeds @c DataTable (CONTRACTS §7.1). Every field is editable from QML through
 * @ref setCell(); rows are added / removed with @ref addCookie() /
 * @ref removeRows(). On construction it seeds itself from
 * @c Net::DownloadManager::allCookies(); @ref save() writes the edited list back
 * via @c setAllCookies().
 *
 * The @c expiration column is exposed as an ISO-8601 string so it round-trips
 * cleanly through an inline text editor.
 */
class CookiesModel final : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum Roles
    {
        DomainRole = Qt::UserRole + 1, ///< "domain"
        PathRole,                      ///< "path"
        NameRole,                      ///< "name"
        ValueRole,                     ///< "value"
        ExpirationRole                 ///< "expiration" (ISO-8601 string)
    };

    explicit CookiesModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        reload();
    }

    // ---- QAbstractListModel ------------------------------------------------

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_cookies.size());
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || (index.row() < 0) || (index.row() >= m_cookies.size()))
            return {};

        const QNetworkCookie &cookie = m_cookies.at(index.row());
        switch (role)
        {
        case DomainRole:
            return cookie.domain();
        case PathRole:
            return cookie.path();
        case NameRole:
            return QString::fromLatin1(cookie.name());
        case ValueRole:
            return QString::fromLatin1(cookie.value());
        case ExpirationRole:
            return cookie.expirationDate().toString(Qt::ISODate);
        default:
            return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {DomainRole, QByteArrayLiteral("domain")},
            {PathRole, QByteArrayLiteral("path")},
            {NameRole, QByteArrayLiteral("name")},
            {ValueRole, QByteArrayLiteral("value")},
            {ExpirationRole, QByteArrayLiteral("expiration")}
        };
    }

    // ---- QML actions -------------------------------------------------------

    /// Reload the whole cookie list from the network stack (resets the model).
    Q_INVOKABLE void reload()
    {
        beginResetModel();
        m_cookies = Net::DownloadManager::instance()->allCookies();
        endResetModel();
        qCDebug(lcModel) << "CookiesModel loaded" << m_cookies.size() << "cookie(s)";
        emit countChanged();
    }

    /// Commit the edited cookie list back to the network stack.
    Q_INVOKABLE void save()
    {
        Net::DownloadManager::instance()->setAllCookies(m_cookies);
        qCInfo(lcModel) << "CookiesModel saved" << m_cookies.size() << "cookie(s)";
    }

    /// Insert a fresh cookie after @p afterRow (default expiration now + 2 years);
    /// returns the row index of the new entry so QML can select it.
    Q_INVOKABLE int addCookie(int afterRow = -1)
    {
        const int row = ((afterRow >= 0) && (afterRow < m_cookies.size()))
                ? (afterRow + 1) : static_cast<int>(m_cookies.size());

        QNetworkCookie cookie;
        cookie.setExpirationDate(QDateTime::currentDateTime().addYears(2));

        beginInsertRows({}, row, row);
        m_cookies.insert(row, cookie);
        endInsertRows();

        qCDebug(lcModel) << "CookiesModel added cookie at row" << row;
        emit countChanged();
        return row;
    }

    /// Remove the given rows (any order); returns the number actually removed.
    Q_INVOKABLE int removeRows(const QList<int> &rows)
    {
        QList<int> sorted = rows;
        std::sort(sorted.begin(), sorted.end(), std::greater<int>());

        int removed = 0;
        for (const int row : std::as_const(sorted))
        {
            if ((row < 0) || (row >= m_cookies.size()))
                continue;
            beginRemoveRows({}, row, row);
            m_cookies.removeAt(row);
            endRemoveRows();
            ++removed;
        }

        if (removed > 0)
        {
            qCDebug(lcModel) << "CookiesModel removed" << removed << "cookie(s)";
            emit countChanged();
        }
        return removed;
    }

    /// Edit one field of one row (@p role is a role name from @ref roleNames()).
    Q_INVOKABLE bool setCell(int row, const QString &role, const QVariant &value)
    {
        if ((row < 0) || (row >= m_cookies.size()))
            return false;

        QNetworkCookie &cookie = m_cookies[row];
        int roleId = -1;
        if (role == QLatin1String("domain"))
        {
            cookie.setDomain(value.toString());
            roleId = DomainRole;
        }
        else if (role == QLatin1String("path"))
        {
            cookie.setPath(value.toString());
            roleId = PathRole;
        }
        else if (role == QLatin1String("name"))
        {
            cookie.setName(value.toString().toLatin1());
            roleId = NameRole;
        }
        else if (role == QLatin1String("value"))
        {
            cookie.setValue(value.toString().toLatin1());
            roleId = ValueRole;
        }
        else if (role == QLatin1String("expiration"))
        {
            cookie.setExpirationDate(QDateTime::fromString(value.toString(), Qt::ISODate));
            roleId = ExpirationRole;
        }
        else
        {
            qCWarning(lcModel) << "CookiesModel setCell: unknown role" << role;
            return false;
        }

        const QModelIndex idx = index(row);
        emit dataChanged(idx, idx, {roleId, Qt::DisplayRole, Qt::EditRole});
        qCDebug(lcModel) << "CookiesModel edited row" << row << "role" << role;
        return true;
    }

    /// The current in-memory cookie list (for callers that need it directly).
    [[nodiscard]] QList<QNetworkCookie> cookies() const { return m_cookies; }

signals:
    /// Emitted whenever a row is added / removed / the whole list is reloaded.
    void countChanged();

private:
    QList<QNetworkCookie> m_cookies;
};
