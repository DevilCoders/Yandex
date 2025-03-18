#!/skynet/python/bin/python
"""Checks on group dispenser.project_key is leaf"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

from core.db import CURDB
from gaux.aux_dispenser import check_groups_project


def main():
    error_groups = check_groups_project(CURDB)

    if error_groups:
        error_message = ', '.join([x[0] for x in error_projects])
        raise Exception('Invalid project_key in group: {}'.format(error_message))

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)

