# coding: utf-8
from __future__ import unicode_literals

from six.moves import map
from ids.repositories.base import RepositoryBase
from ids.resource import Resource
from ids.services.at.connector import Connector


class AtBaseRepository(RepositoryBase):

    connector_cls = Connector

    def getiter_from_service(self, lookup):
        return self.handle_result(self.search(lookup))

    def handle_result(self, raw):
        return map(self.wrap, raw)

    def wrap(self, obj):
        return Resource(obj) if obj is not None else None

    def get_user_session_id(self):
        return 'NOT-NEEDED'
