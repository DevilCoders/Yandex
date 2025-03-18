#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from gaux.aux_colortext import red_text
from core.db import CURDB

def main():
    tiers_by_shardid = defaultdict(list)
    for tier in CURDB.tiers.get_tiers():
        for shard_id in xrange(0, tier.get_shards_count()):
            tiers_by_shardid[tier.get_shard_id_for_searcherlookup(shard_id)].append(tier.name)

    status = 0
    for shard_name, tier_names in tiers_by_shardid.iteritems():
        if len(tier_names) > 1:
            print red_text('Shard <{}> found in multiple tiers: {}'.format(shard_name, ','.join(tier_names)))
            status = 1

    return status


if __name__ == '__main__':
    status = main()

    sys.exit(status)

    alltiers = set(CURDB.tiers.get_tier_names())

    usedtiers = set(sum(map(lambda x: [] if x.tiers is None else x.tiers, CURDB.intlookups.get_intlookups()), []))

    unusedtiers = alltiers - usedtiers

    if len(unusedtiers) > 0:
        print red_text("Unused tiers: %s" % ','.join(unusedtiers))
        sys.exit(1)
    sys.exit(0)
