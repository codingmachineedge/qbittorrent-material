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

#include <filesystem>
#include <iterator>

#include <QtContainerFwd>
#include <QMetaType>
#include <QString>

class QStringView;

/// Filesystem path wrapper used throughout the engine.
///
/// @note For fidelity with the legacy engine the class is intentionally kept
/// in the global namespace and named `Path` (not `Utils::Fs::Path`) so that
/// every engine signature returning/taking a `Path` remains identical. Only
/// the physical header location moved to `base/utils/fs/path.h`.
class Path;
using PathList = QList<Path>;

class Path final
{
public:
    class Iterator;

    Path() = default;

    explicit Path(const QString &pathStr);
    explicit Path(std::string_view pathStr);

    bool isValid() const;
    bool isEmpty() const;
    bool isAbsolute() const;
    bool isRelative() const;

    bool exists() const;

    Path rootItem() const;
    Path parentPath() const;

    QString filename() const;

    QString extension() const;
    bool hasExtension(QStringView ext) const;
    void removeExtension();
    Path removedExtension() const;
    void removeExtension(QStringView ext);
    Path removedExtension(QStringView ext) const;

    bool hasAncestor(const Path &other) const;
    Path relativePathOf(const Path &childPath) const;

    QString data() const;
    QString toString() const;
    std::filesystem::path toStdFsPath() const;

    Path &operator/=(const Path &other);
    Path &operator+=(QStringView str);

    Iterator begin() const;
    Iterator end() const;

    static Path commonPath(const Path &left, const Path &right);

    static Path findRootFolder(const PathList &filePaths);
    static void stripRootFolder(PathList &filePaths);
    static void addRootFolder(PathList &filePaths, const Path &rootFolder);

    friend Path operator/(const Path &lhs, const Path &rhs);

private:
    // this constructor doesn't perform any checks
    // so it's intended for internal use only
    static Path createUnchecked(const QString &pathStr);

    QString m_pathStr;
};

Q_DECLARE_METATYPE(Path)

class Path::Iterator final
{
public:
    using iterator_category = std::input_iterator_tag;
    using difference_type = qsizetype;
    using value_type = Path;
    using const_pointer = const value_type *;
    using pointer = const_pointer;
    using const_reference = const value_type &;
    using reference = const_reference;

    struct EndIteratorTag {};

    Iterator(const Path &path);
    Iterator(const Path &path, EndIteratorTag);

    reference operator*() const;
    pointer operator->();
    Iterator &operator++();
    Iterator operator++(int);

    friend bool operator==(const Iterator &a, const Iterator &b);
    friend bool operator!=(const Iterator &a, const Iterator &b);

private:
    const Path &m_path;
    qsizetype m_depth = 0;
    qsizetype m_itemsCount = 0;
    Path m_currentPath;
};

bool operator==(const Path &lhs, const Path &rhs);
Path operator+(const Path &lhs, QStringView rhs);

QDataStream &operator<<(QDataStream &out, const Path &path);
QDataStream &operator>>(QDataStream &in, Path &path);

std::size_t qHash(const Path &key, std::size_t seed = 0);
