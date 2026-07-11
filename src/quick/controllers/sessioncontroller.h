/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026  qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantMap>

#include <qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

/**
 * Read/write QML facade over the app-owned BitTorrent::Session singleton.
 * The engine itself deliberately remains a C++ service; this bridge exposes
 * only the live shell metrics and category/tag verbs consumed by QML.
 */
class SessionController final : public QObject
{
    Q_OBJECT
    QML_NAMED_ELEMENT(Session)
    QML_SINGLETON
    Q_DISABLE_COPY_MOVE(SessionController)

    Q_PROPERTY(bool paused READ paused NOTIFY pausedChanged)
    Q_PROPERTY(bool queueingEnabled READ queueingEnabled NOTIFY statsUpdated)
    Q_PROPERTY(int torrentCount READ torrentCount NOTIFY torrentCountChanged)
    Q_PROPERTY(qint64 downloadRate READ downloadRate NOTIFY statsUpdated)
    Q_PROPERTY(qint64 uploadRate READ uploadRate NOTIFY statsUpdated)
    Q_PROPERTY(bool dhtEnabled READ dhtEnabled NOTIFY statsUpdated)
    Q_PROPERTY(qint64 dhtNodes READ dhtNodes NOTIFY statsUpdated)
    Q_PROPERTY(bool listening READ listening NOTIFY statsUpdated)
    Q_PROPERTY(bool hasIncomingConnections READ hasIncomingConnections NOTIFY statsUpdated)
    Q_PROPERTY(QString externalIPv4 READ externalIPv4 NOTIFY statsUpdated)
    Q_PROPERTY(QString externalIPv6 READ externalIPv6 NOTIFY statsUpdated)
    Q_PROPERTY(qint64 freeDiskSpace READ freeDiskSpace NOTIFY statsUpdated)

public:
    static SessionController *create(QQmlEngine *, QJSEngine *);

    [[nodiscard]] bool paused() const;
    [[nodiscard]] bool queueingEnabled() const;
    [[nodiscard]] int torrentCount() const;
    [[nodiscard]] qint64 downloadRate() const;
    [[nodiscard]] qint64 uploadRate() const;
    [[nodiscard]] bool dhtEnabled() const;
    [[nodiscard]] qint64 dhtNodes() const;
    [[nodiscard]] bool listening() const;
    [[nodiscard]] bool hasIncomingConnections() const;
    [[nodiscard]] QString externalIPv4() const;
    [[nodiscard]] QString externalIPv6() const;
    [[nodiscard]] qint64 freeDiskSpace() const;

    Q_INVOKABLE QStringList categories() const;
    Q_INVOKABLE QStringList tags() const;
    Q_INVOKABLE QString categorySavePath(const QString &category) const;
    Q_INVOKABLE QString categoryDownloadPath(const QString &category) const;
    Q_INVOKABLE bool addCategory(const QString &category);
    Q_INVOKABLE bool removeCategory(const QString &category);
    Q_INVOKABLE bool setCategoryOptions(const QString &category, const QVariantMap &values);
    Q_INVOKABLE bool addTag(const QString &tag);
    Q_INVOKABLE bool removeTag(const QString &tag);
    Q_INVOKABLE int removeUnusedCategories();
    Q_INVOKABLE int removeUnusedTags();

private:
    explicit SessionController(QObject *parent = nullptr);

signals:
    void pausedChanged();
    void torrentCountChanged();
    void statsUpdated();
    void freeDiskSpaceChecked(qint64 result);
    void categoryAdded(const QString &category);
    void categoryRemoved(const QString &category);
    void tagAdded(const QString &tag);
    void tagRemoved(const QString &tag);
};
