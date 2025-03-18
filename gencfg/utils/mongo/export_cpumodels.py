#!/skynet/python/bin/python

import os
import sys
import pymongo

sys.path.append(os.path.abspath('.'))

import collections
import gencfg
import common
from core.db import CURDB
import mongo_params

def get_cpumodels_collection():
    return pymongo.MongoReplicaSetClient(
        mongo_params.ALL_HEARTBEAT_C_MONGODB.uri,
        connectTimeoutMS=5000,
        replicaSet=mongo_params.ALL_HEARTBEAT_C_MONGODB.replicaset,
        w='3',
        wtimeout=60000,
        read_preference=mongo_params.ALL_HEARTBEAT_C_MONGODB.read_preference
    )['topology_commits']['cpumodels']


def get_cpumodels():
    models = {}
    for model, value in CURDB.cpumodels.models.iteritems():
        value_dict = dict(value.as_dict())
        models[model] = {
            "power": value_dict["power"],
            "ncpu": value_dict["ncpu"]
        }
    return dict(models=models, botmodel_to_model=CURDB.cpumodels.botmodel_to_model)

def export_to_mongo(cpumodels):
    if 'models' not in cpumodels or 'botmodel_to_model' not in cpumodels:
        return
    cpumodels_collection = get_cpumodels_collection()

    bulk_op = cpumodels_collection.initialize_unordered_bulk_op()
    bulk_op.find({}).upsert().replace_one({
        'models': cpumodels['models'],
        'botmodel_to_model': cpumodels['botmodel_to_model']
    })
    bulk_op.execute()

def main():
    cpumodels = get_cpumodels()
    export_to_mongo(cpumodels)

if __name__ == '__main__':
    main()
