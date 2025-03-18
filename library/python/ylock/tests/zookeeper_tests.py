# coding: utf-8
from __future__ import absolute_import

import os

from library.python.ylock.tests import base

from ylock import create_manager


class TTLZookeeperTestCase(base.BaseTestCase):
    backend = 'zookeeper'
    hosts = [os.environ.get('YLOCK_TEST_ZOOKEEPER_HOST', 'localhost')]

    def test_get_hosts(self):
        hosts1 = create_manager(self.backend, hosts='foobar:123').get_hosts()
        self.assertEqual(hosts1, 'foobar:123')

        hosts2 = create_manager(self.backend,
                                hosts=['foo:123', 'bar:321']).get_hosts()
        self.assertEqual(hosts2, 'foo:123,bar:321')


class ZookeeperTestCase(TTLZookeeperTestCase):
    """Тест лока через эфемерную ноду
    """
    timeout = None
