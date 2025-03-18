#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

import re
from argparse import ArgumentParser
import copy
from collections import defaultdict
import random

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types
from core.card.node import CardNode
from core.instances import TMultishardGroup, TIntGroup


class QueueDescr(object):
    def __init__(self, s):
        self.queue = s.split(' ')[0]
        self.power = float(s.split(' ')[1]) / 2
        self.method = s.strip().split(' ')[2]


def parse_cmd():
    parser = ArgumentParser(description="Add extra replicas to intlookup (from free instances)")
    parser.add_argument("-c", "--config", type=argparse_types.some_content, required=True,
                        help="Obligatory. Config with queues and their requirements")
    parser.add_argument("-g", "--main-group", type=argparse_types.group, required=True,
                        help="Obligatory. Name of golovan main group")
    parser.add_argument("-r", "--reserved-groups", type=argparse_types.groups, required=True,
                        help="Obligatory. Name of groups with reserved hosts")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def _get_slave_name(queue_name, main_group_name):
    name_parts = main_group_name.split('_')
    name_parts.insert(2, re.sub('\.', '_', queue_name.upper()))
    return '_'.join(name_parts)


def _get_candidate_hosts(groups):
    free_hosts = set()

    for group in groups:
        free_hosts |= set(group.getHosts()) - set(group.get_busy_hosts())

        for slave in group.slaves:
            if slave.card.host_donor is None:
                free_hosts -= set(slave.getHosts())

    return free_hosts


def _choose_candidates(allhosts, candidates, flt, power):
    filtered = filter(flt, candidates)

    assert (sum(map(lambda x: x.power, filtered)) >= power)
    random.shuffle(filtered)

    for i in range(len(filtered)):
        power -= filtered[i].power
        if power <= 0:
            for host in filtered[:i + 1]:
                candidates.remove(host)
                allhosts.remove(host)
            return filtered[:i + 1]


# FIXME: make better than look all hosts
def _get_dc_by_queue():
    result = dict()
    for host in CURDB.hosts.get_hosts():
        result[host.queue] = host.dc
    return result


def _assign_extra(group, first_replica, second_replica):
    # first move host to our groups and prepare instances
    CURDB.groups.move_hosts(first_replica + second_replica, group.card.master)
    CURDB.groups.add_slave_hosts(first_replica + second_replica, group)
    first_replica = map(lambda x: group.get_host_instances(x)[0], first_replica)
    second_replica = map(lambda x: group.get_host_instances(x)[0], second_replica)

    # add data to intlookup
    intlookup = CURDB.intlookups.get_intlookup(group.card.intlookups[0])
    for first_host, second_host in zip(first_replica, second_replica):
        brigade_group = TMultishardGroup()
        brigade_group.brigades.append(TIntGroup([[first_host]], []))
        brigade_group.brigades.append(TIntGroup([[second_host]], []))

        intlookup.brigade_groups.append(brigade_group)

    intlookup.brigade_groups_count = len(intlookup.brigade_groups)


def create_extra_slaves(options):
    ports = map(lambda x: x.card.legacy.funcs.instancePort, [options.main_group] + options.main_group.slaves)
    ports = map(lambda x: int(re.match('old(\d+)', x).group(1)), ports)  # FIXME: all port funcs in format old%d
    free_port = max(ports) + 1

    for queue_descr in options.config:
        queuename = queue_descr.queue
        slavename = _get_slave_name(queuename, options.main_group.name)

        if slavename not in map(lambda x: x.name, options.main_group.slaves):
            newslave = CURDB.groups.add_group(slavename,
                                              description="Slave group for queue %s" % queuename,
                                              owners=options.main_group.owners,
                                              watchers=options.main_group.watchers,
                                              funcs={'instanceCount': 'default', 'instancePower': 'default',
                                                     'instancePort': 'old%s' % free_port},
                                              master=options.main_group.name,
                                              )
            newslave.tags = options.main_group.tags
            newslave.properties = copy.deepcopy(options.main_group.properties)

            kventry = CardNode()
            kventry.add_field('name', 'served_queue')
            kventry.add_field('value', queuename)
            newslave.properties.kvstore.append(kventry)

            intlookupname = 'intlookup-%s.py' % re.sub('_', '-', slavename.lower())
            intlookup = CURDB.intlookups.create_empty_intlookup(intlookupname)
            intlookup.brigade_groups_count = 0
            intlookup.hosts_per_group = 1
            intlookup.base_type = slavename
            CURDB.intlookups.update_intlookup_links(intlookup.file_name)

            free_port += 1

        queue_descr.group = CURDB.groups.get_group(slavename)


