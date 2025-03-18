#!/skynet/python/bin/python
import os
import sys
import json
import logging

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import gencfg

from argparse import ArgumentParser
from core.db import CURDB


def get_group_to_aggregate_keys_mapping(db=CURDB):
    mapping = {}
    for group in db.groups.get_groups():
        group_name = group.card.name
        abc_service = group.card.dispenser.project_key
        master_group = str(group.card.master or group.card.name)
        itype = group.card.tags.itype

        mapping[group_name] = {
            'abc_service': abc_service,
            'master_group': master_group,
            'itype': itype,
        }
    return mapping


def parse_cmd():
    parser = ArgumentParser(description='Script to get mapping group -> (abc service, master, itype')

    parser.add_argument('-v', '--verbose', action='store_true', default=False,
                        help='Optional. Explain what is being done.')

    options = parser.parse_args()

    return options


def main():
    options = parse_cmd()

    if options.verbose:
        logging.basicConfig(level=logging.INFO)

    mapping = get_group_to_aggregate_keys_mapping(CURDB)
    print(json.dumps(mapping, indent=4))

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)
