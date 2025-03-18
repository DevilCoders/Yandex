#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
import core.argparse.types as argparse_types

from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from gaux.aux_staff import unwrap_dpts


def get_parser():
    parser = ArgumentParserExt("Utility to change group hbf range")
    parser.add_argument("--db", type=argparse_types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument("-g", "--group", type=argparse_types.group, required=True,
                        help="Obligatory. Group name.")
    parser.add_argument("--hbf-range", type=argparse_types.hbf_range, required=True,
                        help="Optional. Hbf range name.")

    return parser


def main(db, group, hbf_range):
    if hbf_range.name == group.card.properties.hbf_range:
        print('Group {} has {} hbf range yet.'.format(group.card.name, hbf_range.name))
        return

    hbf_range_acl = set(unwrap_dpts(hbf_range.acl))
    group_owners = set(unwrap_dpts(group.card.owners))
    if hbf_range.name != '_GENCFG_SEARCHPRODNETS_ROOT_' and not hbf_range_acl.intersection(group_owners):
        raise ValueError('Group {}({}) and range {}({}) do not have common owners'.format(
            group.card.name, ','.join(group.card.owners), hbf_range.name, ','.join(hbf_range.acl)
        ))

    if group.card.properties.hbf_project_id not in group.card.properties.hbf_old_project_ids:
        group.card.properties.hbf_old_project_ids.append(group.card.properties.hbf_project_id)
    group.card.properties.hbf_project_id = hbf_range.get_next_project_id()
    group.card.properties.hbf_range = hbf_range.name

    group.mark_as_modified()
    db.groups.update(smart=True)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options.db, options.group, options.hbf_range)

