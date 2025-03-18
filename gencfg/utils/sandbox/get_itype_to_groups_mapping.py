#!/skynet/python/bin/python
import os
import sys
import json
import logging

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

from argparse import ArgumentParser
from core.db import CURDB


def get_itype_to_groups_mapping(db=CURDB):
    mapping = {}
    for group in db.groups.get_groups():
        itype = group.card.tags.itype
        if itype not in mapping:
            mapping[itype] = {}
        master = str(group.card.master or group.card.name)
        if master not in mapping[itype]:
            mapping[itype][master] = []
        mapping[itype][master].append(group.card.name)
    return mapping


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

    mapping = get_itype_to_groups_mapping(CURDB)
    print(json.dumps(mapping, indent=4))

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)
