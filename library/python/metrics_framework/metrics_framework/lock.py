# coding: utf-8
from __future__ import unicode_literals

from datetime import datetime
import logging
import time

from django.conf import settings
import ylock.decorators


logger = logging.getLogger(__name__)
manager = ylock.backends.create_manager(**settings.YLOCK)


class Lock(object):
    default_manager = manager

    def __init__(self, name=None, manager=None, block=False, min_lock_time=settings.METRIC_MIN_LOCK_TIME):
        self.name = name
        self.manager = manager
        if self.manager is None:
            self.manager = self.default_manager
        self.block = block
        self.min_lock_time = min_lock_time

    def __enter__(self):
        self.lock = self.manager.lock(name=self.name, block=self.block)
        self.start = datetime.utcnow()
        self.is_free = self.lock.__enter__()
        if not self.is_free:
            logger.debug('lock with name "%s" was already taken', self.name)
        return self.is_free

    def __exit__(self, exc_type, exc_val, exc_tb):
        if self.min_lock_time is not None and self.is_free:
            now = datetime.utcnow()
            diff = (now - self.start).total_seconds()
            if 0 < diff < self.min_lock_time:
                time.sleep(self.min_lock_time - diff)
        return self.lock.__exit__(exc_type, exc_val, exc_tb)
