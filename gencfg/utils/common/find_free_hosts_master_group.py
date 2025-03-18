#!/skynet/python/bin/python
# condig: utf8
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB


if __name__ == '__main__':
    if len(sys.argv) < 2:
        print('./find_free_hosts_master_group.py <GROUP1>[,<GROUP2>,...] [-v]')
        sys.exit(1)

    all_slaves_hosts = []
    for master_group_name in sys.argv[1].split(','):
        master_group = CURDB.groups.get_group(master_group_name)
        only_master_hosts = set(master_group.getHosts())

        for group in master_group.card.slaves:
            only_master_hosts -= set(group.getHosts())

        if len(sys.argv) > 2 and sys.argv[2] == '-v':
            print('{}: {}'.format(
                master_group_name,
                ','.join(x.name for x in only_master_hosts)
            ))
        all_slaves_hosts.extend(only_master_hosts)

    if len(sys.argv) < 3 or sys.argv[2] != '-v':
        print(','.join(x.name for x in set(all_slaves_hosts)))

