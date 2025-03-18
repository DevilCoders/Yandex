#!/skynet/python/bin/python
# -*- coding: utf-8 -*-

import logging
import os
import sys
import re

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg  # noqa

from utils.common import update_card, update_igroups
from core.svnapi import SvnRepository
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
import utils.pregen.find_most_memory_free_machines as find_most_memory_free_machines

LOCATIONS = ['SAS', 'VLA', 'IVA', 'MAN']
MASTER_GROUP_ENVS = ['TEST', 'PROD']
GB = 1024 * 1024 * 1024

logger = logging.getLogger(__name__)
logger.addHandler(logging.StreamHandler())
logger.setLevel(logging.INFO)


def check_is_market_group(master_name):
    pattern = re.compile('(' + '|'.join(LOCATIONS) + ')' +
                         '_MARKET_' +
                         '(' + '|'.join(MASTER_GROUP_ENVS) + ')' + '.*')
    return pattern.search(master_name) is not None


def find_master_group(group_name):
    group = CURDB.groups.get_group(group_name)
    master = group.card.master

    if master is None:
        raise Exception("Group %s is master group itself, script must be run on slave group" % group_name)

    if check_is_market_group(master.card.name):
        return master
    else:
        raise Exception("No market master group found for group %s" % group_name)


def get_existing_satisfying_hosts(options):
    group = CURDB.groups.get_group(options.group)
    hosts_info = find_most_memory_free_machines.jsmain(
        dict(action='show', hosts=group.getHosts())
    )

    satisfied_hosts = [
        info.host for info in hosts_info if
        info.memory_left >= options.memory - group.card.reqs.instances.memory_guarantee.value / GB and
        info.cpu_left >= options.power - try_parse_power_from(group.card.legacy.funcs.instancePower)
    ]
    return satisfied_hosts


def parse_power_from(power_str, group_name):
    try:
        return try_parse_power_from(power_str)
    except IndexError:
        raise Exception("legacy.funcs.instancePower is incorrect in group %s" % group_name)


def try_parse_power_from(power_str):
    pattern = r'exactly(\d+)'
    match = re.match(pattern, power_str)
    return int(match.group(1))


def run_update_card(options):
    if options.memory is not None:
        update_card.jsmain(dict(groups=[options.group],
                                key='reqs.instances.memory_guarantee',
                                value=str(options.memory) + 'Gb',
                                apply=True))

    if options.power is not None:
        update_card.jsmain(dict(groups=[options.group],
                                key='legacy.funcs.instancePower',
                                value='exactly' + str(options.power),
                                apply=True))


def check_limits(options):
    group = CURDB.groups.get_group(options.group)
    current_memory = group.card.reqs.instances.memory_guarantee.value / GB
    if options.memory is not None and current_memory >= options.memory:
        raise Exception("Requested memory limit (%f) must be greater then current memory limit (%f)"
                        % (options.memory, current_memory))

    current_power = group.card.legacy.funcs.instancePower
    current_power = parse_power_from(current_power, options.group)
    if options.power is not None and current_power >= options.power:
        raise Exception("Requested power limit (%d) must be greater then current power limit (%d)"
                        % (current_power, options.power))


def find_satisfied_hosts_from_master(group_hosts, options):
    master_group_host_infos = find_most_memory_free_machines.jsmain(
        dict(action='show', hosts=options.master_group.getHosts())
    )
    master_group_host_infos = [info for info in master_group_host_infos if info.host not in group_hosts]
    satisfied_hosts_from_master = [
        info.host for info in master_group_host_infos if
        info.cpu_left >= options.power and
        info.memory_left >= options.memory
    ]
    return satisfied_hosts_from_master


def add_hosts(options, hosts):
    update_igroups.jsmain(dict(action="addslavehosts", hosts=hosts, group=options.group))


def remove_hosts(options, hosts):
    update_igroups.jsmain(dict(action="removeslavehosts", hosts=hosts, group=options.group))


def main():
    options = get_parser().parse_cmd()

    normalize(options)
    # 1. check if config was edited manually (.instances file exists)
    if os.path.isfile(os.path.abspath(os.path.join(os.path.dirname(__file__),
                                                   '..', '..',
                                                   'db', 'groups',
                                                   options.master_group.card.name,
                                                   options.group + '.instances'))):
        raise Exception("This group must be handled manually or .instances file should be deleted and script rerun")

    group = CURDB.groups.get_group(options.group)
    group_hosts = group.getHosts()

    # 2. check if existing limits satisfy requirements
    check_limits(options)

    # 3. try to use existing hosts
    existing_satisfied = get_existing_satisfying_hosts(options)
    existing_not_satisfied = list(set(group_hosts) - set(existing_satisfied))
    if len(existing_satisfied) >= len(group_hosts):
        run_update_card(options)
        return

    # 4. not enough existing hosts, try to use hosts from master
    satisfied_hosts_from_master = find_satisfied_hosts_from_master(group_hosts, options)

    # all hosts can't cover hosts from our group, throw exception
    if len(satisfied_hosts_from_master) + len(existing_satisfied) < len(group_hosts):
        raise Exception(
            "Could not update requirements for group %s, not enough hosts matching requirements in master group %s" %
            (group.card.name, group.card.master.card.name))

    # 5. remove not satisfied hosts, add satisfied from master
    satisfied_hosts_from_master = satisfied_hosts_from_master[:len(existing_not_satisfied)]
    remove_hosts(options, existing_not_satisfied)
    add_hosts(options, satisfied_hosts_from_master)

    # 6. update config
    run_update_card(options)


def get_parser():
    parser = ArgumentParserExt(
        description=__doc__)
    parser.add_argument('--master-group', type=argparse_types.group, default=None,
                        help='Optional. Master group name')
    parser.add_argument('--group', type=str, required=True,
                        help='Obligatory. Group name to extend')
    parser.add_argument('--memory', type=float, required=False, help='Optional. Memory in GB')
    parser.add_argument('--power', type=int, required=False, help='Optional. CPU power')
    parser.add_argument('--ticket', type=str, required=True, help='Obligatory. Extending group ticket name')

    return parser


def normalize(options):
    if options.master_group is None:
        options.master_group = find_master_group(group_name=options.group)


if __name__ == '__main__':
    main()
