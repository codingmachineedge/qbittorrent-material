/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026  qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "sessioncontroller.h"

#include <QSet>
#include <QTimer>

#include "base/bittorrent/categoryoptions.h"
#include "base/bittorrent/downloadpathoption.h"
#include "base/bittorrent/session.h"
#include "base/bittorrent/sessionstatus.h"
#include "base/bittorrent/sharelimits.h"
#include "base/bittorrent/torrent.h"
#include "base/logging.h"
#include "base/tag.h"
#include "base/tagset.h"

using BitTorrent::Session;

namespace
{
    Session *nativeSession()
    {
        return Session::instance();
    }
}

SessionController *SessionController::create(QQmlEngine *, QJSEngine *)
{
    qCInfo(lcUi) << "Session QML singleton requested";
    return new SessionController;
}

SessionController::SessionController(QObject *parent)
    : QObject(parent)
{
    Session *const session = nativeSession();
    Q_ASSERT(session);

    connect(session, &Session::paused, this, &SessionController::pausedChanged);
    connect(session, &Session::resumed, this, &SessionController::pausedChanged);
    connect(session, &Session::statsUpdated, this, &SessionController::statsUpdated);
    connect(session, &Session::freeDiskSpaceChecked, this,
        [this](const qint64 result)
        {
            emit freeDiskSpaceChecked(result);
            emit statsUpdated();
        });

    const auto refreshTorrentCount = [this]
    {
        // torrentAboutToBeRemoved fires before the list mutates.
        QTimer::singleShot(0, this, [this] { emit torrentCountChanged(); });
    };
    connect(session, &Session::torrentsLoaded, this,
        [refreshTorrentCount](const QList<BitTorrent::Torrent *> &) { refreshTorrentCount(); });
    connect(session, &Session::torrentAdded, this,
        [refreshTorrentCount](BitTorrent::Torrent *) { refreshTorrentCount(); });
    connect(session, &Session::torrentAboutToBeRemoved, this,
        [refreshTorrentCount](BitTorrent::Torrent *) { refreshTorrentCount(); });

    connect(session, &Session::categoryAdded, this, &SessionController::categoryAdded);
    connect(session, &Session::categoryRemoved, this, &SessionController::categoryRemoved);
    connect(session, &Session::tagAdded, this,
        [this](const Tag &tag) { emit tagAdded(tag.toString()); });
    connect(session, &Session::tagRemoved, this,
        [this](const Tag &tag) { emit tagRemoved(tag.toString()); });

    qCDebug(lcUi) << "SessionController subscribed to BitTorrent::Session";
}

bool SessionController::paused() const { return nativeSession()->isPaused(); }
bool SessionController::queueingEnabled() const { return nativeSession()->isQueueingSystemEnabled(); }
int SessionController::torrentCount() const { return static_cast<int>(nativeSession()->torrentsCount()); }
qint64 SessionController::downloadRate() const { return nativeSession()->status().payloadDownloadRate; }
qint64 SessionController::uploadRate() const { return nativeSession()->status().payloadUploadRate; }
bool SessionController::dhtEnabled() const { return nativeSession()->isDHTEnabled(); }
qint64 SessionController::dhtNodes() const { return nativeSession()->status().dhtNodes; }
bool SessionController::listening() const { return nativeSession()->isListening(); }
bool SessionController::hasIncomingConnections() const { return nativeSession()->status().hasIncomingConnections; }
QString SessionController::externalIPv4() const { return nativeSession()->lastExternalIPv4Address(); }
QString SessionController::externalIPv6() const { return nativeSession()->lastExternalIPv6Address(); }
qint64 SessionController::freeDiskSpace() const { return nativeSession()->freeDiskSpace(); }

QStringList SessionController::categories() const
{
    return nativeSession()->categories();
}

QStringList SessionController::tags() const
{
    QStringList result;
    for (const Tag &tag : nativeSession()->tags())
        result.push_back(tag.toString());
    return result;
}

