# Host regression tests

The QtTest suite exercises the actual app sources against a simulated BlueZ
service. It covers repeated transfers, duplicate/separate service notifications,
slow replies, write retries, failure recovery, discovery ownership, packet
encoding, preset collisions/legacy updates, and preview timer suspension.
It does not replace testing on a Sailfish device and physical badge.

Requires Qt 5 development packages (Core, GUI, Quick, D-Bus, Test), qmake,
Python 3 with dbus-python/PyGObject, and dbus-run-session. These are host test
dependencies only.

Build outside the source checkout:

```sh
mkdir -p /tmp/badgemagic-tests
cd /tmp/badgemagic-tests
qmake /path/to/badgemagic-sailfishos/tests/tests.pro
make -j4
dbus-run-session -- python3 /path/to/badgemagic-sailfishos/tests/run.py ./tst_badgemagic
```

Use a fresh `dbus-run-session` as shown: the runner redirects the test process's
system-bus address to this isolated bus and starts its own `org.bluez` service.
It never talks to Bluetooth hardware. Preset tests use a temporary data directory.
