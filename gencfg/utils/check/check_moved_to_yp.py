#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB


LOCATIONS = {
    'man', 'man-pre', 'msk', 'sas', 'sas-test', 'vla',
}

def main():
    failed = False

    for group in CURDB.groups.get_groups():
        group_name = group.card.name
        endpoint_set = group.card.yp.where.endpoint_set
        location = group.card.yp.where.location
        if bool(endpoint_set) != bool(location):
            print 'group {}: yp.where.endpoint_set and yp.where.location both should be set or null'.format(group_name)
            failed = True
        if location is not None and location not in LOCATIONS:
            print 'group {}: yp.where.location {} is not supported'.format(group_name, location)
            failed = True
    if failed:
        sys.exit(1)


if __name__ == '__main__':
    main()