def recluster_queue(queue_descr, candidates):
    power = queue_descr.power
    group = queue_descr.group
    intlookup = CURDB.intlookups.get_intlookup(group.card.intlookups[0])

    # check and reduce intlookup size
    intlookup_power = 0.
    for i in range(len(intlookup.brigade_groups)):
        intlookup_power += intlookup.brigade_groups[i].brigades[0].power
        if intlookup_power >= power:
            intlookup.brigade_groups = intlookup.brigade_groups[:i + 1]
            return

    needed_power = power - intlookup_power
    my_queue = queue_descr.queue
    my_dc = queue_descr.dc

    #    my_candidates = filter(lambda x: x.dc == my_dc and x.n_disks == 4 and x.memory >= 64 and x.queue != my_queue, candidates)
    if queue_descr.method == 'default':
        my_candidates = filter(lambda x: x.dc == my_dc and x.queue != my_queue and x.n_disks == 4 and x.memory < 120,
                               candidates)
    elif queue_descr.method == 'myt':
        my_candidates = filter(lambda x: x.dc == my_dc and x.queue in ['myt3', 'myt4'] and x.n_disks == 4, candidates)
    elif queue_descr.method == 'ams':
        my_candidates = filter(lambda x: x.dc == my_dc and x.n_disks == 4, candidates)
    elif queue_descr.method == 'ugr':
        my_candidates = filter(lambda x: x.dc in ['ugra', 'ugrb'] and x.queue != my_queue and x.n_disks == 4,
                               candidates)
    elif queue_descr.method == 'man':
        my_candidates = filter(lambda x: x.dc == my_dc, candidates)
    my_candidates = set(my_candidates)

    my_candidates_by_queue = defaultdict(list)
    for my_candidate in my_candidates:
        my_candidates_by_queue[my_candidate.queue].append(my_candidate)

    for queue in my_candidates_by_queue:
        my_candidates_by_queue[queue] = set(my_candidates_by_queue[queue])
    #        random.shuffle(my_candidates_by_queue[queue])
    #        my_candidates_by_queue[queue].sort(cmp = lambda x, y: cmp((y.power, y.memory), (x.power, x.memory)))

    #    print "Queue %s, need power %s:" % (my_queue, needed_power)
    #    for queue in my_candidates_by_queue:
    #        print "    queue %s has power %s" % (queue, sum(map(lambda x: x.power, my_candidates_by_queue[queue])))

    power_by_queue_model = defaultdict(float)
    for queue in my_candidates_by_queue:
        for host in my_candidates_by_queue[queue]:
            power_by_queue_model[(queue, host.model)] += host.power

    power_by_queue_model = map(lambda ((queue, model), power): (queue, model, power), power_by_queue_model.items())
    power_by_queue_model = filter(lambda (queue, model, power): power >= needed_power, power_by_queue_model)

    print "    %s" % power_by_queue_model

    if queue_descr.method in ['ams', 'man']:
        queue, model, power = power_by_queue_model[0]
        first_replica = _choose_candidates(candidates, my_candidates,
                                           lambda x: x.queue == queue and x.model == model, needed_power)
        second_replica = _choose_candidates(candidates, my_candidates,
                                            lambda x: x.queue == queue and x.model == model, needed_power)
    else:
        model_counts = defaultdict(int)
        for queue, model, power in power_by_queue_model:
            model_counts[model] += 1
        power_by_queue_model = filter(lambda (queue, model, power): model_counts[model] > 1, power_by_queue_model)

        choosen_model = power_by_queue_model[0][1]
        power_by_queue_model = filter(lambda (queue, model, power): model == choosen_model, power_by_queue_model)

        assert (len(power_by_queue_model) > 1)

        queue, model, power = power_by_queue_model[0]
        first_replica = _choose_candidates(candidates, my_candidates,
                                           lambda x: x.queue == queue and x.model == model, needed_power)

        queue, model, power = power_by_queue_model[1]
        second_replica = _choose_candidates(candidates, my_candidates,
                                            lambda x: x.queue == queue and x.model == model, needed_power)

    _assign_extra(group, first_replica, second_replica)


def create_replicated_intlookup(my_group, options):
    my_intlookup_name = 'intlookup-%s.py' % my_group.name.lower().replace('_', '-')
    if my_intlookup_name in my_group.intlookups:
        my_intlookup = CURDB.intlookups.get_intlookup(my_intlookup_name)
        my_intlookup.brigade_groups = []
    else:
        my_intlookup = CURDB.intlookups.create_empty_intlookup(my_intlookup_name)
        my_intlookup.base_type = my_group.name
        my_intlookup.hosts_per_group = 1

    for slave in options.main_group.slaves:
        for intlookup in map(lambda x: CURDB.intlookups.get_intlookup(x), slave.intlookups):
            for brigade_group in intlookup.brigade_groups:
                assert len(brigade_group.brigades) == 2, "Found shard with %d shards in file <%s>" % (
                len(brigade_group.brigades), intlookup.file_name)

                first_replica = my_group.get_host_instances(brigade_group.brigades[0].basesearchers[0][0].host)[0]
                second_replica = my_group.get_host_instances(brigade_group.brigades[1].basesearchers[0][0].host)[0]

                my_brigade_group = TMultishardGroup()
                my_brigade_group.brigades.append(TIntGroup([[first_replica]], []))
                my_brigade_group.brigades.append(TIntGroup([[second_replica]], []))
                my_intlookup.brigade_groups.append(my_brigade_group)

    my_intlookup.brigade_groups_count = len(my_intlookup.brigade_groups)


def main(options):
    queue_descrs = map(lambda x: QueueDescr(x), options.config.strip().split('\n'))
    queue_descrs = filter(lambda x: x.power > 0, queue_descrs)

    options.config = queue_descrs

    dc_by_queue = _get_dc_by_queue()
    for queue_descr in queue_descrs:
        queue_descr.dc = dc_by_queue.get(queue_descr.queue, 'unknown')

    create_extra_slaves(options)
    candidates = _get_candidate_hosts(options.reserved_groups)

    for queue_descr in options.config:
        recluster_queue(queue_descr, candidates)

    create_replicated_intlookup(options.main_group, options)
    for group in filter(lambda x: x.host_donor == options.main_group.name, options.main_group.slaves):
        create_replicated_intlookup(group, options)

    CURDB.update()


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
