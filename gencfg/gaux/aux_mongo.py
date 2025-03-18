"""
    Aux functions to work with mongo
"""

from core.settings import SETTINGS

MONGO_CLIENTS_STORAGE = dict()


def get_heartbeat_mongo(mongo_instances, replica_set='heartbeat', timeout=10000):
    # try to load cached client from mongo db storage (as mongo is threadsave we do not need to create extra object in every thread)
    mongo_client_attr = "mongo_client_%s" % hash(frozenset((mongo_instances, replica_set, timeout)))
    mongo_client = MONGO_CLIENTS_STORAGE.get(mongo_client_attr, None)

    if mongo_client is None:
        import pymongo
        mongo_options = {
            'replicaSet': replica_set,
            'read_preference': pymongo.ReadPreference.PRIMARY,
            'socketTimeoutMS': timeout,
            'connectTimeoutMS': 600,
            'waitQueueTimeoutMS': 1000,
        }
        mongo_client = pymongo.MongoReplicaSetClient(mongo_instances, **mongo_options)
        MONGO_CLIENTS_STORAGE[mongo_client_attr] = mongo_client

    return mongo_client


def get_mongo_collection(colname, timeout=10000):
    """
        Function return collection from proper mongo base, based on collection name and internal mapping

        :type colname: str
        :type timeout: int

        :param colname: name of collection
        :param timeout: socket timeout for pymongo
        :return (pymongo.MongoReplicaSetClient): collection with name colname
    """

    MAPPING = {
        'instanceusage': (
            SETTINGS.services.mongo.instanceusage.instances,
            SETTINGS.services.mongo.instanceusage.replicaset,
            SETTINGS.services.mongo.instanceusage.db
        ),
        'instanceusagetagcache': (
            SETTINGS.services.mongo.instanceusage.instances,
            SETTINGS.services.mongo.instanceusage.replicaset,
            SETTINGS.services.mongo.instanceusage.db
        ),
        'commits': (
            SETTINGS.services.mongo.cache.instances,
            SETTINGS.services.mongo.cache.replicaset,
            SETTINGS.services.mongo.cache.db
        ),
        'gencfg_dns': (
            SETTINGS.services.mongo.cache.instances,
            SETTINGS.services.mongo.cache.replicaset,
            SETTINGS.services.mongo.cache.db
        ),
        'gencfgcommits': (
            SETTINGS.services.mongo.cache.instances,
            SETTINGS.services.mongo.cache.replicaset,
            SETTINGS.services.mongo.cache.db
        ),
        'gencfgapiwbecache': (
            SETTINGS.services.mongo.cache.instances,
            SETTINGS.services.mongo.cache.replicaset,
            SETTINGS.services.mongo.cache.db
        ),
        # table with some state data for statefull applications, which can be run on different hosts at different time
        'gencfgstate': (
            SETTINGS.services.mongo.cache.instances,
            SETTINGS.services.mongo.cache.replicaset,
            SETTINGS.services.mongo.cache.db
        ),
        'instanceusageunused': (
            SETTINGS.services.mongo.heartbeat.instances,
            SETTINGS.services.mongo.heartbeat.replicaset,
            SETTINGS.services.mongo.heartbeat.db
        ),
        'histdb': (
            SETTINGS.services.mongo.heartbeat.instances,
            SETTINGS.services.mongo.heartbeat.replicaset,
            SETTINGS.services.mongo.heartbeat.db
        ),
        'instanceusagegraphs': (
            SETTINGS.services.mongo.heartbeat.instances,
            SETTINGS.services.mongo.heartbeat.replicaset,
            SETTINGS.services.mongo.heartbeat.db
        ),
        'instancestatev3': (
            SETTINGS.services.mongo.heartbeat.instances,
            SETTINGS.services.mongo.heartbeat.replicaset,
            SETTINGS.services.mongo.heartbeat.db
        ),
        'hwdata': (
            SETTINGS.services.mongo.heartbeat.instances,
            SETTINGS.services.mongo.heartbeat.replicaset,
            SETTINGS.services.mongo.heartbeat.db
        ),
        'counters': (
            SETTINGS.services.mongo.cache.instances,
            SETTINGS.services.mongo.cache.replicaset,
            SETTINGS.services.mongo.cache.db
        ),
    }

    if colname in MAPPING:
        mongo_instances, replica_set, db_name = MAPPING[colname]
    else:
        raise Exception("Could not find mongo db for collection <%s>" % colname)

    mongoclient = get_heartbeat_mongo(mongo_instances=mongo_instances, replica_set=replica_set, timeout=timeout)
    mongodb = mongoclient[db_name]
    mongocoll = mongodb[colname]

    return mongocoll


def get_last_verified_commit():
    import pymongo

    mongocoll = get_mongo_collection("commits")
    commit = mongocoll.find(
        {"test_passed": True},  # filter
        {"commit": 1, "_id": 0},  # fields to get
    ).sort("commit", pymongo.DESCENDING)[0]

    return int(commit["commit"])


def get_next_hbf_project_id(hbf_range_name=None):
    """
        NOCDEF-479. We have range of hbf projct ids and need to assign every new group unique project id from this range.
        We do this by storing last_used_project_id in mongo and receiving via unique_fetch_and_add
    """

    mongocoll = get_mongo_collection("counters")

    while True:
        collection_id = 'hbf_project_id'
        if hbf_range_name is not None and hbf_range_name != '_GENCFG_SEARCHPRODNETS_ROOT_':
            collection_id = hbf_range_name

        result = mongocoll.find_and_modify(
            {'_id': collection_id},
            {'$inc': {'seq': 1}},
        )
        project_id = int(result['seq'])

        if project_id > int(result['max']):
            raise ValueError('Generated project id out of range ({} > {})'.format(project_id, int(result['max'])))

        if (project_id + 1) % 256 != 0:  # hbf project is a part of ipv6 addr and its last octet can not be 0xFF due to EUI-64 collision
            return project_id
