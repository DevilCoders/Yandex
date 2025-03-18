#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from gaux.aux_colortext import red_text

from gaux.aux_decorators import gcdisable

@gcdisable
def main():
    result = dict()

    for group in CURDB.groups.get_groups():
        if group.card.master is not None:
            continue

        master_hosts = set(group.getHosts())

        for slave in group.slaves:
            extra_hosts = set(slave.getHosts()) - master_hosts
            if len(extra_hosts):
                result[slave] = extra_hosts

    if len(result.keys()) != 0:
        for group in result:
            print red_text("Slave group %s (master %s) has %d extra hosts: % s" % (
                group.card.name, group.card.master.card.name, len(result[group]), ",".join(map(lambda x: x.name, result[group]))))
        sys.exit(1)
    else:
        sys.exit(0)

if __name__ == '__main__':
    main()
