#!/skynet/python/bin/python
"""Checks on resource allocated and dispenser quotas consistency"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg

from core.db import CURDB
from gaux.aux_dispenser import check_projects_allocated


def main():
    error_projects = check_projects_allocated(CURDB)

    if error_projects:
        error_message = ', '.join([x[0] for x in error_projects])
        raise Exception('Invalid allocated/quota in projects: {}'.format(error_message))

    return 0


if __name__ == '__main__':
    ret_value = main()
    sys.exit(ret_value)

