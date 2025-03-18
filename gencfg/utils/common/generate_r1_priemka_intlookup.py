#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'contrib')))

from collections import defaultdict
from argparse import ArgumentParser
from collections import OrderedDict

import gencfg
from core.db import CURDB
from core.instances import TIntGroup, TMultishardGroup
from utils.check import check_disk_size
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Generate intlookup for priemka/R1, take into account different slot sizes")

    parser.add_argument("-o", "--output-file", dest="output_file", type=str, required=True,
                        help="Obligatory. Intlookup to process")
    parser.add_argument("-g", "--group", dest="group", type=argparse_types.group, required=True,
                        help="Obligatory. Group name")
    parser.add_argument("-t", "--tiers", dest="tiers", type=str, required=True,
                        help="Obligatory. Tiers in format '<tier1>:<tier1 slot size>,<tier2><tier2 slot size>,...'")
    parser.add_argument("-b", "--bases-per-group", type=int, dest="bases_per_group", required=True,
                        help="Obligatory (without -r). Hosts per group")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    options.tiers = OrderedDict(map(lambda x: (x.split(':')[0], int(x.split(':')[1])), options.tiers.split(',')))
    options.worst_tier_size = max(
        map(lambda x: CURDB.tiers.tiers[x].disk_size / options.tiers[x], options.tiers.keys()))
    options.N = sum(map(lambda x: CURDB.tiers.get_total_shards(x), options.tiers.keys()))

    for tier in options.tiers.keys():
        if CURDB.tiers.get_total_shards(tier) % options.bases_per_group != 0:
            raise Exception("Tier %s has %s shards, which is not divided by %s" % (tier, CURDB.tiers.get_total_shards(tier), options.bases_per_group))

    return options


def pop_instances(host_instances, slots_per_instance, N):
    result_instances = []
    for i in range(10):  # FIXME: check if result instances was changed during last step
        for host, instances in host_instances:
            if len(instances) < slots_per_instance:
                continue
            result_instances.append(instances[0])
            instances[:slots_per_instance] = []
            if len(result_instances) == N:
                return result_instances
    raise Exception("Can not find enough instances with slot size %s" % slots_per_instance)


def main(options):
    if CURDB.intlookups.has_intlookup(options.output_file):
        CURDB.intlookups.remove_intlookup(options.output_file)
    intlookup = CURDB.intlookups.create_empty_intlookup(options.output_file)
    intlookup.brigade_groups_count = options.N / options.bases_per_group
    intlookup.tiers = options.tiers.keys()
    intlookup.hosts_per_group = options.bases_per_group
    intlookup.base_type = options.group.card.name

    # get all instances and group them by host
    host_instances = defaultdict(list)
    for instance in set(options.group.get_instances()) - set(options.group.get_busy_instances()):
        host_instances[instance.host].append(instance)

    # find current disk size
    suboptions = {
        'hosts': list(set(map(lambda x: x.host, options.group.get_instances()))),
        'verbose': False,
        'show_top': False,
        'check_ssd_size': False,
    }
    host_used_disk = check_disk_size.main(type("CheckDiskSizeOptions", (object,), suboptions)(), from_cmd=False)

    # filter out instances base on disk size
    for host in host_instances:
        N = (host.disk - options.group.card.properties.extra_disk_size - host_used_disk.get(host, 0)) / (
        options.worst_tier_size + options.group.card.properties.extra_disk_size_per_instance) / options.group.card.properties.extra_disk_shards
        host_instances[host] = host_instances[host][:N]

    host_instances = host_instances.items()
    host_instances.sort(cmp=lambda (x1, y1), (x2, y2): cmp('%s:%s' % (x1.dc, x1.name), '%s:%s' % (x2.dc, x2.name)))

    for i in range(intlookup.brigade_groups_count):
        intlookup.brigade_groups.append(TMultishardGroup())

        tier = intlookup.get_tier_for_shard(i * intlookup.hosts_per_group)
        brigade_instances = map(lambda x: [x],
                                pop_instances(host_instances, options.tiers[tier], intlookup.hosts_per_group))
        brigade = TIntGroup(brigade_instances, [])
        intlookup.brigade_groups[-1].brigades.append(brigade)

    CURDB.update()


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
