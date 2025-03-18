#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))
import gencfg

import pymongo
import msgpack
import base64
from argparse import ArgumentParser
import time
import shutil
import multiprocessing
from collections import defaultdict
import hashlib

import gencfg
from core.db import CURDB, DB
from gaux.aux_mongo import get_mongo_collection
import core.argparse.types as argparse_types
from core.svnapi import SvnRepository, SvnError

from mongopack import Serializer


def parse_cmd():
    parser = ArgumentParser(description="Update group statistics")
    parser.add_argument("-g", "--groupnames", type=str, required=True,
                        help="Obligatory. Comma-separated list of groups (ALL to update all groups)")
    parser.add_argument("-l", "--last-seconds", type=int, default=60 * 60 * 5,
                        help="Optional. Time since when we update statistics")

    if len(sys.argv) == 1:
        sys.argv.append('-h')
    options = parser.parse_args()

    if options.groupnames == 'ALL':
        options.groupnames = map(lambda x: x.card.name, CURDB.groups.get_groups())
    else:
        options.groupnames = options.groupnames.split(',')

    return options


def load_usage_result(usagecoll):
    result = usagecoll.find(
        {'ts': {'$gte': int(time.time()) - options.last_seconds}},
        {'host': 1, 'ts': 1, 'binary': 1, '_id': 0,},
    )
    result = list(result)
    result = map(lambda x: (x['host'], x['ts'], Serializer.deserialize(msgpack.unpackb(x['binary']))), result)

    return result


def get_used_tags(instanceusage):
    tags = defaultdict(set)
    for host, ts, data in instanceusage:
        for elem in data:
            tags[(elem['major_tag'], elem['minor_tag'])].add(host)

    return tags


def load_group_by_instance(tagcachecoll, instanceusage):
    # get tag dict
    tags = get_used_tags(instanceusage)
    print '\n'.join(map(lambda (x, y): 'stable-%s-r%s' % (x, y), tags))

    # generate set of all instances we intereseted in
    result_keys = []
    for host, ts, instancesdata in instanceusage:
        for elem in instancesdata:
            result_keys.append((elem['major_tag'], elem['minor_tag'], host, elem['port']))
    result_keys = set(result_keys)

    result = {}
    for major_tag, minor_tag in sorted(tags.keys()):
        if (major_tag, minor_tag) == (0, 0):
            continue

        tag_hosts = list(tags[(major_tag, minor_tag)])

        print "Processing tag stable-%s-r%s (%s hosts)" % (major_tag, minor_tag, len(tag_hosts))

        tagresult = tagcachecoll.find(
            {'major_tag': major_tag, 'minor_tag': minor_tag, 'host': {'$in': tag_hosts}},
        )
        tagresult = list(tagresult)

        if len(tagresult) == 0 and len(tag_hosts) > 20:  # some insurance against tag not found in tagcache
            raise Exception("Can not find tag stable-%s-r%s in tagcache" % (major_tag, minor_tag))
        for elem in tagresult:
            key = (elem['major_tag'], elem['minor_tag'], elem['host'], elem['port'])
            if key in result_keys:
                result[key] = elem

        print "Size of result %s" % len(result)

    """
        Add data on instances with major_tag == 0 and minor_tag == 0. We do not know, when configuration with with tag was created,
        so suppose it match current trunk
    """
    for instance in CURDB.groups.get_all_instances():
        key = (0, 0, instance.host.name, instance.port)
        if key in result_keys:
            result[(0, 0, instance.host.name, instance.port)] = {
                'groupname': instance.type,
                'host': instance.host.name,
                'hostpower': instance.host.power,
                'instancepower': instance.power,
                'major_tag': 0,
                'minor_tag': 0,
                'port': instance.port,
            }

    return result


def zero_stats():
    return {
        'instance_cpu_usage': 0.0,
        'host_cpu_usage': 0.0,
        'host_cpu_usage_rough': 0.0,
        'instance_mem_usage': 0.0,
        'host_mem_usage': 0.0,
        'host_mem_usage_rough': 0.0,
        'instance_count': 0,  # number of current group instance according to mongo stats
        'host_count': 0,  # number of hosts with current group instances according to mongo stats
        'host_count_rough': 0,
    # number of hosts with any statistics (not necessarily current group) on current group hosts
        'host_count_gencfg': 0,  # number of current group hosts in gencfg
        'instance_power': 0.0,
        'instance_count_gencfg': 0,  # number of current group instances in gencfg
        'host_power': 0.0,
        'host_power_rough': 0.0,
    }


