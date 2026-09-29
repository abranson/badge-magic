#!/usr/bin/env python3
"""Run inside dbus-run-session; redirects only this test process's system bus."""
import os
from pathlib import Path
import subprocess
import sys

if not os.environ.get('DBUS_SESSION_BUS_ADDRESS'):
    sys.exit('Run with dbus-run-session -- python3 tests/run.py /path/to/tst_badgemagic')
os.environ['DBUS_SYSTEM_BUS_ADDRESS'] = os.environ['DBUS_SESSION_BUS_ADDRESS']
os.environ.setdefault('QT_QPA_PLATFORM', 'offscreen')
mock = subprocess.Popen([sys.executable, str(Path(__file__).with_name('bluez_mock.py'))],
                        stdout=subprocess.PIPE, text=True)
try:
    if mock.stdout.readline().strip() != 'READY':
        sys.exit('BlueZ test service did not start')
    result = subprocess.run(sys.argv[1:], timeout=90)
    sys.exit(result.returncode)
finally:
    mock.terminate()
    mock.wait(timeout=5)
