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

#ifndef BADGEBLEMANAGER_H
#define BADGEBLEMANAGER_H

#include <QObject>
#include <QByteArray>
#include <QList>
#include <QTimer>
#include <QVariant>
#include <functional>

QT_FORWARD_DECLARE_CLASS(QDBusMessage)

class BadgeBleManager : public QObject
{
    Q_OBJECT

public:
    explicit BadgeBleManager(QObject *parent = nullptr);
    ~BadgeBleManager() override;

    bool busy() const;
    void sendChunks(const QList<QByteArray> &chunks);

signals:
    void busyChanged();
    void statusChanged(const QString &status);
    void errorOccurred(const QString &error);
    void transferFinished();

private:
    enum class State { Idle, Scanning, Connecting, Resolving, Writing, Disconnecting };
    using ReplyHandler = std::function<void(const QDBusMessage &)>;

    void call(const QString &path, const QString &interface, const QString &method,
              const QList<QVariant> &arguments, const ReplyHandler &handler);
    void refreshObjects();
    void startDiscovery();
    void stopDiscovery(const std::function<void()> &finished);
    void connectToBadge();
    void writeNextChunk();
    void finish(const QString &error = QString());
    void complete(const QString &error);

    State m_state = State::Idle;
    quint64 m_generation = 0;
    bool m_discovering = false;
    int m_connectAttempts = 0;
    int m_writeIndex = 0;
    int m_writeAttempts = 0;
    QList<QByteArray> m_pendingChunks;
    QString m_adapterPath;
    QString m_devicePath;
    QString m_characteristicPath;
    QTimer m_stepTimer;
    QTimer m_phaseTimeout;
};

#endif
