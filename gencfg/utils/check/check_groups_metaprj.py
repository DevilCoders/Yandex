#!/skynet/python/bin/python
"""Checks on system metaprj set only in allow groups"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

from core.db import CURDB
from gaux.aux_reserve import UNSORTED_GROUP


STATIC_METAPRJ_MAPPING = {
    'reserve': {
        'MAN_RESERVED',
        'SAS_RESERVED',
        'VLA_RESERVED',
        'MSK_RESERVED',
        'UNKNOWN_RESERVED',
    },
}


def check_groups_metaprj(db=CURDB):
    error_group = []
    for group in db.groups.get_groups():
        group_name = group.card.name
        metaprj = group.card.tags.metaprj

        if (metaprj in STATIC_METAPRJ_MAPPING and group_name not in STATIC_METAPRJ_MAPPING[metaprj]) or \
                (metaprj == 'unsorted' and not group_name.startswith('ALL_UNSORTED')):
            error_group.append((group_name, 'Invalid metaprj {} for {}'.format(metaprj, group_name)))

    return error_group


def main():
    error_group = check_groups_metaprj(CURDB)
    if error_group:
        error_message = ', '.join([x[1] for x in error_group])
        raise Exception('Invalid metaprj for groups: {}'.format(error_message))


if __name__ == '__main__':
    main()