def calculate_statistics(instanceusage, instanceparams, options):
    result = defaultdict(dict)

    foundts = set()

    for host, ts, instancesdata in instanceusage:
        foundts.add(ts)

        # get fake instance corresponding to all host data
        try:
            hostdata = filter(lambda x: x['port'] == 65535, instancesdata)[0]
        except IndexError:  # It happens rarely when host cpu usage detection failed (1 of 100000000. Just skip such elements
            continue

        # fill rough host stats
        if CURDB.hosts.has_host(host):  # check if we still have this host in db
            curhost = CURDB.hosts.get_host_by_name(host)
            current_host_groups = CURDB.groups.get_host_groups(curhost)
            for group in current_host_groups:
                if group.card.name not in options.groupnames:
                    continue

                if ts not in result[group.card.name]:
                    result[group.card.name][ts] = zero_stats()

                hostpower = CURDB.hosts.get_host_by_name(host).power
                result[group.card.name][ts]['host_cpu_usage_rough'] += hostdata['instance_cpu_usage'] * hostpower
                result[group.card.name][ts]['host_mem_usage_rough'] += hostdata['instance_mem_usage']
                result[group.card.name][ts]['host_count_rough'] += 1
                result[group.card.name][ts]['host_power_rough'] += hostpower

        # go though all instances and add statistics to corresponding groups
        groupsonhost = set()
        for instancedata in instancesdata:
            if instancedata['port'] == 65535:  # fake instance with host statistics
                continue

            port = instancedata['port']
            major_tag = instancedata['major_tag']
            minor_tag = instancedata['minor_tag']

            if (major_tag, minor_tag, host, port) not in instanceparams:
                continue

            params = instanceparams[(major_tag, minor_tag, host, port)]

            groupname = params['groupname']
            hostpower = params['hostpower']
            instancepower = params['instancepower']

            if groupname not in options.groupnames:
                continue

            if ts not in result[groupname]:
                result[groupname][ts] = zero_stats()

            result[groupname][ts]['instance_cpu_usage'] += instancedata['instance_cpu_usage'] * hostpower
            result[groupname][ts]['instance_mem_usage'] += instancedata['instance_mem_usage']
            result[groupname][ts]['instance_count'] += 1
            result[groupname][ts]['instance_power'] += instancepower

            if groupname not in groupsonhost:
                groupsonhost.add(groupname)
                result[groupname][ts]['host_cpu_usage'] += hostdata['instance_cpu_usage'] * hostpower
                result[groupname][ts]['host_mem_usage'] += hostdata['instance_mem_usage']
                result[groupname][ts]['host_count'] += 1
                result[groupname][ts]['host_power'] += hostpower

    # fill statistics on groups without any instances
    for groupname in options.groupnames:
        for ts in foundts:
            if ts not in result[groupname]:
                result[groupname][ts] = zero_stats()

    # fill current gencfg stats
    for groupname in options.groupnames:
        if groupname in options.groupnames:
            group = CURDB.groups.get_group(groupname)
            host_count = len(group.getHosts())
            instance_count = len(group.get_kinda_busy_instances())
        else:
            host_count = 0
            instance_count = 0

        for ts in foundts:
            result[groupname][ts]['host_count_gencfg'] = host_count
            result[groupname][ts]['instance_count_gencfg'] = instance_count

    return result


def main(options):
    usagecoll = get_mongo_collection('instanceusage', timeout=100000)
    tagcachecoll = get_mongo_collection('instanceusagetagcache', timeout=100000)
    graphscoll = get_mongo_collection('instanceusagegraphs', timeout=100000)

    # load instance usage for all instances
    instanceusage = load_usage_result(usagecoll)

    # get mapping (major_tag, minor_tag, host, port) -> {groupname and other stuff}
    groupbyinstance = load_group_by_instance(tagcachecoll, instanceusage)

    # calculate all statistics
    result = calculate_statistics(instanceusage, groupbyinstance, options)

    for groupname in result:
        for ts in result[groupname]:
            graphscoll.remove(
                {'groupname': groupname, 'ts': ts,},
                justOne=True,
            )

            d = {'groupname': groupname, 'ts': ts}
            d.update(result[groupname][ts])
            graphscoll.insert(d)

    return result


if __name__ == '__main__':
    options = parse_cmd()
    main(options)
