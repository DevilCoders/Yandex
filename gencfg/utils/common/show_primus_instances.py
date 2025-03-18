#!/usr/bin/env python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
from argparse import ArgumentParser
from core.db import CURDB
from collections import defaultdict


def parse_cmd():
    parser = ArgumentParser(description="Import oxygen shards")

    parser.add_argument("-i", "--intlookups", dest="intlookups", type=str, required=True,
                        help="Obligatory. Comma separated intlookups")
    parser.add_argument("-p", "--primus-prefix", dest="primus_prefix", type=str,
                        help="Optional. Primus prefix")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    options.db = CURDB

    assert options.intlookups
    options.intlookups = options.intlookups.split(',')

    return options


def main(opts):
    primus_instances = defaultdict(list)

    for intlookup in [opts.db.intlookups.get_intlookup(x) for x in opts.intlookups]:
        if intlookup.tiers is None:
            print 'Intlookup %s skipped - no tiers given' % intlookup.file_name
        base_id = 0
        for tier in [opts.db.tiers.get_tier(x) for x in intlookup.tiers]:
            for shard_id in range(tier.get_shards_count()):
                instances = intlookup.get_base_instances_for_shard(base_id + shard_id)
                primus_instances[tier.get_shard_id(shard_id)].extend(instances)
            base_id += tier.get_shards_count()

    for primus in sorted(primus_instances):
        if opts.primus_prefix and not primus.startswith(opts.primus_prefix):
            continue
        print '%s: %s' % (primus, ' '.join('%s:%s' % (i.host.name, i.port) for i in primus_instances[primus]))


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
