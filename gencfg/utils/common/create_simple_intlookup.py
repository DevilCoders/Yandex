#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import heapq

import gencfg
from core.db import CURDB
from core.instances import TMultishardGroup, TIntl2Group, TIntGroup
import core.argparse.types
from core.argparse.parser import ArgumentParserExt


class TShardInfo(object):
    def __init__(self, shard_id, shard_weight):
        self.shard_id = shard_id
        self.shard_weight = shard_weight
        self.instances = []

    @property
    def power(self):
        return sum([x.power for x in self.instances] + [0]) / self.shard_weight

    def __lt__(self, other):
        if self.power == other.power:
            return self.shard_id <= other.shard_id
        else:
            return self.power <= other.power


def get_parser():
    usage = 'Usage %prog [options]'
    parser = ArgumentParserExt(description='Generate simple intlookup for group')
    parser.add_argument('-t', '--tier', type=core.argparse.types.tier, required=True,
                        help='Obligatory. Tier name')
    parser.add_argument('-g', '--group', type=core.argparse.types.group, required=True,
                        help='Obligatory. Group name')

    return parser


def distirbute_instances(instances, shard_weights):
    """Distribute instances among shards with specified shard weights"""

    # prepare instances
    instances.sort(key=lambda x: -x.power)

    # prepare shards
    shards_info = [TShardInfo(shard_id, shard_weight) for shard_id, shard_weight in enumerate(shard_weights)]
    heapq.heapify(shards_info)

    # run optimization
    for instance in instances:
        shard_info = heapq.heappop(shards_info)
        shard_info.instances.append(instance)
        heapq.heappush(shards_info, shard_info)

    # return instances
    shards_info.sort(key=lambda x: x.shard_id)
    return [x.instances for x in shards_info]


def main(options):
    # remove old intlookups if have ones
    for intlookup_name in options.group.card.intlookups:
        CURDB.intlookups.remove_intlookup(intlookup_name)
    options.group.mark_as_modified()

    # create new intlookup
    intlookup = CURDB.intlookups.create_empty_intlookup(options.group.card.name)
    intlookup.brigade_groups_count = 1
    intlookup.tiers = [options.tier.name]
    intlookup.hosts_per_group = 1
    intlookup.base_type = options.group.card.name
    intlookup.intl2_groups = [TIntl2Group()]

    instances = options.group.get_instances()
    shard_weights = [options.tier.get_shard_weight(x) for x in xrange(options.tier.get_shards_count())]
    for shard_replicas in distirbute_instances(instances, shard_weights):
        mgroup = TMultishardGroup()
        for instance in shard_replicas:
            mgroup.brigades.append(TIntGroup([[instance]], []))
        intlookup.intl2_groups[0].multishards.append(mgroup)

    CURDB.update(smart=True)


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    main(options)
