/*
 * qBittorrent (Material rewrite) — a BitTorrent client
 * Copyright (C) 2026 qBittorrent-Material contributors
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

#include <qqmlintegration.h>

class QQmlEngine;
class QJSEngine;

/**
 * @brief QML singleton facade over the git-backed TorrentJournal and
 *        TorrentUndoManager.
 *
 * Drives the design's History panel (Settings repo / Action log tabs), the
 * global undo snackbar ("Removed X — UNDO"), and Ctrl+Z. `actionJournaled`
 * carries the commit id so a queued snackbar always undoes exactly the entry
 * it was shown for, never "whatever is newest by now".
 */
class JournalController final : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    Q_DISABLE_COPY_MOVE(JournalController)

    Q_PROPERTY(bool available READ isAvailable CONSTANT)
    Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(QString lastActionDescription READ lastActionDescription NOTIFY historyChanged)
    Q_PROPERTY(int actionsCount READ actionsCount NOTIFY historyChanged)
    Q_PROPERTY(int settingsCount READ settingsCount NOTIFY historyChanged)
    Q_PROPERTY(int journaledOnlyCount READ journaledOnlyCount NOTIFY historyChanged)
    Q_PROPERTY(bool autoCommit READ isAutoCommitEnabled WRITE setAutoCommitEnabled NOTIFY statusChanged)
    Q_PROPERTY(QString retention READ retention WRITE setRetention NOTIFY statusChanged)

public:
    static JournalController *create(QQmlEngine *engine, QJSEngine *jsEngine);
    static JournalController *instance();

    [[nodiscard]] bool isAvailable() const;
    [[nodiscard]] bool isBusy() const;
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] QString lastActionDescription() const;
    [[nodiscard]] int actionsCount() const;
    [[nodiscard]] int settingsCount() const;
    [[nodiscard]] int journaledOnlyCount() const;
    [[nodiscard]] bool isAutoCommitEnabled() const;
    void setAutoCommitEnabled(bool enabled);
    [[nodiscard]] QString retention() const;
    void setRetention(const QString &retention);

    Q_INVOKABLE void undoLast();
    Q_INVOKABLE void undoEntry(const QString &commitId);
    Q_INVOKABLE void restoreTo(const QString &commitId);
    Q_INVOKABLE void undoSettingsEntry(const QString &commitId);
    Q_INVOKABLE void restoreMissingTorrents();
    Q_INVOKABLE bool openRepository(const QString &repo) const;
    Q_INVOKABLE void copyToClipboard(const QString &text) const;
    Q_INVOKABLE int exportHistoryJson(const QString &repo) const;

signals:
    void busyChanged();
    void historyChanged();
    void statusChanged();
    // Fired for every NEW user-origin action commit — the snackbar hook.
    void actionJournaled(const QString &commitId, const QString &description, bool undoable);
    // Result of an undo/restore operation — surfaced in the snackbar.
    void operationFinished(bool success, const QString &message);

private:
    JournalController();
    ~JournalController() override = default;

    static JournalController *m_instance;
    mutable int m_cachedActionsCount = -1;
    mutable int m_cachedSettingsCount = -1;
};
