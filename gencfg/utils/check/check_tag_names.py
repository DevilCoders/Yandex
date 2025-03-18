#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import re

import gencfg
from core.db import CURDB
from core.itypes import ITYPE_PATTERN
from gaux.aux_decorators import gcdisable


@gcdisable
def main():
    CTYPE_REG = re.compile("^[a-z][a-z0-9-]+$")
    ITYPE_REG = re.compile(ITYPE_PATTERN)
    PRJ_REG = re.compile("^[a-z][a-z0-9-]+$")
    METAPRJ_REG = re.compile("^[a-z][a-z0-9-]+$")

    failed = False
    for group in CURDB.groups.get_groups():
        if not re.match(CTYPE_REG, group.card.tags.ctype):
            print "Group %s has invalid ctype tag <%s> (should satisfy expression <%s>)" % (group.card.name, group.card.tags.ctype, CTYPE_REG.pattern)
            failed = True
        if not re.match(ITYPE_REG, group.card.tags.itype):
            print "Group %s has invalid itype tag <%s> (should satisfy expression <%s>)" % (group.card.name, group.card.tags.itype, ITYPE_REG.pattern)
            failed = True
        for prj in group.card.tags.prj:
            if not re.match(PRJ_REG, prj):
                print "Group %s has invalid prj tag <%s> (should satisfy expression <%s>)" % (group.card.name, group.card.tags.prj, PRJ_REG.pattern)
                failed = True
        if not re.match(METAPRJ_REG, group.card.tags.metaprj):
            print "Group %s has invalid metaprj tag <%s> (should satisfy expression <%s>)" % (group.card.name, group.card.tags.metaprj, METAPRJ_REG.pattern)

    if failed:
        sys.exit(1)

if __name__ == '__main__':
    main()
