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

#include "badgeblemanager.h"

#include "bluezadapter.h"
#include "qbledevice.h"
#include "qbleservice.h"

#include <QMap>
#include <QTimer>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusReply>

typedef QMap<QString, QVariantMap> InterfaceList;
typedef QMap<QDBusObjectPath, InterfaceList> ManagedObjectList;

Q_DECLARE_METATYPE(InterfaceList)
Q_DECLARE_METATYPE(ManagedObjectList)

namespace {

const QString kBadgeServiceUuid = QStringLiteral("0000fee0-0000-1000-8000-00805f9b34fb");
const QString kBadgeCharacteristicUuid = QStringLiteral("0000fee1-0000-1000-8000-00805f9b34fb");
const char kBluezService[] = "org.bluez";
const char kBluezAdapterInterface[] = "org.bluez.Adapter1";
const char kBluezDeviceInterface[] = "org.bluez.Device1";
const char kBluezGattServiceInterface[] = "org.bluez.GattService1";
const char kDbusObjectManagerInterface[] = "org.freedesktop.DBus.ObjectManager";
const int kScanTimeoutMs = 16000;
const int kScanPollIntervalMs = 750;
const int kResolvePollIntervalMs = 500;
const int kResolveAttempts = 24;
const int kWriteAttempts = 3;

QString badgeNotFound()
{
    //% "Badge not found. Make sure it is powered on."
    return qtTrId("badgemagic-sailfish-la-badge-not-found");
}

QString transferAlreadyInProgress()
{
    //% "A transfer is already in progress."
    return qtTrId("badgemagic-sailfish-la-transfer-already-in-progress");
}

QString noBadgeDataToSend()
{
    //% "There is no badge data to send."
    return qtTrId("badgemagic-sailfish-la-no-badge-data-to-send");
}

QString bluetoothNotAvailable()
{
    //% "Bluetooth is not available on this device."
    return qtTrId("badgemagic-sailfish-la-bluetooth-not-available");
}

QString bluetoothTurnedOff()
{
    //% "Bluetooth is turned off. Turn it on and retry."
    return qtTrId("badgemagic-sailfish-la-bluetooth-turned-off");
}

QString connectingToBadge()
{
    //% "Connecting to badge…"
    return qtTrId("badgemagic-sailfish-la-connecting-to-badge");
}

QString bluetoothConnectionFailed()
{
    //% "Bluetooth connection failed."
    return qtTrId("badgemagic-sailfish-la-bluetooth-connection-failed");
}

QString badgeDisconnectedBeforeTransferCompleted()
{
    //% "Badge disconnected before transfer completed."
    return qtTrId("badgemagic-sailfish-la-badge-disconnected-before-transfer-completed");
}

QString scanningForBadge()
{
    //% "Scanning for badge…"
    return qtTrId("badgemagic-sailfish-la-scanning-for-badge");
}

QString sendingBadgeData()
{
    //% "Sending badge data…"
    return qtTrId("badgemagic-sailfish-la-sending-badge-data");
}

QString unsupportedBadgeDevice()
{
    //% "The connected device is not a supported badge."
    return qtTrId("badgemagic-sailfish-la-unsupported-badge-device");
}

QString resolvingBadgeServices()
{
    //% "Resolving badge services…"
    return qtTrId("badgemagic-sailfish-la-resolving-badge-services");
}

QString incompleteTransferState()
{
    //% "Bluetooth transfer state is incomplete."
    return qtTrId("badgemagic-sailfish-la-incomplete-transfer-state");
}

QString transferQueueOutOfRange()
{
    //% "Transfer queue is out of range."
    return qtTrId("badgemagic-sailfish-la-transfer-queue-out-of-range");
}

QString badgeCharacteristicNotWritable()
{
    //% "The badge characteristic is not writable."
    return qtTrId("badgemagic-sailfish-la-badge-characteristic-not-writable");
}

QString badgeUpdatedSuccessfully()
{
    //% "Badge updated successfully."
    return qtTrId("badgemagic-sailfish-la-badge-updated-successfully");
}

QString writingBadgeDataFailed()
{
    //% "Writing badge data failed."
    return qtTrId("badgemagic-sailfish-la-writing-badge-data-failed");
}

ManagedObjectList managedBluezObjects()
{
    qDBusRegisterMetaType<InterfaceList>();
    qDBusRegisterMetaType<ManagedObjectList>();

    QDBusInterface objectManager(QString::fromLatin1(kBluezService),
                                 QStringLiteral("/"),
                                 QString::fromLatin1(kDbusObjectManagerInterface),
                                 QDBusConnection::systemBus());
    QDBusReply<ManagedObjectList> reply = objectManager.call(QStringLiteral("GetManagedObjects"));
    if (!reply.isValid()) {
        return {};
    }

    return reply.value();
}

QString lowerUuid(const QString &uuid)
{
    return uuid.toLower();
}

} // namespace

