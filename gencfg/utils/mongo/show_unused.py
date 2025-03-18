#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gencfg

import pymongo
import time
from argparse import ArgumentParser
from collections import defaultdict

import core.argparse.types as argparse_types
from gaux.aux_mongo import get_mongo_collection
from core.db import CURDB


def parse_cmd():
    parser = ArgumentParser(description="Show list of unused machines")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, required=True,
                        help="Obligatory. List of groups to process")
    parser.add_argument("-u", "--unused-time", type=float, default=14.,
                        help="Optional. Show hosts, which was unused last <N> days (14 days by default)")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda,
                        help="Optional. Filter groups")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.groups is None and options.hosts is None:
        raise Exception("You should specify at least one of --groups , --hosts option")

    return options


def main(options, db=CURDB):
    if options.filter:
        options.groups = filter(options.filter, options.groups)
    options.groups = filter(
        lambda x: x.card.properties.fake_group == False and x.card.properties.background_group == False, options.groups)

    groups_by_host = defaultdict(list)
    hosts = []
    for group in options.groups:
        for host in group.getHosts():
            hosts.append(host)
            groups_by_host[host].append(group)

    unused_ts = int(time.time()) - options.unused_time * 24 * 60 * 60

    mongocoll = get_mongo_collection('instanceusageunused')

    mongoresult = list(mongocoll.find(
        {'host': {'$in': map(lambda x: x.name, hosts)}, 'last_update': {'$gte': unused_ts}},
        {'host': 1, 'unused_since': 1, '_id': 0}
    ))
    mongoresult = map(lambda x: (x['host'], x['unused_since']), mongoresult)

    result = {
        'nodata': [],
        'havedata': defaultdict(list)
    }

    mongohosts = set()
    for hostname, ts in mongoresult:
        host = db.hosts.get_host_by_name(hostname)
        mongohosts.add(host)
        if ts < unused_ts:
            for group in groups_by_host[host]:
                result['havedata'][group].append((host, ts))

    result['nodata'] = list(set(hosts) - set(mongohosts))
    result['nodata'] = filter(lambda x: x.is_vm_guest() == False, result['nodata'])
    for group in result['havedata']:
        result['havedata'][group].sort(cmp=lambda (x1, y1), (x2, y2): cmp(y1, y2))

    return result


if __name__ == '__main__':
    options = parse_cmd()
    result = main(options)

    print "Hosts not found in mongo: %s" % (', '.join(map(lambda x: x.name, result['nodata'])))
    for group in result['havedata']:
        print "Group %s has %d unused hosts (owners %s)" % (
        group.card.name, len(result['havedata'][group]), ",".join(group.card.owners))
        for host, unused_time in result['havedata'][group]:
            print "    Host %s unused %.2f days" % (host.name, (time.time() - unused_time) / 24 / 60 / 60)
