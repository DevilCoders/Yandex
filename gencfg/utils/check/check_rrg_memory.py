#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
from collections import defaultdict

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Check sas slots")
    parser.add_argument("-g", "--group", type=argparse_types.group, dest="group", required=True,
                        help="Obligatory. Master group to check")
    parser.add_argument("-t", "--tiers-sizes", type=str, dest="tiers_sizes", required=True,
                        help="Obligatory. Comma-separated list of <tier_name>:<number of slots per instance>")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    options.tiers_sizes = dict(map(lambda x: (x.split(':')[0], int(x.split(':')[1])), options.tiers_sizes.split(',')))

    return options


def main(options):
    hosts_data = defaultdict(lambda: defaultdict(set))
    for intlookup in options.group.card.intlookups:
        intlookup = CURDB.intlookups.get_intlookup(intlookup)
        for shard_id in range(intlookup.get_shards_count()):
            tier = intlookup.get_tier_for_shard(shard_id)
            primus = intlookup.get_primus_for_shard(shard_id)
            for instance in intlookup.get_base_instances_for_shard(shard_id):
                hosts_data[instance.host][tier].add(primus)

    failed = 0
    for host in hosts_data:
        have_slots = len(options.group.get_host_instances(host))
        need_slots = sum(
            map(lambda tier: options.tiers_sizes[tier] * len(hosts_data[host][tier]), hosts_data[host].keys()))
        if have_slots < need_slots:
            print "Host %s has %d slots while need %d slots: %s" % (host.name, have_slots, need_slots, ','.join(list(hosts_data[host])))
            failed = 1

    return failed


if __name__ == '__main__':
    options = parse_cmd()
    ret = main(options)
    sys.exit(ret)
