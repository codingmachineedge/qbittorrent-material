/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "workspacemanager.h"

#include <algorithm>
#include <functional>
#include <utility>

#include <QColor>
#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QFontDatabase>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSet>
#include <QSettings>
#include <QStandardPaths>
#include <QUuid>

#include <git2.h>

#include "base/logging.h"

namespace
{
    constexpr int SchemaVersion = 1;
    constexpr int MaximumTabs = 100;
    constexpr qsizetype MaximumContentCharacters = 4 * 1024 * 1024;
    constexpr qint64 MaximumWorkspaceBytes = 32LL * 1024 * 1024;
    constexpr qint64 MaximumRepositoryBytes = 256LL * 1024 * 1024;
    constexpr auto ProductDisplayName = "qBittorrent Material";

    QString gitErrorText(const QString &fallback)
    {
        if (const git_error *error = git_error_last(); error && error->message)
            return QString::fromUtf8(error->message);
        return fallback;
    }

    QString isoDate(const QDateTime &dateTime)
    {
        return dateTime.toUTC().toString(Qt::ISODateWithMs);
    }

    bool hasSymlinkIdentity(const QFileInfo &info)
    {
        return info.isSymLink() || info.isJunction() || !info.symLinkTarget().isEmpty();
    }
}

WorkspaceManager::WorkspaceManager(QObject *parent)
    : QAbstractListModel(parent)
{
    git_libgit2_init();

    const QString overrideRoot = qEnvironmentVariable("QBT_WORKSPACE_ROOT").trimmed();
    const QString dataRoot = overrideRoot.isEmpty()
        ? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
        : overrideRoot;
    m_repositoryPath = overrideRoot.isEmpty()
        ? QDir(dataRoot).filePath(QStringLiteral("workspace-tabs"))
        : QDir::cleanPath(dataRoot);

    m_saveTimer.setSingleShot(true);
    m_saveTimer.setInterval(650);
    connect(&m_saveTimer, &QTimer::timeout, this, [this]
    {
        QString error;
        const QString message = m_pendingCommitMessage.isEmpty()
            ? QStringLiteral("workspace: autosave") : m_pendingCommitMessage;
        m_pendingCommitMessage.clear();
        if (!saveNow(message, &error))
            qCWarning(lcUi) << "Workspace autosave failed:" << error;
    });

    loadWorkspace();
    QGuiApplication::setApplicationDisplayName(m_appDisplayName);

    if (m_initializationBlocked)
        return;

    QString error;
    if (!saveNow(QStringLiteral("workspace: initialize"), &error))
    {
        m_repositoryStatus = tr("Saved files; local Git needs attention");
        qCWarning(lcUi) << "Workspace repository initialization failed:" << error;
    }
    else if (!m_recoveryPath.isEmpty())
    {
        m_repositoryStatus = tr("Recovered safely; previous files kept in %1")
            .arg(QDir::toNativeSeparators(m_recoveryPath));
        emit repositoryStatusChanged();
    }
}

WorkspaceManager::~WorkspaceManager()
{
    m_saveTimer.stop();
    if (m_dirty)
    {
        QString error;
        if (!saveNow(QStringLiteral("workspace: shutdown save"), &error))
            qCWarning(lcUi) << "Workspace shutdown save failed:" << error;
    }
    git_libgit2_shutdown();
}

int WorkspaceManager::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : m_tabs.size();
}

QVariant WorkspaceManager::data(const QModelIndex &index, const int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_tabs.size())
        return {};
    const Tab &tab = m_tabs.at(index.row());
    switch (role)
    {
    case TabIdRole: return tab.id;
    case NameRole: return tab.name;
    case ContentRole: return tab.content;
    case FontFamilyRole: return tab.fontFamily;
    case FontStyleRole: return tab.fontStyle;
    case FontPointSizeRole: return tab.fontPointSize;
    case BoldRole: return tab.bold;
    case ItalicRole: return tab.italic;
    case FontColorRole: return tab.fontColor;
    case CreatedAtRole: return isoDate(tab.createdAt);
    case UpdatedAtRole: return isoDate(tab.updatedAt);
    default: return {};
    }
}

QHash<int, QByteArray> WorkspaceManager::roleNames() const
{
    return {
        {TabIdRole, "tabId"},
        {NameRole, "name"},
        {ContentRole, "content"},
        {FontFamilyRole, "fontFamily"},
        {FontStyleRole, "fontStyle"},
        {FontPointSizeRole, "fontPointSize"},
        {BoldRole, "bold"},
        {ItalicRole, "italic"},
        {FontColorRole, "fontColor"},
        {CreatedAtRole, "createdAt"},
        {UpdatedAtRole, "updatedAt"}
    };
}

QString WorkspaceManager::appDisplayName() const
{
    return m_appDisplayName;
}

void WorkspaceManager::setAppDisplayName(const QString &name)
{
    const QString normalized = normalizedName(name, 80, QString::fromLatin1(ProductDisplayName));
    if (m_appDisplayName == normalized)
        return;
    m_appDisplayName = normalized;
    QGuiApplication::setApplicationDisplayName(m_appDisplayName);
    emit appDisplayNameChanged();
    scheduleSave(QStringLiteral("workspace: rename application display"));
}

int WorkspaceManager::count() const
{
    return m_tabs.size();
}

int WorkspaceManager::activeIndex() const
{
    return m_activeIndex;
}

void WorkspaceManager::setActiveIndex(const int index)
{
    const int normalized = (index >= 0 && index < m_tabs.size()) ? index : -1;
    if (m_activeIndex == normalized)
        return;
    m_activeIndex = normalized;
    emit activeIndexChanged();
    if (!m_loading)
        scheduleSave(QStringLiteral("workspace: select tab"));
}

QString WorkspaceManager::activeTabId() const
{
    return (m_activeIndex >= 0 && m_activeIndex < m_tabs.size())
        ? m_tabs.at(m_activeIndex).id : QString();
}

QString WorkspaceManager::repositoryPath() const
{
    return m_repositoryPath;
}

QUrl WorkspaceManager::repositoryUrl() const
{
    return QUrl::fromLocalFile(m_repositoryPath);
}

QString WorkspaceManager::repositoryStatus() const
{
    return m_repositoryStatus;
}

QString WorkspaceManager::lastCommitId() const
{
    return m_lastCommitId;
}

bool WorkspaceManager::dirty() const
{
    return m_dirty;
}

QVariantMap WorkspaceManager::tabAt(const int index) const
{
    return (index >= 0 && index < m_tabs.size()) ? tabMap(m_tabs.at(index)) : QVariantMap();
}

QVariantMap WorkspaceManager::tabById(const QString &tabId) const
{
    const int index = indexOfTab(tabId);
    return (index >= 0) ? tabMap(m_tabs.at(index)) : QVariantMap();
}

