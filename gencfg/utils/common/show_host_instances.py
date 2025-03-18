#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser("Show all host instances (with shard id for sharded instances)")

    parser.add_argument("-t", "--type", type=str,
                        help="Optional. Show instances of specific group (otherwise all instances will be shown)")
    parser.add_argument("-s", "--hosts-or-instances", type=argparse_types.argparse_hosts_or_instances, required=True,
                        help="Obligatory. Comma-separated list of hosts")
    parser.add_argument("-A", "--all", dest="all", default=False, action='store_true',
                        help="Optional. Include non-intlookup instances")
    parser.add_argument('-i', '--ignore-background', action='store_true', default=False,
                        help='Optional. Do not show background groups in output')

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    return options


if __name__ == '__main__':
    options = parse_cmd()

    hosts_instances = options.hosts_or_instances.get_all_instances()
    if options.type is not None:
        hosts_instances = filter(lambda x: x.type == options.type, hosts_instances)
    if options.ignore_background:
        hosts_instances = [x for x in hosts_instances if CURDB.groups.get_group(x.type).card.properties.background_group == False]
    hosts_instances = dict(map(lambda x: (x, []), hosts_instances))

    # calc shards
    groups = list(set(instance.type for instance in hosts_instances))
    groups = [CURDB.groups.get_group(group) for group in groups]
    intlookups = sum([group.card.intlookups for group in groups], [])
    intlookups = set(intlookups)
    intlookups = [CURDB.intlookups.get_intlookup(intlookup) for intlookup in intlookups]
    for intlookup in intlookups:
        if intlookup.tiers is None:
            shards_info = ["???" for _ in range(intlookup.get_shards_count())]
        else:
            shards_info = []
            for tier in intlookup.tiers:
                for shard in range(CURDB.tiers.get_tier(tier).get_shards_count()):
                    shards_info.append('%s[%s]' % (tier, shard))
            assert (len(shards_info) == intlookup.get_shards_count())

        for shard in range(intlookup.get_shards_count()):
            shard_instances = intlookup.get_base_instances_for_shard(shard)
            for instance in shard_instances:
                if instance in hosts_instances:
                    hosts_instances[instance].append(shards_info[shard])

        for shard in range(intlookup.get_shards_count()):
            if shard % intlookup.hosts_per_group != 0:
                continue
            first_shard = shard
            last_shard = shard + intlookup.hosts_per_group - 1
            shard_instances = intlookup.get_int_instances_for_shard(shard)
            for instance in shard_instances:
                if instance in hosts_instances:
                    hosts_instances[instance].append('%s,...,%s' % (shards_info[first_shard], shards_info[last_shard]))

    for instance, instance_shards in list(hosts_instances.items()):
        if not instance_shards:
            if options.all:
                instance_shards.append('-')
            else:
                del hosts_instances[instance]

    for instance in sorted(hosts_instances, cmp=lambda x, y: cmp(str(x.type), str(y.type))):
        print '%s -- with shards\t%s (donor %s)' % (
        instance, ', '.join(hosts_instances[instance]), CURDB.groups.get_group(instance.type).card.host_donor)
