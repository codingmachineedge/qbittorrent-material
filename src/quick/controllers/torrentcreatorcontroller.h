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

#include <QObject>
#include <QPointer>
#include <QQmlEngine>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

namespace BitTorrent
{
    class TorrentCreator;
    struct TorrentCreatorResult;
}

class QThreadPool;

/**
 * @file torrentcreatorcontroller.h
 * @brief Bridge backing the Material @c TorrentCreatorDialog.
 *
 * Drives @c BitTorrent::TorrentCreator (a @c QRunnable) on a private single-slot
 * thread pool. QML supplies the fully-resolved source path and destination
 * @c .torrent path (the OS save dialog runs in QML), a piece size, the format /
 * alignment options, and the free-form tracker / web-seed / comment / source
 * fields; this controller assembles @c TorrentCreatorParams, kicks off creation,
 * and republishes progress + terminal outcome as signals. On success, when the
 * user asked to "start seeding immediately", it also loads the freshly-written
 * torrent and adds it to the session (honouring "ignore share limits").
 *
 * The piece-count estimator (@ref calculateTotalPieces) runs on a throwaway
 * worker thread and reports back through @ref totalPiecesText.
 */
class TorrentCreatorController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    /// True while a creation job is running (dialog inputs disabled).
    Q_PROPERTY(bool creating READ isCreating NOTIFY creatingChanged)

    /// Current creation progress, 0-100.
    Q_PROPERTY(int progress READ progress NOTIFY progressChanged)

    /// Label for the "Calculate number of pieces" result ("", "Calculating…", N).
    Q_PROPERTY(QString totalPiecesText READ totalPiecesText NOTIFY totalPiecesChanged)

    /// Whether the engine is built against libtorrent 2.x (drives format vs align UI).
    Q_PROPERTY(bool libtorrent2 READ isLibtorrent2 CONSTANT)

    /// Piece-size combo model: list of { text, value } (value in bytes, 0 = Auto).
    Q_PROPERTY(QVariantList pieceSizes READ pieceSizes CONSTANT)

public:
    /// QML singleton factory — returns the app-owned instance.
    static TorrentCreatorController *create(QQmlEngine *, QJSEngine *);

    explicit TorrentCreatorController(QObject *parent = nullptr);
    ~TorrentCreatorController() override;

    [[nodiscard]] bool isCreating() const { return m_creating; }
    [[nodiscard]] int progress() const { return m_progress; }
    [[nodiscard]] QString totalPiecesText() const { return m_totalPiecesText; }
    [[nodiscard]] bool isLibtorrent2() const;
    [[nodiscard]] QVariantList pieceSizes() const { return m_pieceSizes; }

    // ---- QML actions -------------------------------------------------------

    /// Estimate the total number of pieces for the given source and options.
    /// @p torrentFormat maps 0=V2, 1=Hybrid, 2=V1 (libtorrent2 only); the
    /// alignment args are used on libtorrent1 only. The result is published
    /// asynchronously through @ref totalPiecesText.
    Q_INVOKABLE void calculateTotalPieces(const QString &sourcePath, int pieceSize
            , bool ignoreDotfiles, int torrentFormat
            , bool isAlignmentOptimized, int paddedFileSizeLimit);

    /// Start creating a torrent. @p params keys:
    /// @c sourcePath, @c torrentFilePath, @c pieceSize, @c ignoreDotfiles,
    /// @c isPrivate, @c torrentFormat, @c isAlignmentOptimized,
    /// @c paddedFileSizeLimit, @c comment, @c source, @c trackers, @c urlSeeds,
    /// @c startSeeding, @c ignoreShareLimits.
    Q_INVOKABLE void createTorrent(const QVariantMap &params);

    /// Request interruption of the in-flight creation job (dialog cancel/close).
    Q_INVOKABLE void cancelCreation();

    /// Clear the piece-count label (dialog re-open / inputs changed).
    Q_INVOKABLE void clearTotalPieces();

signals:
    void creatingChanged();
    void progressChanged();
    void totalPiecesChanged();

    /// The .torrent was written; @p savePath is the content's parent for seeding.
    void creationSucceeded(const QString &torrentFilePath, const QString &savePath);
    /// Creation failed with a human-readable @p message.
    void creationFailed(const QString &message);
    /// "Start seeding" was requested but re-adding the torrent failed.
    void addTorrentFailed(const QString &message);

private:
    void setCreating(bool creating);
    void setProgress(int progress);
    void setTotalPiecesText(const QString &text);

    void onCreationSuccess(const BitTorrent::TorrentCreatorResult &result);
    void onCreationFailure(const QString &message);

    static QVariantList buildPieceSizes();

    QThreadPool *m_threadPool = nullptr;
    QPointer<BitTorrent::TorrentCreator> m_activeCreator;

    QVariantList m_pieceSizes;
    QString m_totalPiecesText;
    int m_progress = 0;
    bool m_creating = false;

    // Captured at createTorrent() time for use in the async success handler.
    bool m_startSeeding = false;
    bool m_ignoreShareLimits = false;
};
