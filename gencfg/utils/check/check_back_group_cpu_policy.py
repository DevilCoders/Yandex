#!/skynet/python/bin/python
"""Check whether reqs.instances.cpu_policy == idle for background groups"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

from core.db import CURDB


def main():
    bad_groups = []
    for group in CURDB.groups.get_groups():
        if group.card.properties.background_group and group.card.reqs.instances.cpu_policy != "idle":
            bad_groups.append(group.card.name)

    if bad_groups:
        print("The following background groups have reqs.instances.cpu_policy != idle: {0}".format(','.join(bad_groups)))
        return -1
    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)

