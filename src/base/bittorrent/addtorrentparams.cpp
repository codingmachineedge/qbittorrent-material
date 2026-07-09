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

#include "addtorrentparams.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QList>
#include <QString>

#include "base/logging.h"
#include "base/utils/sslkey.h"
#include "base/utils/string.h"
#include "downloadpriority.h"

using namespace Qt::Literals::StringLiterals;

namespace
{
    const QString PARAM_CATEGORY = u"category"_s;
    const QString PARAM_TAGS = u"tags"_s;
    const QString PARAM_SAVEPATH = u"save_path"_s;
    const QString PARAM_DOWNLOADPATH = u"download_path"_s;
    const QString PARAM_USEDOWNLOADPATH = u"use_download_path"_s;
    const QString PARAM_OPERATINGMODE = u"operating_mode"_s;
    const QString PARAM_STOPPED = u"stopped"_s;
    const QString PARAM_ADDTOQUEUETOP = u"add_to_top_of_queue"_s;
    const QString PARAM_STOPCONDITION = u"stop_condition"_s;
    const QString PARAM_CONTENTLAYOUT = u"content_layout"_s;
    const QString PARAM_AUTOTMM = u"use_auto_tmm"_s;
    const QString PARAM_UPLOADLIMIT = u"upload_limit"_s;
    const QString PARAM_DOWNLOADLIMIT = u"download_limit"_s;
    const QString PARAM_SEEDINGTIMELIMIT = u"seeding_time_limit"_s;
    const QString PARAM_INACTIVESEEDINGTIMELIMIT = u"inactive_seeding_time_limit"_s;
    const QString PARAM_RATIOLIMIT = u"ratio_limit"_s;
    const QString PARAM_SHARELIMITACTION = u"share_limit_action"_s;
    const QString PARAM_SSL_CERTIFICATE = u"ssl_certificate"_s;
    const QString PARAM_SSL_PRIVATEKEY = u"ssl_private_key"_s;
    const QString PARAM_SSL_DHPARAMS = u"ssl_dh_params"_s;
    const QString PARAM_NAME = u"name"_s;
    const QString PARAM_FIRSTLASTPIECEPRIORITY = u"first_last_piece_priority"_s;
    const QString PARAM_SEQUENTIAL = u"sequential"_s;
    const QString PARAM_SKIPCHECKING = u"skip_checking"_s;

    std::optional<bool> getOptionalBool(const QJsonObject &jsonObj, const QString &key)
    {
        const QJsonValue jsonVal = jsonObj.value(key);
        if (jsonVal.isBool())
            return jsonVal.toBool();
        return std::nullopt;
    }
}

BitTorrent::AddTorrentParams BitTorrent::parseAddTorrentParams(const QJsonObject &jsonObj)
{
    qCDebug(lcEngine) << "Parsing AddTorrentParams from JSON";

    TagSet tags;
    const QJsonArray tagsArray = jsonObj.value(PARAM_TAGS).toArray();
    for (const QJsonValue &tagVal : tagsArray)
        tags.insert(Tag(tagVal.toString()));

    AddTorrentParams params;
    params.name = jsonObj.value(PARAM_NAME).toString();
    params.category = jsonObj.value(PARAM_CATEGORY).toString();
    params.tags = tags;
    params.savePath = Path(jsonObj.value(PARAM_SAVEPATH).toString());
    params.useDownloadPath = getOptionalBool(jsonObj, PARAM_USEDOWNLOADPATH);
    params.downloadPath = Path(jsonObj.value(PARAM_DOWNLOADPATH).toString());
    params.sequential = jsonObj.value(PARAM_SEQUENTIAL).toBool();
    params.firstLastPiecePriority = jsonObj.value(PARAM_FIRSTLASTPIECEPRIORITY).toBool();

    if (const QJsonValue jsonVal = jsonObj.value(PARAM_OPERATINGMODE); jsonVal.isString())
        params.addForced = (Utils::String::toEnum(jsonVal.toString(), TorrentOperatingMode::AutoManaged) == TorrentOperatingMode::Forced);

    params.addToQueueTop = getOptionalBool(jsonObj, PARAM_ADDTOQUEUETOP);
    params.addStopped = getOptionalBool(jsonObj, PARAM_STOPPED);

    if (const QJsonValue jsonVal = jsonObj.value(PARAM_STOPCONDITION); jsonVal.isString())
        params.stopCondition = Utils::String::toEnum(jsonVal.toString(), Torrent::StopCondition::None);

    if (const QJsonValue jsonVal = jsonObj.value(PARAM_CONTENTLAYOUT); jsonVal.isString())
        params.contentLayout = Utils::String::toEnum(jsonVal.toString(), TorrentContentLayout::Original);

    params.useAutoTMM = getOptionalBool(jsonObj, PARAM_AUTOTMM);
    params.uploadLimit = jsonObj.value(PARAM_UPLOADLIMIT).toInt(-1);
    params.downloadLimit = jsonObj.value(PARAM_DOWNLOADLIMIT).toInt(-1);
    params.skipChecking = jsonObj.value(PARAM_SKIPCHECKING).toBool();

    params.shareLimits.ratioLimit = jsonObj.value(PARAM_RATIOLIMIT).toDouble(DEFAULT_RATIO_LIMIT);
    params.shareLimits.seedingTimeLimit = jsonObj.value(PARAM_SEEDINGTIMELIMIT).toInt(DEFAULT_SEEDING_TIME_LIMIT);
    params.shareLimits.inactiveSeedingTimeLimit = jsonObj.value(PARAM_INACTIVESEEDINGTIMELIMIT).toInt(DEFAULT_SEEDING_TIME_LIMIT);
    params.shareLimits.action = Utils::String::toEnum(jsonObj.value(PARAM_SHARELIMITACTION).toString(), ShareLimitAction::Default);

    params.sslParameters =
    {
        Utils::SSLKey::loadCertificateFromString(jsonObj.value(PARAM_SSL_CERTIFICATE).toString()),
        Utils::SSLKey::loadPrivateKeyFromString(jsonObj.value(PARAM_SSL_PRIVATEKEY).toString()),
        jsonObj.value(PARAM_SSL_DHPARAMS).toString().toLatin1()
    };

    return params;
}

