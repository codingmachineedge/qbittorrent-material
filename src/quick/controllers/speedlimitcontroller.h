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
#include <QQmlEngine>

#include "base/bittorrent/session.h"
#include "base/logging.h"

/**
 * @file speedlimitcontroller.h
 * @brief Bridge backing the Material @c SpeedLimitDialog ("Global Speed Limits").
 *
 * Exposes the four global rate limits — regular upload / download and the
 * alternative upload / download — as notifiable properties expressed in
 * @b KiB/s (the engine stores bytes/s; this controller divides / multiplies by
 * 1024). The dialog binds the properties for its initial values via @ref load()
 * and writes user edits back on OK via @ref apply(); only values that actually
 * changed are pushed to the session, mirroring the legacy dialog.
 *
 * The sentinel @c 0 means "unlimited" (rendered as @c ∞ by @c SpeedSpinBox).
 */
class SpeedLimitController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(int uploadLimit READ uploadLimit NOTIFY changed)
    Q_PROPERTY(int downloadLimit READ downloadLimit NOTIFY changed)
    Q_PROPERTY(int altUploadLimit READ altUploadLimit NOTIFY changed)
    Q_PROPERTY(int altDownloadLimit READ altDownloadLimit NOTIFY changed)

public:
    /// QML singleton factory — returns the app-owned instance.
    static SpeedLimitController *create(QQmlEngine *, QJSEngine *)
    {
        qCDebug(lcUi) << "Creating SpeedLimitController";
        return new SpeedLimitController;
    }

    explicit SpeedLimitController(QObject *parent = nullptr)
        : QObject(parent)
    {
        load();
    }

    [[nodiscard]] int uploadLimit() const { return m_uploadLimit; }
    [[nodiscard]] int downloadLimit() const { return m_downloadLimit; }
    [[nodiscard]] int altUploadLimit() const { return m_altUploadLimit; }
    [[nodiscard]] int altDownloadLimit() const { return m_altDownloadLimit; }

    /// (Re)read the four limits from the session, converting bytes/s -> KiB/s.
    Q_INVOKABLE void load()
    {
        const auto *session = BitTorrent::Session::instance();
        m_uploadLimit = toKiB(session->globalUploadSpeedLimit());
        m_downloadLimit = toKiB(session->globalDownloadSpeedLimit());
        m_altUploadLimit = toKiB(session->altGlobalUploadSpeedLimit());
        m_altDownloadLimit = toKiB(session->altGlobalDownloadSpeedLimit());
        qCDebug(lcUi) << "SpeedLimitController loaded limits (KiB/s):"
                      << "up" << m_uploadLimit << "down" << m_downloadLimit
                      << "altUp" << m_altUploadLimit << "altDown" << m_altDownloadLimit;
        emit changed();
    }

    /// Persist edited limits (all in KiB/s). Only changed values are written.
    Q_INVOKABLE void apply(const int uploadKiB, const int downloadKiB
            , const int altUploadKiB, const int altDownloadKiB)
    {
        auto *session = BitTorrent::Session::instance();
        qCInfo(lcUi) << "SpeedLimitController applying limits (KiB/s):"
                     << "up" << uploadKiB << "down" << downloadKiB
                     << "altUp" << altUploadKiB << "altDown" << altDownloadKiB;

        if (uploadKiB != m_uploadLimit)
            session->setGlobalUploadSpeedLimit(fromKiB(uploadKiB));
        if (downloadKiB != m_downloadLimit)
            session->setGlobalDownloadSpeedLimit(fromKiB(downloadKiB));
        if (altUploadKiB != m_altUploadLimit)
            session->setAltGlobalUploadSpeedLimit(fromKiB(altUploadKiB));
        if (altDownloadKiB != m_altDownloadLimit)
            session->setAltGlobalDownloadSpeedLimit(fromKiB(altDownloadKiB));

        load();
    }

signals:
    /// Emitted whenever any of the four cached limits changes.
    void changed();

private:
    [[nodiscard]] static int toKiB(const int bytesPerSec) { return bytesPerSec / 1024; }
    [[nodiscard]] static int fromKiB(const int kibPerSec) { return kibPerSec * 1024; }

    int m_uploadLimit = 0;
    int m_downloadLimit = 0;
    int m_altUploadLimit = 0;
    int m_altDownloadLimit = 0;
};
