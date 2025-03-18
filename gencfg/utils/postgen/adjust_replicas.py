#!/skynet/python/bin/python
"""
    Upon generation some hosts constains more than one instance. Usually every two hosts have at most one common shard. But sometimes hosts have two,
    three or more common shards. This is very bad thing due to following reasons:
        - when one host dies, all its rps is distributed among other repilcas. Thus if two hosts have several same shards, a lot of rps from first
          host goes to second one, making extremely non-uniform distribution of cpu load;

    This utility tries to fix this problem by rearrangment of host in intlookups.
"""

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import random
from collections import defaultdict
from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


class EActions(object):
    SHOW = 'show'
    FIX = 'fix'
    ALL = [SHOW, FIX]


class Row(object):
    def __init__(self, instances):
        self.instances = instances

    def can_swap(self, instance, pos):
        for i in range(len(self.instances)):
            if i == pos:
                continue
            if instance.host.name == self.instances[i].host.name:
                return False
        return True

    def get_pairs(self):
        names = sorted(list(map(lambda x: x.host.name, self.instances)))

        result = []
        for i in range(len(names)):
            for j in range(i + 1, len(names)):
                result.append((names[i], names[j]))
        return result


class ICalculator(object):
    """
        This class calculate some float value, which describes quality of solution (lesser value means better solution).
    """

    def __init__(self, rows):
        self.rows = rows
        self.shards = len(self.rows)

        self.pairs = defaultdict(int)
        self.value = 0

        for row in self.rows:
            self.plusrow(row)

    def plusrow(self, row):
        raise NotImplementedError("Method <plusrow> not implemented")

    def minusrow(self, row):
        raise NotImplementedError("Method <minusrow> not implemented")

    def can_swap(self, column, row1, row2):
        instance1 = self.rows[row1].instances[column]
        instance2 = self.rows[row2].instances[column]
        return self.rows[row1].can_swap(instance2, column) and self.rows[row2].can_swap(instance1, column)

    def swap(self, column, row1, row2):
        instance1 = self.rows[row1].instances[column]
        instance2 = self.rows[row2].instances[column]

        self.minusrow(self.rows[row1])
        self.minusrow(self.rows[row2])

        instance1.swap_data(instance2)

        self.plusrow(self.rows[row1])
        self.plusrow(self.rows[row2])

    def run(self, steps, start_shard=None, finish_shard=None):
        if start_shard is None:
            start_shard = 0
        if finish_shard is None:
            finish_shard = self.shards - 1

        assert (0 <= start_shard < finish_shard <= self.shards - 1),\
            "start_shard %s, finish_shard %s, self.shards %s" % (start_shard, finish_shard, self.shards)

        for i in range(steps):
            row1 = random.randint(start_shard, finish_shard)
            row2 = random.randint(start_shard, finish_shard)

            replicas = min(len(self.rows[row1].instances), len(self.rows[row2].instances))
            column = random.randint(0, replicas - 1)

            if row1 == row2:
                continue

            if self.can_swap(column, row1, row2):
                oldv = self.value
                self.swap(column, row1, row2)
                newv = self.value

                if newv >= oldv:
                    self.swap(column, row1, row2)


class TSameReplicasCalculator(ICalculator):
    """This class reduces number of hosts with same shards (ideally reduce intersection of any to host to at mos one shard)"""

    def __init__(self, rows):
        self.pairs = defaultdict(int)

        super(TSameReplicasCalculator, self).__init__(rows)

    def plusrow(self, row):
        for p in row.get_pairs():
            self.value -= self.mult(self.pairs[p])
            self.pairs[p] += 1
            self.value += self.mult(self.pairs[p])

    def minusrow(self, row):
        for p in row.get_pairs():
            self.value -= self.mult(self.pairs[p])
            self.pairs[p] -= 1
            self.value += self.mult(self.pairs[p])
            if self.pairs[p] < 0:
                raise Exception("OOPS")

    # noinspection PyMethodMayBeStatic
    def mult(self, v):
        if v > 1:
            return v * v * v - 1
        else:
            return 0


