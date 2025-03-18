# coding: utf-8
from __future__ import absolute_import
import logging
import random
import socket
import time

import yt.wrapper as yw

from multiprocessing import current_process
from ylock.base import BaseManager, BaseLock


logger = logging.getLogger(__name__)


INITIAL_POLLING_INTERVAL = 1.0  # seconds
MAX_POLLING_INTERVAL = 16.0  # seconds
POLLING_BACKOFF = 1.5
DEFAULT_BLOCK_TIMEOUT = 60 * 15  # seconds
DEFAULT_PROXY = 'locke'
DEFAULT_DIRECTORY = 'tmp'
FQDN = socket.getfqdn()


class YTTimeout(Exception):
    pass


class Manager(BaseManager):
    """
    YT lock manager.
    """

    def __init__(self, *args, **kwargs):
        self.proxy = kwargs.pop('proxy', DEFAULT_PROXY)
        self.token = kwargs.pop('token')

        super(Manager, self).__init__(*args, **kwargs)

        if not self.prefix:
            self.prefix = DEFAULT_DIRECTORY

    def lock(self, name, timeout=None, block=None, block_timeout=None, common_lock_name=False, **kwargs):
        """
        @param name: name of lock
        @param timeout: lifetime of a lock
        @param block: blocking mode flag
        @param block_timeout: waiting time to receive the lock in the blocking mode
        @param common_lock_name: lock name is used regularly and therefore the node with the same name
        will not be deleted after use
        @return: lock object
        """
        # to replace slash because it is reserved symbol in path value
        name = name.replace('/', '__')

        if isinstance(name, bytes):
            try:
                name = name.decode('UTF-8')
            except UnicodeError:
                pass

        full_name = self.get_full_name(name, '/')
        if not full_name.startswith('//'):
            full_name = "//{}".format(full_name)
        return YTLock(
            manager=self,
            name=full_name,
            timeout=timeout,
            block=block,
            block_timeout=block_timeout,
            common_lock_name=common_lock_name,
        )


class AbstractLockAcquisition(object):
    def check(self):
        raise NotImplementedError()

    def lock_id(self):
        raise NotImplementedError()


class SimpleLockAcquisition(AbstractLockAcquisition):
    def __init__(self, yt, path, **kws):
        self._yt = yt
        self._path = path
        self._kws = kws
        self._lock_id = None

    def check(self):
        try:
            self._lock_id = self._yt.lock(self._path, **self._kws)["lock_id"]
        except yw.YtResponseError as error:
            if error.is_concurrent_transaction_lock_conflict():
                return False
            raise
        return True

    def lock_id(self):
        if self._lock_id is None:
            raise ValueError("Lock is not acquired yet")

        return self._lock_id


class WaitableLockAcquisition(AbstractLockAcquisition):
    def __init__(self, yt, path, **kws):
        self._yt = yt
        self._path = path
        self._kws = kws
        self._lock_id = self._yt.lock(path, waitable=True, wait_for=None, **kws)["lock_id"]
        self._lock_state = '#{}/@state'.format(self._lock_id)
        self._is_acquired = False

    def check(self):
        if self._yt.get(self._lock_state) == 'acquired':
            self._is_acquired = True
        return self._is_acquired

    def lock_id(self):
        return self._lock_id


def lock_acquisition(yt, path, waitable=False, **kws):
    if waitable:
        return WaitableLockAcquisition(yt, path, **kws)
    else:
        return SimpleLockAcquisition(yt, path, **kws)


class YTLock(BaseLock):

    def __init__(self, manager, name, timeout, block, block_timeout, common_lock_name):
        super(YTLock, self).__init__(manager, name, timeout, block, block_timeout)

        self._path = name
        self._timeout = block_timeout
        self.transaction_timeout = timeout
        self._waitable = block
        self.common_lock_name = common_lock_name

        self._yt = self._get_client()

        self._tx = None

    def _get_client(self):
        return yw.YtClient(proxy=self.manager.proxy, token=self.manager.token)

    def _remove_node(self, path):
        """Remove a node if it is exists."""
        def try_remove(self, path):
            try:
                self._yt.remove(path, recursive=True, force=True)
                return True
            except Exception as err:
                logger.debug('Can\'t remove node with path=%s: "%s"', path, repr(err))
                return False
        i = 0
        while not try_remove(self, path) and i < 3:
            i += 1

    def _ensure_node_exists(self, node_type, path):
        """Idempotently create a node if it does not exist."""
        try:
            self._yt.create(node_type, path, recursive=True, ignore_existing=True)
        except Exception as err:
            logger.debug('Can\'t create node with path=%s: "%s"', path, repr(err))

    def _prepare(self):
        """Called in top-level transaction, should idempotently prepare lock object"""
        self._ensure_node_exists('map_node', self._path)

    def _start_acquisition(self):
        """Called in lock transaction, should return lock acquisition object"""

        return lock_acquisition(
            self._yt,
            path=self._path,
            waitable=self._waitable,
            mode='exclusive',
        )

    def _try_get_lock_with_retry(self, lock_func, max_iter_count=3):
        i = 0
        while i < max_iter_count:
            i += 1
            try:
                return lock_func()
            except yw.YtResponseError as error:
                if i < max_iter_count and error.is_resolve_error() and error.contains_text("Error resolving path"):
                    self._prepare()
                else:
                    raise error
        return

    def _acquire(self, deadline):
        tout = INITIAL_POLLING_INTERVAL

        acquisition = self._try_get_lock_with_retry(self._start_acquisition)

        while True:
            if self._try_get_lock_with_retry(acquisition.check):
                return acquisition.lock_id()

            now = time.time()
            if now >= deadline:
                raise YTTimeout()

            to_sleep = min(tout * random.random(), deadline - now)
            logger.debug('lock conflict (path %s), will sleep %s', self._path, to_sleep)
            time.sleep(to_sleep)

            tout = min(POLLING_BACKOFF * tout, MAX_POLLING_INTERVAL)

    def acquire(self, timeout=None):
        """Block until lock is acquired, but no longer than `timeout` seconds. Returns True if lock is acquired."""

        logger.debug('will acquire %s', self._path)

        title = 'Lock tx  @ {}/{}'.format(FQDN, current_process().pid)

        transaction_kwargs = {'attributes': {'title': title}}

        if self.transaction_timeout:
            transaction_kwargs['timeout'] = self.transaction_timeout * 1000  # in msec

        self._tx = self._yt.Transaction(**transaction_kwargs)

        logger.debug('tid %s', self._tx.transaction_id)

        self._prepare()

        self._tx.__enter__()

        if self._timeout is not None:
            _timeout = self._timeout
        else:
            _timeout = timeout

        if _timeout is None:
            _timeout = DEFAULT_BLOCK_TIMEOUT

        if not self.block:
            _timeout = 0

        deadline = time.time() + _timeout

        try:
            lock_id = self._acquire(deadline)
        except YTTimeout:
            self.release()
            return False
        except Exception:
            self.release()
            raise

        logger.debug('acquired lock_id %s', lock_id)
        return True

    def release(self, remove_node=True):
        logger.debug('will release %s', self._path)

        result = False

        if self._tx:
            self._tx.__exit__(None, None, None)
            result = True

        if remove_node and not self.common_lock_name:
            self._remove_node(self._path)

        return result

    def check_acquired(self):
        try:
            return len(self._yt.get('{}/@locks'.format(self._path))) > 0
        except Exception:
            return False

    def __enter__(self, *args, **kwargs):
        return self.acquire()

    def __exit__(self, *args):
        self.release()
