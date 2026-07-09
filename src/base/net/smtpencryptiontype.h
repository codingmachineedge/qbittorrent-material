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

#include <QMetaEnum>

namespace Net
{
    inline namespace SMTPEncryptionNS
    {
        Q_NAMESPACE

        /// SMTP transport security. Numeric values are persisted and MUST NOT change.
        enum class SMTPEncryptionType
        {
            None = 0,
            STARTTLS = 1,
            SMTPS = 2
        };
        Q_ENUM_NS(SMTPEncryptionType)
    }
}
