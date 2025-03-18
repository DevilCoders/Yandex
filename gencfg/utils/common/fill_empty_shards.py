#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from optparse import OptionParser
from collections import defaultdict

import gencfg
from core.db import CURDB
from core.instances import TIntGroup


def parse_cmd():
    usage = "Usage %prog [options]"
    parser = OptionParser(usage=usage)

    parser.add_option("-i", "--intlookup", type="str", dest="intlookup", default=None,
                      help="Obligatory. Intlookup file to work with")
    parser.add_option("-m", "--min-replicas", type=int, dest="min_replicas", default=1,
                      help="Optional. Minimal replicas count (1 by default)")
    parser.add_option("-M", "--max-replicas", type=int, dest="max_replicas", default=100000,
                      help="Optional. Maximum replicas count (too many by default)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    (options, args) = parser.parse_args()

    if options.intlookup is None:
        raise Exception("You must specify --intlookup")
    if options.max_replicas < options.min_replicas:
        raise Exception("Max replicas more than min replicas")

    options.intlookup = CURDB.intlookups.get_intlookup(options.intlookup)
    if options.intlookup.hosts_per_group != 1:
        raise Exception("Intlookups with more than 1 shard per groups not supported")

    return options


if __name__ == '__main__':
    options = parse_cmd()

    empty_brigade_groups = filter(lambda x: len(x.brigades) == 0, options.intlookup.brigade_groups)

    group = CURDB.groups.get_group(options.intlookup.base_type)
    free_instances = list(set(group.get_instances()) - set(group.get_busy_instances()))
    free_instances_by_host = defaultdict(list)
    for instance in free_instances:
        free_instances_by_host[instance.host].append(instance)
    free_instances = sum(map(lambda x: x[:len(empty_brigade_groups)], free_instances_by_host.values()), [])

    if len(empty_brigade_groups) * options.min_replicas > len(free_instances):
        raise Exception("Have %s empty brigades (need %s replicas each), but only %s free instances" % (len(empty_brigade_groups), options.min_replicas, len(free_instances)))

    free_instances = free_instances[:len(empty_brigade_groups) * options.max_replicas]
    for i in range(len(free_instances)):
        empty_brigade_group = empty_brigade_groups[i % len(empty_brigade_groups)]
        empty_brigade_group.brigades.append(TIntGroup([[free_instances[i]]], []))

    CURDB.update()
