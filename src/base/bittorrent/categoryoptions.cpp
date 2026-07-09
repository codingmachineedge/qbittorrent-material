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

#include "categoryoptions.h"

#include <QJsonObject>
#include <QJsonValue>

#include "base/utils/string.h"

using namespace Qt::Literals::StringLiterals;

namespace
{
    const QString OPTION_SAVEPATH = u"save_path"_s;
    const QString OPTION_DOWNLOADPATH = u"download_path"_s;

    const QString OPTION_RATIOLIMIT = u"ratio_limit"_s;
    const QString OPTION_SEEDINGTIMELIMIT = u"seeding_time_limit"_s;
    const QString OPTION_INACTIVESEEDINGTIMELIMIT = u"inactive_seeding_time_limit"_s;
    const QString OPTION_SHARELIMITACTION = u"share_limit_action"_s;
    const QString OPTION_SHARELIMITSMODE = u"share_limits_mode"_s;
}

BitTorrent::CategoryOptions BitTorrent::CategoryOptions::fromJSON(const QJsonObject &jsonObj)
{
    CategoryOptions options;
    options.savePath = Path(jsonObj.value(OPTION_SAVEPATH).toString());

    const QJsonValue downloadPathValue = jsonObj.value(OPTION_DOWNLOADPATH);
    if (downloadPathValue.isBool())
        options.downloadPath = {downloadPathValue.toBool(), {}};
    else if (downloadPathValue.isString())
        options.downloadPath = {true, Path(downloadPathValue.toString())};

    options.shareLimits.ratioLimit = jsonObj.value(OPTION_RATIOLIMIT).toDouble(DEFAULT_RATIO_LIMIT);
    options.shareLimits.seedingTimeLimit = jsonObj.value(OPTION_SEEDINGTIMELIMIT).toInt(DEFAULT_SEEDING_TIME_LIMIT);
    options.shareLimits.inactiveSeedingTimeLimit = jsonObj.value(OPTION_INACTIVESEEDINGTIMELIMIT).toInt(DEFAULT_SEEDING_TIME_LIMIT);
    options.shareLimits.action = Utils::String::toEnum<ShareLimitAction>(
            jsonObj.value(OPTION_SHARELIMITACTION).toString(), ShareLimitAction::Default);
    options.shareLimits.mode = Utils::String::toEnum<ShareLimitsMode>(
            jsonObj.value(OPTION_SHARELIMITSMODE).toString(), ShareLimitsMode::Default);

    return options;
}

QJsonObject BitTorrent::CategoryOptions::toJSON() const
{
    QJsonValue downloadPathValue = QJsonValue::Null;
    if (downloadPath)
    {
        if (downloadPath->enabled)
            downloadPathValue = downloadPath->path.data();
        else
            downloadPathValue = false;
    }

    return {
        {OPTION_SAVEPATH, savePath.data()},
        {OPTION_DOWNLOADPATH, downloadPathValue},
        {OPTION_RATIOLIMIT, shareLimits.ratioLimit},
        {OPTION_SEEDINGTIMELIMIT, shareLimits.seedingTimeLimit},
        {OPTION_INACTIVESEEDINGTIMELIMIT, shareLimits.inactiveSeedingTimeLimit},
        {OPTION_SHARELIMITACTION, Utils::String::fromEnum(shareLimits.action)},
        {OPTION_SHARELIMITSMODE, Utils::String::fromEnum(shareLimits.mode)}
    };
}
