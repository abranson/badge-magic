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

#include "badgeblemanager.h"

#include <QDebug>
#include <QMap>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusMetaType>
#include <QtDBus/QDBusObjectPath>
#include <QtDBus/QDBusPendingCallWatcher>
#include <QtDBus/QDBusPendingReply>

namespace {

typedef QMap<QString, QVariantMap> InterfaceList;
typedef QMap<QDBusObjectPath, InterfaceList> ManagedObjectList;

const QString kBadgeServiceUuid = QStringLiteral("0000fee0-0000-1000-8000-00805f9b34fb");
const QString kBadgeCharacteristicUuid = QStringLiteral("0000fee1-0000-1000-8000-00805f9b34fb");
const QString kBluezAdapterInterface = QStringLiteral("org.bluez.Adapter1");
const QString kBluezDeviceInterface = QStringLiteral("org.bluez.Device1");
const QString kBluezGattServiceInterface = QStringLiteral("org.bluez.GattService1");
const QString kBluezGattCharacteristicInterface = QStringLiteral("org.bluez.GattCharacteristic1");
const int kScanTimeoutMs = 16000;
const int kScanPollIntervalMs = 750;
const int kResolvePollIntervalMs = 500;
const int kResolveTimeoutMs = 12000;
const int kConnectAttempts = 3;
const int kConnectRetryDelayMs = 750;
const int kWriteAttempts = 3;
const int kWriteNextChunkDelayMs = 20;
const int kWriteRetryDelayMs = 80;

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

bool isTransientConnectError(const QString &message)
{
    const QString lowered = message.toLower();
    return lowered.contains(QStringLiteral("le-connection-abort-by-local"))
            || lowered.contains(QStringLiteral("doesn't exist"))
            || lowered.contains(QStringLiteral("not connected"));
}

QDBusMessage bluezCall(const QString &path, const QString &interface, const QString &method)
{
    return QDBusMessage::createMethodCall(QStringLiteral("org.bluez"), path, interface, method);
}

} // namespace

Q_DECLARE_METATYPE(InterfaceList)
Q_DECLARE_METATYPE(ManagedObjectList)

BadgeBleManager::BadgeBleManager(QObject *parent)
    : QObject(parent)
{
    qDBusRegisterMetaType<InterfaceList>();
    qDBusRegisterMetaType<ManagedObjectList>();
    m_stepTimer.setSingleShot(true);
    m_phaseTimeout.setSingleShot(true);
    connect(&m_stepTimer, &QTimer::timeout, this, [this]() {
        switch (m_state) {
        case State::Scanning:
        case State::Resolving:
            refreshObjects();
            break;
        case State::Connecting:
            connectToBadge();
            break;
        case State::Writing:
            writeNextChunk();
            break;
        default:
            break;
        }
    });
    connect(&m_phaseTimeout, &QTimer::timeout, this, [this]() {
        finish(m_state == State::Scanning ? badgeNotFound() : unsupportedBadgeDevice());
    });
}

BadgeBleManager::~BadgeBleManager()
{
    // Best-effort cleanup without blocking the UI or waiting during shutdown.
    if (m_discovering) {
        QDBusConnection::systemBus().asyncCall(bluezCall(m_adapterPath, kBluezAdapterInterface,
                                                       QStringLiteral("StopDiscovery")));
    }
    if (!m_devicePath.isEmpty() && m_state != State::Disconnecting) {
        QDBusConnection::systemBus().asyncCall(bluezCall(m_devicePath, kBluezDeviceInterface,
                                                       QStringLiteral("Disconnect")));
    }
}

bool BadgeBleManager::busy() const
{
    return m_state != State::Idle;
}

void BadgeBleManager::call(const QString &path, const QString &interface, const QString &method,
                           const QList<QVariant> &arguments, const ReplyHandler &handler)
{
    // QDBusInterface construction/property access can introspect synchronously.
    // Build messages directly so every BlueZ operation stays asynchronous.
    QDBusMessage message = bluezCall(path, interface, method);
    message.setArguments(arguments);
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::systemBus().asyncCall(message), this);
    const quint64 generation = m_generation;
    connect(watcher, &QDBusPendingCallWatcher::finished, this,
            [this, watcher, generation, handler]() {
        const QDBusMessage reply = watcher->reply();
        watcher->deleteLater();
        if (generation == m_generation) {
            handler(reply);
        }
    });
}

