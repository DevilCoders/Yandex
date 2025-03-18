#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import os
import sys
from copy import deepcopy

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
import core.argparse.types
import utils.common.update_igroups as update_igroups
import utils.common.update_card as update_card

from core.db import CURDB
from core.instances import TIntl2Group, TMultishardGroup
from core.argparse.parser import ArgumentParserExt


def get_parser():
    usage = 'Usage %prog [options]'
    parser = ArgumentParserExt(description='Copy group to other DC')
    parser.add_argument('-s', '--src-group', type=core.argparse.types.group, required=True,
                        help='Obligatory. Tier name')
    parser.add_argument('-d', '--dst-group', type=str, required=True,
                        help='Obligatory. Group name')
    return parser


def get_dc(options):
    return dict(
        src=options.src_group.card.name.split('_')[0],
        dst=options.dst_group.split('_')[0]
    )


def prepare_properties(options):
    """Prepares properties for the group created.

    :rtype: str
    """
    return ','.join([
        'dispenser.project_key=marketito',
        'properties.hbf_parent_macros={}'.format(options.src_group.card.properties.hbf_parent_macros),
        'properties.hbf_resolv_conf=',
        'properties.monitoring_ports_ready={}'.format(options.src_group.card.properties.monitoring_ports_ready),
        'reqs.instances.memory_guarantee={}'.format(options.src_group.card.reqs.instances.memory_guarantee)
    ])


def prepare_tags(options):
    """Prepare tags for the group created.

    :rtype: dict
    """
    return dict(
        ctype=options.src_group.card.tags.ctype,
        itype=options.src_group.card.tags.itype,
        metaprj=options.src_group.card.tags.metaprj,
        prj=options.src_group.card.tags.prj
    )


def create_group(options):
    dc = get_dc(options)
    # Create empty card
    print('Creating a group "{}"'.format(options.dst_group))
    update_igroups_params = dict(
        action='addgroup',
        db=CURDB,
        description=options.src_group.card.description,
        group=options.dst_group,
        instance_count_func=options.src_group.card.legacy.funcs.instanceCount,
        instance_power_func=options.src_group.card.legacy.funcs.instancePower,
        instance_port_func='auto',
        parent_group=options.src_group.card.master.card.name.replace(dc['src'], dc['dst'], 1),
        properties=prepare_properties(options),
        tags=prepare_tags(options)
    )
    update_igroups.jsmain(update_igroups_params)


def copy_card_nodes(options, nodes):
    nodes = map(lambda x: x.split('.'), nodes)
    for path in nodes:
        try:
            options.src_group.card.resolve_card_path(path)
        except core.card.node.ResolveCardPathError as e:
            print(e)
            nodes.remove(path)

    for path in nodes:
        src_node = options.src_group.card.resolve_card_path(path)

        dst_group = CURDB.groups.get_group(options.dst_group)
        dst_node = dst_group.card.resolve_card_path(path)
        if isinstance(dst_node, list):
            del dst_node[:]
            dst_node.extend(deepcopy(src_node))
        else:
            dst_node.replace_self(deepcopy(src_node))
        dst_group.mark_as_modified()
    CURDB.groups.update(smart=True)


def copy_intlookups(options):
    try:
        src_intlookup = CURDB.intlookups.get_intlookup(options.src_group.card.name)
    except Exception as e:
        print(e)
        return

    dst_group = CURDB.groups.get_group(options.dst_group)

    dst_intlookup = CURDB.intlookups.create_empty_intlookup(options.dst_group)
    dst_intlookup.brigade_groups_count = src_intlookup.brigade_groups_count
    dst_intlookup.tiers = src_intlookup.tiers
    dst_intlookup.hosts_per_group = src_intlookup.hosts_per_group
    dst_intlookup.base_type = dst_group.card.name
    dst_intlookup.intl2_groups = [TIntl2Group()]

    for _ in range(dst_intlookup.brigade_groups_count):
        dst_intlookup.intl2_groups[0].multishards.append(TMultishardGroup())

    dst_group.mark_as_modified()
    CURDB.update(smart=True)


def fix_port(options):
    group_card = CURDB.groups.get_group(options.dst_group)
    default_port = group_card.get_default_port()
    reqs_instances_port = group_card.guest_group_card_dict().get('reqs', {}).get('instances', {}).get('port')

    if default_port != reqs_instances_port:
        params = dict(
            groups=options.dst_group,
            key='reqs.instances.port',
            value=default_port
        )
        update_card.jsmain(params)
        CURDB.groups.update(smart=True)
        print('Fix reqs.instances.port from {} to {}'.format(reqs_instances_port, default_port))


def main(options):
    create_group(options)
    nodes = ['dispenser', 'reqs', 'searcherlookup_postactions', 'tags']
    copy_card_nodes(options, nodes)
    fix_port(options)
    copy_intlookups(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)
