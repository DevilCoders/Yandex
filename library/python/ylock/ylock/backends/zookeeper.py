# coding: utf-8

from __future__ import absolute_import

from datetime import datetime
from calendar import timegm
import logging
import os
import sys
import json
from collections import namedtuple

from ylock.base import BaseManager, BaseLock

from kazoo.client import KazooClient
from kazoo.recipe.lock import Lock as KazooLock
from kazoo.exceptions import (
    KazooException, BadVersionError, NoNodeError, NotEmptyError,
)


log = logging.getLogger(__name__)


class BackendConnectionError(Exception):
    pass


class Manager(BaseManager):
    '''
    Zookeeper lock manager.
    Usage:

    >>> from ylock import create_manager

    >>> # Blocking lock scenario
    >>> mgr = create_manager('kazoo')
    >>> with mgr.lock('lock_name'):
    ...     do_stuff_after_lock_is_aquired()
    ...

    >>> # Non-blocking lock scenario
    >>> with mgr.lock('lock_name', block=False) as acquired:
    ...     if acquired:
    ...         do_stuff_only_if_lock_is_acquired()
    '''

    default_connect_timeout = 10
    default_timeout = 0

    def __init__(self, *args, **kwargs):
        self.connect_timeout = kwargs.pop(
            'connect_timeout', self.default_connect_timeout
        )
        self.allow_ro_connect = kwargs.pop('allow_ro_connect', None)

        super(Manager, self).__init__(*args, **kwargs)

        self._client = None
        self._lock_count = 0

    def get_hosts(self):
        return ','.join(super(Manager, self).get_hosts())

    def close(self):
        if self._client is None:
            return

        try:
            self._client.stop()
        except Exception as e:
            log.warning('Stopped KazooClient improperly: %s', repr(e))

        try:
            self._client.close()
        except Exception as e:
            log.warning('Closed KazooClient improperly: %s', repr(e))

        self._client = None

    @property
    def client(self):
        if self._client is not None:
            return self._client

        self._client = KazooClient(
            hosts=self.get_hosts(),
            read_only=self.allow_ro_connect,
        )

        try:
            self._client.start(self.connect_timeout)

        except self._client.handler.timeout_exception as e:
            log.exception('Timeout when connecting to ZooKeeper: %s', repr(e))

            self._client = None

            raise BackendConnectionError(e)

        except KazooException as e:
            log.exception('Failed to communicate with ZooKeeper: %s', repr(e))

            self._client = None

            raise BackendConnectionError(e)

        return self._client

    def client_release(self):
        self._lock_count -= 1

        if self._lock_count <= 0:
            self.close()

            self._lock_count = 0

    # Публичный интерфейс

    def lock(self, name, timeout=None, block=None, block_timeout=None, **kwargs):
        name = self.get_full_name(name, '/')

        if timeout is None:
            timeout = self.default_timeout
        if block_timeout is None:
            block_timeout = self.default_block_timeout

        has_lifetime = not block and timeout > 0

        if has_lifetime:
            lock = LockImplWithTTL(name, self, timeout)
        else:
            client_data = json.dumps({
                'created': datetime.now().isoformat(),
                'host': os.uname()[1],
                'pid': os.getpid(),
                'ppid': os.getppid(),
            })
            lock = KazooLockWrapper(
                self.client, name, client_data,
                block_timeout=block_timeout
            )

        self._lock_count += 1

        return ZookeeperLock(
            self, name, timeout, block, block_timeout, impl=lock
        )

    def lock_from_context(self, context):
        return LockImplWithTTL.create_from_context(self, context)

    # Прокси-методы в клиент

    def set(self, *args, **kwargs):
        return self.client.set(*args, **kwargs)

    def get(self, *args, **kwargs):
        return self.client.get(*args, **kwargs)

    def delete(self, *args, **kwargs):
        return self.client.delete(*args, **kwargs)

    def ensure_path(self, *args, **kwargs):
        return self.client.ensure_path(*args, **kwargs)


LockImplWithTTLContext = namedtuple(
    'LockImplWithTTLContext', ('name', 'timeout', 'stat')
)


