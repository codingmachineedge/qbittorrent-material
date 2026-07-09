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

#include "torrentfileswatcher.h"

#include <chrono>

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileSystemWatcher>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>
#include <QThread>
#include <QTimer>
#include <QVariant>

#include "base/algorithm.h"
#include "base/bittorrent/session.h"
#include "base/bittorrent/torrent.h"
#include "base/bittorrent/torrentcontentlayout.h"
#include "base/exceptions.h"
#include "base/global.h"
#include "base/logging.h"
#include "base/profile.h"
#include "base/settingsstorage.h"
#include "base/tagset.h"
#include "base/utils/fs.h"
#include "base/utils/fs/path.h"
#include "base/utils/io.h"
#include "base/utils/string.h"

using namespace std::chrono_literals;

const std::chrono::seconds WATCH_INTERVAL {10};
const int MAX_FAILED_RETRIES = 5;
const QString CONF_FILE_NAME = u"watched_folders.json"_s;

const QString OPTION_ADDTORRENTPARAMS = u"add_torrent_params"_s;
const QString OPTION_RECURSIVE = u"recursive"_s;

namespace
{
    TorrentFilesWatcher::WatchedFolderOptions parseWatchedFolderOptions(const QJsonObject &jsonObj)
    {
        TorrentFilesWatcher::WatchedFolderOptions options;
        options.addTorrentParams = BitTorrent::parseAddTorrentParams(jsonObj.value(OPTION_ADDTORRENTPARAMS).toObject());
        options.recursive = jsonObj.value(OPTION_RECURSIVE).toBool();

        return options;
    }

    QJsonObject serializeWatchedFolderOptions(const TorrentFilesWatcher::WatchedFolderOptions &options)
    {
        return {{OPTION_ADDTORRENTPARAMS, BitTorrent::serializeAddTorrentParams(options.addTorrentParams)},
                {OPTION_RECURSIVE, options.recursive}};
    }
}

/// The watcher owns a dedicated IO thread running a Worker. All filesystem
/// scanning happens on the Worker thread; results are delivered back to the
/// main thread via the queued torrentFound() -> onTorrentFound() connection,
/// which then hands the descriptor to BitTorrent::Session.
class TorrentFilesWatcher::Worker final : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(Worker)

public:
    Worker(QFileSystemWatcher *watcher);

public slots:
    void setWatchedFolder(const Path &path, const TorrentFilesWatcher::WatchedFolderOptions &options);
    void removeWatchedFolder(const Path &path);

signals:
    void torrentFound(const BitTorrent::TorrentDescriptor &torrentDescr, const BitTorrent::AddTorrentParams &addTorrentParams);

private:
    void onTimeout();
    void scheduleWatchedFolderProcessing(const Path &path);
    void processWatchedFolder(const Path &path);
    void processFolder(const Path &path, const Path &watchedFolderPath, const TorrentFilesWatcher::WatchedFolderOptions &options);
    void processFailedTorrents();
    void addWatchedFolder(const Path &path, const TorrentFilesWatcher::WatchedFolderOptions &options);
    void updateWatchedFolder(const Path &path, const TorrentFilesWatcher::WatchedFolderOptions &options);

    QFileSystemWatcher *m_watcher = nullptr;
    QTimer *m_watchTimer = nullptr;
    QHash<Path, TorrentFilesWatcher::WatchedFolderOptions> m_watchedFolders;
    QSet<Path> m_watchedByTimeoutFolders;

    // Failed torrents
    QTimer *m_retryTorrentTimer = nullptr;
    QHash<Path, QHash<Path, int>> m_failedTorrents;
};

TorrentFilesWatcher *TorrentFilesWatcher::m_instance = nullptr;

void TorrentFilesWatcher::initInstance()
{
    if (!m_instance)
    {
        qCDebug(lcEngine) << "TorrentFilesWatcher: creating singleton instance";
        m_instance = new TorrentFilesWatcher;
    }
}

void TorrentFilesWatcher::freeInstance()
{
    qCDebug(lcEngine) << "TorrentFilesWatcher: freeing singleton instance";
    delete m_instance;
    m_instance = nullptr;
}

