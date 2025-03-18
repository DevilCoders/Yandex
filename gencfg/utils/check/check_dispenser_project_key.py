#!/skynet/python/bin/python
"""Checks whether dispenser project key is valid"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

from core.db import CURDB


def main():
    abc_names = {x['slug'] for x in CURDB.abcgroups.abc_services.values()}
    none_prj_key_groups = []
    invalid_prj_key_groups = []
    for group in CURDB.groups.get_groups():
        if group.card.tags['metaprj'] == 'unsorted':
            continue

        if group.card.dispenser['project_key'] is None:
            none_prj_key_groups.append(str(group))
            continue

        if group.card.dispenser['project_key'].lower() not in abc_names:
            invalid_prj_key_groups.append(str(group))
            continue

    if none_prj_key_groups or invalid_prj_key_groups:
        if none_prj_key_groups:
            print('Dispenser project_key is not set for the following groups: {}'.format(', '.join(none_prj_key_groups)))

        if invalid_prj_key_groups:
            print('Dispenser project_key does not match any known ABC for the following groups: {}'.format(', '.join(invalid_prj_key_groups)))

        return -1

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)
