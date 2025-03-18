#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB
from gaux.aux_colortext import red_text

from gaux.aux_decorators import gcdisable

@gcdisable
def main():
    hosts_by_tier_shard = defaultdict(set)

    for intlookup in CURDB.intlookups.get_intlookups():
        if intlookup.tiers is None:
            continue
        for shard_id in range(intlookup.get_shards_count()):
            instances = intlookup.get_base_instances_for_shard(shard_id)
            hosts_by_tier_shard[intlookup.get_tier_and_shard_id_for_shard(shard_id)] |= set(
                map(lambda x: x.host, instances))

    for group in CURDB.groups.get_groups():
        if group.card.searcherlookup_postactions.custom_tier.enabled:
            hosts_by_tier_shard[(group.card.searcherlookup_postactions.custom_tier.tier_name, 0)] |= set(
                group.getHosts())

    retcode = 0
    for tier in CURDB.tiers.get_tiers():
        for shard_id in range(tier.get_shards_count()):
            lfound = hosts_by_tier_shard[(tier.name, shard_id)]
            if len(lfound) < tier.properties['min_replicas']:
                print red_text("Shard %d of tier %s found only on %d hosts (%d needed): %s" % (
                    shard_id, tier.name, len(lfound), tier.properties['min_replicas'],
                    " ".join(map(lambda x: x.name, lfound))))
                retcode = 1

    # =============================== GENCFG-1713 START =========================================
    for tier_name, replicas_count in [('MailTier0', 4), ('DiskTier0', 4)]:
        tier = CURDB.tiers.get_tier(tier_name)
        intlookups = [x for x in CURDB.intlookups.get_intlookups() if x.tiers == [tier.name]]
        intlookups = [x for x in intlookups if x.file_name != 'SAS_SAS_MAILSEARCH_TEST8GB2']
        for shard_id in xrange(tier.get_shards_count()):
            instances = [intlookup.get_base_instances_for_shard(shard_id) for intlookup in intlookups]
            instances = sum(instances, [])
            if len(instances) != replicas_count:
                print red_text('Tier <{}>, shard <{}> has <{}> replicas (exactly <{}> required): {}'.format(
                                   tier.name, shard_id, len(instances), replicas_count, ' '.join(x.full_name() for x in instances)))
                retcode = 1
    # =============================== GENCFG-1713 FINISH ========================================

    sys.exit(retcode)

if __name__ == '__main__':
    main()
