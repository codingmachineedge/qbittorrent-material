# FindLibtorrentRasterbar.cmake
# Copyright (C) 2026  qBittorrent-Material contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Locate the libtorrent-rasterbar library (the engine layer links against it).
#
# This module first tries the CONFIG package that modern libtorrent-rasterbar
# (>= 1.2 / 2.0) installs, then falls back to pkg-config, then to a manual
# header/library search. In every case it produces the imported target
#
#     LibtorrentRasterbar::torrent-rasterbar
#
# and sets the following result variables:
#
#     LibtorrentRasterbar_FOUND
#     LibtorrentRasterbar_VERSION
#     LibtorrentRasterbar_INCLUDE_DIRS
#     LibtorrentRasterbar_LIBRARIES
#     LibtorrentRasterbar_USES_LIBTORRENT2   (TRUE when version >= 2.0)
#
# Usage:
#     find_package(LibtorrentRasterbar 2.0.7 REQUIRED)
#     target_link_libraries(mytarget PRIVATE LibtorrentRasterbar::torrent-rasterbar)

include(FindPackageHandleStandardArgs)

# ---------------------------------------------------------------------------
# 1) Preferred: the CMake CONFIG package shipped by libtorrent itself.
# ---------------------------------------------------------------------------
if (NOT TARGET LibtorrentRasterbar::torrent-rasterbar)
    find_package(LibtorrentRasterbar ${LibtorrentRasterbar_FIND_VERSION}
        CONFIG QUIET
        NAMES LibtorrentRasterbar libtorrent-rasterbar libtorrent
    )
endif()

if (TARGET LibtorrentRasterbar::torrent-rasterbar)
    if (NOT DEFINED LibtorrentRasterbar_VERSION AND DEFINED LibtorrentRasterbar_VERSION_STRING)
        set(LibtorrentRasterbar_VERSION "${LibtorrentRasterbar_VERSION_STRING}")
    endif()
else()
    # -----------------------------------------------------------------------
    # 2) pkg-config fallback.
    # -----------------------------------------------------------------------
    find_package(PkgConfig QUIET)
    if (PkgConfig_FOUND)
        pkg_check_modules(PC_LIBTORRENT QUIET libtorrent-rasterbar)
    endif()

    # -----------------------------------------------------------------------
    # 3) Manual header + library search.
    # -----------------------------------------------------------------------
    find_path(LibtorrentRasterbar_INCLUDE_DIR
        NAMES libtorrent/session.hpp
        HINTS
            ${PC_LIBTORRENT_INCLUDEDIR}
            ${PC_LIBTORRENT_INCLUDE_DIRS}
            ENV LIBTORRENT_ROOT
        PATH_SUFFIXES include
    )

    find_library(LibtorrentRasterbar_LIBRARY
        NAMES torrent-rasterbar libtorrent-rasterbar libtorrent
        HINTS
            ${PC_LIBTORRENT_LIBDIR}
            ${PC_LIBTORRENT_LIBRARY_DIRS}
            ENV LIBTORRENT_ROOT
        PATH_SUFFIXES lib lib64
    )

    # Resolve the version from the installed version.hpp if pkg-config was silent.
    if (PC_LIBTORRENT_VERSION)
        set(LibtorrentRasterbar_VERSION "${PC_LIBTORRENT_VERSION}")
    elseif (LibtorrentRasterbar_INCLUDE_DIR AND
            EXISTS "${LibtorrentRasterbar_INCLUDE_DIR}/libtorrent/version.hpp")
        file(STRINGS "${LibtorrentRasterbar_INCLUDE_DIR}/libtorrent/version.hpp"
            _lt_version_line REGEX "^#define[ \t]+LIBTORRENT_VERSION[ \t]+\"[^\"]+\"")
        if (_lt_version_line MATCHES "\"([0-9]+\\.[0-9]+\\.[0-9]+)")
            set(LibtorrentRasterbar_VERSION "${CMAKE_MATCH_1}")
        endif()
        unset(_lt_version_line)
    endif()

    find_package_handle_standard_args(LibtorrentRasterbar
        REQUIRED_VARS LibtorrentRasterbar_LIBRARY LibtorrentRasterbar_INCLUDE_DIR
        VERSION_VAR LibtorrentRasterbar_VERSION
    )

    if (LibtorrentRasterbar_FOUND AND NOT TARGET LibtorrentRasterbar::torrent-rasterbar)
        add_library(LibtorrentRasterbar::torrent-rasterbar UNKNOWN IMPORTED)
        set_target_properties(LibtorrentRasterbar::torrent-rasterbar PROPERTIES
            IMPORTED_LOCATION "${LibtorrentRasterbar_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LibtorrentRasterbar_INCLUDE_DIR}"
            INTERFACE_COMPILE_FEATURES cxx_std_17
        )
        if (PC_LIBTORRENT_CFLAGS_OTHER)
            set_property(TARGET LibtorrentRasterbar::torrent-rasterbar
                PROPERTY INTERFACE_COMPILE_OPTIONS "${PC_LIBTORRENT_CFLAGS_OTHER}")
        endif()
    endif()

    mark_as_advanced(LibtorrentRasterbar_INCLUDE_DIR LibtorrentRasterbar_LIBRARY)
endif()

# ---------------------------------------------------------------------------
# Normalize result variables + derive the ABI-major flag used by the engine.
# ---------------------------------------------------------------------------
if (TARGET LibtorrentRasterbar::torrent-rasterbar)
    set(LibtorrentRasterbar_FOUND TRUE)

    if (NOT LibtorrentRasterbar_INCLUDE_DIRS)
        get_target_property(LibtorrentRasterbar_INCLUDE_DIRS
            LibtorrentRasterbar::torrent-rasterbar INTERFACE_INCLUDE_DIRECTORIES)
    endif()
    if (NOT LibtorrentRasterbar_LIBRARIES)
        set(LibtorrentRasterbar_LIBRARIES LibtorrentRasterbar::torrent-rasterbar)
    endif()

    if (LibtorrentRasterbar_VERSION AND (LibtorrentRasterbar_VERSION VERSION_GREATER_EQUAL "2.0.0"))
        set(LibtorrentRasterbar_USES_LIBTORRENT2 TRUE)
    else()
        set(LibtorrentRasterbar_USES_LIBTORRENT2 FALSE)
    endif()

    # Convey the ABI major to the engine sources.
    if (LibtorrentRasterbar_USES_LIBTORRENT2)
        set_property(TARGET LibtorrentRasterbar::torrent-rasterbar
            APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS QBT_USES_LIBTORRENT2)
    endif()
endif()

find_package_handle_standard_args(LibtorrentRasterbar
    REQUIRED_VARS LibtorrentRasterbar_LIBRARIES
    VERSION_VAR LibtorrentRasterbar_VERSION
)
