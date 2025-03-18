# coding: utf-8
from __future__ import absolute_import

from contextlib import contextmanager

from celery.app import app_or_default
from kombu.utils import symbol_by_name

from .base import Checker

from .. import tasks


class CeleryWorkerChecker(Checker):

    def __init__(self, name, stamper, app=None, priority=None, queue=None):
        super(CeleryWorkerChecker, self).__init__(name, stamper)

        self.app = symbol_by_name(app) if app else app_or_default()
        self.priority = priority or 9  # 9 - use max priority by default
        self.queue = queue or 'celery@%h.dq'

    def check(self, group):
        with switch_app(self.app):
            tasks.check.apply_async(args=(self.name, group),
                                    queue=self.get_host_queue(),
                                    priority=self.priority)

    def touch(self, group, host=None):
        self.do_stamp(group, data={'app': self.app.main}, host=host)

    def get_host_queue(self):
        return self.queue.replace('%h', self.get_current_host())


class CeleryBeatChecker(CeleryWorkerChecker):
    def check(self, group):
        pass


@contextmanager
def switch_app(app, *tasks):
    """Переключаем дефолтное приложение"""
    old_app = app_or_default()

    app.set_current()

    for task in tasks:
        task.bind(app)

    try:
        yield app
    finally:
        old_app.set_current()

        for task in tasks:
            task.bind(old_app)
