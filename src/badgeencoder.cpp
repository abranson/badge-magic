/*
 * SPDX-FileCopyrightText: 2026 Badge Magic for SailfishOS contributors
 * SPDX-License-Identifier: Apache-2.0
 *
 * Copyright (C) 2026 Badge Magic for SailfishOS contributors
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

#include "badgeencoder.h"

#include <QDateTime>
#include <QHash>
#include <QtGlobal>

namespace {

constexpr int kPacketByteSize = 16;
constexpr int kModeLabelCount = 9;
const char kPacketStart[] = "77616E670000";

const QStringList kSpeedLabels = {
    QStringLiteral("1"),
    QStringLiteral("2"),
    QStringLiteral("3"),
    QStringLiteral("4"),
    QStringLiteral("5"),
    QStringLiteral("6"),
    QStringLiteral("7"),
    QStringLiteral("8"),
};

const QStringList kSpeedHexValues = {
    QStringLiteral("0x00"),
    QStringLiteral("0x10"),
    QStringLiteral("0x20"),
    QStringLiteral("0x30"),
    QStringLiteral("0x40"),
    QStringLiteral("0x50"),
    QStringLiteral("0x60"),
    QStringLiteral("0x70"),
};

const QStringList kModeHexValues = {
    QStringLiteral("0x00"),
    QStringLiteral("0x01"),
    QStringLiteral("0x02"),
    QStringLiteral("0x03"),
    QStringLiteral("0x04"),
    QStringLiteral("0x05"),
    QStringLiteral("0x06"),
    QStringLiteral("0x07"),
    QStringLiteral("0x08"),
};

const QHash<QChar, QString> kCharCodes = {
    {QChar('0'), QStringLiteral("007CC6CEDEF6E6C6C67C00")},
    {QChar('1'), QStringLiteral("0018387818181818187E00")},
    {QChar('2'), QStringLiteral("007CC6060C183060C6FE00")},
    {QChar('3'), QStringLiteral("007CC606063C0606C67C00")},
    {QChar('4'), QStringLiteral("000C1C3C6CCCFE0C0C1E00")},
    {QChar('5'), QStringLiteral("00FEC0C0FC060606C67C00")},
    {QChar('6'), QStringLiteral("007CC6C0C0FCC6C6C67C00")},
    {QChar('7'), QStringLiteral("00FEC6060C183030303000")},
    {QChar('8'), QStringLiteral("007CC6C6C67CC6C6C67C00")},
    {QChar('9'), QStringLiteral("007CC6C6C67E0606C67C00")},
    {QChar('#'), QStringLiteral("006C6CFE6C6CFE6C6C0000")},
    {QChar('&'), QStringLiteral("00386C6C3876DCCCCC7600")},
    {QChar('_'), QStringLiteral("00000000000000000000FF")},
    {QChar('-'), QStringLiteral("0000000000FE0000000000")},
    {QChar('?'), QStringLiteral("007CC6C60C181800181800")},
    {QChar('@'), QStringLiteral("00003C429DA5ADB6403C00")},
    {QChar('('), QStringLiteral("000C183030303030180C00")},
    {QChar(')'), QStringLiteral("0030180C0C0C0C0C183000")},
    {QChar('='), QStringLiteral("0000007E00007E00000000")},
    {QChar('+'), QStringLiteral("00000018187E1818000000")},
    {QChar('!'), QStringLiteral("00183C3C3C181800181800")},
    {QChar('\''), QStringLiteral("1818081000000000000000")},
    {QChar(':'), QStringLiteral("0000001818000018180000")},
    {QChar('%'), QStringLiteral("006092966C106CD2920C00")},
    {QChar('/'), QStringLiteral("000002060C183060C08000")},
    {QChar('"'), QStringLiteral("6666222200000000000000")},
    {QChar('['), QStringLiteral("003C303030303030303C00")},
    {QChar(']'), QStringLiteral("003C0C0C0C0C0C0C0C3C00")},
    {QChar(' '), QStringLiteral("0000000000000000000000")},
    {QChar('*'), QStringLiteral("000000663CFF3C66000000")},
    {QChar(','), QStringLiteral("0000000000000030301020")},
    {QChar('.'), QStringLiteral("0000000000000000303000")},
    {QChar('$'), QStringLiteral("107CD6D6701CD6D67C1010")},
    {QChar('~'), QStringLiteral("0076DC0000000000000000")},
    {QChar('{'), QStringLiteral("000E181818701818180E00")},
    {QChar('}'), QStringLiteral("00701818180E1818187000")},
    {QChar('<'), QStringLiteral("00060C18306030180C0600")},
    {QChar('>'), QStringLiteral("006030180C060C18306000")},
    {QChar('^'), QStringLiteral("386CC60000000000000000")},
    {QChar('`'), QStringLiteral("1818100800000000000000")},
    {QChar(';'), QStringLiteral("0000001818000018180810")},
    {QChar('\\'), QStringLiteral("0080C06030180C06020000")},
    {QChar('|'), QStringLiteral("0018181818001818181800")},
    {QChar('a'), QStringLiteral("00000000780C7CCCCC7600")},
    {QChar('b'), QStringLiteral("00E060607C666666667C00")},
    {QChar('c'), QStringLiteral("000000007CC6C0C0C67C00")},
    {QChar('d'), QStringLiteral("001C0C0C7CCCCCCCCC7600")},
    {QChar('e'), QStringLiteral("000000007CC6FEC0C67C00")},
    {QChar('f'), QStringLiteral("001C363078303030307800")},
    {QChar('g'), QStringLiteral("00000076CCCCCC7C0CCC78")},
    {QChar('h'), QStringLiteral("00E060606C76666666E600")},
    {QChar('i'), QStringLiteral("0018180038181818183C00")},
    {QChar('j'), QStringLiteral("0C0C001C0C0C0C0CCCCC78")},
    {QChar('k'), QStringLiteral("00E06060666C78786CE600")},
    {QChar('l'), QStringLiteral("0038181818181818183C00")},
    {QChar('m'), QStringLiteral("00000000ECFED6D6D6C600")},
    {QChar('n'), QStringLiteral("00000000DC666666666600")},
    {QChar('o'), QStringLiteral("000000007CC6C6C6C67C00")},
    {QChar('p'), QStringLiteral("000000DC6666667C6060F0")},
    {QChar('q'), QStringLiteral("0000007CCCCCCC7C0C0C1E")},
    {QChar('r'), QStringLiteral("00000000DE76606060F000")},
    {QChar('s'), QStringLiteral("000000007CC6701CC67C00")},
    {QChar('t'), QStringLiteral("00103030FC303030341800")},
    {QChar('u'), QStringLiteral("00000000CCCCCCCCCC7600")},
    {QChar('v'), QStringLiteral("00000000C6C6C66C381000")},
    {QChar('w'), QStringLiteral("00000000C6D6D6D6FE6C00")},
    {QChar('x'), QStringLiteral("00000000C66C38386CC600")},
    {QChar('y'), QStringLiteral("000000C6C6C6C67E060CF8")},
    {QChar('z'), QStringLiteral("00000000FE8C183062FE00")},
    {QChar('A'), QStringLiteral("00386CC6C6FEC6C6C6C600")},
    {QChar('B'), QStringLiteral("00FC6666667C666666FC00")},
    {QChar('C'), QStringLiteral("007CC6C6C0C0C0C6C67C00")},
    {QChar('D'), QStringLiteral("00FC66666666666666FC00")},
    {QChar('E'), QStringLiteral("00FE66626878686266FE00")},
    {QChar('F'), QStringLiteral("00FE66626878686060F000")},
    {QChar('G'), QStringLiteral("007CC6C6C0C0CEC6C67E00")},
    {QChar('H'), QStringLiteral("00C6C6C6C6FEC6C6C6C600")},
    {QChar('I'), QStringLiteral("003C181818181818183C00")},
    {QChar('J'), QStringLiteral("001E0C0C0C0C0CCCCC7800")},
    {QChar('K'), QStringLiteral("00E6666C6C786C6C66E600")},
    {QChar('L'), QStringLiteral("00F060606060606266FE00")},
    {QChar('M'), QStringLiteral("0082C6EEFED6C6C6C6C600")},
    {QChar('N'), QStringLiteral("0086C6E6F6DECEC6C6C600")},
    {QChar('O'), QStringLiteral("007CC6C6C6C6C6C6C67C00")},
    {QChar('P'), QStringLiteral("00FC6666667C606060F000")},
    {QChar('Q'), QStringLiteral("007CC6C6C6C6C6D6DE7C06")},
    {QChar('R'), QStringLiteral("00FC6666667C6C6666E600")},
    {QChar('S'), QStringLiteral("007CC6C660380CC6C67C00")},
    {QChar('T'), QStringLiteral("007E7E5A18181818183C00")},
    {QChar('U'), QStringLiteral("00C6C6C6C6C6C6C6C67C00")},
    {QChar('V'), QStringLiteral("00C6C6C6C6C6C66C381000")},
    {QChar('W'), QStringLiteral("00C6C6C6C6D6FEEEC68200")},
    {QChar('X'), QStringLiteral("00C6C66C7C387C6CC6C600")},
    {QChar('Y'), QStringLiteral("00666666663C1818183C00")},
    {QChar('Z'), QStringLiteral("00FEC6860C183062C6FE00")},
};

QString toHex(const QByteArray &bytes)
{
    return QString::fromLatin1(bytes.toHex()).toUpper();
}

int parseHexValue(const QString &hexValue)
{
    return hexValue.mid(2).toInt(nullptr, 16);
}

}

QStringList BadgeEncoder::encodeText(const QString &text)
{
    QStringList encoded;
    encoded.reserve(text.size());

    for (const QChar character : text) {
        const auto iterator = kCharCodes.constFind(character);
        if (iterator != kCharCodes.constEnd()) {
            encoded.append(iterator.value());
        }
    }

    return encoded;
}

QList<QByteArray> BadgeEncoder::buildTransferChunks(const BadgeMessage &message)
{
    QString payload = QString::fromLatin1(kPacketStart)
            + getFlash(message)
            + getMarquee(message)
            + getOptions(message)
            + getSize(message)
            + QStringLiteral("000000000000")
            + getTime()
            + QStringLiteral("0000000000000000000000000000000000000000")
            + message.textHex.join(QString());

    payload.append(fillZeros(payload.length()));

    QList<QByteArray> chunks;
    const int chunkSize = kPacketByteSize * 2;

    for (int index = 0; index < payload.length(); index += chunkSize) {
        const QString chunk = payload.mid(index, chunkSize);
        chunks.append(QByteArray::fromHex(chunk.toLatin1()));
    }

    return chunks;
}

QString BadgeEncoder::speedLabel(int speedIndex)
{
    const int safeIndex = qBound(0, speedIndex, kSpeedLabels.size() - 1);
    return kSpeedLabels.at(safeIndex);
}

QString BadgeEncoder::modeLabel(int modeIndex)
{
    switch (qBound(0, modeIndex, kModeLabelCount - 1)) {
    case 0:
        //% "Left"
        return qtTrId("badgemagic-sailfish-la-mode-left");
    case 1:
        //% "Right"
        return qtTrId("badgemagic-sailfish-la-mode-right");
    case 2:
        //% "Up"
        return qtTrId("badgemagic-sailfish-la-mode-up");
    case 3:
        //% "Down"
        return qtTrId("badgemagic-sailfish-la-mode-down");
    case 4:
        //% "Fixed"
        return qtTrId("badgemagic-sailfish-la-mode-fixed");
    case 5:
        //% "Snowflake"
        return qtTrId("badgemagic-sailfish-la-mode-snowflake");
    case 6:
        //% "Picture"
        return qtTrId("badgemagic-sailfish-la-mode-picture");
    case 7:
        //% "Animation"
        return qtTrId("badgemagic-sailfish-la-mode-animation");
    case 8:
    default:
        //% "Laser"
        return qtTrId("badgemagic-sailfish-la-mode-laser");
    }
}

QString BadgeEncoder::speedHex(int speedIndex)
{
    const int safeIndex = qBound(0, speedIndex, kSpeedHexValues.size() - 1);
    return kSpeedHexValues.at(safeIndex);
}

QString BadgeEncoder::modeHex(int modeIndex)
{
    const int safeIndex = qBound(0, modeIndex, kModeHexValues.size() - 1);
    return kModeHexValues.at(safeIndex);
}

int BadgeEncoder::speedIndexFromHex(const QString &hexValue)
{
    const int index = kSpeedHexValues.indexOf(hexValue);
    return index >= 0 ? index : 0;
}

int BadgeEncoder::modeIndexFromHex(const QString &hexValue)
{
    const int index = kModeHexValues.indexOf(hexValue);
    return index >= 0 ? index : 0;
}

QString BadgeEncoder::getFlash(const BadgeMessage &message)
{
    return toHex(QByteArray(1, message.flash ? char(0x01) : char(0x00)));
}

QString BadgeEncoder::getMarquee(const BadgeMessage &message)
{
    return toHex(QByteArray(1, message.marquee ? char(0x01) : char(0x00)));
}

QString BadgeEncoder::getOptions(const BadgeMessage &message)
{
    const int combined = parseHexValue(speedHex(message.speedIndex))
            | parseHexValue(modeHex(message.modeIndex));
    QString options = toHex(QByteArray(1, static_cast<char>(combined)));
    options = options.leftJustified(16, QLatin1Char('0'));
    return options;
}

QString BadgeEncoder::getSize(const BadgeMessage &message)
{
    const int length = message.textHex.size();
    QByteArray bytes;
    bytes.append(static_cast<char>((length >> 8) & 0xFF));
    bytes.append(static_cast<char>(length & 0xFF));

    QString size = toHex(bytes);
    size = size.leftJustified(32, QLatin1Char('0'));
    return size;
}

QString BadgeEncoder::getTime()
{
    const QDateTime now = QDateTime::currentDateTime();
    const int year = now.date().year() & 0xFF;

    QByteArray bytes;
    bytes.append(static_cast<char>(year));
    bytes.append(static_cast<char>(now.date().month() + 1));
    bytes.append(static_cast<char>(now.date().day()));
    bytes.append(static_cast<char>(now.time().hour()));
    bytes.append(static_cast<char>(now.time().minute()));
    bytes.append(static_cast<char>(now.time().second()));
    return toHex(bytes);
}

QString BadgeEncoder::fillZeros(int length)
{
    const int totalSize = (((length / (kPacketByteSize * 2)) + 1) * kPacketByteSize * 2) - length;
    return QString(totalSize, QLatin1Char('0'));
}
