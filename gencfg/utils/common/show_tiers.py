#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import re
from argparse import ArgumentParser

import gencfg
import core.argparse.types as argparse_types
from core.db import CURDB

FIELDS = {
    'shards': lambda tier: tier.get_shards_count(),
    'disk': lambda tier: tier.disk_size,
}


def show_tiers_fields(tiers, fields):
    result = []
    for tier in tiers:
        result.append("%s: %s" % (tier.name, " ".join(map(lambda x: "%s -> %s" % (x, FIELDS[x](tier)), fields))))
    return "\n".join(result)


def parse_cmd():
    parser = ArgumentParser(description="Show tiers with shard count (and primuses)")
    parser.add_argument("-p", "--pattern", dest="patterns", type=str, action='append',
                        help="Obligatory. Regexp on tier name")
    parser.add_argument("-i", "--intlookups", dest="intlookups", type=argparse_types.intlookups,
                        help="Optional. Comma-separated list of intlookups")
    parser.add_argument("-g", "--groups", dest="groups", type=argparse_types.groups,
                        help="Optional. Comma-seaprated list of groups")
    parser.add_argument("-f", "--field", dest="fields", type=str, action='append',
                        choices=sorted(FIELDS.keys()),
                        help="Obligatory. List of fields to show")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if int(options.patterns is not None) + int(options.intlookups is not None) + int(options.groups is not None) != 1:
        raise Exception("You must specify exactly one of --patterns, --intlookups, --groups option")
    if len(options.fields) == 0:
        raise Exception("You must specify at least one field to show")

    if options.groups:
        options.intlookups = list(set(sum(map(lambda x: x.intlookups, options.groups), [])))
    if options.intlookups:
        options.patterns = list(
            set(sum(map(lambda x: '^%s$' % CURDB.intlookups.get_intlookup(x).tiers, options.intlookups), [])))
    options.patterns = map(lambda x: re.compile(x), options.patterns)

    return options


def main(options):
    tiers = CURDB.tiers.get_tiers()
    tiers = filter(
        lambda tier: True in map(lambda pattern: re.search(pattern, tier.name) is not None, options.patterns), tiers)
    tiers.sort(cmp=lambda x, y: cmp(x.name, y.name))

    print show_tiers_fields(tiers, options.fields)


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
