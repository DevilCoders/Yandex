#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
from argparse import ArgumentParser
import pymongo
import msgpack
from collections import defaultdict

import gencfg
from gaux.aux_mongo import get_mongo_collection
import core.argparse.types as argparse_types
from core.instances import Instance
from core.igroups import IGroup
from core.argparse.parser import ArgumentParserExt

from mongopack import Serializer

TIMESTEP = 60 * 60


def _get_current_timestamps(timestamp):
    ts = 60 * 60  # FIXME: timestep defined twice: here and svn+ssh://arcadia.yandex.ru/arc/trunk/arcadia/skynet/services/heartbeat-server/src/bulldozer/plugins/instanceusage.py
    if timestamp is None:
        return [(int(time.time()) / ts) * ts, (int(time.time()) / ts) * ts - ts]
    else:
        return [(timestamp / ts) * ts]


class StatSignals(object):
    SIGNALS = [
        ('instance_cpu_usage', float),
        ('host_cpu_usage', float),
        ('host_cpu_usage_rough', float),
        ('instance_mem_usage', float),
        ('host_mem_usage', float),
        ('host_mem_usage_rough', float),
        ('instance_count', int),
        ('host_count', int),
        ('host_count_rough', int),
        ('instance_power', float),
        ('host_power', float),
        ('host_power_rough', float),
    ]

    def __init__(self, start_data=None):
        if start_data is None:
            start_data = {}
        self.data = {}
        for sname, stype in self.SIGNALS:
            self.data[sname] = stype()

        self.update(start_data)

    def update(self, other_dict):
        for sname, stype in self.SIGNALS:
            self.data[sname] += other_dict.get(sname, stype())


def get_parser():
    parser = ArgumentParserExt(description="Show some mongo statistics")
    parser.add_argument("-s", "--stats-type", type=str, required=True,
                        choices=["hist", "current"],
                        help="Obligatory. Type of statistics needed: hist or current")
    parser.add_argument("-i", "--instances", type=argparse_types.extended_instances,
                        help="Optional. List of instnaces")
    parser.add_argument("-o", "--hosts", type=argparse_types.hosts,
                        help="Optional. List of hosts (host hist result differs from instance and group results")
    parser.add_argument("-n", "--intlookups", type=argparse_types.intlookups,
                        help="Optional. List of intlookups")
    parser.add_argument("-g", "--groups", type=argparse_types.groups,
                        help="Optional. List of groups")
    parser.add_argument("-t", "--timestamp", type=argparse_types.xtimestamp, default=None,
                        help="Optional. Get data for specified timestamp (now by default)")
    parser.add_argument("--startt", type=argparse_types.xtimestamp, default=None,
                        help="Optional. Get result since specified timestamp (e.g. '14d' for date 14 days ago, '4h' for date 4 hour ago)")
    parser.add_argument("--endt", type=argparse_types.xtimestamp, default=None,
                        help="Optional. Get result since specified timestamp (e.g. '14d' for date 14 days ago, '4h' for date 4 hour ago)")
    parser.add_argument("--show-avg", action="store_true", default=False,
                        help="Optional. Show only average values")
    parser.add_argument("--show-median", action="store_true", default=False,
                        help="Optional. Show only median values")
    parser.add_argument("--show-empty", action="store_true", default=False,
                        help="Optional. Show only hosts without result (valid only in host mode with acction 'current')")

    return parser


def normalize(options):
    if options.instances is None and options.intlookups is None and \
                    options.hosts is None and options.groups is None:
        raise Exception("You must specify --instances or --groups option")

    if int(options.startt is None) + int(options.endt is None) == 1:
        raise Exception("You must specify both --startt and --endt options")

    if options.startt is None:
        options.startt = argparse_types.xtimestamp('now')
    if options.endt is None:
        options.endt = argparse_types.xtimestamp('now')

    if options.show_avg and options.show_median:
        raise Exception("You must specify at most one of --show-avg --show-median option")


