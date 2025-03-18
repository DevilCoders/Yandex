#!/skynet/python/bin/python
"""
    We have to create "shifted" group with intlookup on shifted ports way too often. Thus, we made special script to make it easier.
"""

import os
import sys

sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types
from core.db import CURDB
from core.card.types import ByteSize
from core.card.node import CardNode

import utils.postgen.shift_intlookup
import utils.common.find_most_unused_port


def get_parser():
    parser = ArgumentParserExt(description="Create group with intlookups on shifted ports")

    parser.add_argument("-g", "--group", type=core.argparse.types.group, required=True,
                        help="Obligatory. Group to created shifted intlookups/group from")
    parser.add_argument("-i", "--shifted-group", type=str, default="%(name)s_NIDX",
                        help="Optional. Shifted group name. By default we use group name with _NIDX postfix")
    parser.add_argument("-d", "--description", type=str, default=None,
                        help="Optional. Custom description to shifted group")

    return parser


def normalize(options):
    options.shifted_group = options.shifted_group % {'name': options.group.card.name}


def main(options):
    # copy group
    if options.group.card.master is None:
        parent_name = options.group.card.name
    else:
        parent_name = options.group.card.master.card.name

    if options.group.card.host_donor is not None:
        donor_name = options.group.card.host_donor
    else:
        donor_name = options.group.card.name

    instance_count_func = options.group.card.legacy.funcs.instanceCount
    instance_power_func = 'zero'

    find_port_params = {'port_range': 1}
    subresult = utils.common.find_most_unused_port.jsmain(find_port_params)
    instance_port_func = 'new%s' % subresult.port

    shifted_group = CURDB.groups.copy_group(options.group.card.name,
                                            options.shifted_group,
                                            parent_name=parent_name,
                                            donor_name=donor_name,
                                            instance_count_func=instance_count_func,
                                            instance_port_func=instance_port_func,
                                            instance_power_func=instance_power_func)

    # clear some stuff in shifted group
    if options.description is not None:
        shifted_group.description = "%s (NIDX group)" % options.description
    else:
        shifted_group.description = "Shifted group, created from %s" % options.group.card.name
    shifted_group.card.reqs.instances.memory_guarantee = ByteSize("1 Mb")

    # create intlookups
    if len(options.group.card.intlookups) > 0:
        jsparams = {
            'db': CURDB,
            'input': CURDB.intlookups.get_intlookup(options.group.card.intlookups[0]),
            'output_group': shifted_group,
            'remove_ints': True,
        }
        utils.postgen.shift_intlookup.jsmain(jsparams)

        # add generate_intlookups section
        generate_intlookup_node = CardNode()
        generate_intlookup_node.command = "./utils/postgen/shift_intlookup.py -i {{group.card.host_donor}} -o {{group.card.name}} -I"
        generate_intlookup_node.id = "0"
        generate_intlookup_node.prerequisites = ["%s.generate_intlookups.0" % options.group.card.name]
        shifted_group.card.recluster.generate_intlookups = [generate_intlookup_node]

        # add cleanup section
        cleanup_node = CardNode()
        cleanup_node.command = "./utils/common/reset_intlookups.py -g {{group.card.name}}"
        cleanup_node.id = "0"
        cleanup_node.prerequisites = []
        shifted_group.card.recluster.cleanup = [cleanup_node]

        shifted_group.card.reminders = []
        shifted_group.card.properties.nidx_for_group = options.group.card.name

    CURDB.intlookups.update(smart=True)
    CURDB.groups.update(smart=True)

    return shifted_group


def print_result(result_group):
    if len(result_group.card.intlookups) > 0:
        intlookup = CURDB.intlookups.get_intlookup(result_group.card.intlookups[0])
        print "Created group <%s> with tier <%s> (intlookup <%s>)" % (
        result_group.card.name, ",".join(intlookup.tiers), intlookup.file_name)
    else:
        print "Created group <%s> with tier <%s>" % (
        result_group.card.name, result_group.card.searcherlookup_postactions.custom_tier.tier_name)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result)
