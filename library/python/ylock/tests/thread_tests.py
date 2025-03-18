# coding: utf-8
from __future__ import absolute_import

from library.python.ylock.tests import base


class ThreadTestCase(base.BaseTestCase):
    backend = 'thread'
    hosts = None

    def test_legacy_context_manager_concurrency(self):
        self.skipTest('there is not support for multiple processes')
