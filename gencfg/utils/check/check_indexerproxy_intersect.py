#!/skynet/python/bin/python
"""Check that every host has at most one instance with itype <indexerproxy,indexerproxydisc>"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB
from gaux.aux_colortext import red_text


def main():
    groups_by_host = defaultdict(list)
    for group in CURDB.groups.get_groups():
        if group.card.tags.itype not in ('indexerproxy', 'indexerproxydisc'):
            continue
        if group.card.master and group.card.master.card.name == "ALL_DYNAMIC":
            continue

        for host in group.getHosts():
            groups_by_host[host].append(group)

    status = 0
    for host, groups in groups_by_host.iteritems():
        if len(groups) > 1:
            print red_text('Host {} have more than one indexerproxy instance in groups: {}'.format(host.name, ' '.join((x.card.name for x in groups))))
            status = 1

    return status


if __name__ == '__main__':
    retval = main()

    sys.exit(retval)
