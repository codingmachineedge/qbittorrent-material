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

#include <chrono>
#include <functional>

#include <QAbstractListModel>
#include <QHash>
#include <QList>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>

#include <qqmlintegration.h>

#include "base/bittorrent/session.h"
#include "base/bittorrent/torrentcontentremoveoption.h"
#include "base/logging.h"
#include "base/preferences.h"
#include "base/utils/fs/path.h"

/**
 * @file advancedsettingsmodel.h
 * @brief List model backing the Options → Advanced tab.
 *
 * The legacy Qt-Widgets Advanced tab is a flat 2-column `QTableWidget` of every
 * low-level qBittorrent + libtorrent knob, split by two bold section-header rows.
 * This model reproduces that as a Material settings list: one row per knob, each
 * carrying enough metadata (control type, range, suffix, sentinel text, combo
 * items, documentation link) for a QML delegate to render the right editor, plus
 * two non-editable @c Section rows ("qBittorrent" and "libtorrent").
 *
 * Every editable row binds to the engine through a getter/setter pair captured at
 * construction. Editing is **staged**: `setValue()` writes an in-memory copy;
 * nothing reaches the engine until `apply()` is called from the dialog OK/Apply.
 * `reset()` reloads live values, discarding staged edits.
 */
class AdvancedSettingsModel final : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool modified READ isModified NOTIFY modifiedChanged)

public:
    /// The kind of editor a row needs.
    enum RowType
    {
        Section = 0,  ///< non-editable bold divider row
        CheckBox,     ///< bool
        SpinBox,      ///< int (with optional range/suffix/special value)
        ComboBox,     ///< enum-ish, `options` holds {text, value} pairs
        LineEdit      ///< free text
    };
    Q_ENUM(RowType)

    /// Named roles consumed by the QML Advanced-tab delegate.
    enum Roles
    {
        TypeRole = Qt::UserRole + 1,   ///< "type"      — RowType int
        KeyRole,                       ///< "key"       — stable identifier
        LabelRole,                     ///< "label"     — visible setting name
        ValueRole,                     ///< "value"     — staged value
        MinRole,                       ///< "min"       — spin minimum
        MaxRole,                       ///< "max"       — spin maximum
        SuffixRole,                    ///< "suffix"    — spin unit suffix (e.g. " MiB")
        SpecialValueRole,              ///< "specialValue" — sentinel text (e.g. "0 (disabled)")
        OptionsRole,                   ///< "options"   — combo items [{text, value}]
        PlaceholderRole,               ///< "placeholder" — line-edit hint
        DocLinkRole,                   ///< "docLink"   — (?) documentation URL, may be empty
        IsSectionRole                  ///< "isSection" — convenience bool
    };
    Q_ENUM(Roles)

    explicit AdvancedSettingsModel(QObject *parent = nullptr)
        : QAbstractListModel(parent)
    {
        qCDebug(lcModel) << "AdvancedSettingsModel: constructing";
        buildRows();
        loadFromEngine();
    }

    // --- QAbstractListModel ------------------------------------------------

    int rowCount(const QModelIndex &parent = {}) const override
    {
        return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid() || (index.row() < 0) || (index.row() >= static_cast<int>(m_rows.size())))
            return {};

        const Row &row = m_rows.at(index.row());
        switch (role)
        {
        case Qt::DisplayRole:
        case LabelRole:
            return row.label;
        case TypeRole:
            return static_cast<int>(row.type);
        case KeyRole:
            return row.key;
        case ValueRole:
            return row.staged;
        case MinRole:
            return row.min;
        case MaxRole:
            return row.max;
        case SuffixRole:
            return row.suffix;
        case SpecialValueRole:
            return row.specialValue;
        case OptionsRole:
            return row.options;
        case PlaceholderRole:
            return row.placeholder;
        case DocLinkRole:
            return row.docLink;
        case IsSectionRole:
            return (row.type == Section);
        default:
            return {};
        }
    }

    QHash<int, QByteArray> roleNames() const override
    {
        return {
            {TypeRole, "type"},
            {KeyRole, "key"},
            {LabelRole, "label"},
            {ValueRole, "value"},
            {MinRole, "min"},
            {MaxRole, "max"},
            {SuffixRole, "suffix"},
            {SpecialValueRole, "specialValue"},
            {OptionsRole, "options"},
            {PlaceholderRole, "placeholder"},
            {DocLinkRole, "docLink"},
            {IsSectionRole, "isSection"}
        };
    }

    bool isModified() const { return m_modified; }

    // --- QML API -----------------------------------------------------------

    /// Stage a new value for the row at @p row (ignored for Section rows).
    Q_INVOKABLE void setValue(int row, const QVariant &value)
    {
        if ((row < 0) || (row >= static_cast<int>(m_rows.size())))
            return;
        Row &r = m_rows[row];
        if ((r.type == Section) || (r.staged == value))
            return;

        qCDebug(lcModel) << "AdvancedSettingsModel: staging" << r.key << "=" << value;
        r.staged = value;
        const QModelIndex idx = index(row, 0);
        emit dataChanged(idx, idx, {ValueRole});
        markModified();
    }

    /// The currently-staged value for @p row.
    Q_INVOKABLE QVariant value(int row) const
    {
        if ((row < 0) || (row >= static_cast<int>(m_rows.size())))
            return {};
        return m_rows.at(row).staged;
    }

    /// Commit all staged values to the engine.
    Q_INVOKABLE void apply()
    {
        if (!m_modified)
        {
            qCDebug(lcModel) << "AdvancedSettingsModel: apply() with no changes";
            return;
        }

        qCInfo(lcModel) << "AdvancedSettingsModel: applying advanced settings";
        for (const Row &row : m_rows)
        {
            if ((row.type == Section) || !row.setter)
                continue;
            row.setter(row.staged);
        }

        m_modified = false;
        emit modifiedChanged();
    }

    /// Reload staged values from the engine, discarding edits.
    Q_INVOKABLE void reset()
    {
        qCDebug(lcModel) << "AdvancedSettingsModel: reset() — reloading from engine";
        loadFromEngine();
    }

