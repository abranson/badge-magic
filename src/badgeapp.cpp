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

#include "badgeapp.h"

#include <QCoreApplication>
#include <QVariantMap>

#include "badgeencoder.h"

namespace {

QString enterMessageBeforeSending()
{
    //% "Enter a message before sending."
    return qtTrId("badgemagic-sailfish-la-enter-message-before-sending");
}

QString messageContainsNoSupportedCharacters()
{
    //% "The message does not contain any supported badge characters."
    return qtTrId("badgemagic-sailfish-la-message-no-supported-characters");
}

QString enterMessageBeforeSaving()
{
    //% "Enter a message before saving."
    return qtTrId("badgemagic-sailfish-la-enter-message-before-saving");
}

QString savingBadgeFailed()
{
    //% "Saving the badge failed."
    return qtTrId("badgemagic-sailfish-la-saving-badge-failed");
}

QString savedBadge(const QString &name)
{
    //% "Saved \"%1\"."
    return qtTrId("badgemagic-sailfish-la-saved-badge").arg(name);
}

QString selectedSavedBadgeUnavailable()
{
    //% "The selected saved badge is no longer available."
    return qtTrId("badgemagic-sailfish-la-selected-saved-badge-unavailable");
}

QString deletedBadge(const QString &name)
{
    //% "Deleted \"%1\"."
    return qtTrId("badgemagic-sailfish-la-deleted-badge").arg(name);
}

QString deletingSavedBadgeFailed()
{
    //% "Deleting the saved badge failed."
    return qtTrId("badgemagic-sailfish-la-deleting-saved-badge-failed");
}

QString selectSavedBadgeSlotsBeforeSending()
{
    //% "Select at least one saved badge slot before sending."
    return qtTrId("badgemagic-sailfish-la-select-saved-badge-slots-before-sending");
}

QString tooManySavedBadgeSlotsSelected()
{
    //% "You can send up to 8 saved badge slots at once."
    return qtTrId("badgemagic-sailfish-la-too-many-saved-badge-slots-selected");
}

}

BadgeApp::BadgeApp(QObject *parent)
    : QObject(parent)
{
    connect(&m_bleManager, &BadgeBleManager::busyChanged, this, &BadgeApp::busyChanged);
    connect(&m_bleManager, &BadgeBleManager::statusChanged, this, [this](const QString &status) {
        setLastError(QString());
        setStatusMessage(status);
    });
    connect(&m_bleManager, &BadgeBleManager::errorOccurred, this, [this](const QString &error) {
        setStatusMessage(QString());
        setLastError(error);
    });

    reloadSavedBadges();
}

bool BadgeApp::busy() const
{
    return m_bleManager.busy();
}

QString BadgeApp::statusMessage() const
{
    return m_statusMessage;
}

QString BadgeApp::lastError() const
{
    return m_lastError;
}

QVariantList BadgeApp::savedBadges() const
{
    return m_savedBadges;
}

void BadgeApp::sendTextBadge(const QString &text,
                             bool flash,
                             bool marquee,
                             int speedIndex,
                             int modeIndex)
{
    const QString trimmedText = text.trimmed();
    if (trimmedText.isEmpty()) {
        setStatusMessage(QString());
        setLastError(enterMessageBeforeSending());
        return;
    }

    const BadgeMessage message = buildMessage(trimmedText, flash, marquee, speedIndex, modeIndex);
    if (message.textHex.isEmpty()) {
        setStatusMessage(QString());
        setLastError(messageContainsNoSupportedCharacters());
        return;
    }

    setLastError(QString());
    m_bleManager.sendChunks(BadgeEncoder::buildTransferChunks(message));
}

bool BadgeApp::saveTextBadge(const QString &name,
                             const QString &text,
                             bool flash,
                             bool marquee,
                             int speedIndex,
                             int modeIndex)
{
    const QString trimmedText = text.trimmed();
    if (trimmedText.isEmpty()) {
        setStatusMessage(QString());
        setLastError(enterMessageBeforeSaving());
        return false;
    }

    const BadgeMessage message = buildMessage(trimmedText, flash, marquee, speedIndex, modeIndex);
    if (message.textHex.isEmpty()) {
        setStatusMessage(QString());
        setLastError(messageContainsNoSupportedCharacters());
        return false;
    }

    const QString finalName = name.trimmed().isEmpty() ? defaultNameForText(trimmedText) : name.trimmed();
    const bool saved = m_store.saveBadge(finalName, message);
    if (!saved) {
        setStatusMessage(QString());
        setLastError(savingBadgeFailed());
        return false;
    }

    reloadSavedBadges();
    setLastError(QString());
    setStatusMessage(savedBadge(finalName));
    return true;
}