TorrentFilesWatcher *TorrentFilesWatcher::instance()
{
    return m_instance;
}

TorrentFilesWatcher::TorrentFilesWatcher(QObject *parent)
    : QObject(parent)
    , m_ioThread {new QThread}
    , m_asyncWorker {new TorrentFilesWatcher::Worker(new QFileSystemWatcher(this))}
{
    qCDebug(lcEngine) << "TorrentFilesWatcher: constructing and starting IO thread";

    connect(m_asyncWorker, &TorrentFilesWatcher::Worker::torrentFound, this, &TorrentFilesWatcher::onTorrentFound);

    m_asyncWorker->moveToThread(m_ioThread.get());
    connect(m_ioThread.get(), &QThread::finished, m_asyncWorker, &QObject::deleteLater);
    m_ioThread->setObjectName("TorrentFilesWatcher m_ioThread");
    m_ioThread->start();

    load();
}

void TorrentFilesWatcher::load()
{
    const int fileMaxSize = 10 * 1024 * 1024;
    const Path path = specialFolderLocation(SpecialFolder::Config) / Path(CONF_FILE_NAME);

    const auto readResult = Utils::IO::readFile(path, fileMaxSize);
    if (!readResult)
    {
        if (readResult.error().status == Utils::IO::ReadError::NotExist)
        {
            qCDebug(lcEngine) << "TorrentFilesWatcher: no watched-folders config found; trying legacy settings";
            loadLegacy();
            return;
        }

        qCWarning(lcEngine).noquote() << tr("Failed to load Watched Folders configuration. %1").arg(readResult.error().message);
        return;
    }

    QJsonParseError jsonError;
    const QJsonDocument jsonDoc = QJsonDocument::fromJson(readResult.value(), &jsonError);
    if (jsonError.error != QJsonParseError::NoError)
    {
        qCWarning(lcEngine).noquote() << tr("Failed to parse Watched Folders configuration from %1. Error: \"%2\"")
            .arg(path.toString(), jsonError.errorString());
        return;
    }

    if (!jsonDoc.isObject())
    {
        qCWarning(lcEngine).noquote() << tr("Failed to load Watched Folders configuration from %1. Error: \"Invalid data format.\"")
            .arg(path.toString());
        return;
    }

    const QJsonObject jsonObj = jsonDoc.object();
    for (auto it = jsonObj.constBegin(); it != jsonObj.constEnd(); ++it)
    {
        const Path watchedFolder {it.key()};
        const WatchedFolderOptions options = parseWatchedFolderOptions(it.value().toObject());
        try
        {
            doSetWatchedFolder(watchedFolder, options);
        }
        catch (const InvalidArgument &err)
        {
            qCWarning(lcEngine).noquote() << err.message();
        }
    }

    qCInfo(lcEngine) << "TorrentFilesWatcher: loaded" << m_watchedFolders.size() << "watched folder(s)";
}

void TorrentFilesWatcher::loadLegacy()
{
    const auto dirs = SettingsStorage::instance()->loadValue<QVariantHash>(u"Preferences/Downloads/ScanDirsV2"_s);

    for (auto it = dirs.cbegin(); it != dirs.cend(); ++it)
    {
        const Path watchedFolder {it.key()};
        BitTorrent::AddTorrentParams params;
        if (it.value().userType() == QMetaType::Int)
        {
            if (it.value().toInt() == 0)
            {
                params.savePath = watchedFolder;
                params.useAutoTMM = false;
            }
        }
        else
        {
            const Path customSavePath {it.value().toString()};
            params.savePath = customSavePath;
            params.useAutoTMM = false;
        }

        try
        {
            doSetWatchedFolder(watchedFolder, {params, false});
        }
        catch (const InvalidArgument &err)
        {
            qCWarning(lcEngine).noquote() << err.message();
        }
    }

    store();
    SettingsStorage::instance()->removeValue(u"Preferences/Downloads/ScanDirsV2"_s);

    qCInfo(lcEngine) << "TorrentFilesWatcher: migrated" << dirs.size() << "legacy watched folder(s)";
}

