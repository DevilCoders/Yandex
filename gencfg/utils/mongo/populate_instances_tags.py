#!/skynet/python/bin/python

import pymongo
import logging
import argparse

import os
import sys
sys.path.append(os.path.abspath('.'))

import gencfg
import core
from core.db import CURDB

from mongo_params import ALL_HEARTBEAT_D_MONGODB
import common


_db = pymongo.MongoReplicaSetClient(
    ALL_HEARTBEAT_D_MONGODB.uri,
    connectTimeoutMS=5000,
    replicaSet=ALL_HEARTBEAT_D_MONGODB.replicaset,
    w='3',
    wtimeout=60000,
    read_preference=ALL_HEARTBEAT_D_MONGODB.read_preference,
)['topology_commits']

collection = _db['instances_tags2']


def get_instances_with_tags(host):
    result = {}
    hostgroups = CURDB.groups.get_host_groups(host)
    for hostgroup in hostgroups:
        generated_slookup = hostgroup.generate_searcherlookup()
        for instance in hostgroup.get_host_instances(host):
            if instance in generated_slookup.instances:
                result[instance.name()] = sorted(generated_slookup.instances[instance])
    return result


def parse_args():
    parser = argparse.ArgumentParser()
    # parser.add_argument('--dry-run', action='store_true', default=False)
    return parser.parse_args()


def main():
    BULK_SIZE = 1000

    commit = int(common.get_current_commit('./'))
    logging.info('populating commit {}'.format(commit))

    hosts = CURDB.hosts.get_all_hosts()
    # hosts = CURDB.groups.get_group('VLA_CLUSTERSTATE').getHosts()
    for index, host in enumerate(hosts):
        if index % BULK_SIZE == 0:
            if index > 0:
                bulk_op.execute()
            bulk_op = collection.initialize_unordered_bulk_op()

        data = get_instances_with_tags(host.name)
        logging.info('{} host {}: {} instances'.format(index, host.name, len(data)))
        bulk_op.insert({'host': host.name.replace('.', '!'),
                        'data': {key.replace('.', '!'): value for key, value in data.iteritems()},
                        'commit': commit})

    if index % BULK_SIZE > 0:
        bulk_op.execute()


if __name__ == '__main__':
    logging.basicConfig(format='[%(asctime)s] %(message)s', level=logging.DEBUG)
    main()
