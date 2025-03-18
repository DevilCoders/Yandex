#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types


def get_parser():
    parser = ArgumentParserExt(description='Change groups instances guarantee')
    parser.add_argument('-g', '--groups', type=argparse_types.groups, required=True,
                        help='Obligatory. List of groups to process (all groups are processed by default)')
    parser.add_argument('-p', '--power', type=int, required=True,
                        help='Obligatory. New cpu guarantee in gencfg power units (40 gencfg power units = 1 default cpu core)')
    return parser


def main(options):
    """Main function to update cpu guarantee in groups and instances"""
    for group in options.groups:
        power = options.power

        instances = group.get_kinda_busy_instances()
        for intlookup in (CURDB.intlookups.get_intlookup(x) for x in group.card.intlookups):
            intlookup.mark_as_modified()

        group.card.legacy.funcs.instancePower = 'exactly{}'.format(power)
        group.card.properties.cpu_guarantee_set = True
        group.card.audit.cpu.last_modified = time.strftime('%Y-%m-%d %H:%M:%S', time.localtime())

        for instance in instances:
            instance.power = power

        group.mark_as_modified()

    CURDB.update(smart=True)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    retcode = main(options)

    sys.exit(retcode)
