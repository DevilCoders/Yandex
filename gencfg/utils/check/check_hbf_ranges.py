#!/skynet/python/bin/python
"""Checks on hbfranges consistency"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

from core.db import CURDB
from gaux.aux_staff import unwrap_dpts


def check_hbf_ranges(db=CURDB):
    error_groups = []
    max_project_id = {x: 0 for x in db.hbfranges.to_json()}

    for group in db.groups.get_groups():
        hbf_range = CURDB.hbfranges.get_range_by_name(group.card.properties.hbf_range)
        hbf_range_by_project_id = CURDB.hbfranges.get_range_by_project_id(group.card.properties.hbf_project_id)

        hbf_range_acl = set(unwrap_dpts(hbf_range.acl))
        group_owners = set(unwrap_dpts(group.card.owners))

        if hbf_range != hbf_range_by_project_id:
            message = '{} ({} != {})'.format(group.card.name, hbf_range.name, hbf_range_by_project_id.name)
            error_groups.append((group, hbf_range, message))

        if hbf_range.name != '_GENCFG_SEARCHPRODNETS_ROOT_' and not hbf_range_acl.intersection(group_owners):
            message = '{}({}) do not have common owners with {}({})'.format(
                group.card.name, ','.join(group.card.owners), hbf_range.name, ','.join(hbf_range.acl)
            )
            error_groups.append((group, hbf_range, message))

        if max_project_id[hbf_range_by_project_id.name] < group.card.properties.hbf_project_id:
            max_project_id[hbf_range_by_project_id.name] = group.card.properties.hbf_project_id

    return error_groups, max_project_id


def main():
    error_groups, max_project_id = check_hbf_ranges(CURDB)

    print('Project IDs left in ranges:')
    for hbf_range_name, max_proj_id in max_project_id.items():
        hbf_range = CURDB.hbfranges.get_range_by_name(hbf_range_name)
        print('    {}: {} (max project_id {})'.format(hbf_range_name, hbf_range.to - max(max_proj_id, hbf_range.from_), max_proj_id))

    if error_groups:
        error_message = ', '.join([x[2] for x in error_groups])
        raise Exception('Invalid project_id/range in groups: {}'.format(error_message))

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)

