# coding: utf-8
from __future__ import absolute_import
import warnings

warnings.warn('Dispofa backend is deprecated.', DeprecationWarning, stacklevel=2)

import logging
import os
import time

import dispofa

from ylock.base import BaseManager, BaseLock


log = logging.getLogger(__name__)


class Manager(BaseManager):
    default_config = '/etc/dispofa.conf'

    default_timeout = 30
    default_sleep_interval = 2
    default_retry_count = 3

    def __init__(self, *args, **kwargs):
        self.retry_count = kwargs.pop('retry_count', self.default_retry_count)
        self.sleep_interval = kwargs.pop('sleep_interval', self.default_sleep_interval)

        super(Manager, self).__init__(*args, **kwargs)

        self._instance = None

    def _init(self):
        self._instance = dispofa.Dispofa()

        hosts = self.hosts

        if hosts:
            if isinstance(hosts, basestring):
                self._instance.ReadEndpoints(hosts)
            else:
                for host in hosts:
                    self._instance.AddEndpoint(host)
        elif os.path.exists(self.default_config):
            self._instance.ReadEndpoints(self.default_config)
        else:
            self._instance.AddEndpoint('localhost')

    @property
    def instance(self):
        if self._instance is None:
            self._init()

        return self._instance

    def acquire_lock(self, name, timeout):
        return bool(self.instance.Lock(self.get_full_name(name), timeout))

    def release_lock(self, name):
        return bool(self.instance.Unlock(self.get_full_name(name)))

    def lock(self, name, timeout=None, block=None, block_timeout=None, **kwargs):
        if block_timeout is not None:
            log.warning('block_timeout is not supported with Dispofa')
        return DispofaLock(self, name, timeout, block)


class DispofaLock(BaseLock):
    def acquire(self):
        if self.timeout is None:
            timeout = self.manager.default_timeout
        else:
            timeout = self.timeout

        retry_count = self.manager.retry_count

        while True:
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
