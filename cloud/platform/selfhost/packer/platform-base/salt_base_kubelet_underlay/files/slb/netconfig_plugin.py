# Copied from https://bb.yandex-team.ru/projects/CLOUD/repos/salt-formula/browse/slb/files/netconfig_plugin.py
"""Disable address configuration for non-tunnel device.

We want netconfig to handle tunneling while leaving everything else to us.
This plugin wipes all address-related configs from the device which is
IPv6 underlay. We don't touch routes though as they can be useful. We also
disable ethtool as we care of hardware settings ourselves as well.
"""


def process(interfaces, params):
    for iface in interfaces:
        if iface.name == '{{ iface }}':
            iface.itype = 'manual'
            iface.address = None
            iface.netmask = None
            iface.network = None
            iface.broadcast = None
            iface.do_ethtool = False


def filter(params):
    return True
