#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..')))

from argparse import ArgumentParser

import gencfg
from core.db import CURDB
import core.argparse.types as argparse_types


def parse_cmd():
    parser = ArgumentParser(description="Vertical split intlookup into 2 or more")
    parser.add_argument("-i", "--orig-intlookup", dest="orig_intlookup", type=argparse_types.intlookup, required=True,
                        help="Obligatory. Input intlookup")
    parser.add_argument("-o", "--new-intlookup", dest="new_intlookup", type=str, required=True,
                        help="Obligatory. Output intlookup name")
    parser.add_argument("-t", "--tiers", dest="tiers", type=str, required=True,
                        help="Obligatory. Tiers in new intlookup")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    options.tiers = options.tiers.split(',')

    for tier in options.tiers:
        if tier not in options.orig_intlookup.tiers:
            raise Exception("No tier %s in intlookup %s" % (tier, options.orig_intlookup.file_name))

    if CURDB.intlookups.has_intlookup(options.new_intlookup):
        raise Exception("Intlookup %s already exists")

    return options


def main(options):
    new_intlookup = CURDB.intlookups.create_empty_intlookup(options.new_intlookup)
    new_intlookup.tiers = options.tiers
    new_intlookup.hosts_per_group = options.orig_intlookup.hosts_per_group
    new_intlookup.base_type = options.orig_intlookup.base_type
    new_intlookup.brigade_groups = map(lambda (x, y): y, filter(
        lambda (i, group): options.orig_intlookup.get_tier_for_shard(
            i * options.orig_intlookup.hosts_per_group) in options.tiers,
        enumerate(options.orig_intlookup.brigade_groups)))
    new_intlookup.brigade_groups_count = len(new_intlookup.brigade_groups)

    options.orig_intlookup.brigade_groups = map(lambda (x, y): y, filter(
        lambda (i, group): options.orig_intlookup.get_tier_for_shard(
            i * options.orig_intlookup.hosts_per_group) not in options.tiers,
        enumerate(options.orig_intlookup.brigade_groups)))
    options.orig_intlookup.brigade_groups_count = len(options.orig_intlookup.brigade_groups)

    CURDB.update()


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
