#!/skynet/python/bin/python
import os
import sys

sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..')))

import gencfg
import pymongo
from core.db import CURDB


MONGO_SETTINGS = {
    'uri': ','.join([
        'myt0-4012.search.yandex.net:27017',
        'myt0-4019.search.yandex.net:27017',
        'sas1-6063.search.yandex.net:27017',
        'sas1-6136.search.yandex.net:27017',
        'vla1-3984.search.yandex.net:27017',
    ]),
    'replicaset': 'heartbeat_mongodb_c',
    'read_preference': pymongo.ReadPreference.PRIMARY
}


def get_mongo_db(mongo_db):
    collection = pymongo.MongoClient(
        MONGO_SETTINGS['uri'],
        connectTimeoutMS=5000,
        replicaSet=MONGO_SETTINGS['replicaset'],
        w='majority',
        wtimeout=15000,
        read_preference=MONGO_SETTINGS['read_preference']
    )[mongo_db]
    return collection


def get_groups_by_hosts():
    groups_by_hosts = {}
    for host in CURDB.hosts.get_hosts():
        groups_by_hosts[host.name] = sorted(x.card.name for x in CURDB.groups.get_host_groups(host))

    return groups_by_hosts


def export_groups_by_hosts_to_mongo():
    db = get_mongo_db('hosts_data')
    groups_by_hosts = get_groups_by_hosts()

    hosts_count = len(groups_by_hosts)
    persent_size = hosts_count / 100

    db.hosts_info.remove({})
    for i, (hostname, groups) in enumerate(groups_by_hosts.iteritems()):
        db.hosts_info.insert({'hostname': hostname, 'groups': groups})

        if i % persent_size == 0:
            print '{}% '.format(i / persent_size),


def main():
    export_groups_by_hosts_to_mongo()


if __name__ == '__main__':
    main()
    print('Finished.')
