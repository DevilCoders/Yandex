#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
from argparse import ArgumentParser
import random

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


# class representing replicas of specific shard
class Row(object):
    def __init__(self, id_, instances):
        self.id_ = id_
        self.instances = instances


# class represenging instance in one group, which can be shuffled
class Column(object):
    def __init__(self, instances):
        self.instances = instances


def parse_cmd():
    parser = ArgumentParser(
        description="Reduce affect of dead hosts by shuffling instances in order to reduce host-vs-host intersections")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=["show", "optimize"],
                        help="Obligatory. Action to execute")
    parser.add_argument("-c", "--sas-config", type=argparse_types.sasconfig, required=False,
                        help="Optional. Get intlookups from specified sas config")
    parser.add_argument("-i", "--intlookups", type=argparse_types.intlookups, required=False,
                        help="Obligatory. Intlookups to process")
    parser.add_argument("-s", "--steps", type=int, default=10000,
                        help="Optional. Number of steps")
    parser.add_argument("--seed", type=int, default=123,
                        help="Optional. Random seed")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if int(options.sas_config is None) + int(options.intlookups is None) != 1:
        raise Exception("You must specify exactly one of --sas-config --intlookups")

    return options


def swap_instances(instance1, instance2, instances_data, hosts_data):
    assert (instance1.host != instance2.host)

    host1 = instance1.host
    host2 = instance2.host
    idata1 = instances_data.pop(instance1)
    idata2 = instances_data.pop(instance2)

    hosts_data[host1]['instances'] = filter(lambda x: x != instance1, hosts_data[host1]['instances'])
    hosts_data[host2]['instances'] = filter(lambda x: x != instance2, hosts_data[host2]['instances'])

    instance1.swap_data(instance2)

    hosts_data[host1]['instances'].append(instance2)
    instances_data[instance1] = idata1
    hosts_data[host2]['instances'].append(instance1)
    instances_data[instance2] = idata2


# try to swap instance, calculate new score
# if new score more then starting swap back
def try_swap(instance1, instance2, instances_data, hosts_data):
    row1 = instances_data[instance1]['row']
    row2 = instances_data[instance2]['row']

    affected_hosts = list(set(map(lambda x: x.host, row1.instances + row2.instances)))
    old_score = sum(map(lambda x: hosts_data[x]['score'], affected_hosts))

    swap_instances(instance1, instance2, instances_data, hosts_data)

    new_score = 0.0
    for host in affected_hosts:
        new_score += calc_dead_host_score(host, hosts_data, instances_data)

    if new_score < old_score:
        for host in affected_hosts:
            hosts_data[host]['score'] = calc_dead_host_score(host, hosts_data, instances_data)
    else:
        swap_instances(instance1, instance2, instances_data, hosts_data)


def calc_dead_host_score(dead_host, hosts_data, instances_data):
    instances = hosts_data[dead_host]['instances']
    rows = list(set(map(lambda x: instances_data[x]['row'], instances)))

    other_hosts_extra = defaultdict(float)
    for row in rows:
        dead_host_power = sum(map(lambda x: x.power if x.host == dead_host else 0.0, row.instances))
        total_power = sum(map(lambda x: x.power, row.instances))

        for instance in row.instances:
            if instance.host == dead_host:
                continue
            other_hosts_extra[instance.host] += instance.power * dead_host_power / total_power

    result = 0.0
    for host in other_hosts_extra:
        p = other_hosts_extra[host] / host.power
        result += p * p * p

    return result


def main(options):
    instances_data = defaultdict(dict)  # various instances data
    hosts_data = defaultdict(dict)  # various hosts data
    rows = []
    columns = []

    if options.sas_config is not None:
        intlookups = map(lambda x: CURDB.intlookups.get_intlookup(x.intlookup), options.sas_config)
    else:
        intlookups = options.intlookups

    # fill starting data
    rid = 0
    for intlookup in intlookups:
        # group shard replicas in Row struct
        for i in range(intlookup.get_shards_count()):
            row = Row(rid + i, intlookup.get_base_instances_for_shard(i))
            for instance in row.instances:
                instances_data[instance]['row'] = row

                if 'instances' not in hosts_data[instance.host]:
                    hosts_data[instance.host]['instances'] = []
                hosts_data[instance.host]['instances'].append(instance)

            rows.append(row)

        rid += intlookup.get_shards_count()

        # group instances which can be switched in Column struct
        for brigade_group in intlookup.brigade_groups:
            for brigade in brigade_group.brigades:
                if len(filter(lambda x: len(x) > 1, brigade.basesearchers)) > 0:
                    raise Exception("In intlookup %s found group with more than 1 instance per shard in group, not supported" % intlookup.file_name)
                column = Column(brigade.get_all_basesearchers())
                columns.append(column)

    # calculate initial score
    for host in hosts_data:
        hosts_data[host]['score'] = calc_dead_host_score(host, hosts_data, instances_data)

    if options.action == 'show':
        total_score = sum(map(lambda x: x['score'], hosts_data.values()))
        print "Total score: %s" % total_score
    elif options.action == 'optimize':
        start_score = sum(map(lambda x: x['score'], hosts_data.values()))
        print "Optimize start score: %s" % start_score

        # main optimization score
        random.seed(options.seed)
        for i in range(options.steps):
            column = columns[random.randrange(0, len(columns) - 1)]
            instance1 = column.instances[random.randrange(0, len(column.instances) - 1)]
            instance2 = column.instances[random.randrange(0, len(column.instances) - 1)]

            if instance1.host == instance2.host:
                continue

            try_swap(instance1, instance2, instances_data, hosts_data)

        #        instance1 = rows[1].instances[0]
        #        instance2 = rows[2].instances[0]

        #        try_swap(instance1, instance2, instances_data, hosts_data)

        #        swap_instances(instance1, instance2, instances_data, hosts_data)
        #        for host in hosts_data:
        #            hosts_data[host]['score'] = calc_dead_host_score(host, hosts_data, instances_data)

        finish_score = sum(map(lambda x: x['score'], hosts_data.values()))
        print "Optimize finish score: %s" % finish_score

        CURDB.update()


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
