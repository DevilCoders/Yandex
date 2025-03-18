#!/skynet/python/bin/python
"""Check for validity of mtn generated names"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB

from gaux.aux_colortext import red_text
from gaux.aux_utils import indent
from gaux.aux_hbf import generate_mtn_hostname


def check_mtn_hostname_len():
    """Check for mnt hostname do not exceed max length"""
    MAX_LENGTH = 63

    failed_data = []
    for group in CURDB.groups.get_groups():
        for instance in group.get_kinda_busy_instances():
            for vlan in instance.host.vlans:
                mtn_hostname = generate_mtn_hostname(instance, group, vlan)
                if len(mtn_hostname) > MAX_LENGTH:
                    failed_data.append((instance, vlan, mtn_hostname))

    if failed_data:
        msg = '\n'.join('Instance {}, vlan {}, name {} len {}'.format(x, y, z, len(z)) for (x, y, z) in failed_data[:100])
        return False, msg

    return True, None


def check_mtn_fastbone_name():
    """Check if fastobne named is constructed as fb-<backbone name>"""
    failed_data = []
    for group in CURDB.groups.get_groups():
        for instance in group.get_kinda_busy_instances():
            if not ('vlan688' in instance.host.vlans or 'vlan788' in instance.host.vlans):
                continue
            bb_name = generate_mtn_hostname(instance, group, '')
            fb_name = generate_mtn_hostname(instance, group, 'fb-')
            if 'fb-{}'.format(bb_name) != fb_name:
                failed_data.append((instance, bb_name, fb_name))

    if failed_data:
        msg = '\n'.join('Instance {} has uncoordinated backbone/fastbone names: {} and {}'.format(x, y, z) for x, y, z in failed_data[:100])
        return False, msg

    return True, None


def main():
    checkers = [
        check_mtn_hostname_len,
        check_mtn_fastbone_name,
    ]

    status = 0
    for checker in checkers:
        is_ok, msg = checker()
        if not is_ok:
            status = 1
            print '{}:\n{}'.format(checker.__doc__.split('\n')[0], red_text(indent(msg)))

    return status


if __name__ == '__main__':
    status = main()

    sys.exit(status)
