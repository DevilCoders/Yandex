# coding: utf-8
from __future__ import unicode_literals

import datetime
import logging
import time
from functools import wraps

import ylock
from django.utils.decorators import available_attrs

from django_abc_data.conf import settings

log = logging.getLogger(__name__)

manager = ylock.backends.create_manager(**settings.ABC_DATA_YLOCK)
min_lock_time = settings.ABC_DATA_MIN_LOCK_TIME


class Lock(object):
    default_manager = manager

    def __init__(self, name=None, manager=None, block=False, min_lock_time=min_lock_time):
        self.name = name
        self.manager = manager
        if self.manager is None:
            self.manager = self.default_manager
        self.block = block
        self.min_lock_time = min_lock_time
        self.start = datetime.datetime.utcnow()

    def __enter__(self):
        self.lock = self.manager.lock(name=self.name, block=self.block)
        self.is_free = self.lock.__enter__()
        if not self.is_free:
            log.info('lock with name "%s" was already taken', self.name)
        return self.is_free

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.min_lock_time is not None and self.is_free:
            now = datetime.datetime.utcnow()
            diff = (now - self.start).total_seconds()
            if diff < self.min_lock_time:
                time.sleep(self.min_lock_time - diff)
        return self.lock.__exit__(exc_type, exc_val, exc_tb)

    def __call__(self, func):
        @wraps(func, assigned=available_attrs(func))
        def inner(*args, **kwargs):
            with self as is_free:
                if not is_free:
                    log.info('lock with name "%s" was already taken', self.name)
                    return
                return func(*args, **kwargs)
        return inner


def lock(name=None, manager=None, block=False, min_lock_time=min_lock_time):
    if callable(name):
        return Lock(name=name.__module__)(name)
    else:
        return Lock(name=name, manager=manager, block=block, min_lock_time=min_lock_time)
