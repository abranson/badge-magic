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

#ifndef BADGEENCODER_H
#define BADGEENCODER_H

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

struct BadgeMessage
{
    QString rawText;
    QStringList textHex;
    bool flash = false;
    bool marquee = false;
    int speedIndex = 0;
    int modeIndex = 0;
};

class BadgeEncoder
{
public:
    static QStringList encodeText(const QString &text);
    static QList<QByteArray> buildTransferChunks(const BadgeMessage &message);
    static QList<QByteArray> buildTransferChunks(const QList<BadgeMessage> &messages);

    static QString speedLabel(int speedIndex);
    static QString modeLabel(int modeIndex);
    static QString speedHex(int speedIndex);
    static QString modeHex(int modeIndex);
    static int speedIndexFromHex(const QString &hexValue);
    static int modeIndexFromHex(const QString &hexValue);

private:
    static QString getFlash(const QList<BadgeMessage> &messages);
    static QString getMarquee(const QList<BadgeMessage> &messages);
    static QString getOptions(const QList<BadgeMessage> &messages);
    static QString getSizes(const QList<BadgeMessage> &messages);
    static QString getMessage(const QList<BadgeMessage> &messages);
    static QString getTime();
    static QString fillZeros(int length);
};

#endif
