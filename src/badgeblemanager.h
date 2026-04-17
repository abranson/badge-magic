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
#include <QStringList>
#include <QVariantMap>

QT_FORWARD_DECLARE_CLASS(QTimer)

class BluezAdapter;
class QBLEDevice;
class QBLEService;

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

private slots:
    void handleScanPollTimeout();
    void handleScanTimeout();
    void handleResolveTimeout();
    void handleCharacteristicWritten(const QString &characteristic, const QByteArray &value);
    void handleCharacteristicWriteFailed(const QString &characteristic, const QString &errorMessage);
    void handleDevicePropertiesChanged(const QString &interface,
                                       const QVariantMap &map,
                                       const QStringList &list);
    void handleDeviceError(const QString &message);

private:
    void beginDiscovery();
    void connectToDevicePath(const QString &devicePath);
    void attemptResolveService();
    void writeNextChunk();
    void finishTransferSuccessfully();
    void finishWithError(const QString &error);
    void resetConnection();
    void setBusy(bool value);
    void stopDiscovery();
    QString findPoweredAdapterPath(bool *hasAdapter) const;
    QString findBadgeDevicePath() const;
    QString findBadgeServicePath() const;
    bool matchesBadgeServiceUuid(const QString &uuid) const;
    bool matchesBadgeCharacteristicUuid(const QString &uuid) const;

    bool m_busy = false;
    int m_connectAttempts = 0;
    int m_writeIndex = 0;
    int m_writeAttempts = 0;
    int m_resolveAttempts = 0;
    QList<QByteArray> m_pendingChunks;
    QString m_adapterPath;
    QString m_devicePath;

    BluezAdapter *m_adapter = nullptr;
    QBLEDevice *m_device = nullptr;
    QBLEService *m_service = nullptr;
    QTimer *m_scanTimeout = nullptr;
    QTimer *m_scanPollTimer = nullptr;
    QTimer *m_resolveTimer = nullptr;
};

#endif
