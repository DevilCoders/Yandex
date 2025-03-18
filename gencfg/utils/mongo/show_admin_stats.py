#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
import pymongo
from argparse import ArgumentParser
import msgpack
from collections import defaultdict

import gencfg
import core.argparse.types as argparse_types
from gaux.aux_mongo import get_mongo_collection

from mongopack import Serializer

HOUR = 60 * 60


def parse_cmd():
    parser = ArgumentParser(description="Show percentage of hosts/instances with statistics")
    parser.add_argument("-s", "--stats-type", type=str, required=True,
                        choices=["hist", "current"],
                        help="Obligatory. Statistics type: hist or current")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, required=True,
                        help="Obligatory. Comma-separated group list")
    parser.add_argument("-t", "--timestamp", type=argparse_types.xtimestamp, default=int(time.time()),
                        help="Optional. Show statistics at specified timestamp (for current stats) or since this timestamp (for hist stats)")
    parser.add_argument("--sum-stats", action="store_true", default=False,
                        help="Optional. Show sum stats")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda,
                        help="Optional. Filter on groups")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    if options.filter is not None:
        options.groups = filter(options.filter, options.groups)
    options.groups = filter(lambda x: x.card.properties.fake_group == False, options.groups)

    options.timestamp = ((options.timestamp - 2 * HOUR) / HOUR) * HOUR

    mongocoll = get_mongo_collection('instanceusagegraphs')

    result = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))
    for group in options.groups:
        if options.sum_stats:
            gname = 'TOTAL'
        else:
            gname = group.card.name

        if options.stats_type == 'current':
            mongocursor = mongocoll.find({'ts': options.timestamp, 'groupname': group.card.name})
        else:
            mongocursor = mongocoll.find({'ts': {'$gte': options.timestamp}, 'groupname': group.card.name})

        for elem in mongocursor:
            result[gname][elem['ts']]['host_count_mongo'] += elem.get('host_count_rough', 0)
            result[gname][elem['ts']]['instance_count_mongo'] += elem.get('instance_count', 0)
            result[gname][elem['ts']]['host_count'] += elem.get('host_count_gencfg', 0)
            result[gname][elem['ts']]['instance_count'] += elem.get('instance_count_gencfg', 0)

    return result


if __name__ == '__main__':
    options = parse_cmd()
    result = main(options)

    for groupname in sorted(result.keys()):
        print "Group %s:" % groupname
        for ts in result[groupname]:
            d = result[groupname][ts]
            print "    Timestamp %s: %s/%s instances (mongo/total), %s/%s hosts(mongo/total)" % (
                time.strftime("%Y-%m-%d %H:%M", time.localtime(ts)), d['instance_count_mongo'],
                d['instance_count'], d['host_count_mongo'], d['host_count'])
