# coding: utf-8

from __future__ import unicode_literals

import logging
import time
from functools import partial

from pymongo import MongoClient, ReadPreference
from pymongo import errors as pymongo_errors

# There is no need to start a ReplicaSetClient for each cache backend - they
# can all reuse one, providing it is the same backend.
CONNECTIONS_CACHE = {
    # hosts_or_uri: MongoReplicaSetClient
}

logger = logging.getLogger(__name__)


default_connection_options = (
    ('socketTimeoutMS', 0.4 * 1000),  # 400 ms
    ('connectTimeoutMS', 0.4 * 1000),  # 400 ms
    ('serverSelectionTimeoutMS', 0.6 * 1000),  # 600 ms
    ('tz_aware', True),
)


RECONNECT_ERROR_MESSAGE = 'Lost primary while using mongodb: "%s"'


def connection_nearest(hosts_or_uri, replica_set, **options):
    """
    Create MongoReplicaSetClient using strategy optimized for read from
    nearest replicaSet member.
    """
    for opt, default_value in default_connection_options:
        options[opt] = options.get(opt) or default_value
    return MongoClient(
        host=hosts_or_uri,
        maxPoolSize=1,
        # use_greenlets=True,
        localThresholdMS=0.5,
        # .5 ms is what we want
        replicaSet=replica_set,
        read_preference=ReadPreference.NEAREST,
        **options
    )


def connection_primary(hosts_or_uri, replica_set, **options):
    """
    Return MongoReplicaSetClient that uses primary member only for read and
    write.
    """
    for opt, default_value in default_connection_options:
        options[opt] = options.get(opt) or default_value
    return MongoClient(
        host=hosts_or_uri,
        maxPoolSize=1,
        # use_greenlets=True,
        replicaSet=replica_set,
        read_preference=ReadPreference.PRIMARY,
        **options
    )


def get_connection(location, replica_set, strategy):

    cachestr = "%s#%s" % (location, strategy)

    if cachestr in CONNECTIONS_CACHE:
        return CONNECTIONS_CACHE[cachestr]

    if strategy == 'NEAREST':
        conn = connection_nearest(location, replica_set=replica_set)
    elif strategy == 'PRIMARY':
        conn = connection_primary(location, replica_set=replica_set)

    CONNECTIONS_CACHE[cachestr] = conn
    return conn


def ensure_indexes(collection, timeout):
    """
    Ensure index is on collection and set ttl for collection records

    @type collection: Collection
    @type timeout: int
    @param timeout: collection ttl, seconds
    """

    collection.create_index(
        'status', expireAfterSeconds=timeout
    )
    collection.create_index(
        'key'
    )

    logger.debug('Set TTL index for %s equal to %s seconds', collection, timeout)


class LoggingCollection(object):

    """
    Логгирующая обертка над pymongo.collection.Collection

    Умеет отлавливать AutoReconnect и пробовать послать запрос снова.
    """

    def __init__(self, collection, logger):
        self._collection = collection
        self.logger = logger

    def _call_method(self, method, *args, **kwargs):
        self.logger.debug(
            'mongo method called: %s, args: %s, kwargs: %s',
            method.__name__, args, kwargs
        )
        start_time = time.time()

        tries = 3
        for i in range(tries):  # три попытки на AutoReconnect;
            try:
                result = method(*args, **kwargs)
                break
            except pymongo_errors.AutoReconnect as exc:
                self.logger.info(RECONNECT_ERROR_MESSAGE, repr(exc))
                if i + 1 == tries:
                    raise
            except Exception as e:
                self.logger.exception(
                    'mongo method "%s" produced an error "%s"',
                    method.__name__, repr(e)
                )
                raise

        runtime = time.time() - start_time
        self.logger.debug(
            'mongo method "%s" finished in %s seconds',
            method.__name__, runtime
        )
        return result

    def __getattr__(self, item):
        self.logger.debug('mongo attribute called: %s', item)
        attribute = getattr(self._collection, item)
        if callable(attribute):
            return partial(self._call_method, attribute)
        return attribute

    def __repr__(self):
        return repr(self._collection)


class MongoDBWrapper(object):
    hosts = None
    _connection = None

    def __init__(self, database_name, hosts=None, replica_set=None, strategy='NEAREST'):
        assert strategy in ('NEAREST', 'PRIMARY')
        self.hosts = hosts
        self.strategy = strategy
        self.replica_set = replica_set
        self.database_name = database_name

    @property
    def connection(self):
        if not self._connection:
            self._connection = get_connection(self.hosts, self.replica_set, self.strategy)
        return self._connection

    @property
    def database(self):
        return getattr(self.connection, self.database_name)
