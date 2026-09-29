#include <QtTest>
#include <QGuiApplication>
#include <QWindow>
#include <QQuickPaintedItem>
#include <QElapsedTimer>
#include <QTimer>
#include <QVector>
#include <QTemporaryDir>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QtDBus/QDBusConnection>
#include <QtDBus/QDBusMessage>
#include <QtDBus/QDBusReply>
#include "badgeblemanager.h"
#include "badgeencoder.h"
#include "badgestore.h"

// Inspect whether idle work is actually suspended, rather than just hidden.
#define private public
#include "badgepreviewitem.h"
#undef private

class BadgeMagicTest : public QObject
{
    Q_OBJECT
private:
    QDBusMessage control(const QString &method, const QList<QVariant> &args = {})
    {
        auto request = QDBusMessage::createMethodCall(QStringLiteral("org.bluez"), QStringLiteral("/"),
                                                     QStringLiteral("org.example.BadgeMagicTest"), method);
        request.setArguments(args);
        const auto reply = QDBusConnection::systemBus().call(request);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qFatal("Mock control failed: %s", qPrintable(reply.errorMessage()));
        }
        return reply;
    }

    QVariantMap stats()
    {
        return QDBusReply<QVariantMap>(control(QStringLiteral("Stats"))).value();
    }

    BadgeMessage message(const QString &text = QStringLiteral("Test"))
    {
        BadgeMessage result;
        result.rawText = text;
        result.textHex = BadgeEncoder::encodeText(text);
        return result;
    }