QString WorkspaceManager::createTab(const QString &name)
{
    if (m_tabs.size() >= MaximumTabs)
    {
        emit operationFinished(false, tr("A workspace can contain at most %1 tabs.").arg(MaximumTabs), {});
        return {};
    }

    Tab tab;
    tab.id = newTabId();
    tab.name = normalizedName(name, 120, tr("New tab"));
    tab.fontFamily = QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
    const QStringList styles = QFontDatabase::styles(tab.fontFamily);
    tab.fontStyle = styles.contains(QStringLiteral("Regular"))
        ? QStringLiteral("Regular") : styles.value(0, QStringLiteral("Regular"));
    tab.createdAt = tab.updatedAt = QDateTime::currentDateTimeUtc();

    const int row = m_tabs.size();
    beginInsertRows({}, row, row);
    m_tabs.push_back(tab);
    endInsertRows();
    emit countChanged();
    setActiveIndex(row);
    scheduleSave(QStringLiteral("workspace: create tab"));
    return tab.id;
}

int WorkspaceManager::duplicateTab(const int index)
{
    if (index < 0 || index >= m_tabs.size() || m_tabs.size() >= MaximumTabs)
        return -1;
    Tab copy = m_tabs.at(index);
    copy.id = newTabId();
    copy.name = normalizedName(tr("%1 copy").arg(copy.name), 120, tr("Tab copy"));
    copy.createdAt = copy.updatedAt = QDateTime::currentDateTimeUtc();
    const int destination = index + 1;
    beginInsertRows({}, destination, destination);
    m_tabs.insert(destination, copy);
    endInsertRows();
    emit countChanged();
    setActiveIndex(destination);
    scheduleSave(QStringLiteral("workspace: duplicate tab"));
    return destination;
}

bool WorkspaceManager::closeTab(const int index)
{
    if (index < 0 || index >= m_tabs.size())
        return false;
    beginRemoveRows({}, index, index);
    m_tabs.removeAt(index);
    endRemoveRows();
    emit countChanged();

    int next = m_activeIndex;
    if (m_tabs.isEmpty())
        next = -1;
    else if (index < m_activeIndex)
        next = m_activeIndex - 1;
    else if (index == m_activeIndex)
        next = qMin(index, m_tabs.size() - 1);
    m_activeIndex = next;
    emit activeIndexChanged();
    scheduleSave(QStringLiteral("workspace: close tab"));
    return true;
}

bool WorkspaceManager::closeOtherTabs(const int index)
{
    if (index < 0 || index >= m_tabs.size())
        return false;
    const Tab retained = m_tabs.at(index);
    beginResetModel();
    m_tabs = {retained};
    m_activeIndex = 0;
    endResetModel();
    emit countChanged();
    emit activeIndexChanged();
    scheduleSave(QStringLiteral("workspace: close other tabs"));
    return true;
}

bool WorkspaceManager::moveTab(const int from, const int to)
{
    if (from < 0 || from >= m_tabs.size() || to < 0 || to >= m_tabs.size() || from == to)
        return false;
    const int destinationChild = (to > from) ? to + 1 : to;
    if (!beginMoveRows({}, from, from, {}, destinationChild))
        return false;
    m_tabs.move(from, to);
    endMoveRows();
    if (m_activeIndex == from)
        m_activeIndex = to;
    else if (from < m_activeIndex && to >= m_activeIndex)
        --m_activeIndex;
    else if (from > m_activeIndex && to <= m_activeIndex)
        ++m_activeIndex;
    emit activeIndexChanged();
    scheduleSave(QStringLiteral("workspace: reorder tabs"));
    return true;
}

bool WorkspaceManager::setTabContent(const QString &tabId, const QString &content)
{
    const int row = indexOfTab(tabId);
    if (row < 0)
        return false;
    if (content.size() > MaximumContentCharacters)
    {
        emit operationFinished(false, tr("A tab can contain at most 4 million characters."), {});
        return false;
    }
    Tab &tab = m_tabs[row];
    if (tab.content == content)
        return true;
    tab.content = content;
    tab.updatedAt = QDateTime::currentDateTimeUtc();
    emit dataChanged(index(row), index(row), {ContentRole, UpdatedAtRole});
    scheduleSave(QStringLiteral("workspace: edit tab"));
    return true;
}

bool WorkspaceManager::updateTab(const QString &tabId, const QString &name,
    const QString &fontFamily, const QString &fontStyle, const double fontPointSize,
    const bool bold, const bool italic, const QString &fontColor)
{
    const int row = indexOfTab(tabId);
    const QColor parsedColor(fontColor);
    if (row < 0 || fontPointSize < 6.0 || fontPointSize > 144.0 || !parsedColor.isValid())
        return false;

    Tab &tab = m_tabs[row];
    tab.name = normalizedName(name, 120, tr("Untitled tab"));
    tab.fontFamily = normalizedName(fontFamily, 128,
        QFontDatabase::systemFont(QFontDatabase::GeneralFont).family());
    tab.fontStyle = normalizedName(fontStyle, 64, QStringLiteral("Regular"));
    tab.fontPointSize = fontPointSize;
    tab.bold = bold;
    tab.italic = italic;
    tab.fontColor = parsedColor.name(QColor::HexArgb).toUpper();
    tab.updatedAt = QDateTime::currentDateTimeUtc();
    emit dataChanged(index(row), index(row), {
        NameRole, FontFamilyRole, FontStyleRole, FontPointSizeRole,
        BoldRole, ItalicRole, FontColorRole, UpdatedAtRole
    });
    scheduleSave(QStringLiteral("workspace: customize tab"));
    return true;
}

QStringList WorkspaceManager::fontFamilies() const
{
    QStringList families = QFontDatabase::families();
    families.sort(Qt::CaseInsensitive);
    return families;
}

QStringList WorkspaceManager::fontStyles(const QString &family) const
{
    QStringList styles = QFontDatabase::styles(family);
    if (styles.isEmpty())
        styles << QStringLiteral("Regular");
    return styles;
}

QFont WorkspaceManager::resolvedFont(const QString &family, const QString &style,
    const double pointSize, const bool bold, const bool italic) const
{
    const double normalizedSize = qBound(6.0, pointSize, 144.0);
    QFont font = QFontDatabase::font(family, style, qRound(normalizedSize));
    // Preserve the selected face's weight/stretch/slant as ordinary QFont
    // attributes, then clear styleName so explicit Bold and Italic additions
    // are not ignored by Qt's named-style matching.
    font.setStyleName(QString());
    font.setPointSizeF(normalizedSize);
    if (bold)
        font.setBold(true);
    if (italic)
        font.setItalic(true);
    return font;
}

QUrl WorkspaceManager::suggestedExportUrl(const QString &fileName) const
{
    const QString documents = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    return QUrl::fromLocalFile(QDir(documents).filePath(QFileInfo(fileName).fileName()));
}

bool WorkspaceManager::openRepository() const
{
    return QDesktopServices::openUrl(repositoryUrl());
}

bool WorkspaceManager::syncNow()
{
    m_saveTimer.stop();
    QString error;
    const bool ok = saveNow(QStringLiteral("workspace: manual sync"), &error);
    emit operationFinished(ok, ok ? tr("Workspace committed to local Git.") : error, repositoryUrl());
    return ok;
}

