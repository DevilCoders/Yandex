#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from argparse import ArgumentParser

import gencfg
from core.db import DB, CURDB
import core.argparse.types as argparse_types
from config import TAG_PATTERN
from gaux.aux_repo import get_last_tag


def parse_cmd():
    parser = ArgumentParser(description="Show how many hosts have same shards in both new and old versions")
    parser.add_argument("-o", "--olddb", type=argparse_types.gencfg_db, default=None,
                        help="Optional. Path to olddb (if not specified last tag is used")
    parser.add_argument("-n", "--newdb", type=argparse_types.gencfg_db, default=CURDB,
                        help="Optional. Path to newdb (CURDB by default)")
    parser.add_argument("-i", "--new-intlookup", type=str, required=True,
                        help="Obligatory. Intlookup name")
    parser.add_argument("--old-intlookup", type=str, default=None,
                        help="Optional. Intlookup name in olddb (if differs from new db)")
    parser.add_argument("--diff-instances", action="store_true", default=False,
                        help="Optional. Check new and old db have same instances instead of checking for same shards")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def normalize(options):
    if options.olddb is None:
        options.olddb = DB("tag@%s" % get_last_tag(TAG_PATTERN, CURDB.get_repo()))

    if options.old_intlookup is None:
        options.old_intlookup = options.new_intlookup

    options.old_intlookup = options.olddb.intlookups.get_intlookup(options.old_intlookup)
    options.new_intlookup = options.newdb.intlookups.get_intlookup(options.new_intlookup)

    assert (options.old_intlookup.get_shards_count() == options.new_intlookup.get_shards_count())


def main(options):
    proximity_list = []
    for shard_id in range(options.old_intlookup.get_shards_count()):
        old_instances = options.old_intlookup.get_base_instances_for_shard(shard_id)
        new_instances = options.new_intlookup.get_base_instances_for_shard(shard_id)

        if options.diff_instances:
            old_ids = set(map(lambda x: x.name(), old_instances))
            new_ids = set(map(lambda x: x.name(), new_instances))
        else:
            old_ids = set(map(lambda x: x.host.name, old_instances))
            new_ids = set(map(lambda x: x.host.name, new_instances))

        proximity_list.append(float(len(old_ids & new_ids)) / max(len(old_ids), 1))

    return proximity_list


def print_result(result, options):
    avg_proximity = sum(result) / len(result) * 100
    if options.diff_instances:
        suffix = "instances"
    else:
        suffix = "hosts"
    print "Intlookup %s: %.2f%% %s are same in <%s> and <%s> dbs" % (
        options.new_intlookup.file_name, avg_proximity, suffix, options.olddb.PATH, options.newdb.PATH
    )


if __name__ == '__main__':
    options = parse_cmd()

    normalize(options)

    result = main(options)

    print_result(result, options)
