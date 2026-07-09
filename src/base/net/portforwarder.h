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
#include <QSet>

class QString;

namespace Net
{
    /// Abstract UPnP/NAT-PMP port-forwarding service. The concrete implementation
    /// is backed by the libtorrent session; ports are grouped by named "profiles"
    /// so distinct subsystems (listen port, etc.) can be mapped/unmapped independently.
    class PortForwarder : public QObject
    {
        Q_OBJECT
        Q_DISABLE_COPY_MOVE(PortForwarder)

    public:
        explicit PortForwarder(QObject *parent = nullptr);
        ~PortForwarder() override;

        static PortForwarder *instance();

        virtual bool isEnabled() const = 0;
        virtual void setEnabled(bool enabled) = 0;

        /// Requests a mapping for every port in `ports` under the given `profile`,
        /// replacing any previous set for that profile.
        virtual void setPorts(const QString &profile, QSet<quint16> ports) = 0;
        /// Removes all mappings associated with `profile`.
        virtual void removePorts(const QString &profile) = 0;

    private:
        static PortForwarder *m_instance;
    };
}