class THostShardMultipleReplicasCalculator(ICalculator):
    """This class reduces number of hosts with two or more replicas of same shard"""

    def __init__(self, rows):
        self.pairs = defaultdict(int)

        super(THostShardMultipleReplicasCalculator, self).__init__(rows)

    def plusrow(self, row):
        self.value += len(row.instances) - len({x.host for x in row.instances})

    def minusrow(self, row):
        self.value -= len(row.instances) - len({x.host for x in row.instances})

    # noinspection PyMethodMayBeStatic
    def mult(self, v):
        if v > 1:
            return v * v * v - 1
        else:
            return 0


def parse_cmd():
    parser = ArgumentParser(
        description="Reduce number of shards assigned to two hosts (every host pair should have at most 1 common shard)")

    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=EActions.ALL,
                        help="Obligatory. Action to execute")
    parser.add_argument('-u', '--calculator', type=str, default='hosts_same_shards',
                        choices=['hosts_same_shards', 'host_multiple_shard_replicas'],
                        help='Optional. Calculator (hosts_same_shards by default)')
    parser.add_argument("-i", "--intlookups", type=argparse_types.intlookups, default=None,
                        help="Optional. Comma-separated list of intlookups to work with (incompatible with --sas-config option)")
    parser.add_argument("-c", "--sas-config", type=argparse_types.sasconfig, default=None,
                        help="Optional. Path to sas config (incompatible with --intlookups option)")
    parser.add_argument("-s", "--steps", type=int, default=1000,
                        help="Optional. Number of steps")
    parser.add_argument("-m", "--multi-brigades", action="store_true", default=False,
                        help="Optional. Optimize multiple groups simultaneously. Be careful when enabling this option.")
    parser.add_argument("--fail-value", type=int, default=None,
                        help="Optional. Exit with non-zero status if <intersection value> more then specified in argument")
    parser.add_argument("-v", "--verbose", action="count", default=0,
                        help="Optional. Add extra verbose output")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def normalize(options):
    if int(options.intlookups is not None) + int(options.sas_config is not None) != 1:
        raise Exception("You must specify exactly one of --intlookup --sas-config option")

    if options.sas_config is not None:
        options.intlookups = map(lambda x: CURDB.intlookups.get_intlookup(x.intlookup), options.sas_config)


def show_intersections(calculator):
    intersection_value = calculator.value
    print "Intersection value: %s" % intersection_value

    if options.verbose > 0:
        print "Hosts with multiple intersection:"
        for (host1, host2), common_shards in calculator.pairs.iteritems():
            print "    Hosts %s and %s: %d common shards" % (host1, host2, common_shards)


def main(options):
    rows = []
    for intlookup in options.intlookups:
        intlookup.mark_as_modified()
        for shard_id in range(intlookup.shards_count()):
            rows.append(Row(intlookup.get_base_instances_for_shard(shard_id)))

    if options.calculator == 'hosts_same_shards':
        calculator = TSameReplicasCalculator(rows)
    elif options.calculator == 'host_multiple_shard_replicas':
        calculator = THostShardMultipleReplicasCalculator(rows)
    else:
        raise Exception('Unknown calculator <{}>'.format(options.calculator))

    if options.action == EActions.SHOW:
        show_intersections(calculator)
    elif options.action == EActions.FIX:
        print "=========== BEFORE FIXING =============="
        show_intersections(calculator)
        print "========================================"

        start_row = 0
        for intlookup in options.intlookups:
            if (not options.multi_brigades) and (intlookup.hosts_per_group == 1):
                raise Exception("Can not optimize intlookup %s with hosts_per_group == 1 and disabled --multi-brigade options" % intlookup.file_name)

            if options.multi_brigades:
                N = 1
                M = intlookup.hosts_per_group * intlookup.brigade_groups_count
            else:
                N = intlookup.brigade_groups_count
                M = intlookup.hosts_per_group

            for i in range(N):
                start_shard = start_row + i * M
                finish_shard = start_shard + M - 1
                calculator.run(options.steps, start_shard=start_shard, finish_shard=finish_shard)

        print "=========== AFTER FIXING ==============="
        show_intersections(calculator)
        print "========================================"

        CURDB.intlookups.update(smart=1)
    else:
        raise Exception("Unknown action %s" % options.action)

    if options.fail_value is not None and options.fail_value < calculator.value:
        return 1
    return 0


if __name__ == '__main__':
    options = parse_cmd()

    normalize(options)

    retcode = main(options)

    sys.exit(retcode)
