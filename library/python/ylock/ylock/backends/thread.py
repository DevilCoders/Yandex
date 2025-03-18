# coding: utf-8
from __future__ import absolute_import

import logging
from threading import Lock

from ylock.base import BaseManager, BaseLock


log = logging.getLogger(__name__)


class Manager(BaseManager):

    def __init__(self, *args, **kwargs):
        super(Manager, self).__init__(*args, **kwargs)

        self._instance = {}

    def lock(self, name, timeout=None, block=None, block_timeout=None, **kwargs):
        if block:
            # ваш поток может быть заблокирован навечно,
            # параметр timeout игнорируется
            log.warning(
                'Using timeouts with thread lock is not supported'
            )
        name = self.get_full_name(name)
        return self._instance.setdefault(
            name,
            InProcessLock(self, name, timeout, block, block_timeout)
        )


class InProcessLock(BaseLock):
    _actual_lock = None
    _acquired = False

    def __init__(self, *args, **kwargs):
        super(InProcessLock, self).__init__(*args, **kwargs)
        self._actual_lock = Lock()

    @property
    def thread_lock(self):
        return self._actual_lock

    def acquire(self):
        self._acquired = self.thread_lock.acquire(1 if self.block else 0)
        return self._acquired

    def release(self):
        self.thread_lock.release()
        self._acquired = False
        return True

    def check_acquired(self):
        return self._acquired
