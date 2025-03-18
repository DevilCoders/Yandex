# coding: utf-8
from __future__ import unicode_literals

from six.moves import map
from ids.repositories.base import RepositoryBase
from ids.resource import Resource
from ids.lib.static_api import StaticApiGeneratedRepository

from .. import connector


class PlanRepository(StaticApiGeneratedRepository):
    connector_cls = connector.PlanConnector


class PlanBaseRepositoryDeprecated(RepositoryBase):
    connector_method = None

    def __init__(self, storage, server=None, **options):
        super(PlanBaseRepositoryDeprecated, self).__init__(storage, **options)
        self.connector = connector.PlanConnectorDeprecated(server=server)

    def getiter_from_service(self, lookup):
        lookup = self.handle_lookup(lookup)
        method = getattr(self.connector, self.connector_method)
        raw = method(self.RESOURCES, **lookup)
        return self.handle_result(raw)

    def handle_result(self, raw):
        if isinstance(raw, dict):
            # один объект
            raw = [raw]
        return map(self.wrap, raw)

    def handle_lookup(self, lookup):
        return lookup

    def wrap(self, obj):
        return Resource(obj) if obj is not None else None

