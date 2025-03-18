#!/skynet/python/bin/python

import os
import sys
from collections import defaultdict, namedtuple

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.instances import TIntGroup, TMultishardGroup, TIntl2Group
from core.tiers import Tier
from core.db import CURDB
from core.card.node import CardNode, load_card_node_from_dict
import core.argparse.types

tiers_map = {
    'Rus-1': 'OxygenExpPlatinumTier0',
    'Rus0': 'OxygenExpRusTier0',
    'Ukr0': 'OxygenExpUkrTier0',
    'Geo0': 'OxygenExpGeoTier0',
    'Rrg0': 'OxygenExpRRGTier0',
    'OxydevTier0': 'OxydevTier0',
    'Eng0': 'OxygenExpEngTier0',
    'Tur-1': 'OxygenExpPlatinumTurTier0',
    'Tur0': 'OxygenExpTurTier0',
    'Div0': 'OxygenExpDivTier0',
    'DivTr0': 'OxygenExpDivTrTier0',
    'WebTier0': 'OxygenExpWebTier0',
    'WebTier1': 'OxygenExpWebTier1',
    'PlatinumTier0': 'OxygenExpPlatinumTier0',
    'RusMaps': 'OxygenExpRusMapsTier0',
    'TurMaps': 'OxygenExpTurMapsTier0',
}

ShardInfo = namedtuple('Shard', ['primus', 'tier', 'hosts'])


def parse_cmd():
    parser = ArgumentParser(description="Import oxygen shards")

    parser.add_argument("-p", "--src-path", dest="src_path", type=str, required=True,
                        help="Obligatory. New shardmap file")
    parser.add_argument("-i", "--intlookup-name", dest="intlookup_name", type=str, required=True,
                        help="Obligatory. Intlookup name")
    parser.add_argument("-s", "--size", dest="group_size", type=int, required=True,
                        help="Obligatory. Group size")
    parser.add_argument("-g", "--group", type=core.argparse.types.group, required=True,
                        help="Obligatory. Group name")
    parser.add_argument("-t", "--tiers-filter", dest="tiers_filter", type=str, required=True,
                        help="Obligatory. Tiers filter")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


if __name__ == '__main__':
    options = parse_cmd()

    src = options.src_path
    intlookup_name = options.intlookup_name
    group_size = options.group_size
    tiers_filter = options.tiers_filter
    if tiers_filter:
        tiers_filter = tiers_filter.split(',')
    src = os.path.abspath(src)
    if not os.path.exists(src):
        raise Exception('Cannot open file %s' % src)

    contents = open(src).read().split('\n')
    contents = [x.strip() for x in contents]
    contents = [x for x in contents if x]

    shards = [x.split() for x in contents]
    shards = [
        ShardInfo(primus=x[0].rpartition('-')[0], tier=tiers_map[x[1]], hosts=[host for host in x[2:]])
        for x in shards]

    shards_by_tier = defaultdict(list)
    for shard in shards:
        shards_by_tier[shard.tier].append(shard)

    if tiers_filter:
        shards_by_tier = {x: y for (x, y) in shards_by_tier.items() if x in tiers_filter}

    instances_per_host = defaultdict(int)
    for il in options.group.card.intlookups:
        intlookup = CURDB.intlookups.get_intlookup(il)
        for instance in intlookup.get_used_base_instances():
            host = instance.host.name
            instances_per_host[host] = max(instances_per_host[host], instance.N + 1)

    # create tiers:
    tiers_order = shards_by_tier.keys()
    for tiername in tiers_order:
        # check if number of shards changed and we have intlookups with this tier (basic request)
        intlookups_with_tier = filter(lambda x: x.tiers is not None and tiername in x.tiers,
                                      CURDB.intlookups.get_intlookups())
        if CURDB.tiers.has_tier(tiername) and (
            len(shards_by_tier[tiername]) != CURDB.tiers.get_tier(tiername).get_shards_count()) and len(
                intlookups_with_tier) > 0:
            raise Exception("Trying to update oxygen tier <%s> size from %d shards to %d, while using this tier in intlookups <%s>" % (
                             tiername, CURDB.tiers.get_tier(tiername).get_shards_count(), len(shards_by_tier[tiername]),
                             ",".join(map(lambda x: x.file_name, intlookups_with_tier))))

        shards = shards_by_tier[tiername]
        primuses = [{'name': shard.primus, 'shards': [0]} for shard in shards]

        primus_nodes = map(lambda x: CardNode(), range(len(shards)))
        for i, shard in enumerate(shards):
            primus_nodes[i].add_field('name', shard.primus)
            primus_nodes[i].add_field('shards', [0])

        tier_card = dict()
        tier_card['name'] = tiername
        tier_card['primuses'] = map(lambda x: {'name': x.primus, 'shards': [0]}, shards)
        tier_card['properties'] = {'min_replicas': 0}

        new_tier = Tier(CURDB, load_card_node_from_dict(tier_card, CURDB.tiers.TIER_SCHEME))
        CURDB.tiers.set_tier(new_tier)

    for tier, tier_shards in shards_by_tier.items():
        count = len(tier_shards)
        valid_count = CURDB.tiers.get_tier(tier).get_shards_count() if CURDB.tiers.has_tier(tier) else None
        if valid_count is not None and count != valid_count:
            raise Exception('Number of shards is invalid has %s should be %s for tier %s' % (count, valid_count, tier))
        if count % group_size != 0:
            raise Exception('Number of shards %s in tier %s is not a multiple of group_size %s' % (count, tier, group_size))
        replicas_count = set(len(shard.hosts) for shard in tier_shards)
        if len(replicas_count) != 1:
            raise Exception('Number of replicas differs for shards in tier %s' % tier)

    brigade_groups = []
    for tier in tiers_order:
        tier_shards = shards_by_tier[tier]
        replicas = len(tier_shards[0].hosts)
        for i in range(len(tier_shards) / group_size):
            brigade_group = TMultishardGroup()
            for replica in range(replicas):
                base_instances = []
                for j in range(group_size):
                    host = tier_shards[i * group_size + j].hosts[replica]
                    instance = CURDB.groups.get_instance_by_N(host, options.group.card.name, instances_per_host[host])
                    base_instances.append([instance])
                    instances_per_host[host] += 1
                brigade = TIntGroup(base_instances, [])
                brigade_group.brigades.append(brigade)
            brigade_groups.append(brigade_group)

    if CURDB.intlookups.has_intlookup(intlookup_name):
        intlookup = CURDB.intlookups.get_intlookup(intlookup_name)
    else:
        intlookup = CURDB.intlookups.create_empty_intlookup(intlookup_name)
    intlookup.base_type = options.group.card.name
    intlookup.tiers = tiers_order[:]
    intlookup.hosts_per_group = group_size
    intlookup.intl2_groups.append(TIntl2Group(brigade_groups))
    intlookup.brigade_groups_count = len(brigade_groups)

    options.group.mark_as_modified()
    intlookup.mark_as_modified()

    CURDB.tiers.update()
    CURDB.intlookups.update(smart=True)
    CURDB.groups.update(smart=True)
    # TODO: add ints
