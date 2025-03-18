#!/skynet/python/bin/python
"""Checks on ipv4tunnels consistency (no double allocated ips, not ips not in range)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB
from gaux.aux_colortext import red_text
from gaux.aux_utils import indent

def check_instance_pool():
    """Check that every instances have allocated tunnel"""
    for group in CURDB.groups.get_groups():
        if group.card.properties.ipip6_ext_tunnel == False and group.card.properties.ipip6_ext_tunnel_v2 == False:
            continue

        pool_name = group.card.properties.ipip6_ext_tunnel_pool_name

        for instance in group.get_kinda_busy_instances():
            if not CURDB.ipv4tunnels.has(instance, pool_name):
                msg = "Instance {} does not have allocated tunnel".format(instance)
                return False, msg
    return True, None

def check_no_double_allocation():
    """Check for no ips allocated multiple times"""
    counts = defaultdict(int)
    for elem in CURDB.ipv4tunnels.allocated.itervalues():
        counts[elem] += 1

    multiple_allocated = [x for x, y in counts.iteritems() if y > 1]

    if not multiple_allocated:
        return True, None
    else:
        msg = 'Multiple allocated ips: {}'.format(' '.join(multiple_allocated))
        return False, msg


def check_no_missed_ips():
    """Check all ips either in free list or used list"""
    # get list of allocatd or used
    allocated_or_free_ips = set(CURDB.ipv4tunnels.allocated.values())
    allocated_or_free_ips |= {x['tunnel_ip'] for x in CURDB.ipv4tunnels.waiting_removal}
    for subnet in CURDB.ipv4tunnels.get_all_subnets():
        allocated_or_free_ips |= {str(x) for x in subnet.free_ips}

    # get list of ips according to netmask
    netmask_ips = set()
    for subnet in CURDB.ipv4tunnels.get_all_subnets():
        ips = list(subnet.subnet)
        netmask_ips |= {str(x) for x in ips}

    extra_allocated = allocated_or_free_ips - netmask_ips
    if extra_allocated:
        msg = 'Allocated ips <{}> not in subnet netmasks'.format(','.join(extra_allocated))
        return False, msg

    extra_netmask = netmask_ips - allocated_or_free_ips
    if extra_netmask:
        msg = 'Netmask ips <{}> not in allocated/freee list'.format(','.join(extra_netmask))
        return False, msg

    return True, None


def main():
    checkers = [
        check_no_double_allocation,
        check_no_missed_ips,
        check_instance_pool
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
