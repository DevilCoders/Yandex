#!/skynet/python/bin/python
"""Check that vlans ipv6 address fit the same format."""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB


def main():
    print("vlans_format")
    """Format like 'Omit leading zeros'. Right example: 2a02:6b8:c14:1608:0:0:0:1"""
    failed_data = []
    for group in CURDB.groups.get_groups():
        for instance in group.get_kinda_busy_instances():
            for vlan in instance.host.vlans:
                parts = vlan.split(':')
                has_empty_flag = any(part == '' for part in parts)
                if has_empty_flag:
                    failed_data.append((instance, vlan))
                has_leading_zeros = any((part != '0' and part.startswith('0')) for part in parts)
                if has_leading_zeros:
                    failed_data.append((instance, vlan))

    if failed_data:
        print('\n'.join('Instance {}, vlan {}'.format(x, y) for (x, y) in failed_data[:100]))
        return 1

    return 0


if __name__ == '__main__':
    status = main()

    sys.exit(status)
