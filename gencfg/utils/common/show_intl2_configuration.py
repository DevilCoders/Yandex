#!/skynet/python/bin/python
"""
    Script to show configuration of intl2 instances in specified intlookup.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from core.argparse.parser import ArgumentParserExt
import core.argparse.types

import gencfg
from core.db import CURDB
from core.instances import TMultishardGroup


def get_parser():
    parser = ArgumentParserExt("Utility to show intl2 configuration")
    parser.add_argument("-i", "--intlookup", type=core.argparse.types.intlookup, required=True,
                        help="Obligatory. Intlookup to process")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Verbosity level (maximum is 1)")

    return parser


def normalize(options):
    del options


def main(options):
    print "Total intl2 groups: %d" % (len(options.intlookup.intl2_groups))
    for intl2_group_id, intl2_group in enumerate(options.intlookup.intl2_groups):
        intl2_power = min(options.intlookup.calc_brigade_power(multishards=intl2_group.multishards))
        print "    Intl2 group %s: %d multishards, %f power" % (
        intl2_group_id, len(intl2_group.multishards), intl2_power)
        if options.verbose >= 1:
            for multishard_id, multishard in enumerate(intl2_group.multishards):
                multishard_power = min(options.intlookup.calc_brigade_power(multishards=[multishard]))
                print "        Multishard %s: %d replicas, %f power" % (
                multishard_id, len(multishard.brigades), multishard_power)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