QVariantMap BadgeApp::loadSavedBadge(int index) const
{
    if (index < 0 || index >= m_savedBadges.size()) {
        return {};
    }

    return m_savedBadges.at(index).toMap();
}

void BadgeApp::sendSavedBadge(int index)
{
    if (index < 0 || index >= m_savedBadges.size()) {
        setStatusMessage(QString());
        setLastError(selectedSavedBadgeUnavailable());
        return;
    }

    setLastError(QString());
    m_bleManager.sendChunks(BadgeEncoder::buildTransferChunks(messageFromVariant(m_savedBadges.at(index).toMap())));
}

void BadgeApp::sendSavedBadgeSlots(const QVariantList &indexes)
{
    if (indexes.isEmpty()) {
        setStatusMessage(QString());
        setLastError(selectSavedBadgeSlotsBeforeSending());
        return;
    }

    if (indexes.size() > 8) {
        setStatusMessage(QString());
        setLastError(tooManySavedBadgeSlotsSelected());
        return;
    }

    QList<BadgeMessage> messages;
    messages.reserve(indexes.size());

    for (const QVariant &value : indexes) {
        bool ok = false;
        const int index = value.toInt(&ok);
        if (!ok || index < 0 || index >= m_savedBadges.size()) {
            setStatusMessage(QString());
            setLastError(selectedSavedBadgeUnavailable());
            return;
        }

        messages.append(messageFromVariant(m_savedBadges.at(index).toMap()));
    }

    setLastError(QString());
    m_bleManager.sendChunks(BadgeEncoder::buildTransferChunks(messages));
}

bool BadgeApp::deleteSavedBadge(int index)
{
    if (index < 0 || index >= m_savedBadges.size()) {
        return false;
    }

    const QVariantMap item = m_savedBadges.at(index).toMap();
    const bool deleted = m_store.deleteBadge(item.value(QStringLiteral("filePath")).toString());
    if (deleted) {
        reloadSavedBadges();
        setLastError(QString());
        setStatusMessage(deletedBadge(item.value(QStringLiteral("name")).toString()));
    } else {
        setStatusMessage(QString());
        setLastError(deletingSavedBadgeFailed());
    }

    return deleted;
}

void BadgeApp::reloadSavedBadges()
{
    m_savedBadges = m_store.loadBadges();
    emit savedBadgesChanged();
}

QString BadgeApp::defaultNameForText(const QString &text) const
{
    const QString simplified = text.simplified();
    if (simplified.isEmpty()) {
        return QStringLiteral("badge");
    }

    return simplified.left(24);
}

void BadgeApp::setStatusMessage(const QString &status)
{
    if (m_statusMessage == status) {
        return;
    }

    m_statusMessage = status;
    emit statusMessageChanged();
}

void BadgeApp::setLastError(const QString &error)
{
    if (m_lastError == error) {
        return;
    }

    m_lastError = error;
    emit lastErrorChanged();
}

BadgeMessage BadgeApp::buildMessage(const QString &text,
                                    bool flash,
                                    bool marquee,
                                    int speedIndex,
                                    int modeIndex) const
{
    BadgeMessage message;
    message.rawText = text;
    message.textHex = BadgeEncoder::encodeText(text);
    message.flash = flash;
    message.marquee = marquee;
    message.speedIndex = speedIndex;
    message.modeIndex = modeIndex;
    return message;
}

BadgeMessage BadgeApp::messageFromVariant(const QVariantMap &item) const
{
    BadgeMessage message;
    message.rawText = item.value(QStringLiteral("rawText")).toString();
    message.textHex = item.value(QStringLiteral("hexText")).toStringList();
    message.flash = item.value(QStringLiteral("flash")).toBool();
    message.marquee = item.value(QStringLiteral("marquee")).toBool();
    message.speedIndex = item.value(QStringLiteral("speedIndex")).toInt();
    message.modeIndex = item.value(QStringLiteral("modeIndex")).toInt();
    return message;
}
