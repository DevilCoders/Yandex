#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from gaux.aux_staff import get_possible_group_owners


if __name__ == '__main__':
    pot_owners = set(get_possible_group_owners())
    pot_watchers = set(get_possible_group_owners())

    failed = False
    for group in CURDB.groups.get_groups():
        if group.card.properties.nonsearch:
            continue
        if group.card.tags.itype == "vmguest":
            # virtual machine guests can have owners from any department
            continue

        if len(set(group.card.owners) - pot_owners) > 0:
            print "Group %s has wrong owners: %s" % (group.card.name, ','.join(list(set(group.card.owners) - pot_owners)))
            failed = True

        if group.card.properties.expires is not None:
            group_pot_watchers = pot_owners | pot_watchers
        else:
            group_pot_watchers = pot_watchers
        if len(set(group.watchers) - group_pot_watchers) > 0:
            print "Group %s has wrong watchers: %s" % (group.card.name, ','.join(list(set(group.watchers) - group_pot_watchers)))
            failed = True

    if failed:
        sys.exit(1)