QString SessionController::categorySavePath(const QString &category) const
{
    return nativeSession()->categorySavePath(category).toString();
}

QString SessionController::categoryDownloadPath(const QString &category) const
{
    return nativeSession()->categoryDownloadPath(category).toString();
}

bool SessionController::addCategory(const QString &category)
{
    qCInfo(lcUi) << "Adding category from QML:" << category;
    return nativeSession()->addCategory(category);
}

bool SessionController::removeCategory(const QString &category)
{
    qCInfo(lcUi) << "Removing category from QML:" << category;
    return nativeSession()->removeCategory(category);
}

bool SessionController::setCategoryOptions(const QString &category, const QVariantMap &values)
{
    BitTorrent::CategoryOptions options = nativeSession()->categoryOptions(category);
    options.savePath = Path(values.value(QStringLiteral("savePath")).toString());

    const int useDownloadPath = values.value(QStringLiteral("useDownloadPath"), 0).toInt();
    if (useDownloadPath == 0)
    {
        options.downloadPath.reset();
    }
    else
    {
        options.downloadPath = BitTorrent::DownloadPathOption {
            useDownloadPath == 1,
            Path(values.value(QStringLiteral("downloadPath")).toString())
        };
    }

    options.shareLimits.ratioLimit = values.value(QStringLiteral("ratioLimit"), -2).toDouble();
    options.shareLimits.seedingTimeLimit = values.value(QStringLiteral("seedingTimeLimit"), -2).toInt();
    options.shareLimits.inactiveSeedingTimeLimit =
        values.value(QStringLiteral("inactiveSeedingTimeLimit"), -2).toInt();
    options.shareLimits.mode = static_cast<BitTorrent::ShareLimitsMode>(
        values.value(QStringLiteral("shareLimitMode"), -1).toInt());
    options.shareLimits.action = static_cast<BitTorrent::ShareLimitAction>(
        values.value(QStringLiteral("shareLimitAction"), -1).toInt());

    qCInfo(lcUi) << "Updating category options from QML:" << category;
    return nativeSession()->setCategoryOptions(category, options);
}

bool SessionController::addTag(const QString &tag)
{
    const Tag value(tag);
    if (!value.isValid())
        return false;
    qCInfo(lcUi) << "Adding tag from QML:" << tag;
    return nativeSession()->addTag(value);
}

bool SessionController::removeTag(const QString &tag)
{
    const Tag value(tag);
    if (!value.isValid())
        return false;
    qCInfo(lcUi) << "Removing tag from QML:" << tag;
    return nativeSession()->removeTag(value);
}

int SessionController::removeUnusedCategories()
{
    Session *const session = nativeSession();
    QSet<QString> used;
    for (const BitTorrent::Torrent *torrent : session->torrents())
    {
        const QString category = torrent->category();
        if (category.isEmpty())
            continue;
        const QStringList hierarchy = Session::expandCategory(category);
        for (const QString &entry : hierarchy)
            used.insert(entry);
    }

    int removed = 0;
    const QStringList existing = session->categories();
    for (const QString &category : existing)
    {
        if (!used.contains(category) && session->removeCategory(category))
            ++removed;
    }
    qCInfo(lcUi) << "Removed unused categories:" << removed;
    return removed;
}

int SessionController::removeUnusedTags()
{
    Session *const session = nativeSession();
    QSet<QString> used;
    for (const BitTorrent::Torrent *torrent : session->torrents())
    {
        for (const Tag &tag : torrent->tags())
            used.insert(tag.toString());
    }

    int removed = 0;
    const TagSet existing = session->tags();
    for (const Tag &tag : existing)
    {
        if (!used.contains(tag.toString()) && session->removeTag(tag))
            ++removed;
    }
    qCInfo(lcUi) << "Removed unused tags:" << removed;
    return removed;
}
