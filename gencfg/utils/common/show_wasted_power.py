#!/skynet/python/bin/python

"""
    Script, showing total power waste for group due to following reasons:
        - instances, which are not used at all
        - instances, which are used ineffectively (e. g. instance with high power in group with instances with low power)
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB

import core.argparse.parser
import core.argparse.types

def get_parser():
    parser = core.argparse.parser.ArgumentParserExt(description="Show wasted power for specified group or intlookup")
    parser.add_argument("-g", "--group", type=core.argparse.types.group, required=True,
        help="Obligatory. Group for calculation")

    return parser

def main(options):
    # first calculate power in down instances
    all_instances = options.group.get_instances()
    all_instances_power = sum(map(lambda x: x.power, all_instances))
    used_instances = options.group.get_kinda_busy_instances()
    used_instances_power = sum(map(lambda x: x.power, used_instances))

    intlookup_instances_power = 0.0
    for intlookup in map(lambda x: CURDB.intlookups.get_intlookup(x), options.group.card.intlookups):
        for int_group in intlookup.get_int_groups():
            intlookup_instances_power += int_group.power * len(int_group.basesearchers)

    print "Group %s:" % options.group.card.name
    print "    Unused instances: %s (%.2f%%) power" % (all_instances_power - used_instances_power, (all_instances_power - used_instances_power) / all_instances_power * 100.)
    print "    Ineffective instances: %s (%.2f%%) power" % (used_instances_power - intlookup_instances_power, (used_instances_power - intlookup_instances_power) / all_instances_power * 100.)

if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)

