#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict

import gencfg
from core.db import CURDB


def main():
    failures = []

    instances_by_host = defaultdict(list)
    for intlookup in CURDB.intlookups.get_intlookups():
        if intlookup.tiers is None:
            continue
        for shard_id in range(intlookup.get_shards_count()):
            tier, tier_shard_id = intlookup.get_tier_and_shard_id_for_shard(shard_id)
            for instance in intlookup.get_base_instances_for_shard(shard_id):
                instances_by_host[instance.host.name].append((instance.type, tier, tier_shard_id))

    for host, hostdata in instances_by_host.iteritems():
        PF = '_HAMSTER'

        all_groups = map(lambda (tp, tier, shard_id): tp, hostdata)

        check_hamster_groups = filter(lambda x: x.endswith(PF), all_groups)
        check_hamster_groups = filter(lambda x: x[:-len(PF)] in all_groups, check_hamster_groups)

        for tp, tier, shard_id in hostdata:
            if tp not in check_hamster_groups:
                continue
            if (tp[:-len(PF)], tier, shard_id) not in hostdata:
                failures.append(
                    "Host %s has hamster shard %s:%s:%s which is not found in main group" % (host, tp, tier, shard_id))

    return failures


if __name__ == '__main__':
    failures = main()

    if len(failures):
        print "\n".join(failures)
        sys.exit(1)

    sys.exit(0)
