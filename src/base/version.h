/*
 * qBittorrent Material — a BitTorrent client
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Version macros. Mirrors upstream qBittorrent's generated version.h so the
 * ported engine (torrent creator "created by", session user-agent, etc.)
 * compiles unchanged.
 */

#pragma once

#define QBT_VERSION_MAJOR 5
#define QBT_VERSION_MINOR 3
#define QBT_VERSION_BUGFIX 0
#define QBT_VERSION_BUILD 0
#define QBT_VERSION_STATUS "-material"  // fork marker

#define QBT__STRINGIFY(x) #x
#define QBT_STRINGIFY(x) QBT__STRINGIFY(x)

#if (QBT_VERSION_BUILD != 0)
#define PROJECT_VERSION QBT_STRINGIFY(QBT_VERSION_MAJOR.QBT_VERSION_MINOR.QBT_VERSION_BUGFIX.QBT_VERSION_BUILD) QBT_VERSION_STATUS
#else
#define PROJECT_VERSION QBT_STRINGIFY(QBT_VERSION_MAJOR.QBT_VERSION_MINOR.QBT_VERSION_BUGFIX) QBT_VERSION_STATUS
#endif

#define QBT_VERSION "v" PROJECT_VERSION
#define QBT_VERSION_2 PROJECT_VERSION