void BadgeBleManager::sendChunks(const QList<QByteArray> &chunks)
{
    if (busy()) {
        emit errorOccurred(transferAlreadyInProgress());
        return;
    }
    if (chunks.isEmpty()) {
        emit errorOccurred(noBadgeDataToSend());
        return;
    }

    ++m_generation;
    m_pendingChunks = chunks;
    m_writeIndex = 0;
    m_writeAttempts = 0;
    m_connectAttempts = 0;
    m_state = State::Scanning;
    emit busyChanged();
    emit statusChanged(scanningForBadge());
    m_phaseTimeout.start(kScanTimeoutMs);
    refreshObjects();
}

void BadgeBleManager::refreshObjects()
{
    call(QStringLiteral("/"), QStringLiteral("org.freedesktop.DBus.ObjectManager"),
         QStringLiteral("GetManagedObjects"), {}, [this](const QDBusMessage &reply) {
        const QDBusPendingReply<ManagedObjectList> result(reply);
        if (result.isError()) {
            finish(bluetoothConnectionFailed());
            return;
        }
        const ManagedObjectList objects = result.value();
        if (m_state == State::Scanning) {
            if (m_adapterPath.isEmpty()) {
                bool hasAdapter = false;
                for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
                    const auto interfaces = it.value();
                    if (interfaces.contains(kBluezAdapterInterface)) {
                        hasAdapter = true;
                        if (interfaces.value(kBluezAdapterInterface).value(QStringLiteral("Powered")).toBool()) {
                            m_adapterPath = it.key().path();
                            break;
                        }
                    }
                }
                if (m_adapterPath.isEmpty()) {
                    finish(hasAdapter ? bluetoothTurnedOff() : bluetoothNotAvailable());
                    return;
                }
            }
            for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
                if (!it.key().path().startsWith(m_adapterPath + QLatin1Char('/'))) {
                    continue;
                }
                const QVariantMap device = it.value().value(kBluezDeviceInterface);
                const QStringList uuids = device.value(QStringLiteral("UUIDs")).toStringList();
                if (!uuids.contains(kBadgeServiceUuid, Qt::CaseInsensitive)) {
                    continue;
                }
                m_devicePath = it.key().path();
                m_phaseTimeout.stop();
                m_state = State::Connecting;
                stopDiscovery([this]() { connectToBadge(); });
                return;
            }
            if (!m_discovering) {
                startDiscovery();
            } else {
                m_stepTimer.start(kScanPollIntervalMs);
            }
            return;
        }
        if (m_state != State::Resolving) {
            return;
        }
        const QVariantMap device = objects.value(QDBusObjectPath(m_devicePath)).value(kBluezDeviceInterface);
        if (!device.value(QStringLiteral("Connected")).toBool()) {
            finish(badgeDisconnectedBeforeTransferCompleted());
            return;
        }
        if (device.value(QStringLiteral("ServicesResolved")).toBool()) {
            for (auto it = objects.constBegin(); it != objects.constEnd(); ++it) {
                const QVariantMap characteristic = it.value().value(kBluezGattCharacteristicInterface);
                if (characteristic.value(QStringLiteral("UUID")).toString().compare(
                            kBadgeCharacteristicUuid, Qt::CaseInsensitive) != 0) {
                    continue;
                }
                const QDBusObjectPath servicePath = qvariant_cast<QDBusObjectPath>(
                            characteristic.value(QStringLiteral("Service")));
                const QVariantMap service = objects.value(servicePath).value(kBluezGattServiceInterface);
                if (!servicePath.path().startsWith(m_devicePath + QLatin1Char('/'))
                        || service.value(QStringLiteral("UUID")).toString().compare(
                            kBadgeServiceUuid, Qt::CaseInsensitive) != 0) {
                    continue;
                }
                if (!characteristic.value(QStringLiteral("Flags")).toStringList().contains(QStringLiteral("write"))) {
                    finish(badgeCharacteristicNotWritable());
                    return;
                }
                m_characteristicPath = it.key().path();
                m_phaseTimeout.stop();
                m_state = State::Writing;
                emit statusChanged(sendingBadgeData());
                writeNextChunk();
                return;
            }
        }
        m_stepTimer.start(kResolvePollIntervalMs);
    });
}

