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

#ifndef BADGEAPP_H
#define BADGEAPP_H

#include <QObject>
#include <QVariantList>

#include "badgeblemanager.h"
#include "badgestore.h"

class BadgeApp : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(QVariantList savedBadges READ savedBadges NOTIFY savedBadgesChanged)

public:
    explicit BadgeApp(QObject *parent = nullptr);

    bool busy() const;
    QString statusMessage() const;
    QString lastError() const;
    QVariantList savedBadges() const;

    Q_INVOKABLE void sendTextBadge(const QString &text,
                                   bool flash,
                                   bool marquee,
                                   int speedIndex,
                                   int modeIndex);
    Q_INVOKABLE bool saveTextBadge(const QString &name,
                                   const QString &text,
                                   bool flash,
                                   bool marquee,
                                   int speedIndex,
                                   int modeIndex);
    Q_INVOKABLE QVariantMap loadSavedBadge(int index) const;
    Q_INVOKABLE void sendSavedBadge(int index);
    Q_INVOKABLE void sendSavedBadgeSlots(const QVariantList &indexes);
    Q_INVOKABLE bool deleteSavedBadge(int index);

signals:
    void busyChanged();
    void statusMessageChanged();
    void lastErrorChanged();
    void savedBadgesChanged();

private:
    void reloadSavedBadges();
    QString defaultNameForText(const QString &text) const;
    void setStatusMessage(const QString &status);
    void setLastError(const QString &error);
    BadgeMessage buildMessage(const QString &text,
                              bool flash,
                              bool marquee,
                              int speedIndex,
                              int modeIndex) const;
    BadgeMessage messageFromVariant(const QVariantMap &item) const;

    QString m_statusMessage;
    QString m_lastError;
    QVariantList m_savedBadges;

    BadgeStore m_store;
    BadgeBleManager m_bleManager;
};

#endif
