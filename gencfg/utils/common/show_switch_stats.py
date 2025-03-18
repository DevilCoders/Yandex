#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
from optparse import OptionParser

import gencfg
from core.db import CURDB


def parse_cmd():
    parser = OptionParser(usage='usage: %prog -s <switches> [-n]')

    parser.add_option("-s", "--switch", type="str", dest="switch", default=None,
                      help="obligatory. Switch.")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options, args = parser.parse_args()
    return options


if __name__ == '__main__':
    options = parse_cmd()

    by_group = defaultdict(int)
    intlookups = CURDB.intlookups.get_intlookups()
    for intlookup in intlookups:
        instances = intlookup.get_used_base_instances() + intlookup.get_used_int_instances()
        for instance in instances:
            if instance.host.switch == options.switch:
                by_group[instance.type] += 1

    print 'Statistics from intlookups:'
    for key in sorted(by_group.keys()):
        print '%s: %s instances' % (key, by_group[key])
    print

    by_group = defaultdict(int)
    for group in CURDB.groups.get_groups():
        for host in group.getHosts():
            if host.switch == options.switch:
                by_group[group.card.name] += 1
    print 'Statistics from groups:'
    for key in sorted(by_group.keys()):
        print '%s: %s hosts' % (key, by_group[key])
    print
