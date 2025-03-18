#!/skynet/python/bin/python
"""
    Add intsearchers to intlookups. Insearchers are chosen to minimize number of ints, which are far away from baseseachers, i. e.
    we will choose for ints hosts from same switch with most of basesearchers.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import string
from collections import defaultdict
import heapq

import gencfg
from core.argparse.parser import ArgumentParserExt
from core.db import CURDB
import core.argparse.types as argparse_types


class EActions(object):
    ALLOC_INT = "alloc_int"
    SHOW_INT = "show_int"
    ALLOC_INTL2 = "alloc_intl2"
    ALL = [ALLOC_INT, SHOW_INT, ALLOC_INTL2]


def get_parser():
    parser = ArgumentParserExt(
        description="Add ints to intlookup (every group could have instances from different switches and even dcs)")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument("-i", "--intlookups", type=argparse_types.intlookups, required=True,
                        help="Obligatory. List of intlookups to process")
    parser.add_argument("-n", "--ints-per-group", type=int, default=3,
                        help="Optional. Number of ints per group (3 by default)")
    parser.add_argument("-t", "--int-group", type=argparse_types.group, default=None,
                        help="Optional. Int group type")
    parser.add_argument("--max-ints-per-host", type=int, default=None,
                        help="Optional. Limit number of intsearchers per host")
    parser.add_argument("--add-ints-to-group", action="store_true", default=False,
                        help="Optional. In case int group is empty add hosts from master group")
    parser.add_argument("-f", "--flt", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Filter on instances in int group")
    parser.add_argument("--extra-instances", type=int, default=None,
                        help="Optional. Number of extra int instances to be added to most powerful int groups")

    return parser


def get_distance(cur_instances, new_instance):
    distance = 0
    for instance in cur_instances:
        if new_instance.host.dc != instance.host.dc:
            distance += 100
        elif new_instance.host.queue != instance.host.queue:
            distance += 50
        elif new_instance.host.switch != instance.host.switch:
            distance += 10
        elif new_instance.host != instance.host:
            distance += 1
    return distance


def find_endmost_instance(cur_instances, avail_instances):
    candidate = next(iter(avail_instances))
    candidate_distance = get_distance(cur_instances, candidate)

    for instance in avail_instances:
        d = get_distance(cur_instances, instance)
        if d < candidate_distance:
            candidate, candidate_distance = instance, d

    return candidate

def add_extra_instances(int_groups, avail_instances, N):
    """
        After main algorithm finished, every int group have same number of ints. Because every int group has different power,
        computational load, caused by ints, can significantly change from group to group. Here we add N instances to groups
        with most power.
    """

    assert N <= avail_instances, "Have only <%d> avail instances, when need <%d>" % (avail_instances, N)

    class TCompareGroup(object):
        def __init__(self, int_group):
            self.int_group = int_group

        def __cmp__(self, other):
            self_value = - self.int_group.power / len(self.int_group.intsearchers)
            other_value = - other.int_group.power / len(other.int_group.intsearchers)
            return cmp(self_value, other_value)

    myheap = []
    for int_group in int_groups:
        heapq.heappush(myheap, TCompareGroup(int_group))

    for i in xrange(N):
        heap_top = heapq.heappop(myheap)
        int_group = heap_top.int_group

        group_used_int_hosts = set(map(lambda x: x.host, int_group.intsearchers))
        group_avail_int_instances = filter(lambda x: x.host not in group_used_int_hosts, avail_instances)
        candidate = find_endmost_instance(int_group.get_all_basesearchers(), group_avail_int_instances)
        int_group.intsearchers.append(candidate)
        avail_instances.remove(candidate)

        heapq.heappush(myheap, heap_top)


def allocate_intl1(options):
    if options.int_group is None:
        options.int_group = CURDB.groups.get_group(
            string.replace(options.intlookups[0].base_type, '_BASE', '_INT'))  # FIXME: make better

    # clear all ints
    for intlookup in options.intlookups:
        for brigade_group in intlookup.get_multishards():
            for brigade in brigade_group.brigades:
                brigade.intsearchers = []

    if options.add_ints_to_group:
        new_hosts = set(options.int_group.card.master.getHosts()) - set(options.int_group.getHosts())
        for host in new_hosts:
            options.int_group.addHost(host)

    avail_int_instances = set(options.int_group.get_instances()) - set(options.int_group.get_busy_instances())
    avail_int_instances = filter(options.flt, avail_int_instances)

    if options.max_ints_per_host is not None:
        d = defaultdict(list)
        for instance in avail_int_instances:
            d[instance.host].append(instance)
        for host in d:
            d[host].sort(key=lambda x: x.port)
        avail_int_instances = sum(map(lambda x: x[:options.max_ints_per_host], d.itervalues()), [])

    # add ints replica by replica
    for replica in range(options.ints_per_group):
        for intlookup in options.intlookups:
            print "Avail %d instances" % (len(avail_int_instances))

            def f(brigade):
                group_used_int_hosts = set(map(lambda x: x.host, brigade.intsearchers))
                group_avail_int_instances = filter(lambda x: x.host not in group_used_int_hosts, avail_int_instances)
                candidate = find_endmost_instance(brigade.get_all_basesearchers(), group_avail_int_instances)
                brigade.intsearchers.append(candidate)
                avail_int_instances.remove(candidate)

            intlookup.for_each_brigade(f)
            intlookup.mark_as_modified()

    # add extra instances if required
    if options.extra_instances is not None:
        int_groups = sum(map(lambda x: list(x.get_int_groups()), options.intlookups), [])
        add_extra_instances(int_groups, avail_int_instances, options.extra_instances)

    if options.add_ints_to_group:
        for host in set(map(lambda x: x.host, avail_int_instances)):
            options.int_group.removeHost(host)

    options.int_group.mark_as_modified()


def find_instance_for_intl2(intl2_group, free_instances):
    """
        Find most distant instance (intl2 instances should be located as uniformly as possible)
    """

    result_distance, result_instance = -1, None
    for instance in free_instances:
        distance = get_distance(intl2_group.intl2searchers, instance)
        if distance > result_distance:
            result_distance = distance
            result_instance = instance

    return result_instance


def allocate_intl2(options):
    if options.int_group is None:
        options.int_group = CURDB.groups.get_group(string.replace(options.intlookups[0].base_type, '_BASE', '_INTL2'))

    for intlookup in options.intlookups:
        for intl2_group in intlookup.intl2_groups:
            intl2_group.intl2searchers = []

    if options.add_ints_to_group:
        new_hosts = set(options.int_group.card.master.getHosts()) - set(options.int_group.getHosts())
        for host in new_hosts:
            options.int_group.addHost(host)

    free_instances = set(options.int_group.get_instances())  # FIXME: get only free instances

    for i in xrange(options.ints_per_group):
        for intlookup in options.intlookups:
            for intl2_group in intlookup.intl2_groups:
                instance = find_instance_for_intl2(intl2_group, free_instances)
                free_instances.remove(instance)
                intl2_group.intl2searchers.append(instance)
            intlookup.mark_as_modified()

    if options.add_ints_to_group:
        for host in set(map(lambda x: x.host, free_instances)):
            options.int_group.removeHost(host)

    options.int_group.mark_as_modified()


def show_intl1(options):
    for intlookup in options.intlookups:
        intlookup_score = 0
        intlookup_int_count = 0
        for intgroup in intlookup.get_int_groups():
            basesearchers = intgroup.get_all_basesearchers()
            intsearchers = intgroup.get_all_intsearchers()
            for intsearch in intsearchers:
                intlookup_score += get_distance(basesearchers, intsearch)
                intlookup_int_count += 1
        print "Intlookup %s: %d ints %s (%.1f per int) score" % (
            intlookup.file_name, intlookup_int_count, intlookup_score,
            intlookup_score / float(intlookup_int_count))


def normalize(options):
    del options


def main(options):
    if options.action == EActions.ALLOC_INTL2:
        allocate_intl2(options)
    elif options.action == EActions.ALLOC_INT:
        allocate_intl1(options)
    elif options.action == EActions.SHOW_INT:
        show_intl1(options)
    else:
        raise Exception("Unknown action %s" % options.action)

    CURDB.intlookups.update(smart=True)
    CURDB.groups.update(smart=True)


def jsmain(d):
    options = get_parser().parse_json(d)
    normalize(options)
    return main(options)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
