#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
import pymongo
import time
from argparse import ArgumentParser
from mongopack import Serializer
import msgpack

import gencfg
from gaux.aux_mongo import get_mongo_collection
import core.argparse.types as argparse_types

MIN_USAGE = 0.05


def parse_cmd():
    parser = ArgumentParser(description="Update statistics on unused machines")
    parser.add_argument("-s", "--hosts", type=argparse_types.hosts, required=True,
                        help="Obligatory. Comma-separated list of hosts")
    parser.add_argument("-t", "--timestamp", type=int, default=int(time.time()) - 3 * 24 * 60 * 60,
                        help="Optional. Time since when update (2 weeks ago by default)")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def main(options):
    now = int(time.time())

    # all mongo data located in two different mongos: big tables instanceusage and instanceusagetagcache in separate mongo
    usagecoll = get_mongo_collection('instanceusage', timeout=100000)
    unusedcoll = get_mongo_collection('instanceusageunused')

    mongoresult = usagecoll.find(
        {'ts': {'$gte': options.timestamp}, 'host': {'$in': map(lambda x: x.name, options.hosts)},},
        {'host': 1, 'ts': 1, 'binary': 1, '_id': 0,}
    )

    usage_by_host = defaultdict(list)
    for elem in mongoresult:
        usage = Serializer.deserialize(msgpack.unpackb(elem['binary']))

        try:
            usage = filter(lambda x: x['port'] == 65535, usage)[0]['instance_cpu_usage']
        except IndexError:  # It happens rarely when host cpu usage detection failed (1 of 100000000. Just skip such elements
            continue

        usage_by_host[elem['host']].append((elem['ts'], usage))

    for hostname in usage_by_host:
        dt = sorted(usage_by_host[hostname], cmp=lambda (x1, y1), (x2, y2): cmp(x2, x1))

        # calculate since when host was unuused according to instanceusage
        last_unused = None
        for i in range(len(dt)):
            # transform usage (to smooth random bursts)
            starti = max(0, i - 5)
            endi = min(len(dt), i + 5)

            ts = dt[i][0]
            usage = sum(map(lambda (x, y): y, dt[starti:endi])) / (endi - starti)

            if usage < MIN_USAGE:
                last_unused = ts
            else:
                break
        if not last_unused:
            last_unused = int(time.time())

        # calculate since when host was unused according to instanceusageunused
        last_unused_in_db = map(lambda x: x['unused_since'],
                                unusedcoll.find({'host': hostname}, {'unused_since': 1, '_id': 0}))
        if len(last_unused_in_db) == 0:
            last_unused_in_db = options.timestamp
        else:
            last_unused_in_db = last_unused_in_db[0]

        if last_unused == dt[-1][0]:
            new_last_unused = last_unused_in_db
        else:
            new_last_unused = last_unused

        unusedcoll.remove({'host': hostname}, justOne=True)
        unusedcoll.insert({'host': hostname, 'unused_since': new_last_unused, 'last_update': now})


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
