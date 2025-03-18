#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
from collections import defaultdict

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
from core.instances import TIntGroup, TMultishardGroup
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Create intlookup with instances on primuses accroding to tiers map")

    parser.add_argument("-t", "--tiers", dest="tiers", type=str, required=True,
                        help="Tiers list")
    parser.add_argument("-g", "--group", dest="group", type=argparse_types.group, required=True,
                        help="Group name")
    parser.add_argument("-o", "--hosts-per-group", dest="hosts_per_group", type=int, required=True,
                        help="Bases per int")
    parser.add_argument("-i", "--intlookup-name", dest="intlookup_name", type=str, required=True,
                        help="Intlookup name")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    return options


def main(options):
    tiers = map(lambda x: CURDB.tiers.get_tier(x), options.tiers.split(','))
    total_shards = sum(map(lambda x: x.get_shards_count(), tiers))

    if total_shards % options.hosts_per_group != 0:
        raise Exception("Total number of shards %s is not divided by hosts per group %s" % (total_shards, options.hosts_per_group))

    # add hosts to group
    primuses = set(sum(map(lambda x: x.get_primuses(), tiers), []))
    for primus in primuses:
        options.group.addHost(CURDB.hosts.get_host_by_name(primus))

    # create instances list
    primus_counts = defaultdict(int)
    instances = []
    for tier in tiers:
        for primus_data in tier.primuses:
            for i in range(len(primus_data['shards'])):
                instances.append([CURDB.groups.get_instance_by_N(primus_data['name'], options.group.card.name,
                                                                 primus_counts[primus_data['name']])])
                primus_counts[primus_data['name']] += 1

    intlookup = CURDB.intlookups.create_tmp_intlookup()
    intlookup.hosts_per_group = options.hosts_per_group
    intlookup.brigade_groups_count = total_shards / intlookup.hosts_per_group
    intlookup.tiers = map(lambda x: x.name, tiers)
    intlookup.base_type = options.group.card.name
    if CURDB.intlookups.has_intlookup(options.intlookup_name):
        CURDB.intlookups.remove_intlookup(options.intlookup_name)
    CURDB.intlookups.rename_intlookup(intlookup.file_name, options.intlookup_name)

    for i in range(intlookup.brigade_groups_count):
        brigade_group = TMultishardGroup()
        brigade = TIntGroup(instances[i * intlookup.hosts_per_group: (i + 1) * intlookup.hosts_per_group], [])
        brigade_group.brigades.append(brigade)
        intlookup.brigade_groups.append(brigade_group)

    CURDB.update()


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
