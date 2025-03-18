#!/skynet/python/bin/python
"""
    Various actions with groups custom instances power:
        - fix power on unused instances in order to satisfy constraint sum(intances_power) == host_power
        - fix power on used instances in order to satisfy constraint sum(intances_power) == host_power
        - remove custom instances power
        - add custom instances power to group
"""


import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser
from collections import defaultdict
import math

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types

class EActions(object):
    FIXFREE = "fixfree"
    FIXUSED = "fixused"
    REMOVE = "remove"
    ADD = "add"
    ALL = [FIXFREE, FIXUSED, REMOVE, ADD]

def parse_cmd():
    parser = ArgumentParser(description="Fix custom instances weights by changing weights of unused instances")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute: %s" % ",".join(EActions.ALL))
    parser.add_argument("-g", "--groups", type=argparse_types.groups, required=True,
                        help="Obligatory. List of groups to perform checking and fixing")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def group_instances_by_host(group):
    all_instances_by_host = defaultdict(list)
    for instance in group.get_instances():
        all_instances_by_host[instance.host].append(instance)

    used_instances_by_host = defaultdict(list)
    for instance in group.get_busy_instances():
        used_instances_by_host[instance.host].append(instance)

    return all_instances_by_host, used_instances_by_host


def fixfree(group):
    result = []

    all_instances_by_host, used_instances_by_host = group_instances_by_host(group)
    for host in group.getHosts():
        allinstances = all_instances_by_host[host]
        usedinstances = used_instances_by_host[host]
        if len(allinstances) == len(usedinstances):
            continue
        if math.fabs(sum(map(lambda x: x.power, allinstances)) - host.power) < 0.1:
            continue

        result.append(host)

        free_instances = set(allinstances) - set(usedinstances)
        instance = free_instances.pop()
        instance.power = host.power - sum(map(lambda x: x.power, usedinstances))
        for instance in free_instances:
            instance.power = 0.0

    return result


def fixused(group):
    result = []

    all_instances_by_host, used_instances_by_host = group_instances_by_host(group)

    for host in group.getHosts():
        host_power = host.power
        all_power = sum(map(lambda x: x.power, all_instances_by_host[host]))
        used_power = sum(map(lambda x: x.power, used_instances_by_host[host]))

        if all_power - used_power >= host_power:
            continue
        if used_power == 0:
            continue

        notenough_power = host_power - all_power
        if math.fabs(notenough_power) < 1.:
            continue

        result.append(host)

        coeff = (1 + notenough_power / used_power)
        for instance in used_instances_by_host[host]:
            instance.power *= coeff

    return result


def main(options):
    result = dict()

    for group in options.groups:
        if options.action == EActions.FIXFREE:
            result[group] = fixfree(group)
        elif options.action == EActions.FIXUSED:
            result[group] = fixused(group)
            for intlookup in map(lambda x: CURDB.intlookups.get_intlookup(x), group.card.intlookups):
                intlookup.mark_as_modified()
        elif options.action == EActions.REMOVE:
            group.custom_instance_power_used = False
        elif options.action == EActions.ADD:
            group.custom_instance_power_used = True
        else:
            raise Exception("Unknown action %s" % options.action)

        group.mark_as_modified()

    CURDB.intlookups.update(smart=1)
    CURDB.groups.update(smart=1)

    return result

def print_result(result, options):
    if options.action in [EActions.FIXFREE, EActions.FIXUSED]:
        for group in sorted(result.keys(), key=lambda x: x.card.name):
            print "Group %s (%d fixed hosts): %s" % (
                group.card.name, len(result[group]), ' '.join(map(lambda x: x.name, result[group])))

if __name__ == '__main__':
    options = parse_cmd()

    result = main(options)

    print_result(result, options)
