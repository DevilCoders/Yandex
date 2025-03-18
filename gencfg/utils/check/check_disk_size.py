#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
from argparse import ArgumentParser

import gencfg

from core.db import CURDB
import core.argparse.types as argparse_types

SSD_COEFF = 0.6


def parse_cmd():
    parser = ArgumentParser(description="Check if we have enough disk")
    parser.add_argument("-c", "--sas-config", dest="sas_config", type=argparse_types.sasconfig, required=False,
                        help="Optional. Check for all hosts from specified sas config")
    parser.add_argument("-g", "--groups", dest="groups", type=argparse_types.groups, required=False,
                        help="Optional. Check for all host from speified groups")
    parser.add_argument("-i", "--intlookups", dest="intlookups", type=argparse_types.intlookups, required=False,
                        help="Optional. Check for all hosts from specified intlookups")
    parser.add_argument("-s", "--hosts", dest="hosts", type=argparse_types.hosts, required=False,
                        help="Optional. Show statistics for specified hosts")
    parser.add_argument("-v", "--verbose", dest="verbose", action="store_true", default=False,
                        help="Optional. Show verbose output")
    parser.add_argument("-t", "--show-top", dest="show_top", action="store_true", default=False,
                        help="Optional. Show machines with the least disk left")
    parser.add_argument("--check-ssd-size", dest="check_ssd_size", action="store_true", default=False,
                        help="Optional. Check ssd size also")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if len(filter(lambda x: x is not None,
                  [options.sas_config, options.groups, options.intlookups, options.hosts])) != 1:
        raise Exception("You must specify exactly one of --sas-config --groups --intlookups --hosts option")

    if options.sas_config:
        options.intlookups = map(lambda x: CURDB.intlookups.get_intlookup(x.intlookup), options.sas_config)
    if options.groups:
        options.hosts = sum(map(lambda x: x.getHosts(), options.groups), [])
    if options.intlookups:
        options.hosts = []
        for intlookup in options.intlookups:
            instances = intlookup.get_used_instances()
            options.hosts.extend(map(lambda x: x.host, instances))
    options.hosts = set(options.hosts)

    return options


def main(options, from_cmd=True):
    tiers_sizes = dict(map(lambda x: (x.name, x.disk_size), CURDB.tiers.tiers.values()))

    if len(options.hosts) < 30:
        affected_groups = list(set(sum(map(lambda x: CURDB.groups.get_host_groups(x), options.hosts), [])))
    else:
        affected_groups = CURDB.groups.get_groups()

    seen_intlookups = set()

    hostsdata = defaultdict(list)
    for group in affected_groups:
        if len(group.card.intlookups) == 0:
            for instance in group.get_instances():
                hostsdata[instance.host].append((None, None, group.card.name))
        else:
            for intlookup in map(lambda x: CURDB.intlookups.get_intlookup(x), group.card.intlookups):
                if intlookup.tiers is None:
                    continue

                if intlookup in seen_intlookups:
                    continue
                if intlookup.file_name in ('MSK_IMGS_BASE_PRISM',):
                    continue

                seen_intlookups.add(intlookup)

                for tier in intlookup.tiers:
                    fst, lst = intlookup.get_tier_shards_range(tier)
                    brigade_groups = intlookup.get_multishards()[fst:lst]
                    for i in range(len(brigade_groups)):
                        for brigade in brigade_groups[i].brigades:
                            #                            for j in range(intlookup.hosts_per_group):
                            for j in range(len(brigade.basesearchers)):
                                basesearchers = brigade.basesearchers[j]
                                for instance in basesearchers:
                                    hostsdata[instance.host].append(
                                        (tier, i * intlookup.hosts_per_group + j, instance.type))

    result = {}
    failed = 0
    for host in hostsdata:
        if host not in options.hosts:
            continue

        total_size = 0
        seen_shards = set()
        seen_groups = set()
        explanation = []
        for tier, shard_id, group_name in hostsdata[host]:
            group = CURDB.groups.get_group(group_name)

            # calculate disk per host for group
            extra_per_host = group.card.properties.extra_disk_size
            if group.card.name not in seen_groups:
                if extra_per_host > 0:
                    explanation.append("Need %d extra disk for group %s" % (extra_per_host, group_name))
                total_size += extra_per_host
                seen_groups.add(group.card.name)

            # calculate disk per instance for group
            extra_per_instance = group.card.properties.extra_disk_size_per_instance
            if extra_per_instance > 0:
                explanation.append("Need %d extra disk for shard in group %s" % (extra_per_instance, group_name))
                total_size += extra_per_instance

            # cacluate disk for shards
            extra_disk_shards = group.card.properties.extra_disk_shards
            if (tier, shard_id) == (None, None):
                continue
            if (tier, shard_id) in seen_shards:
                continue
            else:
                seen_shards.add((tier, shard_id))
            explanation.append("Need %d disk for shard with tier %s" % (extra_disk_shards * tiers_sizes[tier], tier))
            total_size += extra_disk_shards * tiers_sizes[tier]

        if options.check_ssd_size:
            total_size *= SSD_COEFF
            if host.ssd > 0:
                host.disk = host.ssd

        if (total_size > host.disk or options.verbose) and from_cmd:
            print "Host %s has %d disk, while need %d disk:\n%s" % (host.name, host.disk, total_size, '\n'.join(map(lambda x: "   %s" % x, explanation)))
            if total_size > host.disk:
                failed += 1

        result[host] = total_size

    if options.show_top:
        print "Hosts with least disk left:"
        for host, disk_used in sorted(result.items(), cmp=lambda (x1, y1), (x2, y2): cmp(x1.disk - y1, x2.disk - y2))[
                               :10]:
            print "    Host %s left %d disk, have %d, used %d" % (host.name, host.disk - disk_used, host.disk, disk_used)

    if from_cmd:
        print "Failed %d of %d hosts" % (failed, len(options.hosts))
        if failed:
            return 1
        else:
            return 0
    else:
        return result


if __name__ == '__main__':
    options = parse_cmd()
    ret = main(options)
    sys.exit(ret)
