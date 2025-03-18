#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Show ideal power of all tiers/intlookups in sas optimizer")
    parser.add_argument("-g", "--group", dest="group", type=argparse_types.group, default=None, required=True,
                        help="Obligatory. Group name")
    parser.add_argument("-c", "--config", dest="sasconfig", type=argparse_types.sasconfig, default=None, required=True,
                        help="Obligatory. Sas config")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    total_power = sum(map(lambda x: x.power, options.group.get_instances()))

    config_power = 0.
    for elem in options.sasconfig:
        config_power += elem.power * CURDB.tiers.get_total_shards(elem.tier)

    for elem in options.sasconfig:
        per_shard_power = total_power * elem.power / config_power
        print "Tier %s (%d shards), intlookup %s: %s power per shard" % (
        elem.tier, CURDB.tiers.get_total_shards(elem.tier), elem.intlookup, per_shard_power)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
