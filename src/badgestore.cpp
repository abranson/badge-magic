/*
 * SPDX-FileCopyrightText: 2026 Andrew Branson
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Andrew Branson
 *
 * Based on the original Badge Magic application by FOSSASIA.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "badgestore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {

QVariantMap toBadgeMap(const QString &filePath,
                       const QString &name,
                       const BadgeMessage &message)
{
    QVariantMap item;
    item.insert(QStringLiteral("filePath"), filePath);
    item.insert(QStringLiteral("name"), name);
    item.insert(QStringLiteral("rawText"), message.rawText);
    item.insert(QStringLiteral("flash"), message.flash);
    item.insert(QStringLiteral("marquee"), message.marquee);
    item.insert(QStringLiteral("speedIndex"), message.speedIndex);
    item.insert(QStringLiteral("modeIndex"), message.modeIndex);
    item.insert(QStringLiteral("speedLabel"), BadgeEncoder::speedLabel(message.speedIndex));
    item.insert(QStringLiteral("modeLabel"), BadgeEncoder::modeLabel(message.modeIndex));
    item.insert(QStringLiteral("hasRawText"), !message.rawText.trimmed().isEmpty());
    item.insert(QStringLiteral("hexText"), message.textHex);
    return item;
}

QJsonObject toJsonObject(const BadgeMessage &message)
{
    QJsonObject encodedMessage;
    encodedMessage.insert(QStringLiteral("text"), QJsonArray::fromStringList(message.textHex));
    encodedMessage.insert(QStringLiteral("flash"), message.flash);
    encodedMessage.insert(QStringLiteral("marquee"), message.marquee);
    encodedMessage.insert(QStringLiteral("speed"), BadgeEncoder::speedHex(message.speedIndex));
    encodedMessage.insert(QStringLiteral("mode"), BadgeEncoder::modeHex(message.modeIndex));

    QJsonObject root;
    root.insert(QStringLiteral("messages"), QJsonArray{encodedMessage});

    if (!message.rawText.isEmpty()) {
        root.insert(QStringLiteral("rawText"), message.rawText);
    }

    return root;
}

BadgeMessage fromJsonObject(const QJsonObject &root)
{
    BadgeMessage message;
    message.rawText = root.value(QStringLiteral("rawText")).toString();

    const QJsonArray messages = root.value(QStringLiteral("messages")).toArray();
    if (!messages.isEmpty()) {
        const QJsonObject first = messages.first().toObject();
        const QJsonArray text = first.value(QStringLiteral("text")).toArray();
        for (const QJsonValue &entry : text) {
            message.textHex.append(entry.toString());
        }

        message.flash = first.value(QStringLiteral("flash")).toBool(false);
        message.marquee = first.value(QStringLiteral("marquee")).toBool(false);
        message.speedIndex = BadgeEncoder::speedIndexFromHex(first.value(QStringLiteral("speed")).toString());
        message.modeIndex = BadgeEncoder::modeIndexFromHex(first.value(QStringLiteral("mode")).toString());
    }

    return message;
}

}

QString BadgeStore::storageDirectory() const
{
    const QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return appDataPath + QStringLiteral("/badges");
}

QVariantList BadgeStore::loadBadges() const
{
    QVariantList badges;
    const QDir directory(storageDirectory());
    if (!directory.exists()) {
        return badges;
    }

    const QFileInfoList files = directory.entryInfoList(
        QStringList{QStringLiteral("*.json")},
        QDir::Files,
        QDir::Name | QDir::IgnoreCase);

    for (const QFileInfo &fileInfo : files) {
        QFile file(fileInfo.absoluteFilePath());
        if (!file.open(QIODevice::ReadOnly)) {
            continue;
        }

        const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
        file.close();

        if (!document.isObject()) {
            continue;
        }

        const BadgeMessage message = fromJsonObject(document.object());
        badges.append(toBadgeMap(fileInfo.absoluteFilePath(), fileInfo.completeBaseName(), message));
    }

    return badges;
}

bool BadgeStore::saveBadge(const QString &name, const BadgeMessage &message) const
{
    const QString trimmedName = name.trimmed();
    if (trimmedName.isEmpty()) {
        return false;
    }

    QDir directory(storageDirectory());
    if (!directory.exists() && !directory.mkpath(QStringLiteral("."))) {
        return false;
    }

    const QString safeName = trimmedName.simplified().replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]+")),
                                                              QStringLiteral("_"));
    QFile file(directory.filePath(safeName + QStringLiteral(".json")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    const QJsonDocument document(toJsonObject(message));
    const qint64 written = file.write(document.toJson(QJsonDocument::Indented));
    file.close();
    return written >= 0;
}

bool BadgeStore::deleteBadge(const QString &filePath) const
{
    if (filePath.isEmpty()) {
        return false;
    }

    return QFile::remove(filePath);
}