BadgeBleManager::BadgeBleManager(QObject *parent)
    : QObject(parent)
    , m_scanTimeout(new QTimer(this))
    , m_scanPollTimer(new QTimer(this))
    , m_resolveTimer(new QTimer(this))
{
    m_scanTimeout->setSingleShot(true);
    connect(m_scanTimeout, &QTimer::timeout, this, &BadgeBleManager::handleScanTimeout);

    m_scanPollTimer->setInterval(kScanPollIntervalMs);
    connect(m_scanPollTimer, &QTimer::timeout, this, &BadgeBleManager::handleScanPollTimeout);

    m_resolveTimer->setSingleShot(true);
    connect(m_resolveTimer, &QTimer::timeout, this, &BadgeBleManager::handleResolveTimeout);
}

BadgeBleManager::~BadgeBleManager()
{
    resetConnection();
}

bool BadgeBleManager::busy() const
{
    return m_busy;
}

void BadgeBleManager::sendChunks(const QList<QByteArray> &chunks)
{
    if (m_busy) {
        emit errorOccurred(transferAlreadyInProgress());
        return;
    }

    if (chunks.isEmpty()) {
        emit errorOccurred(noBadgeDataToSend());
        return;
    }

    resetConnection();
    m_pendingChunks = chunks;
    m_writeIndex = 0;
    m_resolveAttempts = 0;
    setBusy(true);

    bool hasAdapter = false;
    m_adapterPath = findPoweredAdapterPath(&hasAdapter);
    if (!hasAdapter) {
        finishWithError(bluetoothNotAvailable());
        return;
    }

    if (m_adapterPath.isEmpty()) {
        finishWithError(bluetoothTurnedOff());
        return;
    }

    if (m_adapter == nullptr) {
        m_adapter = new BluezAdapter(this);
    }
    m_adapter->setAdapterPath(m_adapterPath);

    beginDiscovery();
}

void BadgeBleManager::handleScanPollTimeout()
{
    const QString devicePath = findBadgeDevicePath();
    if (!devicePath.isEmpty()) {
        connectToDevicePath(devicePath);
    }
}

void BadgeBleManager::handleScanTimeout()
{
    if (!m_busy || !m_devicePath.isEmpty()) {
        return;
    }

    finishWithError(badgeNotFound());
}

void BadgeBleManager::handleResolveTimeout()
{
    attemptResolveService();
}

void BadgeBleManager::handleDevicePropertiesChanged(const QString &interface,
                                                    const QVariantMap &map,
                                                    const QStringList &list)
{
    Q_UNUSED(list)

    if (!m_busy || interface != QLatin1String(kBluezDeviceInterface)) {
        return;
    }

    if (map.contains(QStringLiteral("Connected"))) {
        const bool connected = map.value(QStringLiteral("Connected")).toBool();
        if (!connected && m_writeIndex < m_pendingChunks.size()) {
            finishWithError(badgeDisconnectedBeforeTransferCompleted());
            return;
        }

        if (connected) {
            attemptResolveService();
        }
    }

    if (map.contains(QStringLiteral("ServicesResolved")) &&
        map.value(QStringLiteral("ServicesResolved")).toBool()) {
        attemptResolveService();
    }
}

