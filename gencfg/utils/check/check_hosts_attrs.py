#!/skynet/python/bin/python
"""
    Check, that all virtual hosts have generated addrs:
        - hosts with attrribute HostFlags.IS_HWADDR_GENERATED have hwaddr property
        - hosts with attrirbute HostFlags.IS_IPV6_GENERATED have ipv6addr property
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB
from gaux.aux_colortext import red_text

CHECKERS = [
    (
        "all hosts with HostFlags.IS_HWADDR_GENERATED have 'hwaddr' property",
        lambda host: host.is_hwaddr_generated(),
        lambda host: host.hwaddr == 'unknown',
    ),
    (
        "all hosts with HostFlags.IS_IPV6_GENERATED have 'ipv6addr' property",
        lambda host: host.is_ipv6_generated(),
        lambda host: host.ipv6addr == 'unknown',
    )
]

if __name__ == '__main__':
    status = 0
    for description, filter_func, checker_func in CHECKERS:
        filtered_hosts = filter(filter_func, CURDB.hosts.get_hosts())
        bad_hosts = filter(checker_func, filtered_hosts)
        if len(bad_hosts) > 0:
            status = 1

            bad_hosts_by_group = defaultdict(list)
            for host in bad_hosts:
                bad_hosts_by_group[CURDB.groups.get_host_master_group(host)].append(host)

            print red_text("Check <%s> failed on %d of %d hosts:" % (description, len(bad_hosts), len(filtered_hosts)))
            for group in bad_hosts_by_group.iterkeys():
                print red_text("    Group %s (%d of %d hosts): %s" % (group.card.name, len(bad_hosts_by_group[group]), len(group.getHosts()), ",".join(map(lambda x: x.name, bad_hosts_by_group[group]))))

    sys.exit(status)
