#!/skynet/python/bin/python

import os
import sys
import pymongo

sys.path.append(os.path.abspath('.'))

import gencfg
import common
from core.db import CURDB
import mongo_params


def get_slbs_collection():
    return pymongo.MongoReplicaSetClient(
        mongo_params.ALL_HEARTBEAT_C_MONGODB.uri,
        connectTimeoutMS=5000,
        replicaSet=mongo_params.ALL_HEARTBEAT_C_MONGODB.replicaset,
        w='3',
        wtimeout=60000,
        read_preference=mongo_params.ALL_HEARTBEAT_C_MONGODB.read_preference
    )['topology_commits']['slbs']


def get_slbs():
    slbs = CURDB.slbs.get_all()
    slbs.sort(key=lambda x: x.fqdn)
    result = [dict(fqdn=x.fqdn, ips=x.ips) for x in slbs]
    return dict(slbs=result)


def export_to_mongo(slbs, commit):
    if 'slbs' not in slbs:
        return
    slbs_collection = get_slbs_collection()

    bulk_op = slbs_collection.initialize_unordered_bulk_op()
    for slb_data in slbs['slbs']:
        bulk_op.find({'fqdn': slb_data['fqdn'], 'commit': commit}).upsert().replace_one({
            'fqdn': slb_data['fqdn'],
            'ips': slb_data['ips'],
            'commit': commit
        })
    bulk_op.execute()


if __name__ == '__main__':
    slbs = get_slbs()
    export_to_mongo(slbs, int(common.get_current_commit('./')))