void BadgeBleManager::handleDeviceError(const QString &message)
{
    Q_UNUSED(message)

    if (m_busy) {
        finishWithError(bluetoothConnectionFailed());
    }
}

void BadgeBleManager::beginDiscovery()
{
    if (!m_busy || m_adapter == nullptr) {
        return;
    }

    emit statusChanged(scanningForBadge());

    const QString devicePath = findBadgeDevicePath();
    if (!devicePath.isEmpty()) {
        connectToDevicePath(devicePath);
        return;
    }

    m_adapter->startDiscovery();
    m_scanTimeout->start(kScanTimeoutMs);
    m_scanPollTimer->start();
}

void BadgeBleManager::connectToDevicePath(const QString &devicePath)
{
    if (!m_busy || devicePath.isEmpty()) {
        return;
    }

    m_scanTimeout->stop();
    m_scanPollTimer->stop();
    stopDiscovery();

    if (m_service != nullptr) {
        m_service->deleteLater();
        m_service = nullptr;
    }

    if (m_device != nullptr) {
        m_device->disconnect(this);
        m_device->deleteLater();
        m_device = nullptr;
    }

    m_devicePath = devicePath;
    m_device = new QBLEDevice(this);
    m_device->setDevicePath(devicePath);
    connect(m_device, &QBLEDevice::propertiesChanged,
            this, &BadgeBleManager::handleDevicePropertiesChanged);
    connect(m_device, &QBLEDevice::error,
            this, &BadgeBleManager::handleDeviceError);

    emit statusChanged(connectingToBadge());

    const ManagedObjectList objects = managedBluezObjects();
    const QVariantMap deviceProperties = objects.value(QDBusObjectPath(devicePath))
            .value(QString::fromLatin1(kBluezDeviceInterface));
    if (deviceProperties.value(QStringLiteral("Connected")).toBool()) {
        attemptResolveService();
        return;
    }

    m_device->connectToDevice();
}

void BadgeBleManager::attemptResolveService()
{
    if (!m_busy || m_device == nullptr) {
        return;
    }

    const QString servicePath = findBadgeServicePath();
    if (!servicePath.isEmpty()) {
        if (m_service == nullptr) {
            if (m_service != nullptr) {
                m_service->deleteLater();
            }

            m_service = new QBLEService(kBadgeServiceUuid, servicePath, this);
        }

        if (m_service->characteristic(kBadgeCharacteristicUuid) != nullptr) {
            m_resolveTimer->stop();
            m_resolveAttempts = 0;
            emit statusChanged(sendingBadgeData());
            writeNextChunk();
            return;
        }
    }

    emit statusChanged(resolvingBadgeServices());
    ++m_resolveAttempts;
    if (m_resolveAttempts >= kResolveAttempts) {
        finishWithError(unsupportedBadgeDevice());
        return;
    }

    m_resolveTimer->start(kResolvePollIntervalMs);
}

void BadgeBleManager::writeNextChunk()
{
    if (!m_busy || m_service == nullptr) {
        finishWithError(incompleteTransferState());
        return;
    }

    if (m_writeIndex < 0 || m_writeIndex >= m_pendingChunks.size()) {
        finishWithError(transferQueueOutOfRange());
        return;
    }

    if (m_service->characteristic(kBadgeCharacteristicUuid) == nullptr) {
        finishWithError(badgeCharacteristicNotWritable());
        return;
    }

    while (m_busy && m_writeIndex < m_pendingChunks.size()) {
        bool written = false;
        QString writeError;

        for (int attempt = 0; attempt < kWriteAttempts; ++attempt) {
            if (m_service->writeValueChecked(kBadgeCharacteristicUuid,
                                             m_pendingChunks.at(m_writeIndex),
                                             &writeError)) {
                written = true;
                break;
            }
        }

        if (!written) {
            qWarning() << Q_FUNC_INFO << writeError;
            finishWithError(writingBadgeDataFailed());
            return;
        }

        ++m_writeIndex;
    }

    if (m_busy) {
        finishTransferSuccessfully();
    }
}