def get_current_usage(options, instances, mongocoll):
    hostnames = sorted(map(lambda x: x.host.name, instances))

    mongoresult = mongocoll.find(
        {'ts': {'$in': _get_current_timestamps(options.timestamp)}, 'host': {'$in': hostnames},},
        {'host': 1, 'ts': 1, 'binary': 1, '_id': 0,},
    ).sort('ts')

    hostsresult = dict()
    for elem in mongoresult:
        hostsresult[elem['host']] = dict(
            map(lambda x: (x['port'], x), Serializer.deserialize(msgpack.unpackb(elem['binary']))))

    result = {}
    for instance in instances:
        if instance.host.name not in hostsresult:
            result[instance] = None
            continue
        if instance.port not in hostsresult[instance.host.name]:
            result[instance] = None
            continue

        result[instance] = StatSignals(hostsresult[instance.host.name][instance.port]).data

        sum_stats = hostsresult[instance.host.name][65535]
        result[instance]['host_cpu_usage'] = sum_stats['instance_cpu_usage']
        result[instance]['host_mem_usage'] = sum_stats['instance_mem_usage']
        result[instance]['host_count'] = 1
        result[instance]['host_power'] = instance.host.power

    return result


# FIXME: take into account tags
def get_group_hist_usage(options, groups, mongocoll):
    result = {}
    for group in groups:
        mongoresult = mongocoll.find(
            {'groupname': group.card.name, 'ts': {'$gte': (options.startt / TIMESTEP) * TIMESTEP,
                                                  '$lte': ((options.endt / TIMESTEP) * TIMESTEP + TIMESTEP)}},
        )
        result[group] = sorted(map(lambda x: (x['ts'], StatSignals(x).data), list(mongoresult)),
                               cmp=lambda (ts1, v1), (ts2, v2): cmp(ts1, ts2))

    return result


def get_instance_hist_usage(options, instances, mongocoll):
    instances_by_hostport = dict(map(lambda x: ((x.host.name, x.port), x), instances))

    mongoresult = mongocoll.find(
        {'ts': {'$gte': (options.startt / TIMESTEP) * TIMESTEP,
                '$lte': ((options.endt / TIMESTEP) * TIMESTEP + TIMESTEP)},
         'host': {'$in': map(lambda x: x.host.name, instances)}},
        {'host': 1, 'binary': 1, 'ts': 1, '_id': 0},
    )

    result = defaultdict(list)
    for elem in mongoresult:
        unpacked = Serializer.deserialize(msgpack.unpackb(elem['binary']))
        for instancestats in unpacked:
            iid = (elem['host'], instancestats['port'])
            if iid in instances_by_hostport:
                result[instances_by_hostport[iid]].append((elem['ts'], StatSignals(instancestats).data))
    for v in result.values():
        v.sort(cmp=(lambda (ts1, v1), (ts2, v2): cmp(ts1, ts2)))

    return result


def get_host_current_usage(options, hosts, currentusagecoll):
    result = {}
    for host in hosts:
        mongoresult = currentusagecoll.find(
            {'ts': {'$in': _get_current_timestamps(options.timestamp)}, 'host': host.name},
            {'host': 1, 'binary': 1, 'ts': 1, '_id': 0}
        ).sort([('ts', -1)])

        try:
            result[host] = Serializer.deserialize(msgpack.unpackb(mongoresult[0]['binary']))
        except:
            result[host] = []

    return result


