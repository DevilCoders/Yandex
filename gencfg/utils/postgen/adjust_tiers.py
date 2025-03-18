#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import copy
import random
from collections import defaultdict
from optparse import OptionParser

import gencfg
from core.db import CURDB


def parse_cmd():
    parser = OptionParser(usage="usage: %prog [options]")

    parser.add_option("-i", "--intlookup", type="str", dest="intlookup", help="intlookup to perform changes")
    parser.add_option("-s", "--shuffle", action="store_true", dest="shuffle", default=False, help="shuffle instances")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()

    if not options.intlookup:
        raise Exception("Unspecified intlookup (-i option)")
    if len(args):
        raise Exception("Unknown options: %s" % (' '.join(args)))

    options.intlookup = CURDB.intlookups.get_intlookup(os.path.basename(options.intlookup))

    return options


def process_tier(mytier, options):
    start_b = 0
    end_b = CURDB.tiers.get_tier(mytier).get_shards_count() / options.intlookup.hosts_per_group
    for other_tier in options.intlookup.tiers:
        if other_tier != mytier:
            shift = CURDB.tiers.get_tier(other_tier).get_shards_count() / options.intlookup.hosts_per_group
            start_b += shift
            end_b += shift
        else:
            break

    basesearchers = []
    for brigade_group in options.intlookup.get_multishards()[start_b:end_b]:
        for brigade in brigade_group.brigades:
            basesearchers.extend(brigade.get_all_basesearchers())
    basesearchers = copy.deepcopy(basesearchers)

    if options.shuffle:
        by_port = defaultdict(list)
        for basesearch in basesearchers:
            by_port[basesearch.port].append(basesearch)
        for port in by_port:
            random.shuffle(by_port[port])
        new_basesearchers = []
        # FIXME: hack
        for port in reversed(sorted(by_port.keys())):
            new_basesearchers.extend(by_port[port])
        basesearchers = new_basesearchers

    cur = 0
    for brigade_group in options.intlookup.get_multishards()[start_b:end_b]:
        for brigade in brigade_group.brigades:
            for lst in brigade.basesearchers:
                for i in range(len(lst)):
                    lst[i] = basesearchers[cur]
                    cur += 1


def main(options):
    for tier in options.intlookup.tiers:
        process_tier(tier, options)

    options.intlookup.mark_as_modified()

    CURDB.intlookups.update(smart=True)


if __name__ == '__main__':
    options = parse_cmd()

    main(options)
