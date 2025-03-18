#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from gaux.aux_colortext import red_text
from core.db import CURDB


def main():
    hosts_from_groups = set()
    for group in CURDB.groups.get_groups():
        if group.card.properties.background_group:
            continue
        hosts_from_groups |= set(group.getHosts())

    hosts_from_db = set(CURDB.hosts.get_all_hosts())

    not_in_groups = hosts_from_db - hosts_from_groups

    if len(not_in_groups):
        print red_text("Hosts not assigned to group: %s" % ",".join(map(lambda x: x.name, not_in_groups)))
        return 1
    else:
        return 0


if __name__ == '__main__':
    retcode = main()
    sys.exit(retcode)