bool WorkspaceManager::exportWorkspace(const QUrl &destination)
{
    const QString path = localPath(destination);
    if (path.isEmpty())
    {
        emit operationFinished(false, tr("Choose a local JSON destination."), {});
        return false;
    }
    const QFileInfo destinationInfo(path);
    const QFileInfo destinationParent(destinationInfo.absolutePath());
    if (isInsidePath(path, m_repositoryPath)
        || !destinationParent.isDir() || hasSymlinkIdentity(destinationParent)
        || (destinationInfo.exists() && (!destinationInfo.isFile() || hasSymlinkIdentity(destinationInfo))))
    {
        emit operationFinished(false,
            tr("Choose a safe JSON file outside the managed Git repository."), {});
        return false;
    }
    qint64 contentBytes = 0;
    for (const Tab &tab : std::as_const(m_tabs))
    {
        contentBytes += tab.content.toUtf8().size();
        if (contentBytes > MaximumWorkspaceBytes)
        {
            emit operationFinished(false,
                tr("This workspace is larger than the 32 MB JSON export limit. Export its complete Git repository instead."), {});
            return false;
        }
    }
    QJsonObject object = workspaceObject(true);
    const QByteArray payload = QJsonDocument(object).toJson(QJsonDocument::Indented);
    if (payload.size() > MaximumWorkspaceBytes)
    {
        emit operationFinished(false,
            tr("This workspace is larger than the 32 MB JSON export limit. Export its complete Git repository instead."), {});
        return false;
    }
    QString error;
    const bool ok = writeFileAtomically(path, payload, &error);
    emit operationFinished(ok,
        ok ? tr("Workspace snapshot exported.") : error, QUrl::fromLocalFile(path));
    return ok;
}

bool WorkspaceManager::importWorkspace(const QUrl &source)
{
    const QString path = localPath(source);
    const QFileInfo info(path);
    if (path.isEmpty() || !info.isFile() || info.size() > MaximumWorkspaceBytes || hasSymlinkIdentity(info))
    {
        emit operationFinished(false, tr("The selected workspace JSON is not a safe local file."), {});
        return false;
    }
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly))
    {
        emit operationFinished(false, file.errorString(), QUrl::fromLocalFile(path));
        return false;
    }
    Snapshot snapshot;
    QString error;
    if (!parseWorkspace(file.readAll(), &snapshot, &error, true))
    {
        emit operationFinished(false, error, QUrl::fromLocalFile(path));
        return false;
    }

    Snapshot previous {m_appDisplayName, activeTabId(), m_tabs};
    if (!saveNow(QStringLiteral("workspace: backup before JSON import"), &error))
    {
        emit operationFinished(false, error, repositoryUrl());
        return false;
    }
    applySnapshot(std::move(snapshot));
    if (!saveNow(QStringLiteral("workspace: import JSON snapshot"), &error))
    {
        applySnapshot(std::move(previous));
        QString rollbackError;
        const bool restored = saveNow(QStringLiteral("workspace: restore after failed JSON import"), &rollbackError);
        if (!restored)
            setDirty(true);
        emit operationFinished(false,
            restored ? tr("Import failed and the previous workspace was restored: %1").arg(error)
                     : tr("Import failed; automatic restoration also needs attention: %1 / %2")
                           .arg(error, rollbackError),
            QUrl::fromLocalFile(path));
        return false;
    }
    emit operationFinished(true, tr("Workspace snapshot imported and committed."), repositoryUrl());
    return true;
}

