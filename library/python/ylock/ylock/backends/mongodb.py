# coding: utf-8

from __future__ import absolute_import
from collections import namedtuple
import datetime
import logging
import time

import pymongo
try:
    from pymongo import Connection
except ImportError:
    from pymongo import MongoClient as Connection

from ylock.base import BaseManager, BaseLock


log = logging.getLogger(__name__)


class Manager(BaseManager):
    default_timeout = 30
    default_sleep_interval = 2
    default_retry_count = 3

    def __init__(self, *args, **kwargs):
        self.retry_count = kwargs.pop('retry_count', self.default_retry_count)
        self.sleep_interval = kwargs.pop('sleep_interval', self.default_sleep_interval)

        self.db_name = kwargs.pop('db_name', 'ylock')
        self.collection_name = kwargs.pop('collection_name', 'ylock_locks')
        self.username = kwargs.pop('username', None)
        self.password = kwargs.pop('password', None)
        self.mechanism = kwargs.pop('mechanism', 'MONGODB-CR')

        super(Manager, self).__init__(*args, **kwargs)

        self._collection = None

    def _init(self):
        hosts = self.hosts

        if hosts and isinstance(hosts, (list, tuple)):
            hosts = ','.join(hosts)

        connection = Connection(hosts)
        db = connection[self.db_name]
        if self.username and self.password:
            db.authenticate(self.username, self.password, mechanism=self.mechanism)

        self._collection = db[self.collection_name]

    @property
    def collection(self):
        if self._collection is None:
            self._init()

        return self._collection

    def acquire_lock(self, name, timeout):
        now = datetime.datetime.utcnow()
        then = now + datetime.timedelta(seconds=timeout)

        full_name = self.get_full_name(name)

        try:
            self.collection.find_and_modify({'_id': full_name, 'time': {'$lt': now}},
                                            {'_id': full_name, 'time': then},
                                            upsert=True)
        except pymongo.errors.OperationFailure:
            # Считаем что конфликт по _id и значит такой актуальный лок уже есть
            return False

        return True

    def check_acquired(self, name):
        now = datetime.datetime.utcnow()
        full_name = self.get_full_name(name)

        query = self.collection.find({'_id': full_name, 'time': {'$gte': now}})
        return query.count() > 0

    def release_lock(self, name):
        self.collection.remove({'_id': self.get_full_name(name)})

        return True

    def lock(self, name, timeout=None, block=None, block_timeout=None, **kwargs):
        return MongoDBLock(self, name, timeout, block, block_timeout)

    def lock_from_context(self, context):
        return MongoDBLock.create_from_context(self, context)


MongoDBLockContext = namedtuple('MongoDBLockContext', ('name', 'timeout', 'block', 'block_timeout'))


class MongoDBLock(BaseLock):
    @classmethod
    def create_from_context(cls, manager, context):
        return cls(manager, context.name, context.timeout, context.block, context.block_timeout)

    def acquire(self):
        if self.timeout is None:
            timeout = self.manager.default_timeout
        else:
            timeout = self.timeout

        if self.block_timeout is None:
            block_timeout = self.manager.default_block_timeout
        else:
            block_timeout = self.block_timeout
        if block_timeout > 0:
            block_deadline = time.time() + block_timeout
        else:
            block_deadline = None

        retry_count = self.manager.retry_count

        while True:
            if block_deadline is not None and time.time() > block_deadline:
                log.info('Giving up attempts to take lock because of block_timeout')
                break
            try:
                result = self.manager.acquire_lock(self.name, timeout)
            except Exception:
                retry_count -= 1

                if retry_count <= 0:
                    break

                time.sleep(self.manager.sleep_interval)
            else:
                if not result and self.block:
                    time.sleep(self.manager.sleep_interval)
                    continue

                return result

        return False

    def release(self):
        return self.manager.release_lock(self.name)

    def get_context(self):
        return MongoDBLockContext(
            name=self.name,
            timeout=self.timeout,
            block=self.block,
            block_timeout=self.block_timeout,
        )

    def check_acquired(self):
        return self.manager.check_acquired(self.name)