void BadgeBleManager::startDiscovery()
{
    // Mark ownership before the call: even a timed-out start needs cleanup.
    m_discovering = true;
    call(m_adapterPath, kBluezAdapterInterface, QStringLiteral("StartDiscovery"), {},
         [this](const QDBusMessage &reply) {
        if (reply.type() == QDBusMessage::ErrorMessage) {
            finish(bluetoothConnectionFailed());
        } else {
            m_stepTimer.start(kScanPollIntervalMs);
        }
    });
}

void BadgeBleManager::stopDiscovery(const std::function<void()> &finished)
{
    if (!m_discovering) {
        finished();
        return;
    }
    m_discovering = false;
    call(m_adapterPath, kBluezAdapterInterface, QStringLiteral("StopDiscovery"), {},
         [finished](const QDBusMessage &) { finished(); });
}

void BadgeBleManager::connectToBadge()
{
    emit statusChanged(connectingToBadge());
    ++m_connectAttempts;
    // Connect is also safe for a device which is already connected. Cached
    // GATT objects alone must never bypass this step.
    call(m_devicePath, kBluezDeviceInterface, QStringLiteral("Connect"), {},
         [this](const QDBusMessage &reply) {
        if (reply.type() == QDBusMessage::ErrorMessage
                && reply.errorName() != QStringLiteral("org.bluez.Error.AlreadyConnected")) {
            if (m_connectAttempts < kConnectAttempts && isTransientConnectError(reply.errorMessage())) {
                m_stepTimer.start(kConnectRetryDelayMs);
            } else {
                finish(bluetoothConnectionFailed());
            }
            return;
        }
        m_state = State::Resolving;
        emit statusChanged(resolvingBadgeServices());
        m_phaseTimeout.start(kResolveTimeoutMs);
        refreshObjects();
    });
}

void BadgeBleManager::writeNextChunk()
{
    if (m_writeIndex == m_pendingChunks.size()) {
        finish();
        return;
    }
    const QByteArray chunk = m_pendingChunks.at(m_writeIndex);
    qDebug() << "Async Writing to " << kBadgeCharacteristicUuid << ":" << chunk.toHex();
    QVariantMap options;
    options.insert(QStringLiteral("type"), QStringLiteral("request"));
    // Only this call's completion schedules the next write or retry. Property
    // notifications and discovery cannot start another writer in this phase.
    call(m_characteristicPath, kBluezGattCharacteristicInterface, QStringLiteral("WriteValue"),
         {chunk, options}, [this](const QDBusMessage &reply) {
        if (reply.type() == QDBusMessage::ErrorMessage) {
            if (++m_writeAttempts < kWriteAttempts) {
                m_stepTimer.start(kWriteRetryDelayMs);
            } else {
                qWarning() << Q_FUNC_INFO << reply.errorMessage();
                finish(writingBadgeDataFailed());
            }
            return;
        }
        ++m_writeIndex;
        m_writeAttempts = 0;
        m_stepTimer.start(kWriteNextChunkDelayMs);
    });
}

void BadgeBleManager::finish(const QString &error)
{
    if (m_state == State::Idle || m_state == State::Disconnecting) {
        return;
    }
    m_stepTimer.stop();
    m_phaseTimeout.stop();
    ++m_generation; // Ignore replies from the phase being abandoned.
    m_state = State::Disconnecting;
    stopDiscovery([this, error]() {
        if (m_devicePath.isEmpty()) {
            complete(error);
            return;
        }
        call(m_devicePath, kBluezDeviceInterface, QStringLiteral("Disconnect"), {},
             [this, error](const QDBusMessage &reply) {
            // Keep busy until Disconnect completes so a new send cannot race it.
            if (reply.type() == QDBusMessage::ErrorMessage
                    && reply.errorName() != QStringLiteral("org.bluez.Error.NotConnected")
                    && error.isEmpty()) {
                complete(bluetoothConnectionFailed());
            } else {
                complete(error);
            }
        });
    });
}

void BadgeBleManager::complete(const QString &error)
{
    m_pendingChunks.clear();
    m_adapterPath.clear();
    m_devicePath.clear();
    m_characteristicPath.clear();
    m_state = State::Idle;
    if (error.isEmpty()) {
        emit statusChanged(badgeUpdatedSuccessfully());
    } else {
        emit errorOccurred(error);
    }
    emit busyChanged();
    if (error.isEmpty()) {
        emit transferFinished();
    }
}
