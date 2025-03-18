#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from core.argparse.parser import ArgumentParserExt
from collections import defaultdict

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types

ACTIONS = ["show", "join"]


def get_parser():
    parser = ArgumentParserExt(description="Show most overloaded hosts in case some other host died")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=ACTIONS,
                        help="Obligatory. Action to execute")
    parser.add_argument("-c", "--sasconfig", type=argparse_types.sasconfig, required=False,
                        help="Obligatory. File with sas optimizer config")
    parser.add_argument("-i", "--intlookups", type=argparse_types.intlookups, required=False, default=[],
                        help="Optional. List of intlookups to process")
    parser.add_argument("-v", "--verbose", action="count", dest="verbose_level", default=0,
                        help="Optional. Verbose mode. Multiple -v options increase the verbosity. The maximum is 1.")
    parser.add_argument("--fail-on-unjoined", action="store_true", default=False,
                        help="Optional. Fail if found unjoined groups")
    return parser


def normalize(options):
    if options.sasconfig is None and options.intlookups == []:
        raise Exception("You must specify at least one of --sasconfig --intlookup options")

    if options.sasconfig is not None:
        options.intlookups.extend(map(lambda elem: CURDB.intlookups.get_intlookup(elem.intlookup), options.sasconfig))


def process(intlookup, options):
    groups_by_hash = defaultdict(list)
    for brigade_group in intlookup.brigade_groups:
        for i in range(len(brigade_group.brigades)):
            groups_by_hash[(brigade_group.brigades[i].calculate_unordered_hash(), brigade_group)].append(i)

    if options.action == "show":
        collisions_count = len(filter(lambda x: len(x) > 1, groups_by_hash.itervalues()))
        if collisions_count > 0:
            print "Intlookup %s: %d unjoined groups" % (intlookup.file_name, collisions_count)

        if options.fail_on_unjoined:
            return int(collisions_count > 0)
        else:
            return 0

    elif options.action == "join":
        base_group = CURDB.groups.get_group(intlookup.base_type)

        assert (base_group.custom_instance_power_used == True)

        zero_brigades = set()
        for multikey, lst in groups_by_hash.iteritems():
            if len(lst) == 1:
                continue

            _, brigade_group = multikey

            total_power = sum(map(lambda group_id: brigade_group.brigades[group_id].power, lst))

            # change first group (move power from other groups to it
            first_brigade = brigade_group.brigades[lst[0]]
            first_brigade.power = total_power
            for instance in first_brigade.get_all_basesearchers():
                instance.power = total_power

            # set power of other groups to zero
            for group_id in lst[1:]:
                zero_brigade = brigade_group.brigades[group_id]
                for instance in zero_brigade.get_all_basesearchers():
                    instance.power = 0
                zero_brigades.add(zero_brigade)

        # remove all zero brigades
        for brigade_group in intlookup.brigade_groups:
            brigade_group.brigades = filter(lambda x: x not in zero_brigades, brigade_group.brigades)

        CURDB.update()

        return 0
    else:
        raise Exception("Unknown action <%s>" % options.action)


def main(options):
    retcode = 0
    for intlookup in options.intlookups:
        intlookup_retcode = process(intlookup, options)
        if intlookup_retcode != 0:
            retcode = intlookup_retcode

    return retcode


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    normalize(options)

    result = main(options)
    sys.exit(result)
