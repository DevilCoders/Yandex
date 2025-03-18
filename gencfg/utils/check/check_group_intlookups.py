#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB
from gaux.aux_colortext import red_text

if __name__ == '__main__':
    group_intlookups = defaultdict(set)
    for intlookup in CURDB.intlookups.get_intlookups():
        for instance in intlookup.get_instances():
            group_intlookups[instance.type].add(intlookup.file_name)

    status = 0
    for group in CURDB.groups.get_groups():
        not_found_in_group = group_intlookups[group.card.name] - set(group.card.intlookups)
        if len(not_found_in_group) > 0:
            print red_text("Group %s: intlookups <%s> not found in group card, but contain group instances" % (group.card.name, ",".join(not_found_in_group)))
            status = 1

        not_found_in_intlookups = set(group.card.intlookups) - group_intlookups[group.card.name]
        not_found_in_intlookups = [x for x in not_found_in_intlookups if (len(CURDB.intlookups.get_intlookup(x).get_instances()) > 0)]
        if len(not_found_in_intlookups) > 0:
            print red_text("Group %s: intlookups <%s> from group card do not contain any group instances" % (group.card.name, ",".join(not_found_in_intlookups)))
            status = 1

    sys.exit(status)
