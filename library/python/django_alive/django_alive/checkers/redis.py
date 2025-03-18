# coding: utf-8
from __future__ import unicode_literals, absolute_import

from .base import BaseStorageChecker


class RedisChecker(BaseStorageChecker):
    def put(self, key, value):
        self.client.set(key, value)

    def pick(self, key):
        value = self.client.get(key)

        self.client.delete(key)

        return value
