#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import math
from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Show difference between sas config and generated data")
    parser.add_argument("-c", "--sasconfig", dest="sasconfig", type=argparse_types.sasconfig, default=None,
                        required=True,
                        help="Obligatory. File with sas optimizer config")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    total_config_power = 0.
    total_intlookup_power = 0.
    for elem in options.sasconfig:
        elem.config_power = elem.power * CURDB.tiers.tiers[elem.tier].get_shards_count()
        total_config_power += elem.config_power
        elem.intlookup_power = sum(
            map(lambda x: x.power, CURDB.intlookups.get_intlookup(elem.intlookup).get_used_base_instances()))
        total_intlookup_power += elem.intlookup_power

    for elem in options.sasconfig:
        elem_config_ratio = elem.config_power / float(total_config_power) * 100
        elem_intlookup_ratio = elem.intlookup_power / float(total_intlookup_power) * 100

        if elem_config_ratio > elem_intlookup_ratio:
            prefix = "\033[1;31m-"
        else:
            prefix = "\033[1;32m+"

        print "Tier %s, intlookup %s: config_ratio=%.2f intlookup_ratio=%.2f (%s%.1f%%\033[0m)" % (
        elem.tier, elem.intlookup, elem_config_ratio, elem_intlookup_ratio, prefix,
        math.fabs(elem_intlookup_ratio / float(elem_config_ratio) * 100. - 100.))


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