def get_hosts_hist_usage(options, hosts, currentusagecoll, tagcachecoll):
    tagcacheresult = dict()
    for host in hosts:
        mongoresult = tagcachecoll.find(
            {'host': host.name},
            {'host': 1, 'port': 1, 'major_tag': 1, 'minor_tag': 1, 'groupname': 1, '_id': 0}
        )
        for elem in mongoresult:
            k = (elem['host'], elem['port'], elem['major_tag'], elem['minor_tag'])
            tagcacheresult[k] = elem['groupname']

    """
    Result is dict: groupname -> dict(timestamp : usage) for all groups detected on at least one host.
    Host result in group ALL_SEARCH
    """
    result = defaultdict(lambda: defaultdict(dict))
    for host in hosts:
        mongoresult = currentusagecoll.find(
            {'host': host.name, 'ts': {'$gte': (options.startt / TIMESTEP) * TIMESTEP,
                                       '$lte': ((options.endt / TIMESTEP) * TIMESTEP + TIMESTEP)}},
            {'host': 1, 'ts': 1, 'binary': 1, '_id': 0}
        )

        for elem in mongoresult:
            for instancedata in Serializer.deserialize(msgpack.unpackb(elem['binary'])):
                if instancedata['port'] == 65535:
                    groupname = 'ALL_SEARCH'
                else:
                    k = (host.name, instancedata['port'], instancedata['major_tag'], instancedata['minor_tag'])
                    if k not in tagcacheresult:
                        continue
                    groupname = tagcacheresult[k]

                ss = StatSignals(result[groupname][elem['ts']])
                ss.update(instancedata)
                result[groupname][elem['ts']] = ss.data

    for groupname in result:
        result[groupname] = sorted(result[groupname].items(), cmp=(lambda (ts1, v1), (ts2, v2): cmp(ts1, ts2)))

    return result


def main(options):
    # get list of instances
    instances = []
    if options.instances:
        instances.extend(options.instances)
    if options.intlookups:
        instances.extend(sum(map(lambda x: x.get_base_instances(), options.intlookups), []))
    if options.groups:
        instances.extend(sum(map(lambda x: x.get_instances(), options.groups), []))

    # all mongo data located in two different mongos: big tables instanceusage and instanceusagetagcache in separate mongo
    currentusagecoll = get_mongo_collection('instanceusage')
    histusagecoll = get_mongo_collection('instanceusagegraphs')
    tagcachecoll = get_mongo_collection('instanceusagetagcache')

    if options.stats_type == "current":
        if options.instances is not None or options.groups is not None:
            return get_current_usage(options, instances, currentusagecoll)
        if options.hosts is not None:
            return get_host_current_usage(options, options.hosts, currentusagecoll)
    elif options.stats_type == "hist":
        result = {}
        if options.instances is not None:
            result.update(get_instance_hist_usage(options, options.instances, currentusagecoll))
        if options.hosts is not None:
            result.update(get_hosts_hist_usage(options, options.hosts, currentusagecoll, tagcachecoll))
        if options.groups is not None:
            result.update(get_group_hist_usage(options, options.groups, histusagecoll))
        return result


def _print_by_timestamp(data, indent='    '):
    for timestamp, d in data:
        print "%s%s: %s" % (indent, time.strftime("%Y-%m-%d %H:%M", time.localtime(timestamp)), d)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    normalize(options)
    result = main(options)

    if options.stats_type == "current":
        if options.hosts is not None:
            for host in options.hosts:
                if options.show_empty and len(result[host]) > 0:
                    continue

                print "%s:" % host.name
                for elem in result[host]:
                    print "    %s" % elem
        else:
            if options.show_avg:
                rawdata = filter(lambda x: x is not None, result.values())

                sumsignal = StatSignals()
                for elem in rawdata:
                    sumsignal.update(elem)
                for key in sumsignal.data:
                    sumsignal.data[key] /= len(rawdata)

                print "Average result: %s (%d instances)" % (sumsignal.data, len(rawdata))
            elif options.show_median:
                rawdata = sorted(filter(lambda x: x is not None, result.values()))

                medsignal = defaultdict([])
                for elem in rowdata:
                    for key in elem:
                        medsignal.append(elem[key])
                for elem in medsignal:
                    medsignal[elem].sort()
                    medsignal[elem] = medsignal[elem][len(rawdata) / 2]

                print "Median result: %s (%d instances)" % (medsignal, len(rawdata))
            else:
                for instance in result:
                    print "Instance %s: %s" % (instance.name(), result[instance])
    elif options.stats_type == "hist":
        for key in result:
            if isinstance(key, Instance):
                print "Instance %s: %s" % (key.name(), result[key])
            elif isinstance(key, IGroup):
                print "Group %s" % key.name
                for ts, data in result[key]:
                    print "    %s: %s" % (time.strftime("%Y-%m-%d %H:%M", time.localtime(ts)), data)
            else:
                print "Something %s" % key
                _print_by_timestamp(result[key])