void BadgeBleManager::finishTransferSuccessfully()
{
    emit statusChanged(badgeUpdatedSuccessfully());
    if (m_device != nullptr) {
        m_device->disconnect(this);
        m_device->disconnectFromDevice();
    }
    setBusy(false);
    m_pendingChunks.clear();
    m_writeIndex = 0;
    emit transferFinished();
}

void BadgeBleManager::finishWithError(const QString &error)
{
    resetConnection();
    setBusy(false);
    emit errorOccurred(error);
}

void BadgeBleManager::resetConnection()
{
    m_scanTimeout->stop();
    m_scanPollTimer->stop();
    m_resolveTimer->stop();
    stopDiscovery();

    if (m_service != nullptr) {
        m_service->deleteLater();
        m_service = nullptr;
    }

    if (m_device != nullptr) {
        m_device->disconnect(this);
        m_device->disconnectFromDevice();
        m_device->deleteLater();
        m_device = nullptr;
    }

    m_pendingChunks.clear();
    m_writeIndex = 0;
    m_resolveAttempts = 0;
    m_devicePath.clear();
}

void BadgeBleManager::setBusy(bool value)
{
    if (m_busy == value) {
        return;
    }

    m_busy = value;
    emit busyChanged();
}

void BadgeBleManager::stopDiscovery()
{
    if (m_adapter != nullptr && !m_adapterPath.isEmpty()) {
        m_adapter->stopDiscovery();
    }
}

QString BadgeBleManager::findPoweredAdapterPath(bool *hasAdapter) const
{
    const ManagedObjectList objects = managedBluezObjects();
    QString firstAdapterPath;

    for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
        const QVariantMap adapterProperties = it.value().value(QString::fromLatin1(kBluezAdapterInterface));
        if (adapterProperties.isEmpty()) {
            continue;
        }

        if (hasAdapter != nullptr) {
            *hasAdapter = true;
        }

        if (firstAdapterPath.isEmpty()) {
            firstAdapterPath = it.key().path();
        }

        if (adapterProperties.value(QStringLiteral("Powered")).toBool()) {
            return it.key().path();
        }
    }

    return {};
}

QString BadgeBleManager::findBadgeDevicePath() const
{
    if (m_adapterPath.isEmpty()) {
        return {};
    }

    const ManagedObjectList objects = managedBluezObjects();
    const QString adapterPrefix = m_adapterPath + QLatin1Char('/');
    const QString wantedUuid = lowerUuid(kBadgeServiceUuid);

    for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
        const QString objectPath = it.key().path();
        if (!objectPath.startsWith(adapterPrefix)) {
            continue;
        }

        const QVariantMap deviceProperties = it.value().value(QString::fromLatin1(kBluezDeviceInterface));
        if (deviceProperties.isEmpty()) {
            continue;
        }

        const QStringList uuids = deviceProperties.value(QStringLiteral("UUIDs")).toStringList();
        for (const QString &uuid : uuids) {
            if (lowerUuid(uuid) == wantedUuid) {
                return objectPath;
            }
        }
    }

    return {};
}

QString BadgeBleManager::findBadgeServicePath() const
{
    if (m_devicePath.isEmpty()) {
        return {};
    }

    const ManagedObjectList objects = managedBluezObjects();
    const QString devicePrefix = m_devicePath + QLatin1Char('/');

    for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
        const QString objectPath = it.key().path();
        if (!objectPath.startsWith(devicePrefix)) {
            continue;
        }

        const QVariantMap serviceProperties = it.value().value(QString::fromLatin1(kBluezGattServiceInterface));
        if (!serviceProperties.isEmpty() &&
            matchesBadgeServiceUuid(serviceProperties.value(QStringLiteral("UUID")).toString())) {
            return objectPath;
        }
    }

    return {};
}

bool BadgeBleManager::matchesBadgeServiceUuid(const QString &uuid) const
{
    return lowerUuid(uuid) == lowerUuid(kBadgeServiceUuid);
}

bool BadgeBleManager::matchesBadgeCharacteristicUuid(const QString &uuid) const
{
    return lowerUuid(uuid) == lowerUuid(kBadgeCharacteristicUuid);
}
