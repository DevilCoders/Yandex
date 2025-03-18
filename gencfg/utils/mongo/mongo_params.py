import collections

import pymongo

MongoStorage = collections.namedtuple('MongoStorage', ['uri', 'replicaset', 'read_preference'])

ALL_HEARTBEAT_A_MONGODB = MongoStorage(
    uri=':27017,'.join([
        'myt0-4007.search.yandex.net',
        'myt0-4010.search.yandex.net',
        'sas1-5874.search.yandex.net',
        'sas1-5919.search.yandex.net',
        'vla1-6003.search.yandex.net',
    ]),
    replicaset='heartbeat_mongodb_a',
    read_preference=pymongo.ReadPreference.SECONDARY_PREFERRED
)

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

ALL_HEARTBEAT_D_MONGODB = MongoStorage(
    uri=':27017,'.join([
        'myt0-4013.search.yandex.net',
        'myt0-4020.search.yandex.net',
        'sas4-4916.search.yandex.net',
        'sas4-4917.search.yandex.net',
        'vla1-6006.search.yandex.net',
    ]),
    replicaset='heartbeat_mongodb_d',
    read_preference=pymongo.ReadPreference.SECONDARY_PREFERRED
)
