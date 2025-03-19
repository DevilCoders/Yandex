#!/usr/bin/env python
"""
Generate correct-dc check configuration
"""

import ipaddress
import json
import sys


VLANS = [
    333,
    688,
    640,
    1526,
    1589,
]

URL = 'https://noc-export.yandex.net/rt/l3-segments2.json'


def gen_config(path):
    """
    Get subnet <-> dc pairs for correct-dc check
    """
    with open(path) as inp:
        segments = json.load(inp)

    subnets = {
        ipaddress.IPv6Network('2a02:6b8:c02::/48'): {'datacenter_name': 'SAS'},
        ipaddress.IPv6Network('2a02:6b8:c03::/48'): {'datacenter_name': 'MYT'},
        ipaddress.IPv6Network('2a02:6b8:c0e::/48'): {'datacenter_name': 'VLA'},
    }

    for subnet, data in segments['data'].items():
        if ':' in subnet and data.get('vlan') in VLANS and 'datacenter_name' in data:
            subnets[ipaddress.IPv6Network(subnet)] = data

    for subnet in subnets.copy():
        for other_subnet in subnets.copy():
            if subnet != other_subnet and subnet[0] in other_subnet and subnet[-1] in other_subnet:
                del subnets[subnet]

    by_dc = dict()
    for subnet, data in subnets.items():
        if data['datacenter_name'] not in by_dc:
            by_dc[data['datacenter_name']] = []
        by_dc[data['datacenter_name']].append(str(subnet))

    for datacenter, subnet_list in by_dc.items():
        print(f"    # {datacenter} nets")
        for subnet in sorted(subnet_list):
            print(f"    ipaddress.IPv6Network('{subnet}'): '{datacenter.lower()}',")


def _main():
    """
    Console entry-point
    """
    if len(sys.argv) != 2:
        print(f'Usage: {sys.argv[0]} <l3-segments2.json> (get it from {URL})')
        sys.exit(1)
    gen_config(sys.argv[1])


if __name__ == '__main__':
    _main()