void TorrentFilesWatcher::store() const
{
    QJsonObject jsonObj;
    for (auto it = m_watchedFolders.cbegin(); it != m_watchedFolders.cend(); ++it)
    {
        const Path &watchedFolder = it.key();
        const WatchedFolderOptions &options = it.value();
        jsonObj[watchedFolder.data()] = serializeWatchedFolderOptions(options);
    }

    const Path path = specialFolderLocation(SpecialFolder::Config) / Path(CONF_FILE_NAME);
    const QByteArray data = QJsonDocument(jsonObj).toJson();
    const nonstd::expected<void, QString> result = Utils::IO::saveToFile(path, data);
    if (!result)
    {
        qCWarning(lcEngine).noquote() << tr("Couldn't store Watched Folders configuration to %1. Error: %2")
            .arg(path.toString(), result.error());
        return;
    }

    qCDebug(lcEngine) << "TorrentFilesWatcher: stored" << m_watchedFolders.size() << "watched folder(s)";
}

QHash<Path, TorrentFilesWatcher::WatchedFolderOptions> TorrentFilesWatcher::folders() const
{
    return m_watchedFolders;
}

void TorrentFilesWatcher::setWatchedFolder(const Path &path, const WatchedFolderOptions &options)
{
    qCInfo(lcEngine).noquote() << "TorrentFilesWatcher: setting watched folder" << path.toString()
                               << "recursive=" << options.recursive;
    doSetWatchedFolder(path, options);
    store();
}

void TorrentFilesWatcher::doSetWatchedFolder(const Path &path, const WatchedFolderOptions &options)
{
    if (path.isEmpty())
        throw InvalidArgument(tr("Watched folder Path cannot be empty."));

    if (path.isRelative())
        throw InvalidArgument(tr("Watched folder Path cannot be relative."));

    m_watchedFolders[path] = options;

    QMetaObject::invokeMethod(m_asyncWorker, [this, path, options]
    {
        m_asyncWorker->setWatchedFolder(path, options);
    });

    emit watchedFolderSet(path, options);
}

void TorrentFilesWatcher::removeWatchedFolder(const Path &path)
{
    if (m_watchedFolders.remove(path))
    {
        qCInfo(lcEngine).noquote() << "TorrentFilesWatcher: removing watched folder" << path.toString();

        if (m_asyncWorker)
        {
            QMetaObject::invokeMethod(m_asyncWorker, [this, path]()
            {
                m_asyncWorker->removeWatchedFolder(path);
            });
        }

        emit watchedFolderRemoved(path);

        store();
    }
}

void TorrentFilesWatcher::onTorrentFound(const BitTorrent::TorrentDescriptor &torrentDescr
        , const BitTorrent::AddTorrentParams &addTorrentParams)
{
    qCDebug(lcEngine).noquote() << "TorrentFilesWatcher: found torrent" << torrentDescr.name() << "-> adding to session";
    BitTorrent::Session::instance()->addTorrent(torrentDescr, addTorrentParams);
}

TorrentFilesWatcher::Worker::Worker(QFileSystemWatcher *watcher)
    : m_watcher {watcher}
    , m_watchTimer {new QTimer(this)}
    , m_retryTorrentTimer {new QTimer(this)}
{
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &path)
    {
        qCDebug(lcEngine).noquote() << "TorrentFilesWatcher: directory changed" << path;
        scheduleWatchedFolderProcessing(Path(path));
    });
    connect(m_watchTimer, &QTimer::timeout, this, &Worker::onTimeout);

    connect(m_retryTorrentTimer, &QTimer::timeout, this, &Worker::processFailedTorrents);
}

void TorrentFilesWatcher::Worker::onTimeout()
{
    for (const Path &path : asConst(m_watchedByTimeoutFolders))
        processWatchedFolder(path);
}

void TorrentFilesWatcher::Worker::setWatchedFolder(const Path &path, const TorrentFilesWatcher::WatchedFolderOptions &options)
{
    if (m_watchedFolders.contains(path))
        updateWatchedFolder(path, options);
    else
        addWatchedFolder(path, options);
}