QJsonObject BitTorrent::serializeAddTorrentParams(const AddTorrentParams &params)
{
    qCDebug(lcEngine) << "Serializing AddTorrentParams to JSON, name:" << params.name;

    QJsonArray tagsArray;
    for (const Tag &tag : params.tags)
        tagsArray.append(tag.toString());

    QJsonObject jsonObj
    {
        {PARAM_NAME, params.name},
        {PARAM_CATEGORY, params.category},
        {PARAM_TAGS, tagsArray},
        {PARAM_SAVEPATH, params.savePath.data()},
        {PARAM_DOWNLOADPATH, params.downloadPath.data()},
        {PARAM_SEQUENTIAL, params.sequential},
        {PARAM_FIRSTLASTPIECEPRIORITY, params.firstLastPiecePriority},
        {PARAM_OPERATINGMODE, Utils::String::fromEnum(
                params.addForced ? TorrentOperatingMode::Forced : TorrentOperatingMode::AutoManaged)},
        {PARAM_UPLOADLIMIT, params.uploadLimit},
        {PARAM_DOWNLOADLIMIT, params.downloadLimit},
        {PARAM_SKIPCHECKING, params.skipChecking},
        {PARAM_RATIOLIMIT, params.shareLimits.ratioLimit},
        {PARAM_SEEDINGTIMELIMIT, params.shareLimits.seedingTimeLimit},
        {PARAM_INACTIVESEEDINGTIMELIMIT, params.shareLimits.inactiveSeedingTimeLimit},
        {PARAM_SHARELIMITACTION, Utils::String::fromEnum(params.shareLimits.action)},
        {PARAM_SSL_CERTIFICATE, QString::fromLatin1(params.sslParameters.certificate.toPem())},
        {PARAM_SSL_PRIVATEKEY, QString::fromLatin1(params.sslParameters.privateKey.toPem())},
        {PARAM_SSL_DHPARAMS, QString::fromLatin1(params.sslParameters.dhParams)}
    };

    if (params.useDownloadPath)
        jsonObj[PARAM_USEDOWNLOADPATH] = *params.useDownloadPath;
    if (params.addToQueueTop)
        jsonObj[PARAM_ADDTOQUEUETOP] = *params.addToQueueTop;
    if (params.addStopped)
        jsonObj[PARAM_STOPPED] = *params.addStopped;
    if (params.stopCondition)
        jsonObj[PARAM_STOPCONDITION] = Utils::String::fromEnum(*params.stopCondition);
    if (params.contentLayout)
        jsonObj[PARAM_CONTENTLAYOUT] = Utils::String::fromEnum(*params.contentLayout);
    if (params.useAutoTMM)
        jsonObj[PARAM_AUTOTMM] = *params.useAutoTMM;

    return jsonObj;
}
