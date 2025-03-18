#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import re

import gencfg

from core.db import CURDB
from gaux.aux_colortext import red_text


def main():
    failed = False

    groups_by_prj_id = dict()
    groups_without_prj_id = []

    for group in CURDB.groups.get_groups():
        hbf_id = group.card.properties.hbf_project_id

        if hbf_id == 0:
            print red_text("Group <%s> does not have hbf project id" % group.card.name)
            failed = True
            continue
        if hbf_id in groups_by_prj_id:
            print red_text("Groups <%s> and <%s> have same hbf project id <%s>" % (group.card.name, groups_by_prj_id[hbf_id], hbf_id))
            failed = True
            continue

        groups_by_prj_id[hbf_id] = group.card.name

    return int(failed)

if __name__ == '__main__':
    status = main()
    sys.exit(status)
