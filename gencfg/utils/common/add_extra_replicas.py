#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
from core.instances import TIntGroup
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Add extra replicas to intlookup (from free instances)")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=["addextra", "addtotal"],
                        help="Obligatory. Action to execute")
    parser.add_argument("-i", "--intlookup", dest="intlookup", type=argparse_types.intlookup, required=True,
                        help="Obligatory. Intlookup to process")
    parser.add_argument("-r", "--replicas", dest="replicas", type=int, required=True,
                        help="Obligatory. Number of replicas to add")
    parser.add_argument("-f", "--filter", dest="filter", type=argparse_types.pythonlambda, default=None,
                        help="Optional. Filter on free instances")
    parser.add_argument("-n", "--ignore-replicas-intersection", dest="ignore_replicas_intersection",
                        action="store_true", default=False,
                        help="Optional. Ignore repilcas intersection (some hosts can be used twice or more in every shard)")
    parser.add_argument("--ignore-not-enough", action="store_true", default=False,
                        help="Optional. Do not fail if we do not have enough instances")
    parser.add_argument("--free-instances-from-intlookup", action="store_true", default=False,
                        help="Optional. If do not have enough instances, get them from intlookup")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.intlookup.base_type is None:
        raise Exception("Undefined base_type in %s" % options.intlookup.file_name)

    return options


def normalize(options):
    if options.action == "addextra" and options.free_instances_from_intlookup:
        raise Exception("Option <--action addextra> and <--free-instances-from-intlookup> are incompatable")


def free_lacking_instances(options, mygroup, free_instances):
    assert (len(mygroup.card.intlookups) == 1), "Group %s has %d intlookups: %s" % (mygroup.card.name, len(mygroup.card.intlookups), ",".join(mygroup.card.intlookups))
    intlookup = CURDB.intlookups.get_intlookup(mygroup.card.intlookups[0])

    # calculate how many instances we need to reclaim
    need_instances = 0
    max_replicas_per_shard = 0
    for shardid in range(intlookup.get_shards_count()):
        N = len(intlookup.get_base_instances_for_shard(shardid))

        max_replicas_per_shard = max(N, max_replicas_per_shard)
        if N < options.replicas:
            need_instances += options.replicas - N
    need_instances -= len(free_instances)

    # free instances
    while need_instances > 0:
        max_replicas_per_shard -= 1
        print "MAX REPICAS IS %d" % max_replicas_per_shard
        if max_replicas_per_shard < options.replicas:
            print "Not enough %d instances while freeing instances from intlookup" % need_instances
        for brigade_group in intlookup.get_multishards():
            for i in range(max_replicas_per_shard, len(brigade_group.brigades)):
                reclaimed = brigade_group.brigades[i].get_all_basesearchers()
                need_instances -= len(reclaimed)
                free_instances.extend(reclaimed)
            brigade_group.brigades = brigade_group.brigades[:max_replicas_per_shard]


def main(options):
    mygroup = CURDB.groups.get_group(options.intlookup.base_type)

    free_instances = set(mygroup.get_instances()) - set(mygroup.get_busy_instances())
    if options.filter:
        free_instances = set(filter(lambda x: options.filter(x), free_instances))
    free_instances = list(free_instances)

    if options.free_instances_from_intlookup:
        free_lacking_instances(options, mygroup, free_instances)

    # check if we have enough free instances
    need_instances = 0
    for brigade_group in options.intlookup.get_multishards():
        if options.action == "addextra":
            need_instances += options.replicas * options.intlookup.hosts_per_group
        elif options.action == "addtotal":
            need_instances += max(0, options.replicas - len(brigade_group.brigades)) * options.intlookup.hosts_per_group
    if need_instances > len(free_instances) and not options.ignore_not_enough:
        raise Exception("Not enough instances: have %s, need %s" % (len(free_instances), need_instances))

    no_more_instances = False
    for i in range(options.replicas):
        for brigade_group in options.intlookup.get_multishards():
            if options.action == "addtotal" and len(brigade_group.brigades) >= options.replicas:
                continue

            if options.ignore_replicas_intersection:
                free_for_me = list(free_instances)
            else:
                myhosts = map(lambda x: x.host,
                              sum(sum(map(lambda x: x.basesearchers, brigade_group.brigades), []), []))
                free_for_me = filter(lambda x: x.host not in myhosts, free_instances)

            """
                Combine instances by locality
            """

            def locality_sort(i1, i2):
                return cmp((i1.port, i1.host.name, i1.host.queue, i1.host.dc, i1.host.location),
                           (i2.port, i2.host.name, i2.host.queue, i2.host.dc, i2.host.location))

            free_for_me.sort(cmp=locality_sort)

            if len(free_for_me) < options.intlookup.hosts_per_group:
                if options.ignore_not_enough:
                    no_more_instances = True
                    break
                else:
                    raise Exception("Not enough free instances: have %s, need %s" % (len(free_for_me), options.intlookup.hosts_per_group))

            brigade_group.brigades.append(
                TIntGroup(map(lambda x: [x], free_for_me[:options.intlookup.hosts_per_group]), []))
            for j in range(options.intlookup.hosts_per_group):
                free_instances.remove(free_for_me[j])

        if no_more_instances:
            break

    options.intlookup.mark_as_modified()
    CURDB.intlookups.update(smart=1)


if __name__ == '__main__':
    options = parse_cmd()

    normalize(options)

    main(options)
