#!/skynet/python/bin/python
"""Try shrink intlookup (reduce power of some instances) in order to reduce overcommit"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
from gaux.aux_colortext import red_text
import core.argparse.types


class EActions(object):
    SHRINK = 'shrink'  # shrink basesearchers instances cpu power
    MOVEINTS = 'moveints'  # move ints to separate hosts with more cpu power
    EQBASE = 'eqbase'  # make power of all basesearchers in intgroup equal to each other
    EXPAND = 'expand'  # expand basesearchers cpu power up to host cpu power
    ALL = [SHRINK, MOVEINTS, EQBASE, EXPAND]

def get_parser():
    parser = ArgumentParserExt(description='Shrink intlookup')
    parser.add_argument('-a', '--action', type=str, required=True,
                        choices=EActions.ALL,
                        help='Obligatory. Action to execute')
    parser.add_argument('-g', '--group', type=core.argparse.types.group, required=True,
                        help='Obligatory. Shrink intlookup of specified group')
    parser.add_argument('--step-power', type=int, default=30,
                        help='Optional. Shrink IntGroup power by specified amount in every step')
    parser.add_argument('--total-steps', type=int, default=1,
                        help='Optional. Number of steps to perform')
    parser.add_argument('--allow-any-hosts', action='store_true', default=False,
                        help='Optional (for action <{}>). Move ints to hosts from differend IntGroups'.format(EActions.MOVEINTS))
    parser.add_argument('-y', '--apply', action='store_true', default=False,
                        help='Optional. Apply changes')

    # EXPAND-specific options
    parser.add_argument('--ignore-other', action='store_true', default=False,
                        help='Optional (for action <{}>). Ignore other groups when calculating free cpu')

    return parser


def shrink_brigade_gain(brigade, hosts_overcommit, step_power):
    """Calculate gain from reducing all basesearchers of brigade by <step_power>"""
    total_gain = 0.
    for basesearcher in brigade.get_all_basesearchers():
        host_overcommit = min(step_power, max(hosts_overcommit[basesearcher.host], 0.0), basesearcher.power)
        total_gain += host_overcommit

    return total_gain


def shrink_brigade(brigade, hosts_overcommit, step_power):
    """Shrink brigade by specified amount"""
    for basesearcher in brigade.get_all_basesearchers():
        host_overcommit = min(step_power, max(hosts_overcommit[basesearcher.host], 0.0), basesearcher.power)
        basesearcher.power -= host_overcommit
        hosts_overcommit[basesearcher.host] -= host_overcommit


def find_intsearcher_on_free_machine(brigade, hosts_cpu_ovecommit, have_memory_hosts, allow_any_hosts=False):
    """Find host with enough cpu and memory to replace int"""
    needed_cpu = brigade.intsearchers[0].power

    if allow_any_hosts:
        candidate_hosts = have_memory_hosts - {x.host for x in brigade.get_all_intsearchers()}
    else:
        candidate_hosts = {x.host for x in brigade.get_all_basesearchers()} - {x.host for x in brigade.get_all_intsearchers()}

    for candidate in candidate_hosts:
        if hosts_cpu_ovecommit[candidate] > -needed_cpu + 0.1:
            continue
        if candidate not in have_memory_hosts:
            continue
        return candidate

    return None


def get_hosts_cpu_overcommit(options):
    """Calculate overcommit"""

    # gather all instances data
    by_host_data = defaultdict(list)
    for group in CURDB.groups.get_groups():
        if group.card.properties.full_host_group:  # skip full host group (temoporary)
            continue
        if group.card.properties.created_from_portovm_group:  # skip portovm guest groups
            continue
        if options.ignore_other and (group != options.group):
            continue

        for instance in group.get_kinda_busy_instances():
            by_host_data[instance.host].append(instance)

    # get list of hosts we are intersted in
    if options.group.card.master is not None:
        master_group = options.group.card.master
    else:
        master_group = options.group

    # calculate overcommit
    hosts_overcommit = dict()
    for host in master_group.getHosts():
        host_power = host.power
        instances_power = sum(x.power for x in by_host_data[host])
        hosts_overcommit[host] = instances_power - host_power

    return hosts_overcommit


def get_hosts_avail_memory(hosts):
    """Calculate for hosts amount of memory available"""
    instances = sum([CURDB.groups.get_host_instances(x) for x in hosts], [])
    groups = {x.type for x in instances}
    groups = [CURDB.groups.get_group(x) for x in groups]
    groups = {x.card.name: x.card.reqs.instances.memory_guarantee.value for x in groups}

    avail_memory = {x: x.get_avail_memory() for x in hosts}
    for instance in instances:
        avail_memory[instance.host] -= groups[instance.type]

    return avail_memory


def main_shrink(options):
    intlookup = CURDB.intlookups.get_intlookup(options.group.card.intlookups[0])

    # calculate overcommit
    hosts_overcommit = get_hosts_cpu_overcommit(options)

    # perform steps of reducing power
    for i in xrange(options.total_steps):
        int_groups = intlookup.get_int_groups()
        int_groups.sort(key=lambda x: (-shrink_brigade_gain(x, hosts_overcommit, options.step_power), -x.basesearchers[0][0].power))

        best_int_group = int_groups[0]
        best_int_group_gain = shrink_brigade_gain(best_int_group, hosts_overcommit, options.step_power)
        print 'Shrinking group {}, gained {}'.format(best_int_group.intsearchers[0].full_name(), best_int_group_gain)
        shrink_brigade(best_int_group, hosts_overcommit, options.step_power)

    options.group.mark_as_modified()
    intlookup.mark_as_modified()


def main_moveints(options):
    intlookup = CURDB.intlookups.get_intlookup(options.group.card.intlookups[0])
    int_group = CURDB.groups.get_group(intlookup.get_int_groups()[0].intsearchers[0].type)

    # find intsearchers to replace
    hosts_cpu_overcommit = get_hosts_cpu_overcommit(options)
    instances_cpu_overcommit = {x for x in intlookup.get_int_instances() if hosts_cpu_overcommit[x.host] > 0}
    print 'Found <{}> instances with overcommit'.format(len(instances_cpu_overcommit))

    # find candidates (by memory)
    hosts_avail_memory = get_hosts_avail_memory(options.group.getHosts())
    have_memory_hosts = []
    for host in options.group.getHosts():
        if hosts_avail_memory[host] > int_group.card.reqs.instances.memory_guarantee.value:
            have_memory_hosts.append(host)
    have_memory_hosts = set(have_memory_hosts)
    print 'Found <{}> hosts with enough memory'.format(len(have_memory_hosts))

    replaced_instances_count = 0
    for brigade in intlookup.get_int_groups():
        brigade_overcommit_instances = set(brigade.intsearchers) & instances_cpu_overcommit
        if len(brigade_overcommit_instances):
            print 'Intgroup {} has {} instances with cpu overcommit'.format(brigade.intsearchers[0].full_name(), len(brigade_overcommit_instances))
            for overcommit_instance in brigade_overcommit_instances:
                replace_host = find_intsearcher_on_free_machine(brigade, hosts_cpu_overcommit, have_memory_hosts, allow_any_hosts=options.allow_any_hosts)
                if replace_host is not None:
                    if not int_group.hasHost(replace_host):
                        int_group.addHost(replace_host)
                    replace_instance = int_group.get_host_instances(replace_host)[0]
                    replace_instance.power = overcommit_instance.power

                    print '    Found replacement: {} -> {}'.format(overcommit_instance.full_name(), replace_instance.full_name())

                    brigade.intsearchers = [x for x in brigade.intsearchers if not (x == overcommit_instance)]
                    brigade.intsearchers.append(replace_instance)
                    have_memory_hosts.remove(replace_instance.host)
                    replaced_instances_count += 1

                if replaced_instances_count >= options.total_steps:
                    break

        if replaced_instances_count >= options.total_steps:
            break

    intlookup.mark_as_modified()
    int_group.mark_as_modified()


def main_eqbase(options):
    intlookup = CURDB.intlookups.get_intlookup(options.group.card.intlookups[0])
    for brigade in intlookup.get_int_groups():
        min_power = min(x.power for x in brigade.get_all_basesearchers())
        for basesearch in brigade.get_all_basesearchers():
            basesearch.power = min_power

    intlookup.mark_as_modified()
    options.group.mark_as_modified()


def main_expand(options):
    free_cpu = {x:-y for x,y  in get_hosts_cpu_overcommit(options).iteritems()}

    intlookup = CURDB.intlookups.get_intlookup(options.group.card.intlookups[0])
    for brigade in intlookup.get_int_groups():
        basesearchers = brigade.get_all_basesearchers()
        brigade_free_cpu = min(free_cpu[x.host] for x in basesearchers) - 0.05
        if brigade_free_cpu > 0.1:
            print 'Intgroup {} can increase cpu by {:.2f}'.format(basesearchers[0].full_name(), brigade_free_cpu)
        for basesearcher in basesearchers:
            basesearcher.power += brigade_free_cpu
            free_cpu[basesearcher.host] -= brigade_free_cpu

    intlookup.mark_as_modified()
    options.group.mark_as_modified()


def main(options):
    if options.action == EActions.SHRINK:
        main_shrink(options)
    elif options.action == EActions.MOVEINTS:
        main_moveints(options)
    elif options.action == EActions.EQBASE:
        main_eqbase(options)
    elif options.action == EActions.EXPAND:
        main_expand(options)
    else:
        raise Exception('Unknown action <{}>'.format(options.action))

    # apply changes
    if options.apply:
        CURDB.update(smart=True)
    else:
        print red_text('Not applied. Add <--apply> option to apply')


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    main(options)
