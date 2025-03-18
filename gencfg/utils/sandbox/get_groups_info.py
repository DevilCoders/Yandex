#!/skynet/python/bin/python
import os
import sys
import json
import logging

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

from argparse import ArgumentParser
from core.db import CURDB


def get_groups_info(db=CURDB):
    groups_info = []

    for group in db.groups.get_groups():
        name = group.card.name
        itype = group.card.tags.itype
        master = str(group.card.master or name)
        hosts_count = len(group.get_hosts())

        if group.card.properties.background_group:
            continue

        groups_info.append((name, master, itype, hosts_count))

    return groups_info


def parse_cmd():
    parser = ArgumentParser(description='Script to get mapping itype -> master -> slave')

    parser.add_argument('-v', '--verbose', action='store_true', default=False,
                        help='Optional. Explain what is being done.')

    options = parser.parse_args()

    return options


def main():
    options = parse_cmd()

    if options.verbose:
        logging.basicConfig(level=logging.INFO)

    groups_info = get_groups_info(CURDB)
    for group_info in groups_info:
        print(' '.join(map(str, group_info)))

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)
