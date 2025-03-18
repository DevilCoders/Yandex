#!/skynet/python/bin/python
"""
    Create intlookup for new group on same hosts as intlookup in old group. The only difference is ports (in new group all ports are shifted).
"""

from argparse import ArgumentParser

import os
import sys
import copy

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from core.instances import TIntGroup, TMultishardGroup, TIntl2Group


def get_parser():
    parser = ArgumentParserExt(description="""Make intlookup as copy from intlookup from other group""")
    parser.add_argument("--db", type=core.argparse.types.gencfg_db, default=CURDB,
                        help="Optional. Path to db")
    parser.add_argument("-i", "--input", type=core.argparse.types.intlookup, required=True,
                        help="Obligatory. Input intlookup.")
    parser.add_argument("-t", "--copy-tiers", type=core.argparse.types.comma_list, default=None,
                        help="Optional. Shift only specified tiers")
    parser.add_argument("-o", "--output-group", type=core.argparse.types.group, required=True,
                        help="Obligatory. Output intlookup base type.")
    parser.add_argument("--hosts-name-mapping", type=core.argparse.types.pythonlambda, default=None,
                        help="Optional. Name mapping")
    parser.add_argument("-I", '--remove-ints', dest='remove_ints', default=False, action="store_true",
                        help="Optional. Remove ints in output intlookup.")

    return parser


def main(options):
    if len(options.input.intl2_groups) > 1 and options.copy_tiers is not None:
        raise Exception("Copy tiers with more than one intl2 group is not yet supported")

    input_intlookup = options.input

    if len(input_intlookup.get_int_instances()) == 0:
        options.remove_ints = True

    output_intlookup_name = options.output_group.card.name

    if options.db.intlookups.has_intlookup(options.output_group.card.name):
        if not output_intlookup_name in options.output_group.card.intlookups:
            raise Exception("Intlookup <%s> not in group <%s>" % (output_intlookup_name, options.output_group.card.name))
        options.db.intlookups.remove_intlookup(options.output_group.card.name)

    output_intlookup = options.db.intlookups.create_empty_intlookup(output_intlookup_name)
    output_intlookup.brigade_groups_count = input_intlookup.brigade_groups_count
    output_intlookup.hosts_per_group = input_intlookup.hosts_per_group
    output_intlookup.tiers = copy.deepcopy(input_intlookup.tiers)
    output_intlookup.base_type = options.output_group.card.name
#    output_intlookup.intl2_groups = copy.deepcopy(input_intlookup.intl2_groups)  # FIXME: deepcopy is always very bad

    # deepcopy does to work with Cython, so we have to copy manually
    output_intlookup.intl2_groups = []
    for intl2_group in input_intlookup.intl2_groups:
        new_intl2_group = TIntl2Group()
        new_intl2_group.intl2searchers = copy.copy(intl2_group.intl2searchers)
        for multishard in intl2_group.multishards:
            new_multishard = TMultishardGroup()
            new_multishard.weight = multishard.weight
            for int_group in multishard.brigades:
                new_int_group = TIntGroup(copy.copy(int_group.basesearchers), copy.copy(int_group.intsearchers))
                new_multishard.brigades.append(new_int_group)
            new_intl2_group.multishards.append(new_multishard)
        output_intlookup.intl2_groups.append(new_intl2_group)


    if options.copy_tiers is not None:
        new_multishards = []
        for i, multishard in enumerate(input_intlookup.get_multishards()):
            if input_intlookup.get_tier_for_shard(i * input_intlookup.hosts_per_group) in options.copy_tiers:
                new_multishards.append(multishard)
        output_intlookup.intl2_groups[0].multishards = new_multishards
        output_intlookup.brigade_groups_count = len(new_multishards)
        output_intlookup.tiers = filter(lambda x: x in options.copy_tiers, input_intlookup.tiers)

    output_intlookup.mark_as_modified()

    options.output_group.card.properties.nidx_for_group = input_intlookup.base_type
    options.output_group.mark_as_modified()

    if not options.remove_ints:
        output_int_group = options.db.groups.get_group(output_intlookup.base_type).get_int_group_name()
        output_int_group = options.db.groups.get_group(output_int_group)
        output_int_group.mark_as_modified()

#        output_intl2_group = options.db.groups.get_group(output_intlookup.base_type).get_intl2_group_name()
#        output_intl2_group = options.db.groups.get_group(output_intl2_group)
#        output_intl2_group.mark_as_modified()

    input_ints_group_name = options.db.groups.get_group(input_intlookup.base_type).get_int_group_name()
    output_ints_group_name = options.output_group.get_int_group_name()

    type_mapping = {
        input_intlookup.base_type: output_intlookup.base_type,
        input_ints_group_name: output_ints_group_name,
    }

    def find_or_replace(instance):
        if options.remove_ints and (
            options.db.groups.get_group(instance.type).is_int_group() or options.db.groups.get_group(
                instance.type).is_intl2_group()):
            return None

        if options.hosts_name_mapping is None:
            new_host_name = instance.host.name
            N = instance.N
        else:
            new_host_name = options.hosts_name_mapping(instance)
            N = 0

        new_host = options.db.hosts.get_host_by_name(new_host_name)
        new_group = options.db.groups.get_group(type_mapping[instance.type])

        if not new_group.hasHost(new_host):
            new_group.addHost(new_host)

        return options.db.groups.get_instance_by_N(new_host_name, type_mapping[instance.type], N)

    output_intlookup.replace_instances(find_or_replace, run_base=True, run_int=True, run_intl2=True)
    if options.remove_ints:
        def remove_ints_func(x):
            x.intsearchers = []

        output_intlookup.for_each_brigade(remove_ints_func)

        for intl2_group in output_intlookup.intl2_groups:
            intl2_group.intl2searchers = []

    # FIXME: order is so important
    options.db.intlookups.update(smart=True)
    options.db.groups.update(smart=True)


def jsmain(d):
    options = get_parser().parse_json(d)

    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)
