#!/skynet/python/bin/python

import os
import sys
import random

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '../contrib')))

from collections import defaultdict
import math
import copy

import gencfg
from core.argparse.parser import ArgumentParserExt
from core.db import CURDB
from core.instances import TMultishardGroup, TIntGroup, TIntl2Group
from gaux.aux_colortext import red_text
import core.argparse.types as argparse_types
import utils.pregen.find_most_memory_free_machines as find_most_memory_free_machines

from contrib.max_matching import max_matching


def get_parser():
    parser = ArgumentParserExt("Utility to add hamster instances")

    parser.add_argument("-i", "--intlookups-info", type=str, dest="intlookups_info",
                      help="obligatory option. Comma-separated list of <<intlookup:replicas>>")
    parser.add_argument("-o", "--out-intlookup", type=str, dest="out_intlookup",
                      help="Optional option. Output intlookup")
    parser.add_argument("-g", "--group", type=str, dest="group",
                      help="obligatory option. Output group")
    parser.add_argument("-x", "--exclude-hosts", type=argparse_types.hosts, default=[],
                      help="Optional. List of hosts to exclude")
    parser.add_argument("--seed", type=int, default=2842183,
                      help="Optional. Random seed")


    return parser


def solve(graph, left_replicas):
    # heuristics
    result = dict((left_node, []) for left_node in graph)
    attempt = 0
    while True:
        working_graph = {}
        for left_node in graph:
            used_hosts = result[left_node]
            for left_replica in range(left_replicas - len(result[left_node])):
                working_graph[(left_node, left_replica)] = \
                    [host for host in graph[left_node] if host not in used_hosts]
        sub_solution, _, _ = max_matching(working_graph)
        for right_node, (left_node, _) in sub_solution.items():
            result[left_node].append(right_node)

        if all(len(x) == left_replicas for x in result.values()):
            return result

        attempt += 1


class OData(object):
    def __init__(self, max_instances_count):
        self.host_counts = defaultdict(int)
        self.result = 0
        self.max_instances_count = copy.copy(max_instances_count)

    def _f(self, host, N):
        if host.memory <= 24:
            return N * N * N * 100
        return N * N * N

    def add_instance(self, instance):
        assert self.max_instances_count[instance.host] >= 1

        self.result -= self._f(instance.host, self.host_counts[instance.host])
        self.host_counts[instance.host] += 1
        self.max_instances_count[instance.host] -= 1
        self.result += self._f(instance.host, self.host_counts[instance.host])

    def remove_instance(self, instance):
        if self.host_counts[instance.host] == 0:
            raise Exception("Removing empty host")
        self.result -= self._f(instance.host, self.host_counts[instance.host])
        self.host_counts[instance.host] -= 1
        self.max_instances_count[instance.host] += 1
        self.result += self._f(instance.host, self.host_counts[instance.host])

    def can_add_instance(self, instance):
        return self.max_instances_count[instance.host] >= 1