signals:
    void modifiedChanged();

private:
    using Getter = std::function<QVariant()>;
    using Setter = std::function<void(const QVariant &)>;

    struct Row
    {
        RowType type = CheckBox;
        QString key;
        QString label;
        QVariant staged;
        QVariant min;
        QVariant max;
        QString suffix;
        QString specialValue;
        QVariantList options;
        QString placeholder;
        QString docLink;
        Getter getter;
        Setter setter;
    };

    void markModified()
    {
        if (!m_modified)
        {
            m_modified = true;
            emit modifiedChanged();
        }
    }

    void loadFromEngine()
    {
        beginResetModel();
        for (Row &row : m_rows)
        {
            if (row.getter)
                row.staged = row.getter();
        }
        endResetModel();

        if (m_modified)
        {
            m_modified = false;
            emit modifiedChanged();
        }
    }

    /// Build a combo `options` list from (text, value) pairs.
    static QVariantList combo(std::initializer_list<std::pair<QString, int>> items)
    {
        QVariantList list;
        for (const auto &[text, value] : items)
            list.append(QVariantMap{{"text", text}, {"value", value}});
        return list;
    }

    void addSection(const QString &title, const QString &docLink)
    {
        Row row;
        row.type = Section;
        row.label = title;
        row.docLink = docLink;
        m_rows.append(std::move(row));
    }

    void addCheck(const QString &key, const QString &label, Getter g, Setter s)
    {
        Row row;
        row.type = CheckBox;
        row.key = key;
        row.label = label;
        row.getter = std::move(g);
        row.setter = std::move(s);
        m_rows.append(std::move(row));
    }

    void addSpin(const QString &key, const QString &label, int min, int max
            , const QString &suffix, const QString &specialValue, Getter g, Setter s)
    {
        Row row;
        row.type = SpinBox;
        row.key = key;
        row.label = label;
        row.min = min;
        row.max = max;
        row.suffix = suffix;
        row.specialValue = specialValue;
        row.getter = std::move(g);
        row.setter = std::move(s);
        m_rows.append(std::move(row));
    }

    void addCombo(const QString &key, const QString &label, QVariantList options, Getter g, Setter s)
    {
        Row row;
        row.type = ComboBox;
        row.key = key;
        row.label = label;
        row.options = std::move(options);
        row.getter = std::move(g);
        row.setter = std::move(s);
        m_rows.append(std::move(row));
    }

    void addLine(const QString &key, const QString &label, const QString &placeholder, Getter g, Setter s)
    {
        Row row;
        row.type = LineEdit;
        row.key = key;
        row.label = label;
        row.placeholder = placeholder;
        row.getter = std::move(g);
        row.setter = std::move(s);
        m_rows.append(std::move(row));
    }

    void buildRows()
    {
        using BitTorrent::Session;
        using namespace BitTorrent;
        auto *session = Session::instance();
        auto *pref = Preferences::instance();

        constexpr int kIntMax = 2147483647;

        // ==================== qBittorrent Section ====================
        addSection(tr("qBittorrent Section")
                , QStringLiteral("https://github.com/qbittorrent/qBittorrent/wiki/Explanation-of-Options-in-qBittorrent"));

        addCombo(QStringLiteral("resumeDataStorageType"), tr("Resume data storage type (requires restart)")
                , combo({{tr("Fastresume files"), static_cast<int>(ResumeDataStorageType::Legacy)}
                        , {tr("SQLite database (experimental)"), static_cast<int>(ResumeDataStorageType::SQLite)}})
                , [session] { return static_cast<int>(session->resumeDataStorageType()); }
                , [session](const QVariant &v) { session->setResumeDataStorageType(static_cast<ResumeDataStorageType>(v.toInt())); });

        addCombo(QStringLiteral("torrentContentRemoveOption"), tr("Torrent content removing mode")
                , combo({{tr("Delete files permanently"), static_cast<int>(TorrentContentRemoveOption::Delete)}
                        , {tr("Move files to trash (if possible)"), static_cast<int>(TorrentContentRemoveOption::MoveToTrash)}})
                , [session] { return static_cast<int>(session->torrentContentRemoveOption()); }
                , [session](const QVariant &v) { session->setTorrentContentRemoveOption(static_cast<TorrentContentRemoveOption>(v.toInt())); });

        addSpin(QStringLiteral("saveResumeDataInterval"), tr("Save resume data interval [0: disabled]")
                , 0, kIntMax, tr(" min"), tr("0 (disabled)")
                , [session] { return session->saveResumeDataInterval(); }
                , [session](const QVariant &v) { session->setSaveResumeDataInterval(v.toInt()); });

        addSpin(QStringLiteral("saveStatisticsInterval"), tr("Save statistics interval [0: disabled]")
                , 0, kIntMax, tr(" min"), tr("0 (disabled)")
                , [session] { return static_cast<int>(session->saveStatisticsInterval().count()); }
                , [session](const QVariant &v) { session->setSaveStatisticsInterval(std::chrono::minutes(v.toInt())); });

        addSpin(QStringLiteral("torrentFileSizeLimit"), tr(".torrent file size limit")
                , 1, (kIntMax / (1024 * 1024)), tr(" MiB"), {}
                , [pref] { return static_cast<int>(pref->getTorrentFileSizeLimit() / (1024 * 1024)); }
                , [pref](const QVariant &v) { pref->setTorrentFileSizeLimit(static_cast<qint64>(v.toInt()) * 1024 * 1024); });

        addCheck(QStringLiteral("confirmTorrentRecheck"), tr("Confirm torrent recheck")
                , [pref] { return pref->confirmTorrentRecheck(); }
                , [pref](const QVariant &v) { pref->setConfirmTorrentRecheck(v.toBool()); });

        addCheck(QStringLiteral("recheckCompleted"), tr("Recheck torrents on completion")
                , [pref] { return pref->recheckTorrentsOnCompletion(); }
                , [pref](const QVariant &v) { pref->recheckTorrentsOnCompletion(v.toBool()); });

        addSpin(QStringLiteral("refreshInterval"), tr("Refresh interval")
                , 30, 99999, tr(" ms"), {}
                , [session] { return session->refreshInterval(); }
                , [session](const QVariant &v) { session->setRefreshInterval(v.toInt()); });

        addCheck(QStringLiteral("resolveCountries"), tr("Resolve peer countries")
                , [pref] { return pref->resolvePeerCountries(); }
                , [pref](const QVariant &v) { pref->resolvePeerCountries(v.toBool()); });

        addCheck(QStringLiteral("resolveHosts"), tr("Resolve peer host names")
                , [pref] { return pref->resolvePeerHostNames(); }
                , [pref](const QVariant &v) { pref->resolvePeerHostNames(v.toBool()); });

        addCheck(QStringLiteral("confirmRemoveAllTags"), tr("Confirm removal of all tags")
                , [pref] { return pref->confirmRemoveAllTags(); }
                , [pref](const QVariant &v) { pref->setConfirmRemoveAllTags(v.toBool()); });

        addCheck(QStringLiteral("confirmRemoveTrackerFromAllTorrents"), tr("Confirm removal of tracker from all torrents")
                , [pref] { return pref->confirmRemoveTrackerFromAllTorrents(); }
                , [pref](const QVariant &v) { pref->setConfirmRemoveTrackerFromAllTorrents(v.toBool()); });

        addCheck(QStringLiteral("reannounceWhenAddressChanged"), tr("Reannounce to all trackers when IP or port changed")
                , [session] { return session->isReannounceWhenAddressChangedEnabled(); }
                , [session](const QVariant &v) { session->setReannounceWhenAddressChangedEnabled(v.toBool()); });

        addSpin(QStringLiteral("savePathHistoryLength"), tr("Save path history length")
                , 0, 99, {}, {}
                , [pref] { return pref->addNewTorrentDialogSavePathHistoryLength(); }
                , [pref](const QVariant &v) { pref->setAddNewTorrentDialogSavePathHistoryLength(v.toInt()); });

        addCheck(QStringLiteral("speedWidgetEnabled"), tr("Enable speed graphs")
                , [pref] { return pref->isSpeedWidgetEnabled(); }
                , [pref](const QVariant &v) { pref->setSpeedWidgetEnabled(v.toBool()); });

#ifndef Q_OS_MACOS
        addCheck(QStringLiteral("iconsInMenus"), tr("Enable icons in menus")
                , [pref] { return pref->iconsInMenusEnabled(); }
                , [pref](const QVariant &v) { pref->setIconsInMenusEnabled(v.toBool()); });
#endif

        addCheck(QStringLiteral("attachedAddNewTorrentDialog"), tr("Attach \"Add new torrent\" dialog to main window")
                , [pref] { return pref->isAddNewTorrentDialogAttached(); }
                , [pref](const QVariant &v) { pref->setAddNewTorrentDialogAttached(v.toBool()); });

        addCheck(QStringLiteral("trackerEnabled"), tr("Enable embedded tracker")
                , [session] { return session->isTrackerEnabled(); }
                , [session](const QVariant &v) { session->setTrackerEnabled(v.toBool()); });

        addSpin(QStringLiteral("trackerPort"), tr("Embedded tracker port")
                , 1, 65535, {}, {}
                , [pref] { return pref->getTrackerPort(); }
                , [pref](const QVariant &v) { pref->setTrackerPort(v.toInt()); });

        addCheck(QStringLiteral("trackerPortForwarding"), tr("Enable port forwarding for embedded tracker")
                , [pref] { return pref->isTrackerPortForwardingEnabled(); }
                , [pref](const QVariant &v) { pref->setTrackerPortForwardingEnabled(v.toBool()); });

        addCheck(QStringLiteral("markOfTheWeb"), tr("Enable Mark-of-the-Web (MOTW) for downloaded files")
                , [pref] { return pref->isMarkOfTheWebEnabled(); }
                , [pref](const QVariant &v) { pref->setMarkOfTheWebEnabled(v.toBool()); });

        addCheck(QStringLiteral("ignoreSSLErrors"), tr("Ignore SSL errors")
                , [pref] { return pref->isIgnoreSSLErrors(); }
                , [pref](const QVariant &v) { pref->setIgnoreSSLErrors(v.toBool()); });

        addLine(QStringLiteral("pythonExecutablePath"), tr("Python executable path (may require restart)")
                , tr("(Auto detect if empty)")
                , [pref] { return pref->getPythonExecutablePath().toString(); }
                , [pref](const QVariant &v) { pref->setPythonExecutablePath(Path(v.toString())); });

        addCheck(QStringLiteral("startSessionPaused"), tr("Start BitTorrent session in paused state")
                , [session] { return session->isStartPaused(); }
                , [session](const QVariant &v) { session->setStartPaused(v.toBool()); });

        addSpin(QStringLiteral("sessionShutdownTimeout"), tr("BitTorrent session shutdown timeout [-1: unlimited]")
                , -1, kIntMax, tr(" sec"), tr("-1 (unlimited)")
                , [session] { return session->shutdownTimeout(); }
                , [session](const QVariant &v) { session->setShutdownTimeout(v.toInt()); });

        // ==================== libtorrent Section ====================
        addSection(tr("libtorrent Section")
                , QStringLiteral("https://libtorrent.org/reference-Settings.html"));

        addSpin(QStringLiteral("bdecodeDepthLimit"), tr("Bdecode depth limit")
                , 0, kIntMax, {}, {}
                , [pref] { return pref->getBdecodeDepthLimit(); }
                , [pref](const QVariant &v) { pref->setBdecodeDepthLimit(v.toInt()); });

        addSpin(QStringLiteral("bdecodeTokenLimit"), tr("Bdecode token limit")
                , 0, kIntMax, {}, {}
                , [pref] { return pref->getBdecodeTokenLimit(); }
                , [pref](const QVariant &v) { pref->setBdecodeTokenLimit(v.toInt()); });

        addSpin(QStringLiteral("asyncIOThreads"), tr("Asynchronous I/O threads")
                , 1, 1024, {}, {}
                , [session] { return session->asyncIOThreads(); }
                , [session](const QVariant &v) { session->setAsyncIOThreads(v.toInt()); });

#ifdef QBT_USES_LIBTORRENT2
        addSpin(QStringLiteral("hashingThreads"), tr("Hashing threads")
                , 1, 1024, {}, {}
                , [session] { return session->hashingThreads(); }
                , [session](const QVariant &v) { session->setHashingThreads(v.toInt()); });
#endif

        addSpin(QStringLiteral("filePoolSize"), tr("File pool size")
                , 1, kIntMax, {}, {}
                , [session] { return session->filePoolSize(); }
                , [session](const QVariant &v) { session->setFilePoolSize(v.toInt()); });

        addSpin(QStringLiteral("checkingMemUsage"), tr("Outstanding memory when checking torrents")
                , 1, 1024, tr(" MiB"), {}
                , [session] { return session->checkingMemUsage(); }
                , [session](const QVariant &v) { session->setCheckingMemUsage(v.toInt()); });

#ifndef QBT_USES_LIBTORRENT2
        addSpin(QStringLiteral("diskCacheSize"), tr("Disk cache")
                , -1, 33554431, tr(" MiB"), tr("-1 (auto)")
                , [session] { return session->diskCacheSize(); }
                , [session](const QVariant &v) { session->setDiskCacheSize(v.toInt()); });

        addSpin(QStringLiteral("diskCacheTTL"), tr("Disk cache expiry interval")
                , 1, kIntMax, tr(" s"), {}
                , [session] { return session->diskCacheTTL(); }
                , [session](const QVariant &v) { session->setDiskCacheTTL(v.toInt()); });
#endif

        addSpin(QStringLiteral("diskQueueSize"), tr("Disk queue size")
                , 1, kIntMax, tr(" KiB"), {}
                , [session] { return static_cast<int>(session->diskQueueSize() / 1024); }
                , [session](const QVariant &v) { session->setDiskQueueSize(static_cast<qint64>(v.toInt()) * 1024); });

#ifdef QBT_USES_LIBTORRENT2
        addCombo(QStringLiteral("diskIOType"), tr("Disk IO type (requires restart)")
                , combo({{tr("Default"), static_cast<int>(DiskIOType::Default)}
                        , {tr("Memory mapped files"), static_cast<int>(DiskIOType::MMap)}
                        , {tr("POSIX-compliant"), static_cast<int>(DiskIOType::Posix)}
                        , {tr("Simple pread/pwrite"), static_cast<int>(DiskIOType::SimplePreadPwrite)}})
                , [session] { return static_cast<int>(session->diskIOType()); }
                , [session](const QVariant &v) { session->setDiskIOType(static_cast<DiskIOType>(v.toInt())); });
#endif

        addCombo(QStringLiteral("diskIOReadMode"), tr("Disk IO read mode")
                , combo({{tr("Disable OS cache"), static_cast<int>(DiskIOReadMode::DisableOSCache)}
                        , {tr("Enable OS cache"), static_cast<int>(DiskIOReadMode::EnableOSCache)}})
                , [session] { return static_cast<int>(session->diskIOReadMode()); }
                , [session](const QVariant &v) { session->setDiskIOReadMode(static_cast<DiskIOReadMode>(v.toInt())); });

        addCombo(QStringLiteral("diskIOWriteMode"), tr("Disk IO write mode")
                , combo({{tr("Disable OS cache"), static_cast<int>(DiskIOWriteMode::DisableOSCache)}
                        , {tr("Enable OS cache"), static_cast<int>(DiskIOWriteMode::EnableOSCache)}})
                , [session] { return static_cast<int>(session->diskIOWriteMode()); }
                , [session](const QVariant &v) { session->setDiskIOWriteMode(static_cast<DiskIOWriteMode>(v.toInt())); });

#ifndef QBT_USES_LIBTORRENT2
        addCheck(QStringLiteral("coalesceRW"), tr("Coalesce reads & writes")
                , [session] { return session->isCoalesceReadWriteEnabled(); }
                , [session](const QVariant &v) { session->setCoalesceReadWriteEnabled(v.toBool()); });
#endif

        addCheck(QStringLiteral("pieceExtentAffinity"), tr("Use piece extent affinity")
                , [session] { return session->usePieceExtentAffinity(); }
                , [session](const QVariant &v) { session->setPieceExtentAffinity(v.toBool()); });

        addCheck(QStringLiteral("suggestMode"), tr("Send upload piece suggestions")
                , [session] { return session->isSuggestModeEnabled(); }
                , [session](const QVariant &v) { session->setSuggestMode(v.toBool()); });

        addSpin(QStringLiteral("sendBufferWatermark"), tr("Send buffer watermark")
                , 1, kIntMax, tr(" KiB"), {}
                , [session] { return session->sendBufferWatermark(); }
                , [session](const QVariant &v) { session->setSendBufferWatermark(v.toInt()); });

        addSpin(QStringLiteral("sendBufferLowWatermark"), tr("Send buffer low watermark")
                , 1, kIntMax, tr(" KiB"), {}
                , [session] { return session->sendBufferLowWatermark(); }
                , [session](const QVariant &v) { session->setSendBufferLowWatermark(v.toInt()); });

        addSpin(QStringLiteral("sendBufferWatermarkFactor"), tr("Send buffer watermark factor")
                , 1, kIntMax, tr(" %"), {}
                , [session] { return session->sendBufferWatermarkFactor(); }
                , [session](const QVariant &v) { session->setSendBufferWatermarkFactor(v.toInt()); });

        addSpin(QStringLiteral("connectionSpeed"), tr("Outgoing connections per second")
                , 0, kIntMax, {}, {}
                , [session] { return session->connectionSpeed(); }
                , [session](const QVariant &v) { session->setConnectionSpeed(v.toInt()); });

        addCheck(QStringLiteral("seedingOutgoingConnections"), tr("Allow outgoing connections when seeding")
                , [session] { return session->isSeedingOutgoingConnectionsEnabled(); }
                , [session](const QVariant &v) { session->setSeedingOutgoingConnections(v.toBool()); });

        addSpin(QStringLiteral("socketSendBufferSize"), tr("Socket send buffer size [0: system default]")
                , 0, kIntMax, tr(" KiB"), tr("0 (system default)")
                , [session] { return session->socketSendBufferSize() / 1024; }
                , [session](const QVariant &v) { session->setSocketSendBufferSize(v.toInt() * 1024); });

        addSpin(QStringLiteral("socketReceiveBufferSize"), tr("Socket receive buffer size [0: system default]")
                , 0, kIntMax, tr(" KiB"), tr("0 (system default)")
                , [session] { return session->socketReceiveBufferSize() / 1024; }
                , [session](const QVariant &v) { session->setSocketReceiveBufferSize(v.toInt() * 1024); });

        addSpin(QStringLiteral("socketBacklogSize"), tr("Socket backlog size")
                , 1, kIntMax, {}, {}
                , [session] { return session->socketBacklogSize(); }
                , [session](const QVariant &v) { session->setSocketBacklogSize(v.toInt()); });

        addSpin(QStringLiteral("outgoingPortsMin"), tr("Outgoing ports (Min) [0: disabled]")
                , 0, 65535, {}, tr("0 (disabled)")
                , [session] { return session->outgoingPortsMin(); }
                , [session](const QVariant &v) { session->setOutgoingPortsMin(v.toInt()); });

        addSpin(QStringLiteral("outgoingPortsMax"), tr("Outgoing ports (Max) [0: disabled]")
                , 0, 65535, {}, tr("0 (disabled)")
                , [session] { return session->outgoingPortsMax(); }
                , [session](const QVariant &v) { session->setOutgoingPortsMax(v.toInt()); });

        addSpin(QStringLiteral("upnpLeaseDuration"), tr("UPnP lease duration [0: permanent lease]")
                , 0, kIntMax, tr(" s"), tr("0 (permanent lease)")
                , [session] { return session->UPnPLeaseDuration(); }
                , [session](const QVariant &v) { session->setUPnPLeaseDuration(v.toInt()); });

        addSpin(QStringLiteral("peerDSCP"), tr("Differentiated Services Code Point (DSCP) for connections to peers")
                , 0, 255, {}, {}
                , [session] { return session->peerDSCP(); }
                , [session](const QVariant &v) { session->setPeerDSCP(v.toInt()); });

        addCombo(QStringLiteral("utpMixedMode"), tr("µTP-TCP mixed mode algorithm")
                , combo({{tr("Prefer TCP"), static_cast<int>(MixedModeAlgorithm::TCP)}
                        , {tr("Peer proportional (throttles TCP)"), static_cast<int>(MixedModeAlgorithm::Proportional)}})
                , [session] { return static_cast<int>(session->utpMixedMode()); }
                , [session](const QVariant &v) { session->setUtpMixedMode(static_cast<MixedModeAlgorithm>(v.toInt())); });

        addSpin(QStringLiteral("hostnameCacheTTL"), tr("Internal hostname resolver cache expiry interval")
                , 0, kIntMax, tr(" s"), {}
                , [session] { return session->hostnameCacheTTL(); }
                , [session](const QVariant &v) { session->setHostnameCacheTTL(v.toInt()); });

        addCheck(QStringLiteral("idnSupport"), tr("Support internationalized domain name (IDN)")
                , [session] { return session->isIDNSupportEnabled(); }
                , [session](const QVariant &v) { session->setIDNSupportEnabled(v.toBool()); });

        addCheck(QStringLiteral("multiConnectionsPerIp"), tr("Allow multiple connections from the same IP address")
                , [session] { return session->multiConnectionsPerIpEnabled(); }
                , [session](const QVariant &v) { session->setMultiConnectionsPerIpEnabled(v.toBool()); });

        addCheck(QStringLiteral("validateHTTPSTrackerCertificate"), tr("Validate HTTPS tracker certificates")
                , [session] { return session->validateHTTPSTrackerCertificate(); }
                , [session](const QVariant &v) { session->setValidateHTTPSTrackerCertificate(v.toBool()); });

        addCheck(QStringLiteral("ssrfMitigation"), tr("Server-side request forgery (SSRF) mitigation")
                , [session] { return session->isSSRFMitigationEnabled(); }
                , [session](const QVariant &v) { session->setSSRFMitigationEnabled(v.toBool()); });

        addCheck(QStringLiteral("blockPeersOnPrivilegedPorts"), tr("Disallow connection to peers on privileged ports")
                , [session] { return session->blockPeersOnPrivilegedPorts(); }
                , [session](const QVariant &v) { session->setBlockPeersOnPrivilegedPorts(v.toBool()); });

        addCombo(QStringLiteral("chokingAlgorithm"), tr("Upload slots behavior")
                , combo({{tr("Fixed slots"), static_cast<int>(ChokingAlgorithm::FixedSlots)}
                        , {tr("Upload rate based"), static_cast<int>(ChokingAlgorithm::RateBased)}})
                , [session] { return static_cast<int>(session->chokingAlgorithm()); }
                , [session](const QVariant &v) { session->setChokingAlgorithm(static_cast<ChokingAlgorithm>(v.toInt())); });

        addCombo(QStringLiteral("seedChokingAlgorithm"), tr("Upload choking algorithm")
                , combo({{tr("Round-robin"), static_cast<int>(SeedChokingAlgorithm::RoundRobin)}
                        , {tr("Fastest upload"), static_cast<int>(SeedChokingAlgorithm::FastestUpload)}
                        , {tr("Anti-leech"), static_cast<int>(SeedChokingAlgorithm::AntiLeech)}})
                , [session] { return static_cast<int>(session->seedChokingAlgorithm()); }
                , [session](const QVariant &v) { session->setSeedChokingAlgorithm(static_cast<SeedChokingAlgorithm>(v.toInt())); });

        addCheck(QStringLiteral("announceAllTrackers"), tr("Always announce to all trackers in a tier")
                , [session] { return session->announceToAllTrackers(); }
                , [session](const QVariant &v) { session->setAnnounceToAllTrackers(v.toBool()); });

        addCheck(QStringLiteral("announceAllTiers"), tr("Always announce to all tiers")
                , [session] { return session->announceToAllTiers(); }
                , [session](const QVariant &v) { session->setAnnounceToAllTiers(v.toBool()); });

        addLine(QStringLiteral("announceIP"), tr("IP address reported to trackers (requires restart)"), {}
                , [session] { return session->announceIP(); }
                , [session](const QVariant &v) { session->setAnnounceIP(v.toString()); });

        addSpin(QStringLiteral("announcePort"), tr("Port reported to trackers (requires restart) [0: listening port]")
                , 0, 65535, {}, tr("0 (listening port)")
                , [session] { return session->announcePort(); }
                , [session](const QVariant &v) { session->setAnnouncePort(v.toInt()); });

        addSpin(QStringLiteral("maxConcurrentHTTPAnnounces"), tr("Max concurrent HTTP announces")
                , 0, kIntMax, {}, {}
                , [session] { return session->maxConcurrentHTTPAnnounces(); }
                , [session](const QVariant &v) { session->setMaxConcurrentHTTPAnnounces(v.toInt()); });

        addSpin(QStringLiteral("stopTrackerTimeout"), tr("Stop tracker timeout [0: disabled]")
                , 0, kIntMax, tr(" s"), tr("0 (disabled)")
                , [session] { return session->stopTrackerTimeout(); }
                , [session](const QVariant &v) { session->setStopTrackerTimeout(v.toInt()); });

        addSpin(QStringLiteral("peerTurnover"), tr("Peer turnover disconnect percentage")
                , 0, 100, tr(" %"), {}
                , [session] { return session->peerTurnover(); }
                , [session](const QVariant &v) { session->setPeerTurnover(v.toInt()); });

        addSpin(QStringLiteral("peerTurnoverCutoff"), tr("Peer turnover threshold percentage")
                , 0, 100, tr(" %"), {}
                , [session] { return session->peerTurnoverCutoff(); }
                , [session](const QVariant &v) { session->setPeerTurnoverCutoff(v.toInt()); });

        addSpin(QStringLiteral("peerTurnoverInterval"), tr("Peer turnover disconnect interval")
                , 30, 3600, tr(" s"), {}
                , [session] { return session->peerTurnoverInterval(); }
                , [session](const QVariant &v) { session->setPeerTurnoverInterval(v.toInt()); });

        addSpin(QStringLiteral("requestQueueSize"), tr("Maximum outstanding requests to a single peer")
                , 1, kIntMax, {}, {}
                , [session] { return session->requestQueueSize(); }
                , [session](const QVariant &v) { session->setRequestQueueSize(v.toInt()); });

        addLine(QStringLiteral("dhtBootstrapNodes"), tr("DHT bootstrap nodes"), tr("Resets to default if empty")
                , [session] { return session->getDHTBootstrapNodes(); }
                , [session](const QVariant &v) { session->setDHTBootstrapNodes(v.toString()); });

#if defined(QBT_USES_LIBTORRENT2) && defined(TORRENT_USE_I2P)
        addSpin(QStringLiteral("i2pInboundQuantity"), tr("I2P inbound quantity")
                , 1, 16, {}, {}
                , [session] { return session->I2PInboundQuantity(); }
                , [session](const QVariant &v) { session->setI2PInboundQuantity(v.toInt()); });

        addSpin(QStringLiteral("i2pOutboundQuantity"), tr("I2P outbound quantity")
                , 1, 16, {}, {}
                , [session] { return session->I2POutboundQuantity(); }
                , [session](const QVariant &v) { session->setI2POutboundQuantity(v.toInt()); });

        addSpin(QStringLiteral("i2pInboundLength"), tr("I2P inbound length")
                , 0, 7, {}, {}
                , [session] { return session->I2PInboundLength(); }
                , [session](const QVariant &v) { session->setI2PInboundLength(v.toInt()); });

        addSpin(QStringLiteral("i2pOutboundLength"), tr("I2P outbound length")
                , 0, 7, {}, {}
                , [session] { return session->I2POutboundLength(); }
                , [session](const QVariant &v) { session->setI2POutboundLength(v.toInt()); });
#endif

        qCDebug(lcModel) << "AdvancedSettingsModel: built" << m_rows.size() << "rows";
    }

    QList<Row> m_rows;
    bool m_modified = false;
};