void TorrentFilesWatcher::Worker::removeWatchedFolder(const Path &path)
{
    m_watchedFolders.remove(path);

    m_watcher->removePath(path.data());
    m_watchedByTimeoutFolders.remove(path);
    if (m_watchedByTimeoutFolders.isEmpty())
        m_watchTimer->stop();

    m_failedTorrents.remove(path);
    if (m_failedTorrents.isEmpty())
        m_retryTorrentTimer->stop();
}

void TorrentFilesWatcher::Worker::scheduleWatchedFolderProcessing(const Path &path)
{
    QTimer::singleShot(2s, Qt::CoarseTimer, this, [this, path]
    {
        processWatchedFolder(path);
    });
}

void TorrentFilesWatcher::Worker::processWatchedFolder(const Path &path)
{
    const TorrentFilesWatcher::WatchedFolderOptions options = m_watchedFolders.value(path);
    processFolder(path, path, options);

    if (!m_failedTorrents.empty() && !m_retryTorrentTimer->isActive())
        m_retryTorrentTimer->start(WATCH_INTERVAL);
}

void TorrentFilesWatcher::Worker::processFolder(const Path &path, const Path &watchedFolderPath
                                              , const TorrentFilesWatcher::WatchedFolderOptions &options)
{
    QDirIterator dirIter {path.data(), {u"*.torrent"_s, u"*.magnet"_s}, QDir::Files};
    while (dirIter.hasNext())
    {
        const Path filePath {dirIter.next()};
        BitTorrent::AddTorrentParams addTorrentParams = options.addTorrentParams;
        if (path != watchedFolderPath)
        {
            const Path subdirPath = watchedFolderPath.relativePathOf(path);
            const bool useAutoTMM = addTorrentParams.useAutoTMM.value_or(!BitTorrent::Session::instance()->isAutoTMMDisabledByDefault());
            if (useAutoTMM)
            {
                addTorrentParams.category = addTorrentParams.category.isEmpty()
                        ? subdirPath.data() : (addTorrentParams.category + u'/' + subdirPath.data());
            }
            else
            {
                addTorrentParams.savePath = addTorrentParams.savePath / subdirPath;
            }
        }

        if (filePath.hasExtension(u".magnet"_s))
        {
            const int fileMaxSize = 100 * 1024 * 1024;

            QFile file {filePath.data()};
            if (file.open(QIODevice::ReadOnly | QIODevice::Text))
            {
                if (file.size() <= fileMaxSize)
                {
                    while (!file.atEnd())
                    {
                        const auto line = QString::fromLatin1(file.readLine()).trimmed();
                        if (const auto parseResult = BitTorrent::TorrentDescriptor::parse(line))
                            emit torrentFound(parseResult.value(), addTorrentParams);
                        else
                            qCWarning(lcEngine).noquote() << tr("Invalid Magnet URI. URI: %1. Reason: %2").arg(line, parseResult.error());
                    }

                    file.close();
                    Utils::Fs::removeFile(filePath);
                }
                else
                {
                    qCWarning(lcEngine).noquote() << tr("Magnet file too big. File: %1").arg(file.errorString());
                }
            }
            else
            {
                qCWarning(lcEngine).noquote() << tr("Failed to open magnet file: %1").arg(file.errorString());
            }
        }
        else
        {
            if (const auto loadResult = BitTorrent::TorrentDescriptor::loadFromFile(filePath))
            {
                emit torrentFound(loadResult.value(), addTorrentParams);
                Utils::Fs::removeFile(filePath);
            }
            else
            {
                if (!m_failedTorrents.value(watchedFolderPath).contains(filePath))
                {
                    m_failedTorrents[watchedFolderPath][filePath] = 0;
                }
            }
        }
    }

    if (options.recursive)
    {
        QDirIterator iter {path.data(), (QDir::Dirs | QDir::NoDotAndDotDot)};
        while (iter.hasNext())
        {
            const Path folderPath {iter.next()};
            // Skip processing of subdirectory that is explicitly set as watched folder
            if (!m_watchedFolders.contains(folderPath))
                processFolder(folderPath, watchedFolderPath, options);
        }
    }
}