bool WorkspaceManager::exportRepository(const QUrl &destinationFolder)
{
    const QString destinationRoot = localPath(destinationFolder);
    QFileInfo destinationInfo(destinationRoot);
    if (destinationRoot.isEmpty() || !destinationInfo.isDir() || hasSymlinkIdentity(destinationInfo))
    {
        emit operationFinished(false, tr("Choose a safe local folder for the Git repository."), {});
        return false;
    }
    QString error;
    if (!saveNow(QStringLiteral("workspace: prepare repository export"), &error))
    {
        emit operationFinished(false, error, repositoryUrl());
        return false;
    }

    const QString folderName = QStringLiteral("%1-workspace-repo-%2-%3")
        .arg(fileSafeName(m_appDisplayName),
            QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")),
            QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
    const QString destination = QDir(destinationRoot).filePath(folderName);
    if (isInsidePath(destination, m_repositoryPath) || isInsidePath(m_repositoryPath, destination))
    {
        emit operationFinished(false, tr("The export folder cannot be inside the managed repository."), {});
        return false;
    }
    if (!QDir(destinationRoot).mkdir(folderName))
    {
        emit operationFinished(false, tr("Could not create a unique repository export folder."), {});
        return false;
    }
    qint64 copied = 0;
    const bool ok = copyTree(m_repositoryPath, destination, &copied, &error)
        && validateRepositoryRoot(destination, &error);
    if (!ok)
    {
        QString removeError;
        (void)removeTree(destination, &removeError);
    }
    emit operationFinished(ok,
        ok ? tr("Complete Git repository exported with its history.") : error,
        ok ? QUrl::fromLocalFile(destination) : QUrl());
    return ok;
}

bool WorkspaceManager::importRepository(const QUrl &sourceFolder)
{
    const QString source = localPath(sourceFolder);
    QString error;
    if (source.isEmpty() || isInsidePath(source, m_repositoryPath)
        || isInsidePath(m_repositoryPath, source)
        || !validateRepositoryRoot(source, &error))
    {
        emit operationFinished(false,
            error.isEmpty() ? tr("Choose an exported workspace Git repository.") : error, {});
        return false;
    }

    Snapshot previous {m_appDisplayName, activeTabId(), m_tabs};
    if (!saveNow(QStringLiteral("workspace: backup before repository import"), &error))
    {
        emit operationFinished(false, error, repositoryUrl());
        return false;
    }

    const QString parent = QFileInfo(m_repositoryPath).absolutePath();
    const QString nonce = QStringLiteral("%1-%2")
        .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")),
            QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
    const QString staging = QDir(parent).filePath(QStringLiteral(".workspace-import-%1").arg(nonce));
    const QString backup = QDir(parent).filePath(QStringLiteral(".workspace-backup-%1").arg(nonce));
    qint64 copied = 0;
    Snapshot importedSnapshot;
    if (!copyTree(source, staging, &copied, &error)
        || !validateRepositoryRoot(staging, &error)
        || !loadSnapshotFromRoot(staging, &importedSnapshot, &error))
    {
        QString removeError;
        (void)removeTree(staging, &removeError);
        emit operationFinished(false, error, QUrl::fromLocalFile(source));
        return false;
    }

    QDir parentDir(parent);
    const QString currentName = QFileInfo(m_repositoryPath).fileName();
    if (!parentDir.rename(currentName, QFileInfo(backup).fileName()))
    {
        (void)removeTree(staging, nullptr);
        emit operationFinished(false, tr("Could not create a safety backup of the current repository."), {});
        return false;
    }
    if (!parentDir.rename(QFileInfo(staging).fileName(), currentName))
    {
        const bool restored = parentDir.rename(QFileInfo(backup).fileName(), currentName);
        if (restored)
            (void)removeTree(staging, nullptr);
        else
        {
            m_initializationBlocked = true;
            m_repositoryStatus = tr("Repository import was interrupted; recovery copies were preserved");
            emit repositoryStatusChanged();
        }
        emit operationFinished(false,
            restored ? tr("Could not activate the imported repository; the previous repository was restored.")
                     : tr("Could not activate or restore the repository automatically. Recovery copies were preserved in %1.")
                           .arg(QDir::toNativeSeparators(parent)),
            restored ? repositoryUrl() : QUrl::fromLocalFile(parent));
        return false;
    }

    applySnapshot(std::move(importedSnapshot));
    QString activationError;
    if (!saveNow(QStringLiteral("workspace: activate imported repository"), &activationError))
    {
        const QString failedName = QStringLiteral(".workspace-failed-import-%1").arg(nonce);
        const bool movedFailedImport = parentDir.rename(currentName, failedName);
        const bool restored = movedFailedImport
            && parentDir.rename(QFileInfo(backup).fileName(), currentName);
        if (restored)
        {
            applySnapshot(std::move(previous));
            updateRepositoryStatus();
            setDirty(false);
        }
        else
        {
            if (movedFailedImport)
                parentDir.rename(failedName, currentName);
            m_initializationBlocked = true;
            setDirty(true);
            m_repositoryStatus = tr("Imported repository needs manual recovery; all copies were preserved");
            emit repositoryStatusChanged();
        }
        emit operationFinished(false,
            restored ? tr("Imported repository validation failed and the previous repository was restored: %1")
                           .arg(activationError)
                     : tr("Imported repository validation failed and automatic restoration needs attention: %1")
                           .arg(activationError),
            QUrl::fromLocalFile(parent));
        return false;
    }

    m_recoveryPath = backup;
    emit operationFinished(true,
        tr("Git repository and complete tab history imported. The previous repository is kept at %1.")
            .arg(QDir::toNativeSeparators(backup)),
        repositoryUrl());
    return true;
}

QString WorkspaceManager::localPath(const QUrl &url)
{
    if (url.isLocalFile())
        return QDir::cleanPath(url.toLocalFile());
    const QString text = url.toString();
    if (url.scheme().isEmpty() && QFileInfo(text).isAbsolute())
        return QDir::cleanPath(text);
    return {};
}

QString WorkspaceManager::normalizedName(const QString &value, const int maximum,
    const QString &fallback)
{
    QString result = value.trimmed();
    result.remove(QChar::Null);
    if (result.isEmpty())
        result = fallback;
    return result.left(maximum);
}

QString WorkspaceManager::normalizedColor(const QString &value)
{
    const QColor color(value);
    return color.isValid() ? color.name(QColor::HexArgb).toUpper()
                           : QStringLiteral("#FF6750A4");
}

QString WorkspaceManager::newTabId()
{
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

QString WorkspaceManager::fileSafeName(const QString &value)
{
    QString result = value.toLower();
    result.replace(QRegularExpression(QStringLiteral("[^a-z0-9._-]+")), QStringLiteral("-"));
    result.remove(QRegularExpression(QStringLiteral("^-+|-+$")));
    return result.isEmpty() ? QStringLiteral("workspace") : result.left(64);
}

bool WorkspaceManager::isInsidePath(const QString &candidate, const QString &root)
{
    const auto resolvedPath = [](const QString &value)
    {
        QString probe = QDir::cleanPath(QFileInfo(value).absoluteFilePath());
        QStringList missingParts;
        QFileInfo info(probe);
        while (!info.exists())
        {
            const QString fileName = info.fileName();
            const QString parent = info.absolutePath();
            if (fileName.isEmpty() || parent == probe)
                break;
            missingParts.prepend(fileName);
            probe = parent;
            info.setFile(probe);
        }
        QString result = info.canonicalFilePath();
        if (result.isEmpty())
            result = info.absoluteFilePath();
        for (const QString &part : std::as_const(missingParts))
            result = QDir(result).filePath(part);
        return QDir::fromNativeSeparators(QDir::cleanPath(result));
    };

    const QString cleanCandidate = resolvedPath(candidate);
    const QString cleanRoot = resolvedPath(root);
#ifdef Q_OS_WIN
    constexpr auto pathCaseSensitivity = Qt::CaseInsensitive;
#else
    constexpr auto pathCaseSensitivity = Qt::CaseSensitive;
#endif
    if (cleanCandidate.compare(cleanRoot, pathCaseSensitivity) == 0)
        return true;
    return cleanCandidate.startsWith(cleanRoot + QStringLiteral("/"), pathCaseSensitivity);
}

bool WorkspaceManager::containsReparsePoint(const QString &path, const QString &root)
{
    if (!isInsidePath(path, root))
        return true;
    QString current = QDir::cleanPath(root);
    const QString relative = QDir(root).relativeFilePath(path);
    const QStringList parts = relative.split(QRegularExpression(QStringLiteral("[/\\\\]+")), Qt::SkipEmptyParts);
    for (const QString &part : parts)
    {
        current = QDir(current).filePath(part);
        const QFileInfo info(current);
        if (info.exists() && hasSymlinkIdentity(info))
            return true;
    }
    return false;
}

QVariantMap WorkspaceManager::tabMap(const Tab &tab) const
{
    return {
        {QStringLiteral("tabId"), tab.id},
        {QStringLiteral("name"), tab.name},
        {QStringLiteral("content"), tab.content},
        {QStringLiteral("fontFamily"), tab.fontFamily},
        {QStringLiteral("fontStyle"), tab.fontStyle},
        {QStringLiteral("fontPointSize"), tab.fontPointSize},
        {QStringLiteral("bold"), tab.bold},
        {QStringLiteral("italic"), tab.italic},
        {QStringLiteral("fontColor"), tab.fontColor},
        {QStringLiteral("createdAt"), isoDate(tab.createdAt)},
        {QStringLiteral("updatedAt"), isoDate(tab.updatedAt)}
    };
}

int WorkspaceManager::indexOfTab(const QString &tabId) const
{
    for (int i = 0; i < m_tabs.size(); ++i)
    {
        if (m_tabs.at(i).id == tabId)
            return i;
    }
    return -1;
}

QJsonObject WorkspaceManager::tabObject(const Tab &tab, const bool includeContent) const
{
    QJsonObject object {
        {QStringLiteral("id"), tab.id},
        {QStringLiteral("name"), tab.name},
        {QStringLiteral("fontFamily"), tab.fontFamily},
        {QStringLiteral("fontStyle"), tab.fontStyle},
        {QStringLiteral("fontPointSize"), tab.fontPointSize},
        {QStringLiteral("bold"), tab.bold},
        {QStringLiteral("italic"), tab.italic},
        {QStringLiteral("fontColor"), tab.fontColor},
        {QStringLiteral("createdAt"), isoDate(tab.createdAt)},
        {QStringLiteral("updatedAt"), isoDate(tab.updatedAt)}
    };
    if (includeContent)
        object.insert(QStringLiteral("content"), tab.content);
    return object;
}

QJsonObject WorkspaceManager::workspaceObject(const bool exportMetadata) const
{
    QJsonArray tabs;
    for (const Tab &tab : m_tabs)
        tabs.append(tabObject(tab, exportMetadata));
    QJsonObject object {
        {QStringLiteral("type"), QStringLiteral("qbt-material-workspace")},
        {QStringLiteral("schemaVersion"), SchemaVersion},
        {QStringLiteral("appDisplayName"), m_appDisplayName},
        {QStringLiteral("activeTabId"), activeTabId()},
        {QStringLiteral("tabs"), tabs}
    };
    if (exportMetadata)
        object.insert(QStringLiteral("exportedAt"), isoDate(QDateTime::currentDateTimeUtc()));
    return object;
}

bool WorkspaceManager::parseWorkspace(const QByteArray &bytes, Snapshot *snapshot,
    QString *error, const bool requireContent) const
{
    if (!snapshot || bytes.size() > MaximumWorkspaceBytes)
    {
        if (error) *error = tr("Workspace data exceeds the 32 MB limit.");
        return false;
    }
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(bytes, &parseError);
    if (!document.isObject())
    {
        if (error) *error = tr("Workspace JSON is invalid: %1").arg(parseError.errorString());
        return false;
    }
    const QJsonObject root = document.object();
    if (root.value(QStringLiteral("type")).toString() != QStringLiteral("qbt-material-workspace")
        || root.value(QStringLiteral("schemaVersion")).toInt(-1) != SchemaVersion)
    {
        if (error) *error = tr("This is not a supported qBittorrent Material workspace.");
        return false;
    }
    if (!root.contains(QStringLiteral("tabs"))
        || !root.value(QStringLiteral("tabs")).isArray())
    {
        if (error) *error = tr("Workspace JSON is missing its tab list.");
        return false;
    }
    const QJsonArray entries = root.value(QStringLiteral("tabs")).toArray();
    if (entries.size() > MaximumTabs)
    {
        if (error) *error = tr("Workspace contains too many tabs.");
        return false;
    }

    Snapshot candidate;
    candidate.appDisplayName = normalizedName(root.value(QStringLiteral("appDisplayName")).toString(),
        80, QString::fromLatin1(ProductDisplayName));
    candidate.activeTabId = root.value(QStringLiteral("activeTabId")).toString();
    QSet<QString> ids;
    for (const QJsonValue &value : entries)
    {
        if (!value.isObject())
        {
            if (error) *error = tr("Workspace contains a malformed tab record.");
            return false;
        }
        const QJsonObject object = value.toObject();
        const QUuid parsedId(object.value(QStringLiteral("id")).toString());
        if (parsedId.isNull())
        {
            if (error) *error = tr("Workspace contains an invalid tab identifier.");
            return false;
        }
        Tab tab;
        tab.id = parsedId.toString(QUuid::WithoutBraces);
        if (ids.contains(tab.id))
        {
            if (error) *error = tr("Workspace contains duplicate tab identifiers.");
            return false;
        }
        ids.insert(tab.id);
        tab.name = normalizedName(object.value(QStringLiteral("name")).toString(), 120, tr("Untitled tab"));
        if (requireContent && (!object.contains(QStringLiteral("content"))
            || !object.value(QStringLiteral("content")).isString()))
        {
            if (error) *error = tr("This file contains repository metadata, not a portable workspace snapshot.");
            return false;
        }
        tab.content = object.value(QStringLiteral("content")).toString();
        if (tab.content.size() > MaximumContentCharacters)
        {
            if (error) *error = tr("A tab exceeds the 4 MB content limit.");
            return false;
        }
        tab.fontFamily = normalizedName(object.value(QStringLiteral("fontFamily")).toString(), 128,
            QFontDatabase::systemFont(QFontDatabase::GeneralFont).family());
        tab.fontStyle = normalizedName(object.value(QStringLiteral("fontStyle")).toString(), 64,
            QStringLiteral("Regular"));
        tab.fontPointSize = object.value(QStringLiteral("fontPointSize")).toDouble(16.0);
        if (tab.fontPointSize < 6.0 || tab.fontPointSize > 144.0)
        {
            if (error) *error = tr("A tab has an invalid font size.");
            return false;
        }
        const QColor color(object.value(QStringLiteral("fontColor")).toString());
        if (!color.isValid())
        {
            if (error) *error = tr("A tab has an invalid font color.");
            return false;
        }
        tab.fontColor = color.name(QColor::HexArgb).toUpper();
        tab.bold = object.value(QStringLiteral("bold")).toBool();
        tab.italic = object.value(QStringLiteral("italic")).toBool();
        tab.createdAt = QDateTime::fromString(object.value(QStringLiteral("createdAt")).toString(), Qt::ISODate);
        tab.updatedAt = QDateTime::fromString(object.value(QStringLiteral("updatedAt")).toString(), Qt::ISODate);
        if (!tab.createdAt.isValid()) tab.createdAt = QDateTime::currentDateTimeUtc();
        if (!tab.updatedAt.isValid()) tab.updatedAt = tab.createdAt;
        candidate.tabs.push_back(std::move(tab));
    }
    if (!candidate.tabs.isEmpty() && !ids.contains(candidate.activeTabId))
        candidate.activeTabId = candidate.tabs.constFirst().id;
    if (candidate.tabs.isEmpty())
        candidate.activeTabId.clear();
    *snapshot = std::move(candidate);
    return true;
}

bool WorkspaceManager::loadSnapshotFromRoot(const QString &root, Snapshot *snapshot,
    QString *error) const
{
    const QString workspacePath = QDir(root).filePath(QStringLiteral("workspace.json"));
    const QFileInfo workspaceInfo(workspacePath);
    if (!workspaceInfo.isFile() || workspaceInfo.size() > MaximumWorkspaceBytes
        || containsReparsePoint(workspacePath, root))
    {
        if (error) *error = tr("Repository workspace.json is missing or unsafe.");
        return false;
    }
    QFile workspaceFile(workspacePath);
    if (!workspaceFile.open(QIODevice::ReadOnly))
    {
        if (error) *error = workspaceFile.errorString();
        return false;
    }
    if (!parseWorkspace(workspaceFile.readAll(), snapshot, error))
        return false;

    for (Tab &tab : snapshot->tabs)
    {
        const QString contentPath = QDir(root).filePath(QStringLiteral("tabs/%1.md").arg(tab.id));
        const QFileInfo contentInfo(contentPath);
        if (!contentInfo.isFile() || contentInfo.size() > MaximumContentCharacters * 4LL
            || containsReparsePoint(contentPath, root))
        {
            if (error) *error = tr("Repository tab content is missing or unsafe.");
            return false;
        }
        QFile contentFile(contentPath);
        if (!contentFile.open(QIODevice::ReadOnly))
        {
            if (error) *error = contentFile.errorString();
            return false;
        }
        tab.content = QString::fromUtf8(contentFile.readAll());
        if (tab.content.size() > MaximumContentCharacters)
        {
            if (error) *error = tr("Repository tab content exceeds the 4 MB limit.");
            return false;
        }
    }
    return true;
}

void WorkspaceManager::applySnapshot(Snapshot snapshot)
{
    m_loading = true;
    beginResetModel();
    m_tabs = std::move(snapshot.tabs);
    m_appDisplayName = normalizedName(snapshot.appDisplayName, 80,
        QString::fromLatin1(ProductDisplayName));
    m_activeIndex = indexOfTab(snapshot.activeTabId);
    if (m_activeIndex < 0 && !m_tabs.isEmpty())
        m_activeIndex = 0;
    endResetModel();
    QGuiApplication::setApplicationDisplayName(m_appDisplayName);
    emit countChanged();
    emit activeIndexChanged();
    emit appDisplayNameChanged();
    m_loading = false;
}

void WorkspaceManager::loadWorkspace()
{
    const QFileInfo configuredRoot(m_repositoryPath);
    const QString parentPath = configuredRoot.absolutePath();
    QDir parentDirectory(parentPath);

    // If the process stopped after moving the current repository aside during
    // an import, restore the newest verified backup before reading any state.
    if (!configuredRoot.exists() && parentDirectory.exists())
    {
        const QStringList backups = parentDirectory.entryList(
            {QStringLiteral(".workspace-backup-*")}, QDir::Dirs | QDir::Hidden, QDir::Time);
        for (const QString &backupName : backups)
        {
            QString recoveryError;
            const QString backupPath = parentDirectory.filePath(backupName);
            if (validateRepositoryRoot(backupPath, &recoveryError)
                && parentDirectory.rename(backupName, configuredRoot.fileName()))
            {
                qCWarning(lcUi) << "Restored workspace after interrupted repository import from"
                                << backupPath;
                break;
            }
        }
    }

    Snapshot snapshot;
    QString error;
    if (QFileInfo::exists(QDir(m_repositoryPath).filePath(QStringLiteral("workspace.json")))
        && loadSnapshotFromRoot(m_repositoryPath, &snapshot, &error))
    {
        applySnapshot(std::move(snapshot));
        return;
    }
    if (!error.isEmpty())
        qCWarning(lcUi) << "Ignoring invalid workspace state:" << error;

    // Existing but invalid data must never be overwritten by the Welcome tab.
    // Move the complete directory aside first so every file and Git object is
    // recoverable. If that cannot be done, keep the invalid data untouched and
    // expose an in-memory Welcome page only.
    const QFileInfo currentRoot(m_repositoryPath);
    const bool hasExistingData = currentRoot.isFile()
        || (currentRoot.isDir() && !QDir(m_repositoryPath).entryList(
            QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System).isEmpty());
    if (hasExistingData)
    {
        const QString recoveryName = QStringLiteral(".workspace-recovery-%1-%2")
            .arg(QDateTime::currentDateTimeUtc().toString(QStringLiteral("yyyyMMdd-HHmmss-zzz")),
                QUuid::createUuid().toString(QUuid::WithoutBraces).left(8));
        if (parentDirectory.rename(currentRoot.fileName(), recoveryName))
        {
            m_recoveryPath = parentDirectory.filePath(recoveryName);
            qCWarning(lcUi) << "Preserved invalid workspace at" << m_recoveryPath;
        }
        else
        {
            m_initializationBlocked = true;
            m_repositoryStatus = tr("Existing workspace needs recovery; its files were left untouched");
            qCWarning(lcUi) << "Could not preserve invalid workspace; automatic initialization blocked";
        }
    }

    QSettings settings;
    snapshot.appDisplayName = normalizedName(
        settings.value(QStringLiteral("Workspace/AppDisplayName"), QString::fromLatin1(ProductDisplayName)).toString(),
        80, QString::fromLatin1(ProductDisplayName));
    Tab welcome;
    welcome.id = newTabId();
    welcome.name = tr("Welcome");
    welcome.content = tr("Your persistent workspace is ready.\n\n"
        "Create tabs like a browser, write on each page, and right-click a tab to customize its name, font, style, and color.");
    welcome.fontFamily = QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
    const QStringList styles = QFontDatabase::styles(welcome.fontFamily);
    welcome.fontStyle = styles.contains(QStringLiteral("Regular"))
        ? QStringLiteral("Regular") : styles.value(0, QStringLiteral("Regular"));
    welcome.createdAt = welcome.updatedAt = QDateTime::currentDateTimeUtc();
    snapshot.tabs.push_back(welcome);
    snapshot.activeTabId = welcome.id;
    applySnapshot(std::move(snapshot));
}

void WorkspaceManager::scheduleSave(const QString &commitMessage)
{
    if (m_loading)
        return;
    m_pendingCommitMessage = commitMessage;
    setDirty(true);
    m_repositoryStatus = tr("Changes pending…");
    emit repositoryStatusChanged();
    m_saveTimer.start();
}

bool WorkspaceManager::saveNow(const QString &commitMessage, QString *error)
{
    m_saveTimer.stop();
    if (m_initializationBlocked)
    {
        if (error) *error = tr("The existing workspace could not be recovered automatically; its files remain untouched.");
        return false;
    }
    if (!writeManagedFiles(error))
        return false;
    setDirty(true);
    if (!ensureRepository(error))
    {
        m_repositoryStatus = tr("Files saved; local Git unavailable");
        emit repositoryStatusChanged();
        return false;
    }
    if (!commitRepository(commitMessage, error))
    {
        m_repositoryStatus = tr("Files saved; Git commit failed");
        emit repositoryStatusChanged();
        return false;
    }
    setDirty(false);
    updateRepositoryStatus();

    QSettings settings;
    settings.setValue(QStringLiteral("Workspace/AppDisplayName"), m_appDisplayName);
    settings.setValue(QStringLiteral("Workspace/ActiveTabId"), activeTabId());
    settings.sync();
    return true;
}

bool WorkspaceManager::writeManagedFiles(QString *error)
{
    const QFileInfo rootInfo(m_repositoryPath);
    if (rootInfo.exists() && (!rootInfo.isDir() || hasSymlinkIdentity(rootInfo)))
    {
        if (error) *error = tr("The managed workspace path is not a safe local directory.");
        return false;
    }
    QDir root(m_repositoryPath);
    if (!root.mkpath(QStringLiteral("tabs")))
    {
        if (error) *error = tr("Could not create the managed workspace folder.");
        return false;
    }
    const QFileInfo tabsInfo(root.filePath(QStringLiteral("tabs")));
    if (!tabsInfo.isDir() || hasSymlinkIdentity(tabsInfo))
    {
        if (error) *error = tr("The managed tabs path is not a safe local directory.");
        return false;
    }

    QSet<QString> expectedFiles;
    for (const Tab &tab : m_tabs)
    {
        const QString fileName = tab.id + QStringLiteral(".md");
        expectedFiles.insert(fileName);
        if (!writeFileAtomically(root.filePath(QStringLiteral("tabs/%1").arg(fileName)),
            tab.content.toUtf8(), error))
            return false;
    }
    if (!writeFileAtomically(root.filePath(QStringLiteral("workspace.json")),
        QJsonDocument(workspaceObject(false)).toJson(QJsonDocument::Indented), error))
        return false;

    const QByteArray readme = QStringLiteral(
        "# %1 workspace\n\n"
        "This is a complete local Git repository managed by qBittorrent Material.\n\n"
        "- `workspace.json` stores the app display name, ordered tabs, and typography.\n"
        "- `tabs/*.md` stores one plain-text page per browser-style tab.\n"
        "- `.git` stores the automatic local history.\n\n"
        "Use the app's Workspace menu to export/import JSON snapshots or the entire repository.\n")
        .arg(m_appDisplayName).toUtf8();
    if (!writeFileAtomically(root.filePath(QStringLiteral("README.md")), readme, error))
        return false;

    QDir tabsDirectory(root.filePath(QStringLiteral("tabs")));
    const QStringList existing = tabsDirectory.entryList({QStringLiteral("*.md")}, QDir::Files);
    for (const QString &fileName : existing)
    {
        if (!expectedFiles.contains(fileName))
        {
            const QString path = tabsDirectory.filePath(fileName);
            if (containsReparsePoint(path, m_repositoryPath) || !QFile::remove(path))
            {
                if (error) *error = tr("Could not remove a closed tab from the repository.");
                return false;
            }
        }
    }
    return true;
}

bool WorkspaceManager::ensureRepository(QString *error)
{
    git_repository *repository = nullptr;
    const QByteArray path = QDir::fromNativeSeparators(m_repositoryPath).toUtf8();
    const QFileInfo gitDirectory(QDir(m_repositoryPath).filePath(QStringLiteral(".git")));
    int result = git_repository_open_ext(&repository, path.constData(),
        GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr);
    if (result == 0 && repository)
    {
        git_repository_free(repository);
        return validateRepositoryRoot(m_repositoryPath, error, true);
    }
    if (result != 0)
    {
        if (gitDirectory.exists())
        {
            if (error) *error = tr("The existing local Git repository is invalid: %1")
                .arg(gitErrorText(tr("unknown libgit2 error")));
            git_repository_free(repository);
            return false;
        }
        git_repository_init_options options = GIT_REPOSITORY_INIT_OPTIONS_INIT;
        options.flags = GIT_REPOSITORY_INIT_MKPATH;
        options.initial_head = "main";
        result = git_repository_init_ext(&repository, path.constData(), &options);
    }
    if (result != 0 || !repository)
    {
        if (error) *error = tr("Could not initialize local Git: %1")
            .arg(gitErrorText(tr("unknown libgit2 error")));
        git_repository_free(repository);
        return false;
    }
    git_repository_free(repository);
    return true;
}

bool WorkspaceManager::commitRepository(const QString &message, QString *error)
{
    git_repository *repository = nullptr;
    git_index *indexHandle = nullptr;
    git_tree *tree = nullptr;
    git_commit *parentCommit = nullptr;
    git_tree *parentTree = nullptr;
    git_signature *signature = nullptr;
    const QByteArray path = QDir::fromNativeSeparators(m_repositoryPath).toUtf8();

    auto fail = [&](const QString &prefix)
    {
        if (error) *error = prefix + QStringLiteral(": ") + gitErrorText(tr("unknown libgit2 error"));
    };

    if (git_repository_open_ext(&repository, path.constData(),
            GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr) != 0
        || git_repository_index(&indexHandle, repository) != 0)
    {
        fail(tr("Could not open the workspace Git index"));
        git_index_free(indexHandle);
        git_repository_free(repository);
        return false;
    }

    char tabsPath[] = "tabs/*";
    char *pathStrings[] = {tabsPath};
    git_strarray pathspec {pathStrings, 1};
    bool staged = git_index_update_all(indexHandle, &pathspec, nullptr, nullptr) == 0
        && git_index_add_bypath(indexHandle, "workspace.json") == 0
        && git_index_add_bypath(indexHandle, "README.md") == 0;
    for (const Tab &tab : std::as_const(m_tabs))
    {
        const QByteArray tabPath = QStringLiteral("tabs/%1.md").arg(tab.id).toUtf8();
        if (git_index_add_bypath(indexHandle, tabPath.constData()) != 0)
        {
            staged = false;
            break;
        }
    }
    if (!staged || git_index_write(indexHandle) != 0)
    {
        fail(tr("Could not stage workspace files"));
        git_index_free(indexHandle);
        git_repository_free(repository);
        return false;
    }

    git_oid treeId;
    if (git_index_write_tree(&treeId, indexHandle) != 0
        || git_tree_lookup(&tree, repository, &treeId) != 0)
    {
        fail(tr("Could not create the workspace Git tree"));
        git_index_free(indexHandle);
        git_repository_free(repository);
        return false;
    }

    git_oid parentId;
    bool hasParent = false;
    if (git_reference_name_to_id(&parentId, repository, "HEAD") == 0
        && git_commit_lookup(&parentCommit, repository, &parentId) == 0)
    {
        hasParent = true;
        if (git_commit_tree(&parentTree, parentCommit) == 0
            && git_oid_equal(git_tree_id(parentTree), &treeId))
        {
            git_tree_free(parentTree);
            git_commit_free(parentCommit);
            git_tree_free(tree);
            git_index_free(indexHandle);
            git_repository_free(repository);
            return true;
        }
    }

    if (git_signature_now(&signature, "qBittorrent Material Workspace", "workspace@local.invalid") != 0)
    {
        fail(tr("Could not create the workspace Git signature"));
        git_tree_free(parentTree);
        git_commit_free(parentCommit);
        git_tree_free(tree);
        git_index_free(indexHandle);
        git_repository_free(repository);
        return false;
    }

    git_oid commitId;
    const QByteArray commitMessage = message.toUtf8();
    const git_commit *parents[] = {parentCommit};
    const int result = git_commit_create(&commitId, repository, "HEAD", signature, signature,
        "UTF-8", commitMessage.constData(), tree, hasParent ? 1 : 0, hasParent ? parents : nullptr);
    if (result != 0)
        fail(tr("Could not commit the workspace"));

    git_signature_free(signature);
    git_tree_free(parentTree);
    git_commit_free(parentCommit);
    git_tree_free(tree);
    git_index_free(indexHandle);
    git_repository_free(repository);
    return result == 0;
}

void WorkspaceManager::updateRepositoryStatus()
{
    git_repository *repository = nullptr;
    git_reference *head = nullptr;
    const QByteArray path = QDir::fromNativeSeparators(m_repositoryPath).toUtf8();
    if (git_repository_open_ext(&repository, path.constData(),
            GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr) == 0
        && git_repository_head(&head, repository) == 0)
    {
        if (const git_oid *target = git_reference_target(head))
        {
            char id[GIT_OID_HEXSZ + 1] {};
            git_oid_tostr(id, sizeof(id), target);
            m_lastCommitId = QString::fromLatin1(id).left(8);
            m_repositoryStatus = tr("Synced to local Git • %1").arg(m_lastCommitId);
        }
    }
    if (m_repositoryStatus.isEmpty())
        m_repositoryStatus = tr("Local Git repository ready");
    git_reference_free(head);
    git_repository_free(repository);
    emit repositoryStatusChanged();
}

void WorkspaceManager::setDirty(const bool dirty)
{
    if (m_dirty == dirty)
        return;
    m_dirty = dirty;
    emit dirtyChanged();
}

bool WorkspaceManager::writeFileAtomically(const QString &path, const QByteArray &bytes,
    QString *error)
{
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly))
    {
        if (error) *error = file.errorString();
        return false;
    }
    if (file.write(bytes) != bytes.size() || !file.commit())
    {
        if (error) *error = file.errorString();
        return false;
    }
    return true;
}

