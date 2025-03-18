#!/skynet/python/bin/python
import os
import sys
import json
import logging

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

from argparse import ArgumentParser
from core.db import CURDB


def get_groups_to_hbf_mapping(db=CURDB):
    mapping = {}
    for group in db.groups.get_groups():
        mapping[group.card.name] = {
            'hbf_range': group.card.properties.hbf_range,
            'hbf_parent_macros': group.card.properties.hbf_parent_macros,
            'hbf_old_project_ids': group.card.properties.hbf_old_project_ids,
            'hbf_project_id': group.card.properties.hbf_project_id,
        }
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

    mapping = get_groups_to_hbf_mapping(CURDB)
    print(json.dumps(mapping, indent=4))

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)