void TorrentFilesWatcher::Worker::processFailedTorrents()
{
    // Check which torrents are still partial
    Algorithm::removeIf(m_failedTorrents, [this](const Path &watchedFolderPath, QHash<Path, int> &partialTorrents)
    {
        const TorrentFilesWatcher::WatchedFolderOptions options = m_watchedFolders.value(watchedFolderPath);
        Algorithm::removeIf(partialTorrents, [this, &watchedFolderPath, &options](const Path &torrentPath, int &value)
        {
            if (!torrentPath.exists())
                return true;

            if (const auto loadResult = BitTorrent::TorrentDescriptor::loadFromFile(torrentPath))
            {
                BitTorrent::AddTorrentParams addTorrentParams = options.addTorrentParams;
                if (torrentPath != watchedFolderPath)
                {
                    const Path subdirPath = watchedFolderPath.relativePathOf(torrentPath).parentPath();
                    const bool useAutoTMM = addTorrentParams.useAutoTMM.value_or(!BitTorrent::Session::instance()->isAutoTMMDisabledByDefault());
                    if (useAutoTMM)
                    {
                        addTorrentParams.category = addTorrentParams.category.isEmpty()
                                ? subdirPath.data() : (addTorrentParams.category + u'/' + subdirPath.data());
                    }
                    else
                    {
                        addTorrentParams.savePath = addTorrentParams.savePath / subdirPath;
                    }
                }

                emit torrentFound(loadResult.value(), addTorrentParams);
                Utils::Fs::removeFile(torrentPath);

                return true;
            }

            if (value >= MAX_FAILED_RETRIES)
            {
                qCWarning(lcEngine).noquote() << tr("Rejecting failed torrent file: %1").arg(torrentPath.toString());
                Utils::Fs::renameFile(torrentPath, (torrentPath + u".qbt_rejected"));
                return true;
            }

            ++value;
            return false;
        });

        if (partialTorrents.isEmpty())
            return true;

        return false;
    });

    // Stop the partial timer if necessary
    if (m_failedTorrents.empty())
        m_retryTorrentTimer->stop();
    else
        m_retryTorrentTimer->start(WATCH_INTERVAL);
}

void TorrentFilesWatcher::Worker::addWatchedFolder(const Path &path, const TorrentFilesWatcher::WatchedFolderOptions &options)
{
    // Check if the `path` points to a network file system or not
    if (Utils::Fs::isNetworkFileSystem(path) || options.recursive)
    {
        m_watchedByTimeoutFolders.insert(path);
        if (!m_watchTimer->isActive())
            m_watchTimer->start(WATCH_INTERVAL);
    }
    else
    {
        m_watcher->addPath(path.data());
        scheduleWatchedFolderProcessing(path);
    }

    m_watchedFolders[path] = options;

    qCInfo(lcEngine).noquote() << tr("Watching folder: \"%1\"").arg(path.toString());
}

void TorrentFilesWatcher::Worker::updateWatchedFolder(const Path &path, const TorrentFilesWatcher::WatchedFolderOptions &options)
{
    const bool recursiveModeChanged = (m_watchedFolders[path].recursive != options.recursive);
    if (recursiveModeChanged && !Utils::Fs::isNetworkFileSystem(path))
    {
        if (options.recursive)
        {
            m_watcher->removePath(path.data());

            m_watchedByTimeoutFolders.insert(path);
            if (!m_watchTimer->isActive())
                m_watchTimer->start(WATCH_INTERVAL);
        }
        else
        {
            m_watchedByTimeoutFolders.remove(path);
            if (m_watchedByTimeoutFolders.isEmpty())
                m_watchTimer->stop();

            m_watcher->addPath(path.data());
            scheduleWatchedFolderProcessing(path);
        }
    }

    m_watchedFolders[path] = options;
}

#include "torrentfileswatcher.moc"