def solve_other(shard_instances, max_instances_count):
    N = len(shard_instances)
    out_instances = []

    odata = OData(max_instances_count)
    for instances, rr in shard_instances:
        ss = []
        for i in range(len(instances)):
            if instances[i].host in map(lambda x: x.host, ss):
                continue
            if not odata.can_add_instance(instances[i]):
                continue

            ss.append(instances[i])
            odata.add_instance(instances[i])

            if len(ss) == rr:
                break
        if len(ss) < rr:
            raise Exception("Can not find %d unique hosts among %s" % (rr, map(lambda x: '%s:%s' % (x.host.name, x.port), instances)))

        out_instances.append(ss)

    for i in range(1300000):
        shard_id = random.randint(0, N - 1)
        n = random.randint(0, len(shard_instances[shard_id][0]) - 1)
        m = random.randint(0, min(shard_instances[shard_id][1], len(shard_instances[shard_id][0])) - 1)

        if shard_instances[shard_id][0][n].host in map(lambda x: x.host, out_instances[shard_id]):
            if out_instances[shard_id][m].host != shard_instances[shard_id][0][n].host:
                continue

        if odata.can_add_instance(shard_instances[shard_id][0][n]):
            old_result = odata.result
            odata.add_instance(shard_instances[shard_id][0][n])
            odata.remove_instance(out_instances[shard_id][m])
            if odata.result <= old_result:
                out_instances[shard_id][m] = shard_instances[shard_id][0][n]
            else:
                odata.add_instance(out_instances[shard_id][m])
                odata.remove_instance(shard_instances[shard_id][0][n])
    return out_instances


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    options.exclude_hosts = set(options.exclude_hosts)
    options.intlookups_info = map(lambda x: (CURDB.intlookups.get_intlookup(x.split(':')[0]), int(x.split(':')[1])),
                                  options.intlookups_info.split(','))

    # set seed to have the same result on each run
    random.seed(options.seed)

    host_load = defaultdict(int)

    shard_instances = []
    for intlookup, replicas in options.intlookups_info:
        rr = replicas

        for i in range(intlookup.hosts_per_group * intlookup.brigade_groups_count):
            filtered_instances = intlookup.get_base_instances_for_shard(i)
            filtered_instances = [x for x in filtered_instances if x.host not in options.exclude_hosts]
            if len(filtered_instances) < rr:
                raise Exception("OOPS, not enough instances in intlookup %s, shard %d: %d of %d" % (intlookup.file_name, i, len(filtered_instances), replicas))

            shard_instances.append((filtered_instances, rr))

    # calculate memory per instance
    mgroups = [CURDB.groups.get_group(options.group)] + [x for x in CURDB.groups.get_groups() if x.card.host_donor == options.group]
    memory_per_instance = sum(x.card.reqs.instances.memory_guarantee.value / 1024. / 1024 / 1024 for x in mgroups)

    # calculate maximal number of instances per host
    candidate_hosts = set()
    for intlookup, _ in options.intlookups_info:
        candidate_hosts |= set(x.host for x in intlookup.get_base_instances())

    candidate_hosts_report = find_most_memory_free_machines.jsmain(dict(action='show', hosts=list(candidate_hosts)))
    max_instances_count = {x.host: int(math.floor(x.memory_left / memory_per_instance)) for x in candidate_hosts_report}

    out_instances = solve_other(shard_instances, max_instances_count)
    for instance in sum(out_instances, []):
        host_load[instance.host] += 1

    if options.out_intlookup is None:
        options.out_intlookup = options.group
    if CURDB.intlookups.has_intlookup(options.out_intlookup):
        out_intlookup = CURDB.intlookups.get_intlookup(options.out_intlookup)
    else:
        out_intlookup = CURDB.intlookups.create_empty_intlookup(options.out_intlookup)

    out_intlookup.brigade_groups_count = sum(map(lambda x: x[0].brigade_groups_count, options.intlookups_info))
    out_intlookup.base_type = options.group
    out_intlookup.tiers = sum(map(lambda t: t[0].tiers, options.intlookups_info), [])
    out_intlookup.hosts_per_group = options.intlookups_info[0][0].hosts_per_group
    out_intlookup.intl2_groups = [TIntl2Group()]

    out_intlookup.mark_as_modified()
    CURDB.groups.get_group(options.group).mark_as_modified()

    nhosts_by_load = defaultdict(int)
    for k, v in host_load.iteritems():
        nhosts_by_load[v] += 1
    for v in nhosts_by_load:
        if v > 1:
            print red_text("Have %d hosts with %d hamster instances" % (nhosts_by_load[v], v))

    nhosts_by_load = defaultdict(int)
    for i in range(len(out_instances) / out_intlookup.hosts_per_group):
        multishard_group = TMultishardGroup()
        # out_intlookup.brigade_groups.append(TMultishardGroup())
        for j in range(len(out_instances[i * out_intlookup.hosts_per_group])):
            basesearchers = map(lambda x: x[j], out_instances[i * out_intlookup.hosts_per_group: (
                                                                                                 i + 1) * out_intlookup.hosts_per_group])
            tmp = []
            for basesearcher in basesearchers:
                n = nhosts_by_load[basesearcher.host.name]
                nhosts_by_load[basesearcher.host.name] += 1
                if not CURDB.groups.get_group(options.group).hasHost(basesearcher.host):
                    CURDB.groups.get_group(options.group).addHost(basesearcher.host)
                instance = CURDB.groups.get_instance_by_N(basesearcher.host.name, options.group, n)
                tmp.append([instance])
            basesearchers = tmp
            brigade = TIntGroup(basesearchers, [])
            multishard_group.brigades.append(brigade)
        out_intlookup.intl2_groups[-1].multishards.append(multishard_group)

    CURDB.intlookups.update(smart=True)
    CURDB.groups.update(smart=True)
    CURDB.hosts.update(smart=True)
