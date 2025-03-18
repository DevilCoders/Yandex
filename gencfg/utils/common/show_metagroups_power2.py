#!/skynet/python/bin/python

import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

from collections import defaultdict
import msgpack

import gencfg
from core.argparse.parser import ArgumentParserExt
import core.argparse.types as argparse_types
from core.db import CURDB

from gaux.aux_mongo import get_mongo_collection
from gaux.aux_multithread import run_in_multiprocess

from utils.mongo.mongopack import Serializer

import utils.blinov2.main

MONGO_STATS_INTERVAL = 3 * 24 * 60 * 60
MONGO_MEDIAN = 0.75


def _median(data, key, median_value):
    return sorted(map(lambda x: x[key], data))[int(len(data) * median_value)]


def _calculate_used(hosts, common_params):
    result = {'used_cpu': 0.0, 'used_mem': 0.0, 'used_hosts': 0}

    hosts_by_name = dict(map(lambda x: (x.name, x), hosts))

    mongocoll = get_mongo_collection('instanceusage')

    mongo_flt = {'ts': {'$gt': common_params['start_ts'], '$lt': common_params['end_ts']},
                 'host': {'$in': hosts_by_name.keys()}}
    mongo_proj = {'host': 1, 'binary': 1, '_id': 0}

    by_instance_mongo_result = defaultdict(list)
    for elem in mongocoll.find(mongo_flt, mongo_proj):
        unpacked = Serializer.deserialize(msgpack.unpackb(elem['binary']))
        for instance_stat in unpacked:
            by_instance_mongo_result[(elem['host'], instance_stat['port'])].append(instance_stat)

    for hostname, port in by_instance_mongo_result:
        if port == 65535:
            result['used_cpu'] += _median(by_instance_mongo_result[(hostname, port)], 'instance_cpu_usage',
                                          MONGO_MEDIAN) * hosts_by_name[hostname].power
        else:
            result['used_mem'] += _median(by_instance_mongo_result[(hostname, port)], 'instance_mem_usage',
                                          MONGO_MEDIAN)
    result['used_hosts'] = len(set(map(lambda (x, y): x, by_instance_mongo_result.iterkeys())))

    return result


class MetaProject(object):
    def __init__(self, db, yaml_tree):
        self.db = db
        self.descr = yaml_tree['descr']
        self.filters = yaml_tree['filters']

        self.groups = []

        self.stats = {'assigned_cpu': 0.0, 'assigned_mem': 0, 'assigned_hosts': 0, 'used_cpu': 0.0, 'used_mem': 0,
                      'used_hosts': 0}

    def calculate_group_names(self):
        blinov_options = {
            'result_type': 'result',
            'transport': 'gg:%s' % self.db.PATH,
        }

        for flt in self.filters:
            blinov_options['formula'] = flt

            parsed_options = utils.blinov2.main.get_parser().parse_json(blinov_options)
            utils.blinov2.main.normalize(parsed_options)

            blinov_result = utils.blinov2.main.main(parsed_options)

            already_found = set(self.groups) - set(blinov_result)
            if len(already_found) > 0:
                raise Exception("Groups %s already in metaprj %s, when applying filter <%s>" % (','.join(already_found), self.name, flt))

            self.groups.extend(blinov_result)

        self.groups = map(lambda x: self.db.groups.get_group(x), self.groups)
        self.groups = filter(lambda x: x.master is None, self.groups)
        # self.groups = filter(lambda x: x.master is None and x.properties.fake_group == False and x.properties.background_group == False, self.groups)

        # print ",".join(map(lambda x: x.name, self.groups))

    def calculate_assigned(self, host_filter):
        for group in self.groups:
            for host in filter(host_filter, group.getHosts()):
                self.stats['assigned_cpu'] += host.power
                self.stats['assigned_mem'] += host.memory
                self.stats['assigned_hosts'] += 1

    def calculate_used(self, host_filter):
        all_hosts = sum(map(lambda x: x.getHosts(), self.groups), [])
        all_hosts = filter(host_filter, all_hosts)

        start_ts = self.db.get_repo().get_last_commit().date
        end_ts = start_ts + MONGO_STATS_INTERVAL

        workers = int(os.sysconf('SC_NPROCESSORS_ONLN'))
        for hosts, subresult, status in run_in_multiprocess(_calculate_used,
                                                            [all_hosts[i::workers] for i in xrange(workers)],
                                                            {'start_ts': start_ts, 'end_ts': end_ts}, workers=workers,
                                                            show_progress=False):
            assert (status is None), "Status of calculate_used worker is %s" % status
            self.stats['used_cpu'] += subresult['used_cpu']
            self.stats['used_mem'] += subresult['used_mem']
            self.stats['used_hosts'] += subresult['used_hosts']


def get_parser():
    parser = ArgumentParserExt(description="Show metagroups power (new version)")
    parser.add_argument("-c", "--config", type=argparse_types.yamlconfig, required=True,
                        help="Obligatory. Path to config")
    parser.add_argument("-d", "--db", type=argparse_types.gencfg_db, default=CURDB,
                        help="Optional. Gencfg db path (curdb used if omitted)")
    parser.add_argument("-f", "--host-filter", type=argparse_types.pythonlambda, default=lambda x: True,
                        help="Optional. Host filter")
    parser.add_argument("--calculate-used", action="store_true", default=False,
                        help="Optional. Calculate used statistics")

    return parser


def normalize(options):
    del options


def main(options, from_cmd=True):
    metaprojects = []
    for elem in options.config:
        metaprojects.append(MetaProject(options.db, elem))

    for i in range(len(metaprojects)):
        for j in range(i + 1, len(metaprojects)):
            intersection = set(metaprojects[i].groups) & set(metaprojects[j].groups)
            if len(intersection) > 0:
                raise Exception("Metaprojects <%s> and <%s> have non-empty intersection: %s" % (metaprojects[i].descr, metaprojects[j].descr, map(lambda x: x.name, intersection)))

    map(lambda x: x.calculate_group_names(), metaprojects)
    map(lambda x: x.calculate_assigned(options.host_filter), metaprojects)

    if options.calculate_used:
        map(lambda x: x.calculate_used(options.host_filter), metaprojects)

    if from_cmd:
        for metaproject in metaprojects:
            print "%s %s" % (metaproject.descr, metaproject.stats)

    return metaprojects


if __name__ == '__main__':
    options = get_parser().parse_cmd()

    normalize(options)

    main(options)
