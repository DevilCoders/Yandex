#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
import copy
from optparse import OptionParser

import gencfg
from core.db import CURDB


def update_slave(i1, i2, slave_intlookups, run_base, run_int):
    # update reserved
    for intlookup in slave_intlookups:
        for instance in intlookup.replaced_instances:
            if instance.name == i1.name and instance.port == i1.port:
                instance.name = i2.name
                instance.port = i2.port
                instance.power = i2.power
                instance.type = i2.type

    # update real instances
    class F:
        def __init__(self, i1, i2):
            self.i1 = i1
            self.i2 = i2

        def __call__(self, instance):
            if instance.name == self.i1.name:
                if instance.port == CURDB.groups.get_group(instance.type).funcs.instancePort(instance.name, self.i1.N):
                    instance.name = self.i2.name
                    instance.port = CURDB.groups.get_group(instance.type).funcs.instancePort(instance.name, self.i2.N)

    f = F(i1, i2)
    for intlookup in slave_intlookups:
        intlookup.for_each(f, run_base=run_base, run_int=run_int, run_used=True, run_free=True)


def parse_cmd():
    parser = OptionParser(usage="usage: %prog [options]")

    parser.add_option("-i", "--intlookups", type="str", dest="intlookups", help="comma-separated list of intlookups")
    parser.add_option("-s", "--slave-intlookups", type="str", dest="slave_intlookups",
                      help="comma-separated list of slave intlookups")
    parser.add_option("-n", "--ints-per-host", type="int", dest="ints_per_host", default=1,
                      help="maximum number of int ints per host")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()

    if not options.intlookups:
        raise Exception("Unspecified intlookups (-i option)")
    if len(args):
        raise Exception("Unknown options: %s" % (' '.join(args)))

    options.intlookups = map(lambda x: CURDB.intlookups.get_intlookup(os.path.basename(x)),
                             options.intlookups.split(','))
    options.slave_intlookups = map(lambda x: CURDB.intlookups.get_intlookup(os.path.basename(x)),
                                   options.slave_intlookups.split(',')) if options.slave_intlookups else []

    return options


if __name__ == '__main__':
    options = parse_cmd()

    int_counts = defaultdict(int)
    for intlookup in options.intlookups:
        def count(instance):
            int_counts[instance.host.name] += 1


        intlookup.for_each(count, run_base=False, run_int=True)

    base_group_type = options.intlookups[0].base_type
    int_group_type = base_group_type + '_INT'

    for intlookup in options.intlookups:
        for brigade_group in intlookup.brigade_groups:
            for brigade in brigade_group.brigades:
                for intsearch in brigade.intsearchers:
                    found = False
                    for lst in brigade.basesearchers:
                        for basesearch in lst:
                            if intsearch.host.switch == basesearch.host.switch and int_counts[basesearch.host.name] < options.ints_per_host:
                                print "Replacing %s:%s <-> %s:%s" % (intsearch.name, intsearch.port, basesearch.name, basesearch.port)

                                old_intsearch, old_basesearch = copy.deepcopy(intsearch), copy.deepcopy(basesearch)
                                int_counts[intsearch.name] -= 1
                                int_counts[basesearch.name] += 1

                                intsearch.swap_data(basesearch)
                                intsearch.port = CURDB.groups.get_group(int_group_type).funcs.instancePort(
                                    intsearch.host.name, intsearch.N)
                                intsearch.type = int_group_type
                                basesearch.port = CURDB.groups.get_group(base_group_type).funcs.instancePort(
                                    basesearch.host.name, basesearch.N)
                                basesearch.type = base_group_type

                                update_slave(old_intsearch, intsearch, options.slave_intlookups, False, True)
                                update_slave(old_basesearch, basesearch, options.slave_intlookups, True, False)

                                found = True
                                break
                        if found:
                            break

                    if not found:
                        raise Exception("Can not find replacement for %s:%s" % (intsearch.host.name, intsearch.port))

    CURDB.update()