bool WorkspaceManager::copyTree(const QString &source, const QString &destination,
    qint64 *bytesCopied, QString *error)
{
    int entryCount = 0;
    std::function<bool(const QString &, const QString &, int)> copyEntry;
    copyEntry = [&](const QString &entrySource, const QString &entryDestination, const int depth)
    {
        if (depth > 64 || ++entryCount > 20000)
        {
            if (error) *error = QObject::tr("Repository has too many files or nested folders.");
            return false;
        }
        const QFileInfo sourceInfo(entrySource);
        if (!sourceInfo.exists() || hasSymlinkIdentity(sourceInfo))
        {
            if (error) *error = QObject::tr("Repository contains a symlink, junction, or missing path.");
            return false;
        }
        if (sourceInfo.isFile())
        {
            if (bytesCopied)
            {
                *bytesCopied += sourceInfo.size();
                if (*bytesCopied > MaximumRepositoryBytes)
                {
                    if (error) *error = QObject::tr("Repository exceeds the 256 MB safety limit.");
                    return false;
                }
            }
            if (!QDir().mkpath(QFileInfo(entryDestination).absolutePath())
                || !QFile::copy(entrySource, entryDestination))
            {
                if (error) *error = QObject::tr("Could not copy repository file: %1")
                    .arg(sourceInfo.fileName());
                return false;
            }
            return true;
        }
        if (!sourceInfo.isDir() || !QDir().mkpath(entryDestination))
        {
            if (error) *error = QObject::tr("Could not create repository export folder.");
            return false;
        }
        const QFileInfoList entries = QDir(entrySource).entryInfoList(
            QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System);
        for (const QFileInfo &entry : entries)
        {
            if (!copyEntry(entry.absoluteFilePath(),
                QDir(entryDestination).filePath(entry.fileName()), depth + 1))
                return false;
        }
        return true;
    };
    return copyEntry(source, destination, 0);
}

