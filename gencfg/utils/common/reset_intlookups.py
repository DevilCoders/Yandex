#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Reset group intlookup (for future manipulations with group hosts)")
    parser.add_argument("-i", "--intlookups", type=str, default=None,
                        help="Optional. Modified intlookups")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, default=[],
                        help="Optional. Modified groups")
    parser.add_argument("-e", "--drop-brigade-groups", action="store_true", default=False,
                        help="Optional. Instead of making empty brigade groups, drop them completeley and change number of shards to zero")
    parser.add_argument("-d", "--delete", action="store_true", default=False,
                        help="Optional. Delete intlookup file")
    parser.add_argument("-s", "--reset-slaves", action="store_true", default=False,
                        help="Optional. Reset slave groups also")
    parser.add_argument("-c", "--clear-custom-instances-power", action="store_true", default=False,
                        help="Optional. Reset custom instances power")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    if options.intlookups is not None:
        options.intlookups = options.intlookups.split(',')
    else:
        options.intlookups = []

    return options


def main(options):
    # extract intlookups from groups
    for group in options.groups:
        group.mark_as_modified()
        options.intlookups.extend(group.card.intlookups)
        if options.reset_slaves:
            for slave in group.card.slaves:
                options.intlookups.extend(slave.card.intlookups)
                slave.mark_as_modified()
    options.intlookups = list(set(options.intlookups))

    if not options.delete:
        intlookups = [CURDB.intlookups.get_intlookup(intlookup) for intlookup in options.intlookups]
        for intlookup in intlookups:
            intlookup.reset(options.drop_brigade_groups)
    else:
        for intlookup in options.intlookups:
            CURDB.intlookups.remove_intlookup(intlookup)

    if options.clear_custom_instances_power:
        for group in options.groups:
            group.custom_instance_power.clear()

    CURDB.update(smart=True)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
