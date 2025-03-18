#!/skynet/python/bin/python

import re
from collections import defaultdict
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from optparse import OptionParser

import gencfg
from core.db import CURDB
from core.tiers import Tier
from core.card.node import load_card_node_from_dict

TIERS_MAP = {
    'Rus0': 'RusTier0',
    'Rus1': 'RusTier1',
    'Eng0': 'EngTier0',
    'Eng1': 'EngTier1',
    'Ukr0': 'UkrTier0',
    'Ukr1': 'UkrTier1',
    'Kz0': 'KzTier0',
    'Geo0': 'GeoTier0',
    'Tur-1': 'PlatinumTurTier0',
    'Tur0': 'TurTier0',
    'Tur1': 'TurTier1',
    'Ita0': 'ItaTier0',
    'Kub0': 'KubTier0',
    'Jud0': 'JudTier0',
    'RusMaps0': 'RusMapsTier0',
    'TurMaps0': 'TurMapsTier0',
    'Rrg0': 'RRGTier0',
}


def parse_cmd():
    parser = OptionParser(usage='usage: %prog -c <hostcfg> [-t tier0,tier1,...] [-r]')

    parser.add_option("-c", "--src-hostcfg", type="str", dest="src_hostcfg", default=None,
                      help="obligatory. Source hostcfg file.")
    parser.add_option("-r", "--replace", dest="replace", action='store_true', default=False,
                      help="optional. Replace existing tiers.")
    parser.add_option("-t", "--tiers", type="str", dest="tiers", default=None,
                      help="optional. List of comma separated tiers to import.")
    parser.add_option("-v", "--verbose", action="store_true", dest="verbose", default=False,
                      help="optional. Explain what is being done.")
    parser.add_option("-y", "--apply", action="store_true", dest="apply", default=False,
                      help="optional. Apply changes.")
    parser.add_option("--tiers-mapping", type="str", dest="tiers_mapping", default=None,
                      help="Options. Tiers mapping")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options, args = parser.parse_args()

    if options.src_hostcfg is None:
        raise Exception('Missing source hostcfg file option (--src-hostcfg).')
    if options.tiers is not None:
        options.tiers = options.tiers.split(',')

    if options.tiers_mapping is None:
        options.tiers_mapping = TIERS_MAP
    else:
        options.tiers_mapping = dict(
            map(lambda x: (x.strip().split(' ')[0], x.strip().split(' ')[1]), open(options.tiers_mapping).readlines()))

    return options


def _find_line(lst, r, cur_line):
    reg = re.compile(r)
    for i in xrange(cur_line, len(lst)):
        if reg.match(lst[i]):
            return i
    return -1


if __name__ == '__main__':
    options = parse_cmd()

    lines = open(options.src_hostcfg).readlines()
    default_primus_set = set()
    scatter_first_line = _find_line(lines, '<ScatterTable>\n', 0)
    if scatter_first_line != -1:
        scatter_last_line = _find_line(lines, '</ScatterTable>\n', 0)
        if scatter_last_line == -1:
            raise Exception('Unpaired tag <ScatterTable>')
        reg = re.compile('\s*(\S+)\s+(\d+)')
        for line in lines[scatter_first_line + 1: scatter_last_line]:
            p = reg.search(line)
            default_primus_set.add(p.group(1))

    current_line = 0
    tiers = []
    unique_tiers = set()
    while True:
        first_line = _find_line(lines, '<SearchClusterConfigs.*>\n', current_line)
        if first_line == -1:
            break
        last_line = _find_line(lines, '</SearchClusterConfigs.*>\n', first_line)
        if last_line == -1:
            raise Exception('Unpaired tag %s in %s' % (lines[first_line], options.src_hostcfg))
        tier_name = lines[first_line].rstrip('\n').replace('<', '').replace('>', '').replace('SearchClusterConfigs', '')
        tier_name = options.tiers_mapping.get(tier_name, tier_name)

        if tier_name in unique_tiers:
            raise Exception("Tier %s described at least twice" % tier_name)
        tiers.append((tier_name, first_line, last_line))
        unique_tiers.add(tier_name)
        current_line = last_line + 1

    reg = re.compile('^\s*([\w\d-]+)\s*:\s*(\d+)\s*\n$')
    primus_shards_count = defaultdict(int)
    affected_primuses = set()
    affected_tiers = set()
    for tier_name, first_line, last_line in tiers:
        primus_tier_shards_count = {}
        default_primus_shards = 0
        for line in lines[first_line + 1: last_line]:
            match = reg.match(line)
            if match.group(1).find('default') != -1:
                default_primus_shards = int(match.group(2))
            else:
                primus_name = match.group(1)
                shards_count = int(match.group(2))
                if primus_name in primus_tier_shards_count:
                    raise Exception('Duplicate primus %s in tier %s in %s' % (primus_name, tier_name, options.src_hostcfg))
                primus_tier_shards_count[primus_name] = shards_count

        if default_primus_shards != 0:
            for primus in default_primus_set:
                if primus not in primus_tier_shards_count:
                    primus_tier_shards_count[primus] = default_primus_shards

        src_obj = dict()
        src_obj['name'] = tier_name
        src_obj['primuses'] = []
        src_obj['properties'] = {'min_replicas': 0}
        for primus in sorted(primus_tier_shards_count.keys()):
            if primus_tier_shards_count[primus] == 0:
                continue
            first = primus_shards_count[primus]
            count = primus_tier_shards_count[primus]
            src_obj['primuses'].append(
                dict(name=primus,
                     shards=list(range(first, first + count)))
            )
            primus_shards_count[primus] += count
            affected_primuses.add(primus)

        new_tier = Tier(CURDB, load_card_node_from_dict(src_obj, CURDB.tiers.TIER_SCHEME))
        if options.verbose:
            sys.stdout.write('Found tier %s with %s shards. ' % (new_tier.name, new_tier.get_shards_count()))

        if (options.tiers and tier_name not in options.tiers) or (tier_name == 'AllL'):
            if options.verbose:
                print 'Tier %s skipped.' % tier_name
            continue

        affected_tiers.add(new_tier.name)
        if CURDB.tiers.has_tier(new_tier.name):
            if new_tier == CURDB.tiers.get_tier(new_tier.name):
                if options.verbose:
                    print 'Tier already exists. Tiers are equal.'
            else:
                if options.replace:
                    if options.verbose:
                        print 'Tier already exists. Tiers are different. Tier will be replaced.'
                    if options.apply:
                        if new_tier.name in CURDB.tiers.tiers:
                            new_tier.disk_size = CURDB.tiers.tiers[new_tier.name].disk_size
                            new_tier.properties.min_replicas = CURDB.tiers.tiers[new_tier.name].properties.min_replicas
                        CURDB.tiers.set_tier(new_tier)
                else:
                    if options.verbose:
                        print 'Tier already exists. Tiers are different. Tier will not be replaced. Use --replace option to replace existing tier.'
        else:
            if options.verbose:
                print 'Tier does not exist. Tier will be added.'
            if options.apply:
                CURDB.tiers.set_tier(new_tier)

    for tier in [CURDB.tiers.get_tier(x) for x in CURDB.tiers.get_tier_names()]:
        if tier.name in affected_tiers:
            continue
        for primus in tier.primuses:
            if primus['name'] in affected_primuses:
                raise Exception('Tier %s has common primus %s with updated tiers but is not changed. This will lead to conflict.' % (tier.name, primus['name']))

    if options.verbose:
        if options.apply:
            print 'Applying changes...'
        else:
            print 'Changes are not applied. Use --apply to save changes.'
    if options.apply:
        CURDB.tiers.update()
