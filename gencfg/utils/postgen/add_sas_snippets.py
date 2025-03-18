#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import copy
from argparse import ArgumentParser

import gencfg
from core.db import CURDB
from core.instances import TMultishardGroup
import core.argparse.types as argparse_types


def _ssd_brigade(brigade):
    return len(filter(lambda x: x.host.ssd == 0, sum(brigade.basesearchers, []))) == 0


def parse_cmd():
    parser = ArgumentParser(description="Add snippets for sas.")
    parser.add_argument("-d", "--dest-intlookup", type=str, dest="dest_intlookup", required=True,
                        help="Obligatory. Output snippet intlookup")
    parser.add_argument("-s", "--src-intlookups", type=argparse_types.intlookups, dest="src_intlookups", required=True,
                        help="Obligatory. Source intlookups")
    parser.add_argument("-g", "--snippets-base-group", type=argparse_types.group, default=None,
                        help="Optinal. Group for snippets (otherwise group will be calculated based on intlookup group)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')

    options = parser.parse_args()

    return options


if __name__ == '__main__':
    options = parse_cmd()

    if not CURDB.intlookups.has_intlookup(options.dest_intlookup):
        dest_intlookup = CURDB.intlookups.create_empty_intlookup(options.dest_intlookup)
    else:
        dest_intlookup = CURDB.intlookups.get_intlookup(options.dest_intlookup)

    dest_intlookup.hosts_per_group = options.src_intlookups[0].hosts_per_group
    dest_intlookup.base_type = options.src_intlookups[0].base_type
    dest_intlookup.brigade_groups_count = sum(map(lambda x: x.brigade_groups_count, options.src_intlookups))
    dest_intlookup.tiers = sum(map(lambda x: x.tiers, options.src_intlookups), [])
    dest_intlookup.brigade_groups = []

    for intlookup in options.src_intlookups:
        for brigade_group in intlookup.brigade_groups:
            new_brigade_group = TMultishardGroup()
            print "Tier %s, new brigade group" % intlookup.tiers[0]
            for brigade in brigade_group.brigades:
                if _ssd_brigade(brigade):
                    new_brigade_group.brigades.append(copy.deepcopy(brigade))
                #                    new_brigade_group.brigades[-1].intsearchers = []
            if len(new_brigade_group.brigades) == 0:
                raise Exception("Empty snippet group")
            dest_intlookup.brigade_groups.append(new_brigade_group)

    if options.snippets_base_group:
        snippets_base_group_name = options.snippets_base_group.name
    else:
        base_group_name = options.src_intlookups[0].brigade_groups[0].brigades[0].basesearchers[0][0].type
        snippets_base_group_name = base_group_name.replace('_BASE', '_SNIPPETS_SSD_BASE')
    if options.snippets_base_group:
        snippets_int_group_name = snippets_base_group_name.replace('_BASE', '_INT')
    else:
        int_group_name = options.src_intlookups[0].brigade_groups[0].brigades[0].intsearchers[0].type
        snippets_int_group_name = int_group_name.replace('_INT', '_SNIPPETS_SSD_INT')

    # change group names
    def f(instance):
        if CURDB.groups.get_group(instance.type).tags.itype == 'base':
            new_instance = CURDB.groups.get_instance_by_N(instance.host.name, snippets_base_group_name, instance.N)
        elif CURDB.groups.get_group(instance.type).tags.itype == 'int':
            new_instance = CURDB.groups.get_instance_by_N(instance.host.name, snippets_int_group_name, instance.N)
        else:
            raise Exception("Unknown group name %s" % instance.type)
        instance.swap_data(new_instance)


    dest_intlookup.for_each(f, run_base=True, run_int=True)

    CURDB.update()
