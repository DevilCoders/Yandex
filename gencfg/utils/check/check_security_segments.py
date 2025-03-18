#!/skynet/python/bin/python
"""Check that all instances on host have same segment (RX-219)"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB


def main():
    host_groups = defaultdict(list)

    # fill data
    for group in CURDB.groups.get_groups():
        if group.card.properties.security_segment == 'infra':
            continue

        for host in group.getHosts():
            host_groups[host].append((group.card.name, group.card.properties.security_segment))

    # check data
    retval = 0
    for host in host_groups:
        host_segments = {x[1] for x in host_groups[host]}
        if len(host_segments) > 1:
            retval = 1
            print 'Host {} has groups in different security segments:'.format(host.name)
            for host_segment in host_segments:
                host_segment_groups = [x[0] for x in host_groups[host] if x[1] == host_segment]
                print '    Segment {}: {}'.format(host_segment, ' '.join(sorted(x for x in host_segment_groups)))

    return retval

if __name__ == '__main__':
    retval = main()
    sys.exit(retval)
