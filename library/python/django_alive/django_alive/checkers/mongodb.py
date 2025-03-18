# coding: utf-8
from __future__ import unicode_literals, absolute_import

from .base import BaseStorageChecker


class MongodbChecker(BaseStorageChecker):
    def put(self, key, value):
        self.client['alive'].update({'_id': key}, {'value': value}, upsert=True)

    def pick(self, key):
        doc = self.client['alive'].find_and_modify({'_id': key}, remove=True)

        if doc:
            return doc['value']
