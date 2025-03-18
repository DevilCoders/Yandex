# coding: utf-8
from __future__ import unicode_literals, absolute_import

from .base import BaseStorageChecker

import kazoo.exceptions


class ZookeeperChecker(BaseStorageChecker):
    key_separator = '/'

    def put(self, key, value):
        try:
            self.client.delete(key, version=-1)
        except kazoo.exceptions.NoNodeError:
            pass

        self.client.create(key, value, makepath=True)

    def pick(self, key):
        try:
            value, node_stat = self.client.get(key)
        except kazoo.exceptions.NoNodeError:
            pass
        else:
            self.client.delete(key, version=nodestat.version)

            return value
