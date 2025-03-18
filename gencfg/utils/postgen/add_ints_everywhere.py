#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import copy
from collections import defaultdict
from random import Random

import gencfg
from core.db import CURDB
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from gaux.aux_utils import draw_hist

infinity = float("inf")


def get_parser():
    parser = ArgumentParserExt(description="Add ints to specified intlookup")

    parser.add_argument("-i", "--intlookups", type=argparse_types.intlookups,
                        help="Optional. Comma-separated list of intlookups")
    parser.add_argument("-n", "--soft-ints-per-group", type=int, default=2,
                        help="Optional. Desired number of ints per group")
    parser.add_argument("-N", "--hard-ints-per-group", type=int, default=2,
                        help="Optional. 'must have' number of ints per group")
    parser.add_argument("-a", "--allow-minor-switches", action="store_true", default=False,
                        help="Optional. Allow ints from loosely connected switches")
    parser.add_argument("-A", "--allow-any-host", action="store_true", default=False,
                        help="Optional. Allow ints from any host in group")
    parser.add_argument("-I", "--max-ints-per-host", type=int, default=infinity,
                        help="Optional. Indicates max number of ints per host")
    parser.add_argument("-d", "--delete-current-ints", action="store_true", dest="delete_cur_ints", default=False,
                        help="Optional. Indicates that current ints will be deleted")
    parser.add_argument("-f", "--ints-on-foreign-hosts", action="store_true", dest="foreign_hosts", default=False,
                        help="Optional. Put ints on brigade hosts only.")
    parser.add_argument("-t", "--int-type", type=argparse_types.group, dest="int_type", default=None,
                        help="Optional. Int group.")
    parser.add_argument("-v", "--verbose", action="store_true", dest="verbose", default=False,
                        help="Optional. Display detailed information.")

    return parser


# our goal is to find as much hosts as possible that are suitable for ints for a given brigade
# i.e. these hosts should be inside switches containing big number of instances of this brigade
def find_major_hosts(brigade, all_switch_hosts=None):
    assert (all(len(x) == 1 for x in brigade.basesearchers))
    instances = [x[0] for x in brigade.basesearchers]
    switch_load = defaultdict(int)
    native_switch_hosts = defaultdict(set)
    for x in instances:
        switch_load[x.host.switch] += 1
        native_switch_hosts[x.host.switch].add(x.host)
    switches = list(switch_load.items())
    switches.sort(cmp=lambda x, y: cmp(-x[1], -y[1]))
    switch_hosts = native_switch_hosts
    if all_switch_hosts:
        min_power = min(x.power for x in sum((list(y) for y in native_switch_hosts.values()), []))
        # add all non-native hosts with power not less than min native hosts power
        for switch in switch_hosts:
            for host in all_switch_hosts[switch]:
                if host.power >= min_power:
                    switch_hosts[switch].add(host)

    max_load = switch_load[brigade.switch]
    assert (max_load >= max(switch_load.values()))

    major_switches = [brigade.switch]
    major_hosts = copy.copy(switch_hosts[brigade.switch])
    for switch, load in switches:
        if switch == brigade.switch:
            continue
        if round(1.2 * load) >= max_load or load + int(len(brigade.basesearchers) / 10) >= max_load:
            major_switches.append(switch)
            major_hosts |= switch_hosts[switch]
        else:
            break
    return list(major_hosts)


