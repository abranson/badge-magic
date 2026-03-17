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

#include <BluezQt/Adapter>
#include <BluezQt/Device>
#include <BluezQt/GattCharacteristicRemote>
#include <BluezQt/GattServiceRemote>
#include <BluezQt/InitManagerJob>
#include <BluezQt/Manager>
#include <BluezQt/PendingCall>

#include <QCoreApplication>
#include <QTimer>
#include <QStringList>
#include <QVariantMap>

namespace {

const QString kBadgeServiceUuid = QStringLiteral("0000FEE0-0000-1000-8000-00805F9B34FB");
const QString kBadgeCharacteristicUuid = QStringLiteral("0000FEE1-0000-1000-8000-00805F9B34FB");

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

QString bluetoothStillInitializing()
{
    //% "Bluetooth is still initializing. Please retry in a moment."
    return qtTrId("badgemagic-sailfish-la-bluetooth-still-initializing");
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

QString bluetoothInitializationFailed()
{
    //% "Bluetooth initialization failed."
    return qtTrId("badgemagic-sailfish-la-bluetooth-initialization-failed");
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

QString discoveryFilterUnavailable()
{
    //% "Discovery filter unavailable. Scanning all Bluetooth LE devices…"
    return qtTrId("badgemagic-sailfish-la-discovery-filter-unavailable");
}

QString bluetoothScanFailed()
{
    //% "Bluetooth scan failed."
    return qtTrId("badgemagic-sailfish-la-bluetooth-scan-failed");
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

QString bluetoothTransferFailed()
{
    //% "Bluetooth transfer failed."
    return qtTrId("badgemagic-sailfish-la-bluetooth-transfer-failed");
}

QString badgeUpdatedSuccessfully()
{
    //% "Badge updated successfully."
    return qtTrId("badgemagic-sailfish-la-badge-updated-successfully");
}

void watchPendingCallForDeletion(BluezQt::PendingCall *call)
{
    if (call != nullptr) {
        QObject::connect(call, &BluezQt::PendingCall::finished, call, &QObject::deleteLater);
    }
}

}

BadgeBleManager::BadgeBleManager(QObject *parent)
    : QObject(parent)
    , m_manager(new BluezQt::Manager(this))
    , m_scanTimeout(new QTimer(this))
{
    m_scanTimeout->setSingleShot(true);
    connect(m_scanTimeout, &QTimer::timeout, this, [this]() {
        if (m_device.isNull()) {
            finishWithError(badgeNotFound());
        }
    });

    BluezQt::InitManagerJob *initJob = m_manager->init();
    connect(initJob, &BluezQt::InitManagerJob::result,
            this, &BadgeBleManager::handleManagerInitResult);
    initJob->start();
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

    if (!m_managerReady) {
        emit errorOccurred(bluetoothStillInitializing());
        return;
    }

    if (!m_manager->isOperational()) {
        emit errorOccurred(bluetoothNotAvailable());
        return;
    }

    resetConnection();
    m_pendingChunks = chunks;
    m_writeIndex = 0;
    setBusy(true);

    m_adapter = m_manager->usableAdapter();
    if (m_adapter.isNull()) {
        finishWithError(bluetoothTurnedOff());
        return;
    }

    if (!m_adapter->isPowered()) {
        finishWithError(bluetoothTurnedOff());
        return;
    }

    connect(m_adapter.data(), &BluezQt::Adapter::deviceAdded,
            this, &BadgeBleManager::handleDeviceAdded);
    connect(m_adapter.data(), &BluezQt::Adapter::deviceChanged,
            this, &BadgeBleManager::handleDeviceChanged);

    beginDiscovery();
}

void BadgeBleManager::handleManagerInitResult(BluezQt::InitManagerJob *job)
{
    m_managerReady = (job->error() == BluezQt::Job::NoError);
    if (!m_managerReady && m_busy) {
        finishWithError(bluetoothInitializationFailed());
    }
}

void BadgeBleManager::handleDeviceAdded(BluezQt::DevicePtr device)
{
    if (m_discoveryFiltered && m_busy && m_device.isNull() && !device.isNull()) {
        m_scanTimeout->stop();
        stopDiscovery();

        m_device = device;
        connect(m_device.data(), &BluezQt::Device::connectedChanged,
                this, &BadgeBleManager::handleDeviceConnectedChanged);
        connect(m_device.data(), &BluezQt::Device::servicesResolvedChanged,
                this, &BadgeBleManager::handleDeviceServicesResolvedChanged);
        connect(m_device.data(), &BluezQt::Device::gattServicesChanged,
                this, &BadgeBleManager::handleGattServicesChanged);

        emit statusChanged(connectingToBadge());

        if (m_device->isConnected()) {
            attemptResolveCharacteristic();
            return;
        }

        BluezQt::PendingCall *connectCall = m_device->connectToDevice();
        watchPendingCallForDeletion(connectCall);
        if (connectCall == nullptr) {
            finishWithError(bluetoothConnectionFailed());
            return;
        }

        connect(connectCall, &BluezQt::PendingCall::finished, this, [this](BluezQt::PendingCall *call) {
            if (!m_busy || m_device.isNull()) {
                return;
            }

            if (call->error() != BluezQt::PendingCall::NoError &&
                call->error() != BluezQt::PendingCall::AlreadyConnected) {
                finishWithError(bluetoothConnectionFailed());
                return;
            }

            attemptResolveCharacteristic();
        });
        return;
    }

    tryUseDevice(device);
}

void BadgeBleManager::handleDeviceChanged(BluezQt::DevicePtr device)
{
    tryUseDevice(device);
}

void BadgeBleManager::handleDeviceConnectedChanged(bool connected)
{
    if (!m_busy) {
        return;
    }

    if (!connected && !m_device.isNull() && m_writeIndex < m_pendingChunks.size()) {
        finishWithError(badgeDisconnectedBeforeTransferCompleted());
        return;
    }

    if (connected) {
        attemptResolveCharacteristic();
    }
}

void BadgeBleManager::handleDeviceServicesResolvedChanged(bool resolved)
{
    if (resolved) {
        attemptResolveCharacteristic();
    }
}

void BadgeBleManager::handleGattServicesChanged(QList<BluezQt::GattServiceRemotePtr> services)
{
    Q_UNUSED(services)
    attemptResolveCharacteristic();
}

void BadgeBleManager::beginDiscovery()
{
    if (!m_busy || m_adapter.isNull()) {
        return;
    }

    m_discoveryFiltered = false;
    emit statusChanged(scanningForBadge());

    const QList<BluezQt::DevicePtr> knownDevices = m_adapter->devices();
    for (const BluezQt::DevicePtr &device : knownDevices) {
        if (tryUseDevice(device)) {
            return;
        }
    }

    QVariantMap filter;
    filter.insert(QStringLiteral("Transport"), QStringLiteral("le"));
    filter.insert(QStringLiteral("UUIDs"), QStringList{ kBadgeServiceUuid });

    BluezQt::PendingCall *filterCall = m_adapter->setDiscoveryFilter(filter);
    watchPendingCallForDeletion(filterCall);
    if (filterCall != nullptr) {
        connect(filterCall, &BluezQt::PendingCall::finished, this, [this](BluezQt::PendingCall *call) {
            if (!m_busy || !m_device.isNull()) {
                return;
            }

            if (call->error() != BluezQt::PendingCall::NoError &&
                call->error() != BluezQt::PendingCall::NotSupported) {
                emit statusChanged(discoveryFilterUnavailable());
            } else if (call->error() == BluezQt::PendingCall::NoError) {
                m_discoveryFiltered = true;
            }

            beginConnection();
        });
        return;
    }

    beginConnection();
}

void BadgeBleManager::beginConnection()
{
    if (!m_busy || m_adapter.isNull() || !m_device.isNull()) {
        return;
    }

    m_scanTimeout->start(16000);

    BluezQt::PendingCall *scanCall = m_adapter->startDiscovery();
    watchPendingCallForDeletion(scanCall);
    if (scanCall == nullptr) {
        finishWithError(bluetoothScanFailed());
        return;
    }

    connect(scanCall, &BluezQt::PendingCall::finished, this, [this](BluezQt::PendingCall *call) {
        if (!m_busy || !m_device.isNull()) {
            return;
        }

        if (call->error() != BluezQt::PendingCall::NoError &&
            call->error() != BluezQt::PendingCall::InProgress) {
            finishWithError(bluetoothScanFailed());
        }
    });
}

void BadgeBleManager::resetConnection()
{
    m_scanTimeout->stop();
    stopDiscovery();

    if (!m_adapter.isNull()) {
        disconnect(m_adapter.data(), nullptr, this, nullptr);
        m_adapter.clear();
    }

    if (!m_device.isNull()) {
        disconnect(m_device.data(), nullptr, this, nullptr);

        if (m_device->isConnected()) {
            BluezQt::PendingCall *disconnectCall = m_device->disconnectFromDevice();
            watchPendingCallForDeletion(disconnectCall);
        }

        m_device.clear();
    }

    m_characteristic.clear();
    m_discoveryFiltered = false;
    m_pendingChunks.clear();
    m_writeIndex = 0;
}

void BadgeBleManager::setBusy(bool value)
{
    if (m_busy == value) {
        return;
    }

    m_busy = value;
    emit busyChanged();
}

void BadgeBleManager::finishWithError(const QString &error)
{
    resetConnection();
    setBusy(false);
    emit errorOccurred(error);
}

void BadgeBleManager::attemptResolveCharacteristic()
{
    if (!m_busy || m_device.isNull()) {
        return;
    }

    const QList<BluezQt::GattServiceRemotePtr> services = m_device->gattServices();
    for (const BluezQt::GattServiceRemotePtr &service : services) {
        if (service.isNull() || !matchesBadgeServiceUuid(service->uuid())) {
            continue;
        }

        const QList<BluezQt::GattCharacteristicRemotePtr> characteristics = service->characteristics();
        for (const BluezQt::GattCharacteristicRemotePtr &characteristic : characteristics) {
            if (!characteristic.isNull() &&
                matchesBadgeCharacteristicUuid(characteristic->uuid())) {
                m_characteristic = characteristic;
                emit statusChanged(sendingBadgeData());
                writeNextChunk();
                return;
            }
        }
    }

    if (m_device->isServicesResolved()) {
        finishWithError(unsupportedBadgeDevice());
    } else {
        emit statusChanged(resolvingBadgeServices());
    }
}

void BadgeBleManager::writeNextChunk()
{
    if (!m_busy || m_characteristic.isNull()) {
        finishWithError(incompleteTransferState());
        return;
    }

    if (m_writeIndex < 0 || m_writeIndex >= m_pendingChunks.size()) {
        finishWithError(transferQueueOutOfRange());
        return;
    }

    QVariantMap options;
    const QStringList flags = m_characteristic->flags();
    if (flags.contains(QStringLiteral("write"))) {
        options.insert(QStringLiteral("type"), QStringLiteral("request"));
    } else if (flags.contains(QStringLiteral("write-without-response"))) {
        options.insert(QStringLiteral("type"), QStringLiteral("command"));
    } else {
        finishWithError(badgeCharacteristicNotWritable());
        return;
    }

    BluezQt::PendingCall *writeCall =
        m_characteristic->writeValue(m_pendingChunks.at(m_writeIndex), options);
    watchPendingCallForDeletion(writeCall);
    if (writeCall == nullptr) {
        finishWithError(bluetoothTransferFailed());
        return;
    }

    connect(writeCall, &BluezQt::PendingCall::finished, this, [this](BluezQt::PendingCall *call) {
        if (!m_busy) {
            return;
        }

        if (call->error() != BluezQt::PendingCall::NoError) {
            finishWithError(bluetoothTransferFailed());
            return;
        }

        ++m_writeIndex;
        if (m_writeIndex >= m_pendingChunks.size()) {
            finishTransferSuccessfully();
            return;
        }

        writeNextChunk();
    });
}

void BadgeBleManager::finishTransferSuccessfully()
{
    emit statusChanged(badgeUpdatedSuccessfully());
    if (!m_device.isNull() && m_device->isConnected()) {
        BluezQt::PendingCall *disconnectCall = m_device->disconnectFromDevice();
        watchPendingCallForDeletion(disconnectCall);
    }
    setBusy(false);
    m_pendingChunks.clear();
    m_writeIndex = 0;
    emit transferFinished();
}

void BadgeBleManager::stopDiscovery()
{
    if (!m_adapter.isNull() && m_adapter->isDiscovering()) {
        BluezQt::PendingCall *stopCall = m_adapter->stopDiscovery();
        watchPendingCallForDeletion(stopCall);
    }
}

bool BadgeBleManager::tryUseDevice(BluezQt::DevicePtr device)
{
    if (!m_busy || !m_device.isNull() || device.isNull() || !isBadgeDevice(device)) {
        return false;
    }

    m_scanTimeout->stop();
    stopDiscovery();

    m_device = device;
    connect(m_device.data(), &BluezQt::Device::connectedChanged,
            this, &BadgeBleManager::handleDeviceConnectedChanged);
    connect(m_device.data(), &BluezQt::Device::servicesResolvedChanged,
            this, &BadgeBleManager::handleDeviceServicesResolvedChanged);
    connect(m_device.data(), &BluezQt::Device::gattServicesChanged,
            this, &BadgeBleManager::handleGattServicesChanged);

    emit statusChanged(connectingToBadge());

    if (m_device->isConnected()) {
        attemptResolveCharacteristic();
        return true;
    }

    BluezQt::PendingCall *connectCall = m_device->connectToDevice();
    watchPendingCallForDeletion(connectCall);
    if (connectCall == nullptr) {
        finishWithError(bluetoothConnectionFailed());
        return true;
    }

    connect(connectCall, &BluezQt::PendingCall::finished, this, [this](BluezQt::PendingCall *call) {
        if (!m_busy || m_device.isNull()) {
            return;
        }

        if (call->error() != BluezQt::PendingCall::NoError &&
            call->error() != BluezQt::PendingCall::AlreadyConnected) {
            finishWithError(bluetoothConnectionFailed());
            return;
        }

        attemptResolveCharacteristic();
    });

    return true;
}

bool BadgeBleManager::isBadgeDevice(const BluezQt::DevicePtr &device) const
{
    if (device.isNull()) {
        return false;
    }

    const QStringList uuids = device->uuids();
    for (const QString &uuid : uuids) {
        if (matchesBadgeServiceUuid(uuid)) {
            return true;
        }
    }

    const QList<BluezQt::GattServiceRemotePtr> services = device->gattServices();
    for (const BluezQt::GattServiceRemotePtr &service : services) {
        if (!service.isNull() && matchesBadgeServiceUuid(service->uuid())) {
            return true;
        }
    }

    return false;
}

bool BadgeBleManager::matchesBadgeServiceUuid(const QString &uuid) const
{
    return uuid.compare(kBadgeServiceUuid, Qt::CaseInsensitive) == 0;
}

bool BadgeBleManager::matchesBadgeCharacteristicUuid(const QString &uuid) const
{
    return uuid.compare(kBadgeCharacteristicUuid, Qt::CaseInsensitive) == 0;
}