bool WorkspaceManager::removeTree(const QString &path, QString *error)
{
    if (!QFileInfo::exists(path))
        return true;
    QDir directory(path);
    if (!directory.removeRecursively())
    {
        if (error) *error = QObject::tr("Could not remove temporary workspace folder.");
        return false;
    }
    return true;
}

bool WorkspaceManager::validateRepositoryRoot(const QString &path, QString *error,
    const bool allowUnbornMain)
{
    const QFileInfo root(path);
    const QFileInfo gitDirectory(QDir(path).filePath(QStringLiteral(".git")));
    const QFileInfo workspaceFile(QDir(path).filePath(QStringLiteral("workspace.json")));
    const QFileInfo readmeFile(QDir(path).filePath(QStringLiteral("README.md")));
    const QFileInfo tabsDirectory(QDir(path).filePath(QStringLiteral("tabs")));
    if (!root.isDir() || hasSymlinkIdentity(root) || !gitDirectory.isDir()
        || hasSymlinkIdentity(gitDirectory)
        || !workspaceFile.isFile() || hasSymlinkIdentity(workspaceFile)
        || !readmeFile.isFile() || hasSymlinkIdentity(readmeFile)
        || !tabsDirectory.isDir() || hasSymlinkIdentity(tabsDirectory))
    {
        if (error) *error = QObject::tr("Selected folder is not a safe workspace Git repository.");
        return false;
    }

    const QSet<QString> allowedTopLevel {
        QStringLiteral(".git"), QStringLiteral("README.md"),
        QStringLiteral("tabs"), QStringLiteral("workspace.json")
    };
    const QFileInfoList topLevelEntries = QDir(path).entryInfoList(
        QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System);
    for (const QFileInfo &entry : topLevelEntries)
    {
        if (!allowedTopLevel.contains(entry.fileName()) || hasSymlinkIdentity(entry))
        {
            if (error) *error = QObject::tr("Repository contains an unexpected or redirected working-tree path.");
            return false;
        }
    }
    const QFileInfoList tabEntries = QDir(tabsDirectory.absoluteFilePath()).entryInfoList(
        QDir::NoDotAndDotDot | QDir::AllEntries | QDir::Hidden | QDir::System);
    for (const QFileInfo &entry : tabEntries)
    {
        const QUuid id(entry.completeBaseName());
        if (!entry.isFile() || hasSymlinkIdentity(entry)
            || entry.suffix().compare(QStringLiteral("md"), Qt::CaseInsensitive) != 0
            || id.isNull())
        {
            if (error) *error = QObject::tr("Repository tabs contain an unexpected or unsafe path.");
            return false;
        }
    }
    if (QFileInfo::exists(QDir(gitDirectory.absoluteFilePath()).filePath(QStringLiteral("commondir")))
        || QFileInfo::exists(QDir(gitDirectory.absoluteFilePath()).filePath(
            QStringLiteral("objects/info/alternates"))))
    {
        if (error) *error = QObject::tr("Linked worktrees and external Git object stores are not supported.");
        return false;
    }

    git_repository *repository = nullptr;
    git_reference *head = nullptr;
    git_commit *headCommit = nullptr;
    const QByteArray encoded = QDir::fromNativeSeparators(path).toUtf8();
    bool valid = git_repository_open_ext(&repository, encoded.constData(),
            GIT_REPOSITORY_OPEN_NO_SEARCH, nullptr) == 0
        && !git_repository_is_bare(repository);
    bool validHead = false;
    if (valid && !git_repository_head_detached(repository)
        && git_repository_head(&head, repository) == 0
        && QString::fromUtf8(git_reference_name(head)) == QStringLiteral("refs/heads/main"))
    {
        const git_oid *target = git_reference_target(head);
        validHead = target && git_commit_lookup(&headCommit, repository, target) == 0;
    }
    else if (valid && allowUnbornMain && git_repository_head_unborn(repository) == 1)
    {
        git_reference *symbolicHead = nullptr;
        validHead = git_reference_lookup(&symbolicHead, repository, "HEAD") == 0
            && git_reference_type(symbolicHead) == GIT_REFERENCE_SYMBOLIC
            && QString::fromUtf8(git_reference_symbolic_target(symbolicHead))
                == QStringLiteral("refs/heads/main");
        git_reference_free(symbolicHead);
    }
    valid = valid && validHead;
    if (valid)
    {
        const QString expectedWorkdir = QFileInfo(path).canonicalFilePath();
        const QString actualWorkdir = QFileInfo(
            QString::fromUtf8(git_repository_workdir(repository))).canonicalFilePath();
        const QString expectedGitdir = gitDirectory.canonicalFilePath();
        const QString actualGitdir = QFileInfo(
            QString::fromUtf8(git_repository_path(repository))).canonicalFilePath();
#ifdef Q_OS_WIN
        constexpr auto pathCaseSensitivity = Qt::CaseInsensitive;
#else
        constexpr auto pathCaseSensitivity = Qt::CaseSensitive;
#endif
        valid = !expectedWorkdir.isEmpty() && !actualWorkdir.isEmpty()
            && !expectedGitdir.isEmpty() && !actualGitdir.isEmpty()
            && expectedWorkdir.compare(actualWorkdir, pathCaseSensitivity) == 0
            && expectedGitdir.compare(actualGitdir, pathCaseSensitivity) == 0;
    }
    if (!valid && error)
        *error = QObject::tr("Selected repository is not a self-contained checked-out main-branch workspace.");
    git_commit_free(headCommit);
    git_reference_free(head);
    git_repository_free(repository);
    return valid;
}
