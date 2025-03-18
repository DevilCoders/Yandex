#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
import pymongo
from argparse import ArgumentParser
from collections import defaultdict
import math

import gencfg
from gaux.aux_mongo import get_mongo_collection
import core.argparse.types as argparse_types
from core.db import CURDB


def parse_cmd():
    parser = ArgumentParser(description="Generate report on all (master?) groups")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, required=True,
                        help="Obligatory. Comma-separated list of groups to make report")
    parser.add_argument("-f", "--filter", type=argparse_types.pythonlambda,
                        help="Optional. Extra group filter")
    parser.add_argument("-t", "--timestamp", type=argparse_types.xtimestamp, default=int(time.time()),
                        help="Optional. Statistics on specific timestamp. Examples: <3d> - 3 days ago, <5h> - 5 hours ago, <2014-10-07 22:00> - at specified timestamp")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    return options


def load_graphs_usage(mongocoll, groups, timestamp):
    mongoresult = mongocoll.find(
        {'groupname': {'$in': map(lambda x: x.card.name, groups)},
         'ts': {'$gt': timestamp - 60 * 60 * 2, '$lt': timestamp + 60 * 60 * 2},},
    ).sort([('ts', 1)])

    result = defaultdict(list)
    for elem in mongoresult:
        result[elem['groupname']].append(elem)

    for groupname in result:
        result[groupname].sort(cmp=lambda x, y: cmp(math.fabs(x['ts'] - timestamp), math.fabs(y['ts'] - timestamp)))
        result[groupname] = result[groupname][0]

    return result


def main(options):
    if options.filter is not None:
        options.groups = filter(options.filter, options.groups)
    options.groups = filter(lambda x: x.card.properties.fake_group == False, options.groups)

    graphscoll = get_mongo_collection('instanceusagegraphs')

    usage_by_graphs = load_graphs_usage(graphscoll, options.groups, options.timestamp)

    nodata = set(map(lambda x: x.card.name, options.groups)) - set(usage_by_graphs.keys())

    result = {}
    for group in filter(lambda x: x.card.name not in nodata, options.groups):
        group_usage = usage_by_graphs[group.card.name]

        # make average values
        result[group.card.name] = {
            'instance_cpu_usage': round(
                group_usage['instance_cpu_usage'] / float(max(group_usage['instance_power'], 1.0)) * 100., 2),
            'host_cpu_usage': round(group_usage['host_cpu_usage'] / float(max(group_usage['host_power'], 1.0)) * 100.,
                                    2),
            'host_cpu_usage_rough': round(
                group_usage['host_cpu_usage_rough'] / float(max(group_usage['host_power_rough'], 1.0)) * 100., 2),
            'instance_mem_usage': round(group_usage['instance_mem_usage'] / max(group_usage['instance_count'], 1), 2),
            'host_mem_usage': round(group_usage['host_mem_usage_rough'] / max(group_usage['host_count_rough'], 1), 2),
            'instance_power': round(group_usage['instance_power'], 2),
            'host_power': round(group_usage['host_power_rough'], 2),
            'instance_count': group_usage['instance_count'],
            'host_count': group_usage['host_count_rough'],
            'ts': group_usage['ts'],
        }

    return nodata, result


if __name__ == '__main__':
    options = parse_cmd()
    nodata, result = main(options)

    if len(nodata):
        print "No data for groups: %s" % (','.join(nodata))
    for gname in sorted(result.keys()):
        group = CURDB.groups.get_group(gname)
        group_power = sum(map(lambda x: x.power, group.get_kinda_busy_instances()))
        group_icount = len(group.get_kinda_busy_instances())
        group_hcount = len(group.getHosts())

        ts_as_str = time.strftime("%Y-%m-%d %H:%M", time.localtime(result[gname]['ts']))
        print "Group %s: %.2f avg cpu, %.2f avg mem, %.2f(%.2f) power, %d(%d) icount, %d(%d) hcount (calculated at timestamp %s)" % (
            gname,
            result[gname]['instance_cpu_usage'],
            result[gname]['instance_mem_usage'],
            result[gname]['instance_power'], group_power,
            result[gname]['instance_count'], group_icount,
            result[gname]['host_count'], group_hcount,
            ts_as_str,
        )
