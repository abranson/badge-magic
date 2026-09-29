#!/usr/bin/env python3
"""Isolated BlueZ simulator for tests/run.py; never uses the real system bus."""
import dbus
import dbus.service
import xml.etree.ElementTree as ET
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

DBusGMainLoop(set_as_default=True)
bus = dbus.SessionBus()
name = dbus.service.BusName('org.bluez', bus)
A = '/org/bluez/hci0'
D = A + '/dev_00_11_22_33_44_55'
S = D + '/service0010'
C = S + '/char0011'
SU = '0000fee0-0000-1000-8000-00805f9b34fb'
CU = '0000fee1-0000-1000-8000-00805f9b34fb'
objects = {}
scenario = 'repeat'
stats = {}
accepted = []
generation = 0


class Obj(dbus.service.Object):
    def __init__(self, path, iface, props):
        super().__init__(bus, path)
        self.iface, self.props = iface, props
        objects[path] = self

    @dbus.service.method('org.freedesktop.DBus.Introspectable', in_signature='', out_signature='s',
                         path_keyword='object_path', connection_keyword='connection')
    def Introspect(self, object_path, connection):
        root = ET.fromstring(dbus.service.Object.Introspect(self, object_path, connection))
        iface = next((i for i in root.findall('interface') if i.get('name') == self.iface), None)
        if iface is None:
            iface = ET.SubElement(root, 'interface', name=self.iface)
        for key, value in self.props.items():
            sig = ('b' if isinstance(value, dbus.Boolean) else
                   'o' if isinstance(value, dbus.ObjectPath) else
                   'as' if isinstance(value, dbus.Array) else 's')
            ET.SubElement(iface, 'property', name=key, type=sig, access='read')
        return ET.tostring(root, encoding='unicode')

    @dbus.service.method('org.freedesktop.DBus.Properties', in_signature='ss', out_signature='v')
    def Get(self, iface, key):
        return self.props[key]

    @dbus.service.method('org.freedesktop.DBus.Properties', in_signature='s', out_signature='a{sv}')
    def GetAll(self, iface):
        return self.props if iface == self.iface else {}

    @dbus.service.signal('org.freedesktop.DBus.Properties', signature='sa{sv}as')
    def PropertiesChanged(self, iface, changed, invalid):
        pass


class Root(dbus.service.Object):
    @dbus.service.method('org.freedesktop.DBus.ObjectManager', in_signature='', out_signature='a{oa{sa{sv}}}',
                         async_callbacks=('ok', 'err'))
    def GetManagedObjects(self, ok, err):
        def done():
            ok({p: {o.iface: o.props} for p, o in objects.items()
                if scenario != 'discovery' or stats['starts'] or p == A})
            return False
        GLib.timeout_add(250 if scenario == 'slow' else 1, done)

    @dbus.service.method('org.example.BadgeMagicTest', in_signature='s', out_signature='')
    def Reset(self, value):
        global scenario, stats, accepted, generation
        generation += 1
        scenario = str(value)
        stats = dict(connects=0, disconnects=0, starts=0, stops=0, writes=0, overlaps=0, disconnectedWrites=0)
        accepted = []
        device.props['Connected'] = dbus.Boolean(False)
        device.props['ServicesResolved'] = dbus.Boolean(False)
        characteristic.pending = False

    @dbus.service.method('org.example.BadgeMagicTest', in_signature='', out_signature='a{sv}')
    def Stats(self):
        return dict(stats, accepted=dbus.Array(accepted, signature='s'))


class Adapter(Obj):
    @dbus.service.method('org.bluez.Adapter1', in_signature='', out_signature='')
    def StartDiscovery(self):
        stats['starts'] += 1

    @dbus.service.method('org.bluez.Adapter1', in_signature='', out_signature='')
    def StopDiscovery(self):
        stats['stops'] += 1


class Device(Obj):
    @dbus.service.method('org.bluez.Device1', in_signature='', out_signature='')
    def Connect(self):
        stats['connects'] += 1
        self.props['Connected'] = dbus.Boolean(True)
        self.props['ServicesResolved'] = dbus.Boolean(scenario != 'separate')
        self.PropertiesChanged(self.iface, self.props, [])
        current = generation
        def resolved():
            if current == generation:
                self.props['ServicesResolved'] = dbus.Boolean(True)
                self.PropertiesChanged(self.iface, {'ServicesResolved': dbus.Boolean(True)}, [])
            return False
        GLib.timeout_add(80, resolved)

    @dbus.service.method('org.bluez.Device1', in_signature='', out_signature='', async_callbacks=('ok', 'err'))
    def Disconnect(self, ok, err):
        stats['disconnects'] += 1
        current = generation
        def done():
            if current == generation:
                self.props['Connected'] = dbus.Boolean(False)
                self.props['ServicesResolved'] = dbus.Boolean(False)
                self.PropertiesChanged(self.iface, self.props, [])
            ok()
            return False
        GLib.timeout_add(150, done)


class Char(Obj):
    pending = False

    @dbus.service.method('org.bluez.GattCharacteristic1', in_signature='aya{sv}', out_signature='',
                         async_callbacks=('ok', 'err'), byte_arrays=True)
    def WriteValue(self, value, options, ok, err):
        stats['writes'] += 1
        if not device.props['Connected']:
            stats['disconnectedWrites'] += 1
            err(dbus.exceptions.DBusException('Not connected', name='org.bluez.Error.Failed'))
            return
        if self.pending:
            stats['overlaps'] += 1
            err(dbus.exceptions.DBusException('In Progress', name='org.bluez.Error.InProgress'))
            return
        if scenario == 'failure' or (scenario == 'retry' and stats['writes'] == 1):
            err(dbus.exceptions.DBusException('Write failed', name='org.bluez.Error.Failed'))
            return
        self.pending = True
        current = generation
        def done():
            if current == generation:
                self.pending = False
                accepted.append(bytes(value).hex())
            ok()
            return False
        GLib.timeout_add(300 if scenario == 'race' else 5, done)


root = Root(bus, '/')
adapter = Adapter(A, 'org.bluez.Adapter1', {'Powered': dbus.Boolean(True)})
device = Device(D, 'org.bluez.Device1', {
    'Connected': dbus.Boolean(False), 'ServicesResolved': dbus.Boolean(False),
    'UUIDs': dbus.Array([SU], signature='s')})
service = Obj(S, 'org.bluez.GattService1', {
    'UUID': SU, 'Device': dbus.ObjectPath(D), 'Primary': dbus.Boolean(True)})
characteristic = Char(C, 'org.bluez.GattCharacteristic1', {
    'UUID': CU, 'Service': dbus.ObjectPath(S), 'Flags': dbus.Array(['write'], signature='s')})
root.Reset('repeat')
print('READY', flush=True)
GLib.MainLoop().run()
