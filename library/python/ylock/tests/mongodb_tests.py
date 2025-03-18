from __future__ import absolute_import

import os

from library.python.ylock.tests import base


class MongoDBTestCase(base.BaseTestCase):
    backend = 'mongodb'
    hosts = [os.environ.get('YLOCK_TEST_MONGODB_HOST', 'localhost')]
