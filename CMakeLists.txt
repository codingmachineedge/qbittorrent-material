# qBittorrent (Material rewrite) — root CMake project
# Copyright (C) 2026  qBittorrent-Material contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Qt6 / C++20 build. Produces a static engine library (qbt_base) and the
# `qbittorrent` Qt Quick (Material) executable with a single QML module
# (URI qBittorrent). See docs/ARCHITECTURE.md and docs/CONTRACTS.md.

cmake_minimum_required(VERSION 3.21 FATAL_ERROR)

project(qbittorrent-material
    VERSION 5.0.0
    DESCRIPTION "qBittorrent — Material (Qt Quick) rewrite"
    HOMEPAGE_URL "https://www.qbittorrent.org/"
    LANGUAGES CXX
)

# --- Global language settings -------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Where our custom Find modules (FindLibtorrentRasterbar) live.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Modules")

# --- Minimum dependency versions (mirrors upstream qBittorrent) ---------------
set(minQt6Version 6.6.0)
set(minBoostVersion 1.76)
set(minOpenSSLVersion 3.0.0)
set(minLibtorrentVersion 2.0.7)
set(minLibtorrent1Version 1.2.19)
set(minZlibVersion 1.2.11)

# --- Feature toggles ----------------------------------------------------------
option(QBT_DBUS "Enable D-Bus desktop notifications/power-management (Linux/BSD)" OFF)
option(QBT_STACKTRACE "Enable stacktrace support" OFF)
option(QBT_BUNDLE_QRC
    "Compile the hand-authored resources/resources.qrc verbatim instead of the \
tolerant globbed asset set (only enable once every listed asset exists)" OFF)

if ((CMAKE_SYSTEM_NAME STREQUAL "Linux") OR (CMAKE_SYSTEM_NAME STREQUAL "FreeBSD"))
    set(QBT_DBUS ON CACHE BOOL "" FORCE)
endif()

# --- Qt -----------------------------------------------------------------------
# Widgets is required because we host a QApplication (system tray + native OS
# file pickers), even though the entire UI surface is QML/Material.
find_package(Qt6 ${minQt6Version} REQUIRED
    COMPONENTS
        Core
        Gui
        Qml
        Quick
        QuickControls2
        Network
        Svg
        Sql
        Widgets
)

if (QBT_DBUS)
    find_package(Qt6 ${minQt6Version} QUIET COMPONENTS DBus)
    if (NOT Qt6DBus_FOUND)
        message(WARNING "Qt6::DBus not found — disabling QBT_DBUS.")
        set(QBT_DBUS OFF)
    endif()
endif()

qt_standard_project_setup(REQUIRES ${minQt6Version})

# Keep generated MOC/UIC/RCC artifacts out of the source tree.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# --- Third-party engine dependencies ------------------------------------------
find_package(OpenSSL ${minOpenSSLVersion} REQUIRED)
find_package(Boost ${minBoostVersion} REQUIRED)
find_package(ZLIB ${minZlibVersion} REQUIRED)
find_package(LibtorrentRasterbar ${minLibtorrentVersion} REQUIRED)

message(STATUS "qbittorrent-material: Qt ${Qt6_VERSION}, "
    "libtorrent ${LibtorrentRasterbar_VERSION}, DBus=${QBT_DBUS}")

add_subdirectory(src)