def print_frequency(header, values):
    frequencies = defaultdict(int)
    for value in values:
        frequencies[value] += 1
    if header:
        print header
    print ', '.join('%s: %s' % (value, frequency) for (value, frequency) in sorted(frequencies.items()))


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    brigades = [brigade
                for intlookup in options.intlookups
                for brigade_group in intlookup.get_multishards()
                for brigade in brigade_group.brigades]

    hosts = (instance.host for brigade in brigades for bases in brigade.basesearchers for instance in bases)
    hosts_by_switch = defaultdict(set)
    for host in hosts:
        hosts_by_switch[host.switch].add(host)

    if options.delete_cur_ints:
        for brigade in brigades:
            brigade.intsearchers = []

    if options.int_type is not None:
        int_type = options.int_type.card.name
    else:
        base_type = brigade.basesearchers[0][0].type
        int_type = base_type.replace('_BASE', '_INT')

    brigades_allowed_hosts = defaultdict(set)
    for brigade in brigades:
        if options.allow_any_host:
            brigades_allowed_hosts[id(brigade)] = set(x for x in CURDB.groups.get_group(int_type).getHosts())
        elif options.allow_minor_switches:
            brigades_allowed_hosts[id(brigade)] = set(x.host for y in brigade.basesearchers for x in y)
        else:
            brigades_allowed_hosts[id(brigade)] = \
                set(find_major_hosts(brigade, hosts_by_switch if options.foreign_hosts else None))

    CURDB.groups.get_group(int_type).mark_as_modified()
    for intlookup in options.intlookups:
        intlookup.mark_as_modified()

    if options.verbose:
        print_frequency('Number of brigades by number of avail hosts for ints:',
                        [len(x) for x in brigades_allowed_hosts.values()])

    hosts_load = defaultdict(float)
    host_ints_count = defaultdict(int)
    brigades_int_hosts = defaultdict(set)
    for brigade in brigades:
        brigades_int_hosts[id(brigade)] = set(x.host for x in brigade.intsearchers)
        for intsearch in brigade.intsearchers:
            host_ints_count[intsearch.host] = max(host_ints_count[intsearch.host], intsearch.N + 1)
            # do not update hosts_load, it will be done later
        brigades_allowed_hosts[id(brigade)] -= brigades_int_hosts[id(brigade)]

    rand = Random()
    rand.seed(39582)
    for n_int in range(options.soft_ints_per_group):
        # recalc hosts_load
        hosts_load = defaultdict(float)
        for brigade in brigades:
            for intsearch in brigade.intsearchers:
                hosts_load[intsearch.host] += float(brigade.power) / len(brigade.intsearchers) / intsearch.host.power
        for host in host_ints_count:
            if host_ints_count[host] >= options.max_ints_per_host:
                hosts_load[host] = infinity

        for brigade in brigades:
            if n_int < len(brigade.intsearchers):
                continue
            hosts = brigades_allowed_hosts[id(brigade)]
            if not hosts:
                continue
            min_load = min(hosts_load[host] for host in hosts)
            if min_load == infinity:
                continue
            hosts = [host for host in hosts if hosts_load[host] == min_load]
            host = hosts[rand.randrange(0, len(hosts))]

            hosts_load[host] += float(brigade.power) / (len(brigade.intsearchers) + 1) / host.power
            N = host_ints_count[host]
            host_ints_count[host] += 1
            if host_ints_count[host] >= options.max_ints_per_host:
                hosts_load[host] = infinity
            brigades_allowed_hosts[id(brigade)].discard(host)
            brigades_int_hosts[id(brigade)].add(host)

            intsearch = CURDB.groups.get_instance_by_N(host.name, int_type, N)
            brigade.intsearchers.append(intsearch)

    # check if we have enough ints per group
    for brigade in brigades:
        if len(brigade.intsearchers) < options.hard_ints_per_group:
            raise Exception("Adding ints failed: not enough ints in group %s:%s : have %d, needed %d" % (
                brigade.basesearchers[0][0].host.name, brigade.basesearchers[0][0].port, len(brigade.intsearchers), options.hard_ints_per_group))

    if options.verbose:
        # recalculate hosts_load to remove infinity values
        hosts_load = defaultdict(float)
        for brigade in brigades:
            for intsearch in brigade.intsearchers:
                hosts_load[intsearch.host] += float(brigade.power) / len(brigade.intsearchers) / intsearch.host.power

        stats = [x for x in hosts_load.values() if x]
        max_load = max([.0] + stats)
        max_load_hosts = [x.name for x in hosts_load if hosts_load[x] == max_load]
        # max_load_hosts = [(x.name, y) for x,y in sorted(hosts_load.items(), cmp=lambda x,y: cmp(hosts_load[y[0]], hosts_load[x[0]]))[:100]]
        print 'Max int load hosts: %s' % max_load_hosts
        if stats:
            draw_hist(stats)

        # min_instances = []
        # for brigade in brigades:
        #    if len(brigade.intsearchers) in [5,6,7]:
        #        min_instances.append(brigade.intsearchers[0])
        # print 'Min ints brigades: %s' % str([str(x) for x in min_instances])

        print_frequency('Number of brigades by number of ints:', [len(x.intsearchers) for x in brigades])
        print_frequency('Number of hosts by number of ints:', host_ints_count.values())

    CURDB.intlookups.update(smart=True)
    CURDB.groups.update(smart=True)
