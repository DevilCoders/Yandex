#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import time
import datetime
import msgpack
from collections import defaultdict

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from gaux.aux_mongo import get_mongo_collection
from gaux.aux_utils import unicode_to_utf8


def get_parser():
    parser = ArgumentParserExt(description="Show percentage of hosts/instances with statistics")
    parser.add_argument("-a", "--action", type=str, required=True,
                        choices=["show_brief", "show_per_host"],
                        help="Obligatory. Statistics type")
    parser.add_argument("-g", "--groups", type=argparse_types.groups, default=[],
                        help="Optional. Comma-separated group list")
    parser.add_argument("-s", "--hosts", type=argparse_types.hosts, default=[],
                        help="Optional. Comma-separated list of hosts (only for action show_per_host)")
    parser.add_argument("-f", "--group-filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Group filter")
    parser.add_argument("--host-filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Host filter")
    parser.add_argument("--filter-fake", action="store_true", default=False,
                        help="Optional. Filter fake/unraisable groups")
    parser.add_argument("-t", "--timestamp", type=argparse_types.xtimestamp, default=int(time.time()) - 3 * 60 * 60,
                        help="Optional. Show hosts found alive since timestamp")

    return parser


def normalize(options):
    if options.action == "show_brief" and len(options.hosts) > 0:
        raise Exception("Options <--show-brief> and <--hosts> are incompatable")
    if options.action == "show_brief" and len(options.groups) == 0:
        raise Exception("You should specify at least one group with <--groups> option (for action <show_brief>)")


def load_instancestate_data(hosts, timestamp):
    mongocoll = get_mongo_collection('instancestatev3')

    mongocursor = mongocoll.find(
        {
            'host': {'$in': map(lambda x: x.name, hosts),},
            'last_update': {'$gt': datetime.datetime.fromtimestamp(timestamp)}
        },
        {'host': 1, '_id': 0, 'state': 1, 'last_update': 1}
    )
    all_mongo_instances = dict()
    all_mongo_hosts = set()
    for elem in mongocursor:
        elem = unicode_to_utf8(elem)

        all_mongo_hosts.add(elem['host'])

        for instance, instance_value in unicode_to_utf8(msgpack.loads(elem['state'])['i']).iteritems():
            fixed_instance = "%s:%s" % (elem['host'], instance.partition(':')[2])
            all_mongo_instances[fixed_instance] = instance_value

    return all_mongo_hosts, all_mongo_instances


def main(options):
    groups = options.groups
    if options.filter_fake:
        groups = filter(lambda x: x.properties.unraisable_group is False and x.properties.fake_group is False,
                        options.groups)
    if options.group_filter:
        groups = filter(options.group_filter, groups)

    all_hosts = list(set(sum(map(lambda x: x.getHosts(), groups), options.hosts)))
    if options.host_filter:
        all_hosts = filter(options.host_filter, all_hosts)

    all_mongo_hosts, all_mongo_instances = load_instancestate_data(all_hosts, options.timestamp)

    if options.action == 'show_brief':
        raised_instances = set(
            map(lambda (x, y): x, filter(lambda (x, y): y.get('a', 0) == 1, all_mongo_instances.iteritems())))

        result = []
        for group in groups:
            group_instances = set(map(lambda x: x.name(), group.get_kinda_busy_instances()))
            down_instances = group_instances - raised_instances
            result.append((group, len(group_instances), len(down_instances), list(down_instances)))
    elif options.action == 'show_per_host':
        result = defaultdict(list)

        for instance, instance_data in all_mongo_instances.iteritems():
            host = instance.partition(':')[0]
            result[host].append((instance, instance_data))

        for host in all_mongo_hosts:
            if host not in result:
                result[host] = []

        for host in map(lambda x: x.name, all_hosts):
            if host not in result:
                result[host] = None

    else:
        raise Exception("Unknown action %s" % options.action)

    return result


def print_result(result):
    if options.action == 'show_brief':
        for group, total_instances, down_instances, example_instances in result:
            example_instances = example_instances[:20]
            if len(example_instances):
                example_instances_str = ", examples: %s" % " ".join(example_instances)
            else:
                example_instances_str = ""

            print "Group %s, %d/%d (%.2f%%) down%s" % (
            group.card.name, down_instances, total_instances, 100 * float(down_instances) / max(total_instances, 1),
            example_instances_str)
    elif options.action == 'show_per_host':
        for host in sorted(result.keys()):
            print "Host %s:" % host
            for instance, instance_data in result[host]:
                print "    Instance: %s, props: %s" % (instance, instance_data)
    else:
        raise Exception("Unknown action %s" % options.action)


if __name__ == '__main__':
    options = get_parser().parse_cmd()
    normalize(options)
    result = main(options)
    print_result(result)
