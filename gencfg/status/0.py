import pymongo
from collections import namedtuple

MongoStorage = namedtuple('MongoStorage', ['uri', 'replicaset', 'read_preference'])

ALL_HEARTBEAT_C_MONGODB = MongoStorage(
    uri=','.join([
        'myt0-4012.search.yandex.net:27017',
        'myt0-4019.search.yandex.net:27017',
        'sas1-6063.search.yandex.net:27017',
        'sas1-6136.search.yandex.net:27017',
        'vla1-3984.search.yandex.net:27017',
    ]),
    replicaset='heartbeat_mongodb_c',
    read_preference=pymongo.ReadPreference.SECONDARY_PREFERRED
)

db = pymongo.MongoReplicaSetClient(
            ALL_HEARTBEAT_C_MONGODB.uri,
            replicaset=ALL_HEARTBEAT_C_MONGODB.replicaset,
            secondary_acceptable_latency_ms=30000,
            connectTimeoutMS=5000,
            read_preference=ALL_HEARTBEAT_C_MONGODB.read_preference
        )['topology_commits']

ts = db.commits.find(sort=[('_id', -1), ('commit', 1)])

for c in ts:
    print c['commit'] + ' ' + ('1' if c['test_passed'] else '0')

