#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Add extra replicas to intlookup (from free instances)")
    parser.add_argument("-d", "--database", type=argparse_types.analyzer_database, dest="database", required=True,
                        help="Obligatory. Database with analyzer results with shard sizes included")
    parser.add_argument("-t", "--tiers", type=argparse_types.comma_list, default=None,
                        help="Optional. List of tiers to process (if not specified process all tiers")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    new_sizes = defaultdict(list)
    for v in options.database.values():
        if v['instance_database_size'] is None:
            continue
        new_sizes[v['tier']].append(v['instance_database_size'] / 1024 / 1024 / 1024)
    new_sizes = dict(
        map(lambda tier: (tier, sorted(new_sizes[tier])[int(len(new_sizes[tier]) * 0.95)]), new_sizes.keys()))

    for tiername in new_sizes.keys():
        if tiername == 'None':  # FIXME
            continue
        if options.tiers is not None and tiername not in options.tiers:
            continue

        tier = CURDB.tiers.get_tier(tiername)
        tier.disk_size = new_sizes[tiername]

    CURDB.tiers.update()


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