class LockImplWithTTL(object):
    """
    Zookeeper lock with lifetime.
    """

    def __init__(self, name, manager, timeout, stat=None):
        self.name = name
        self.manager = manager
        self.timeout = timeout

        self._stat = stat

    @classmethod
    def create_from_context(cls, manager, context):
        return cls(context.name, manager, context.timeout, context.stat)

    def acquire(self, blocking):
        assert(not blocking)

        if self._stat is not None:
            raise RuntimeError('Lock already acquired: %s' % self.name)

        self.manager.ensure_path(self.name)

        value, stat = self.manager.get(self.name)

        try:
            value = int(value)
        except ValueError:
            # Assume this is empty string or some unrecoverable error accured.
            value = 0

        version = stat.version
        now = timegm(datetime.utcnow().timetuple())

        _acquired = False

        if value < now:
            try:
                value = str(now + self.timeout)
                if sys.version_info > (3, 0):
                    value = value.encode("utf-8")
                self._stat = self.manager.set(
                    self.name, value, version
                )
            except BadVersionError:
                # Too late, lock is not acquired
                pass
            else:
                _acquired = True

        return _acquired

    def release(self):
        if self._stat is None:
            return False

        try:
            self.manager.delete(self.name, self._stat.version)
        except BadVersionError:
            # Его уже взял кто-то другой
            return False

        self._stat = None

        return True

    def get_context(self):
        return LockImplWithTTLContext(name=self.name, timeout=self.timeout,
                                      stat=self._stat)

    def contenders(self):
        try:
            value, stat = self.manager.get(self.name)
        except NoNodeError:
            return []

        try:
            value = int(value)
        except ValueError:
            value = 0

        now = timegm(datetime.utcnow().timetuple())
        if now <= value:
            return [value]

        return []


class KazooLockWrapper(KazooLock):
    def __init__(self, *args, **kwargs):
        self.block_timeout = kwargs.pop('block_timeout') or None
        super(KazooLockWrapper, self).__init__(*args, **kwargs)

    def acquire(self, blocking=True):
        return super(KazooLockWrapper, self).acquire(
            blocking, timeout=self.block_timeout
        )

    def _inner_acquire(self, blocking, timeout, **kwargs):
        while True:
            try:
                return super(KazooLockWrapper, self)._inner_acquire(
                    blocking, timeout
                )
            except NoNodeError:
                # из-за реализации _inner_release в этом классе KazooLockWrapper
                # возможна ситуация когда есть два клиента:
                # А взял лок и уже отпускает его, удалив self.path,
                # Б только выполнил client.ensure_path и приступает к client.create,
                # без бесконечного цикла здесь Б упадет с NoNodeError,
                # поэтому мы делаем повторную попытку.
                continue

    def _inner_release(self):
        result = super(KazooLockWrapper, self)._inner_release()
        try:
            self.client.delete(self.path)
        except NotEmptyError:
            # Бросается если родительская нода self.path содержит эфемерные ноды.
            # А давно взял лок и пытается его отпустить. Б успел создать свою
            # эфемерную ноду. А отпуская лок удаляет за собой эфемерную ноду.
            # Потом пытается удалить родительскую ноду self.path. Он не сможет ее
            # удалить, потому что там находится эфемерная нода Б. Это штатная
            # ситуация и мы оставляем ноду self.path чтобы потом ее удалил Б
            # за собой.
            pass
        return result


class ZookeeperLock(BaseLock):
    def __init__(self, *args, **kwargs):
        self._impl = kwargs.pop('impl')

        super(ZookeeperLock, self).__init__(*args, **kwargs)

        self._acquired = False

    def acquire(self):
        try:
            self._acquired = self._impl.acquire(blocking=self.block)
        except KazooException:
            log.exception('Failed to get lock `%s`', self.name)

        return self._acquired

    def release(self):
        if self._acquired:
            try:
                self._impl.release()
                self._acquired = False
            except KazooException:
                log.exception('Error when releasing lock')

            self.manager.client_release()

        return not self._acquired

    def get_context(self):
        try:
            return self._impl.get_context()
        except AttributeError:
            pass

        return None

    def check_acquired(self):
        return len(self._impl.contenders()) > 0

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.release()