private slots:
    void packetModesAndSlots()
    {
        for (int mode = 0; mode < 9; ++mode) {
            for (int speed = 0; speed < 8; ++speed) {
                auto msg = message();
                msg.modeIndex = mode;
                msg.speedIndex = speed;
                const auto chunks = BadgeEncoder::buildTransferChunks(msg);
                QCOMPARE(static_cast<unsigned char>(chunks.first().at(8)), static_cast<unsigned char>((speed << 4) | mode));
                for (const auto &chunk : chunks) {
                    QCOMPARE(chunk.size(), 16);
                }
            }
        }
        QList<BadgeMessage> messages;
        QByteArray expected;
        for (int i = 0; i < 8; ++i) {
            auto msg = message(QString(i + 1, QChar('A' + i)));
            msg.modeIndex = i;
            msg.speedIndex = 7 - i;
            msg.flash = i % 2 == 0;
            msg.marquee = i % 2 != 0;
            messages.append(msg);
            expected += QByteArray::fromHex(msg.textHex.join(QString()).toLatin1());
        }
        QByteArray payload;
        for (const auto &chunk : BadgeEncoder::buildTransferChunks(messages)) {
            payload += chunk;
        }
        QCOMPARE(static_cast<unsigned char>(payload.at(6)), static_cast<unsigned char>(0x55));
        QCOMPARE(static_cast<unsigned char>(payload.at(7)), static_cast<unsigned char>(0xaa));
        for (int i = 0; i < 8; ++i) {
            QCOMPARE(static_cast<unsigned char>(payload.at(8 + i)), static_cast<unsigned char>(((7 - i) << 4) | i));
            QCOMPARE(payload.at(16 + i * 2), char(0));
            QCOMPARE(payload.at(17 + i * 2), char(i + 1));
        }
        QCOMPARE(payload.mid(64, expected.size()), expected);
    }

    void consecutiveTransfers_data()
    {
        QTest::addColumn<QString>("scenario");
        for (const char *value : {"repeat", "race", "separate", "retry", "discovery"}) {
            QTest::newRow(value) << QString::fromLatin1(value);
        }
    }

    void consecutiveTransfers()
    {
        QFETCH(QString, scenario);
        control(QStringLiteral("Reset"), {scenario});
        BadgeBleManager manager;
        QSignalSpy finished(&manager, &BadgeBleManager::transferFinished);
        QSignalSpy errors(&manager, &BadgeBleManager::errorOccurred);
        // Distinct values detect duplicated, skipped or reordered chunks.
        const QList<QByteArray> chunks = {QByteArray(16, 'A'), QByteArray(16, 'B'), QByteArray(16, 'C')};
        manager.sendChunks(chunks);
        QVERIFY(manager.busy());
        QTRY_COMPARE(finished.count(), 1);
        QVERIFY(!manager.busy());
        manager.sendChunks(chunks);
        QTRY_COMPARE(finished.count(), 2);
        QCOMPARE(errors.count(), 0);
        const auto result = stats();
        QCOMPARE(result.value(QStringLiteral("connects")).toInt(), 2);
        QCOMPARE(result.value(QStringLiteral("disconnects")).toInt(), 2);
        QCOMPARE(result.value(QStringLiteral("overlaps")).toInt(), 0);
        QCOMPARE(result.value(QStringLiteral("disconnectedWrites")).toInt(), 0);
        QStringList expected;
        for (int i = 0; i < 2; ++i) {
            for (const auto &chunk : chunks) {
                expected.append(QString::fromLatin1(chunk.toHex()));
            }
        }
        QCOMPARE(qdbus_cast<QStringList>(result.value(QStringLiteral("accepted"))), expected);
        QCOMPARE(result.value(QStringLiteral("starts")).toInt(), scenario == QStringLiteral("discovery") ? 1 : 0);
        QCOMPARE(result.value(QStringLiteral("stops")).toInt(), scenario == QStringLiteral("discovery") ? 1 : 0);
    }

    void failedTransferCanRetry()
    {
        control(QStringLiteral("Reset"), {QStringLiteral("failure")});
        BadgeBleManager manager;
        QSignalSpy errors(&manager, &BadgeBleManager::errorOccurred);
        QSignalSpy finished(&manager, &BadgeBleManager::transferFinished);
        manager.sendChunks({QByteArray(16, 'A')});
        QTRY_COMPARE(errors.count(), 1);
        QVERIFY(!manager.busy());
        QCOMPARE(stats().value(QStringLiteral("writes")).toInt(), 3);
        control(QStringLiteral("Reset"), {QStringLiteral("repeat")});
        manager.sendChunks({QByteArray(16, 'B')});
        QTRY_COMPARE(finished.count(), 1);
        QTest::qWait(200);
        QCOMPARE(errors.count(), 1);
        QCOMPARE(stats().value(QStringLiteral("writes")).toInt(), 1);
    }

    void slowBluezDoesNotBlock()
    {
        control(QStringLiteral("Reset"), {QStringLiteral("slow")});
        BadgeBleManager manager;
        QSignalSpy finished(&manager, &BadgeBleManager::transferFinished);
        int ticks = 0;
        QTimer heartbeat;
        connect(&heartbeat, &QTimer::timeout, this, [&ticks]() { ++ticks; });
        heartbeat.start(10);
        QElapsedTimer elapsed;
        elapsed.start();
        manager.sendChunks({QByteArray(16, 'A')});
        QVERIFY(elapsed.elapsed() < 100);
        QTest::qWait(100);
        QVERIFY(ticks >= 5);
        QCOMPARE(finished.count(), 0);
        QTRY_COMPARE(finished.count(), 1);
    }

    void presetNamesAndLegacyUpdates()
    {
        QTemporaryDir root;
        QVERIFY(root.isValid());
        const QByteArray previous = qgetenv("XDG_DATA_HOME");
        qputenv("XDG_DATA_HOME", root.path().toUtf8());
        BadgeStore store;
        const QStringList names = {QStringLiteral("A B"), QStringLiteral("A?B"),
                                   QString::fromUtf8("日本"), QString::fromUtf8("中文"), QStringLiteral(".hidden")};
        for (const auto &name : names) {
            QVERIFY(store.saveBadge(name, message(name)));
        }
        QCOMPARE(store.loadBadges().size(), names.size());
        for (const auto &item : store.loadBadges()) {
            QVERIFY(names.contains(item.toMap().value(QStringLiteral("name")).toString()));
        }
        QVERIFY(store.saveBadge(names.first(), message(QStringLiteral("Replacement"))));
        QCOMPARE(store.loadBadges().size(), names.size());
        for (const auto &item : store.loadBadges()) {
            if (item.toMap().value(QStringLiteral("name")).toString() == names.first()) {
                QCOMPARE(item.toMap().value(QStringLiteral("rawText")).toString(), QStringLiteral("Replacement"));
            }
        }
        const QString legacyPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                + QStringLiteral("/badges/Legacy.json");
        QFile legacy(legacyPath);
        QVERIFY(legacy.open(QIODevice::WriteOnly));
        legacy.write("{\"rawText\":\"Old\",\"messages\":[{\"text\":[],\"mode\":\"0x00\",\"speed\":\"0x00\"}]}");
        legacy.close();
        QVERIFY(store.saveBadge(QStringLiteral("Legacy"), message(QStringLiteral("Updated"))));
        QCOMPARE(store.loadBadges().size(), names.size() + 1);
        QVERIFY(legacy.open(QIODevice::ReadOnly));
        QVERIFY(legacy.readAll().contains("Updated"));
        legacy.close();
        qputenv("XDG_DATA_HOME", previous);
    }

    void previewSuspendsIdleWork()
    {
        QWindow window;
        window.show();
        window.requestActivate();
        QTRY_COMPARE(QGuiApplication::applicationState(), Qt::ApplicationActive);
        BadgePreviewItem preview;
        preview.setText(QStringLiteral("Test"));
        preview.setSpeedIndex(7);
        QVERIFY(preview.m_timer.isActive());
        QTest::qWait(80);
        QVERIFY(preview.m_animationIndex > 0);
        preview.setVisible(false);
        QVERIFY(!preview.m_timer.isActive());
        const int frame = preview.m_animationIndex;
        QTest::qWait(80);
        QCOMPARE(preview.m_animationIndex, frame);
        preview.setVisible(true);
        QVERIFY(preview.m_timer.isActive());
        preview.setActive(false);
        QVERIFY(!preview.m_timer.isActive());
        preview.setActive(true);
        QVERIFY(preview.m_timer.isActive());
        preview.setModeIndex(4);
        QVERIFY(!preview.m_timer.isActive());
        preview.setFlash(true);
        QVERIFY(preview.m_timer.isActive());
        preview.setFlash(false);
        QVERIFY(!preview.m_timer.isActive());
        preview.setMarquee(true);
        QVERIFY(preview.m_timer.isActive());
        preview.setMarquee(false);
        QVERIFY(!preview.m_timer.isActive());
    }
};

QTEST_MAIN(BadgeMagicTest)
#include "tst_badgemagic.moc"
