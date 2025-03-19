#!/usr/bin/env python3
"""
Verify that host is in correct dc
"""

import ipaddress
import os
import socket
import sys


DC_LIST = ['iva', 'myt', 'sas', 'man', 'vla', 'il1-a', 'vlx']
DC2DC = dict(kv for kv in zip(DC_LIST, DC_LIST))

PREFIX_MAP = {
    'rc1a-': 'vla',
    'rc1b-': 'sas',
    'rc1c-': 'myt',

    # Israel
    'il1a-': 'il1-a',
}
PREFIX_MAP.update(DC2DC)

SUFFIX_MAP = {
    'e': 'iva',
    'f': 'myt',
    'h': 'sas',
    'i': 'man',
    'k': 'vla',
    '-rc1a': 'vla',
    '-rc1b': 'sas',
    '-rc1c': 'myt',
}
SUFFIX_MAP.update(DC2DC)

NETMAP = {
    # SAS nets
    ipaddress.IPv6Network('2a02:6b8:0:1a2d::/64'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:b010:c00::/56'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:bf00:1000::/56'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:bf00:1100::/56'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:bf00:1300::/56'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c02::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c07::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c08::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c10::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c11::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c14::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c16::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c1b::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c1c::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c1e::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c21::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c23::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c24::/48'): 'sas',
    ipaddress.IPv6Network('2a02:6b8:c28::/48'): 'sas',
    # MYT nets
    ipaddress.IPv6Network('2a02:6b8:0:1402::/64'): 'myt',
    ipaddress.IPv6Network('2a02:6b8:0:1472::/64'): 'myt',
    ipaddress.IPv6Network('2a02:6b8:b010:7600::/56'): 'myt',
    ipaddress.IPv6Network('2a02:6b8:b010:9600::/56'): 'myt',
    ipaddress.IPv6Network('2a02:6b8:bf00:2000::/56'): 'myt',
    ipaddress.IPv6Network('2a02:6b8:bf00:2200::/56'): 'myt',
    ipaddress.IPv6Network('2a02:6b8:bf00:2300::/56'): 'myt',
    ipaddress.IPv6Network('2a02:6b8:c00::/48'): 'myt',
    ipaddress.IPv6Network('2a02:6b8:c03::/48'): 'myt',
    ipaddress.IPv6Network('2a02:6b8:c05::/48'): 'myt',
    ipaddress.IPv6Network('2a02:6b8:c12::/48'): 'myt',
    # VLA nets
    ipaddress.IPv6Network('2a02:6b8:bf00:100::/56'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:bf00:300::/56'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:bf00:800::/56'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:bf00::/56'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:c0d::/48'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:c0e::/48'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:c0f::/48'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:c15::/48'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:c17::/48'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:c18::/48'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:c19::/48'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:c1d::/48'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:c1f::/48'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:c20::/48'): 'vla',
    ipaddress.IPv6Network('2a02:6b8:c22::/48'): 'vla',
    # VLX nets
    ipaddress.IPv6Network('2a02:6b8:c33::/48'): 'vlx',
    ipaddress.IPv6Network('2a02:6b8:c34::/48'): 'vlx',
    # MAN nets
    ipaddress.IPv6Network('2a02:6b8:b011:3700::/56'): 'man',
    ipaddress.IPv6Network('2a02:6b8:b011:3c00::/56'): 'man',
    ipaddress.IPv6Network('2a02:6b8:b011:900::/56'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c01:600::/56'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c01:700::/56'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c01:800::/56'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c01::/56'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c01:b00::/56'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c01:c00::/56'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c09::/48'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c0a::/48'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c0b::/48'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c13::/48'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c1a::/48'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c25::/48'): 'man',
    ipaddress.IPv6Network('2a02:6b8:c26::/48'): 'man',
    # IVA nets
    ipaddress.IPv6Network('2a02:6b8:0:1619::/64'): 'iva',
    ipaddress.IPv6Network('2a02:6b8:0:801::/64'): 'iva',
    ipaddress.IPv6Network('2a02:6b8:b010:4000::/56'): 'iva',
    ipaddress.IPv6Network('2a02:6b8:c04:100::/56'): 'iva',
    ipaddress.IPv6Network('2a02:6b8:c04:200::/56'): 'iva',
    ipaddress.IPv6Network('2a02:6b8:c0c::/48'): 'iva',

    # Israel
    ipaddress.IPv6Network('2a11:f740:1::/64'): 'il1-a',
}


def get_dc_from_hostname():
    """
    Guess datacenter from fqdn
    """
    fqdn = socket.getfqdn()
    for prefix, geo in PREFIX_MAP.items():
        if fqdn.startswith(prefix):
            return geo
    short_name = fqdn.split('.')[0]
    for suffix, geo in SUFFIX_MAP.items():
        if short_name.endswith(suffix):
            return geo

    return None


def get_dc_by_net_address():
    """
    Guess datacenter by network address
    """
    if not os.path.exists('/proc/net/if_inet6'):
        return None
    with open('/proc/net/if_inet6') as inp:
        if_inet6 = inp.read()

    for line in if_inet6.splitlines():
        # Lines in /proc/net/if_inet6 have very simple format:
        # 00000000000000000000000000000001 01 80 10 80       lo
        # First part is ipv6-address
        merged = line.split()[0]
        splitted = [merged[i:i + 4] for i in range(0, len(merged), 4)]
        address = ipaddress.IPv6Address(':'.join(splitted))
        if not address.is_global:
            continue
        for network, geo in NETMAP.items():
            if address in network:
                return geo

    return None


def die(status=0, message='OK'):
    """
    Print status;message and exit
    """
    print('{status};{message}'.format(status=status, message=message))
    sys.exit(0)


def _main():
    from_fqdn = get_dc_from_hostname()
    if not from_fqdn:
        die(1, 'Unable to guess geo from fqdn')
    from_net = get_dc_by_net_address()
    if not from_net:
        die(1, 'Unable to guess geo from network address')

    if from_fqdn == from_net:
        die()

    die(1, 'Geo from fqdn: {from_fqdn}, geo from net: {from_net}'.format(from_fqdn=from_fqdn, from_net=from_net))


if __name__ == '__main__':
    _main()
