import logging
import os
import random
import socket
import time

import yt.wrapper as yw

logger = logging.getLogger(__name__)

INITIAL_POLLING_INTERVAL = 1.0  # seconds
MAX_POLLING_INTERVAL = 16.0  # seconds
POLLING_BACKOFF = 1.5
INFINITE_TIMEOUT = 1000000000  # seconds


class Timeout(Exception):
    pass


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


class AbstractLock(object):
    def __init__(self, path, client=yw, identifier=None, user_attrs=None, transaction_kwargs=None, timeout=None, waitable=False):
        # XXX: YT client is mutable, it stores active transaction stack inside.
        # Here, the client is cloned, while its transaction stack is untouched.
        # Thus,
        # 1. Lock() always operates in top-level transaction
        # 2. Lock() does not affect client's internal state in any way
        if client is yw:
            self._yt = yw.YtClient(config=client.config.config)
        else:
            self._yt = yw.YtClient(config=client.config)

        self._user = self._yt.get_user_name()
        self._path = path.replace('{user}', self._user)
        self._identifier = identifier
        self._user_attrs = user_attrs
        self._transaction_kwargs = transaction_kwargs
        self._timeout = timeout
        self._waitable = waitable

    def _prepare(self):
        """Called in top-level transaction, should idempotently prepare lock object"""

        raise NotImplementedError()

    def _start_acquisition(self):
        """Called in lock transaction, should return lock acquisition object"""

        raise NotImplementedError()

    def _ensure_node_exists(self, node_type, path):
        """Idempotently create a node if it does not exist.

        Optimizes YT load by avoiding mutating request in case target node already exists."""

        if not self._yt.exists(path):
            self._yt.create(node_type, path, recursive=True, ignore_existing=True)

    def acquire(self, timeout=None):
        """Block until lock is acquired, but no longer than `timeout` seconds. Returns True iff lock is acquired."""

        logger.debug('will acquire %s', self._path)

        self._prepare()

        title = 'Lock tx'
        if self._identifier is not None:
            title += ' for ' + str(self._identifier)
        title += ' @ ' + socket.getfqdn()

        attrs = {'title': title, 'identifier': self._identifier}
        if self._user_attrs:
            attrs.update(self._user_attrs)

        transaction_kwargs = {'attributes': attrs}

        if self._transaction_kwargs is not None:
            transaction_kwargs.update(self._transaction_kwargs)

        self._tx = self._yt.Transaction(**transaction_kwargs)

        logger.debug('tid %s', self._tx.transaction_id)

        self._tx.__enter__()

        if self._timeout is not None:
            _timeout = self._timeout
        else:
            _timeout = timeout

        if _timeout is None:
            _timeout = INFINITE_TIMEOUT

        deadline = time.time() + _timeout

        try:
            lock_id = self._acquire(deadline)
        except Timeout:
            self.release()
            return False
        except Exception:
            self.release()
            raise

        logger.debug('acquired lock_id %s', lock_id)
        return True

    def _acquire(self, deadline):
        tout = INITIAL_POLLING_INTERVAL

        acquisition = self._start_acquisition()

        while True:
            if acquisition.check():
                return acquisition.lock_id()

            now = time.time()
            if now >= deadline:
                raise Timeout()

            to_sleep = min(tout * random.random(), deadline - now)
            logger.debug('lock conflict (path %s), will sleep %s', self._path, to_sleep)
            time.sleep(to_sleep)

            tout = min(POLLING_BACKOFF * tout, MAX_POLLING_INTERVAL)

    def release(self):
        logger.debug('will release %s', self._path)

        self._tx.__exit__(None, None, None)

    # tx lock is acquired under might die, that would lead to lock release
    # NOTE: use only when for lock transaction "ping" attribute is set to True (default is True)
    def is_lock_pinger_alive(self):
        tx = self._tx
        if not tx:
            return None
        return tx.is_pinger_alive()

    def __enter__(self, *args, **kwargs):
        self.acquire()
        return self

    def __exit__(self, *args):
        self.release()


class Lock(AbstractLock):
    """Creates new `map_node` for locking, holds exclusive lock on it."""

    def _prepare(self):
        self._ensure_node_exists('map_node', self._path)

    def _start_acquisition(self):
        return lock_acquisition(
            self._yt,
            path=self._path,
            waitable=self._waitable,
            mode='exclusive',
        )


class SharedNodeLock(AbstractLock):
    """Only creates parent `map_node` for lock node, holds shared lock on full path.

    Useful for concurrent idempotent uploads, does not leave dangling lock nodes.
    """

    def _prepare(self):
        self._ensure_node_exists('map_node', os.path.dirname(self._path))

    def _start_acquisition(self):
        return lock_acquisition(
            self._yt,
            path=os.path.dirname(self._path),
            waitable=self._waitable,
            mode='shared',
            child_key=os.path.basename(self._path),
        )
