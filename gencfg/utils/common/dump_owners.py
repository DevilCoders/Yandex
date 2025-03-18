#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
import config
from core.db import CURDB

CONFIG = [
    ('owners', lambda x: x.card.owners),
    ('sudoers', lambda x: x.card.access.sudoers),
    ('sshers', lambda x: x.card.access.sshers),
]


def source_data(func):
    source_groups = [x for x in CURDB.groups.get_groups() if
                     ((x.card.properties.nonsearch == False) or (x.card.properties.export_to_cauth == True))]
    source_groups = sorted(source_groups, cmp=lambda x, y: cmp(x.card.name, y.card.name))
    for group in source_groups:
        owners = func(group)
        if len(owners) == 0:
            continue
        yield (group.card.name, ','.join(owners), group.card.description)


if __name__ == '__main__':
    for access_type, owners_func in CONFIG:
        with open(os.path.join(config.WEB_GENERATED_DIR, 'group.%s' % access_type), 'w') as file_owners:
            with open(os.path.join(config.WEB_GENERATED_DIR, 'group.%s.extended' % access_type),
                      'w') as file_owners_extended:
                for name, owners, description in source_data(owners_func):
                    print >> file_owners, '\t'.join((name, owners))
                    print >> file_owners_extended, "\t".join((name, owners, description))
