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

#include <QMetaType>

#include "orderedset.h"
#include "tag.h"
#include "utils/compare.h"

/// Natural, case-insensitive ordering for tags.
class TagLessThan
{
public:
    bool operator()(const Tag &left, const Tag &right) const;

private:
    Utils::Compare::NaturalCompare<Qt::CaseInsensitive> m_compare;
    Utils::Compare::NaturalCompare<Qt::CaseSensitive> m_subCompare;
};

using TagSet = OrderedSet<Tag, TagLessThan>;

Q_DECLARE_METATYPE(TagSet)
