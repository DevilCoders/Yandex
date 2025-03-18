# coding: utf-8

from __future__ import unicode_literals

from ids.registry import registry
from ids.repositories.base import RepositoryBase
from ids.services.goals import connector
from ids.lib.linked_pagination import LinkedPageFetcher
from ids.lib.pagination import ResultSet


@registry.add_simple
class GoalsRepository(RepositoryBase):
    SERVICE = 'goals'
    RESOURCES = 'goals'

    connector_cls = connector.GoalsConnector

    def getiter_from_service(self, lookup):
        fetcher = LinkedPageFetcher(
            connector=self.connector,
            resource=self.RESOURCES,
            params=lookup,
        )
        return ResultSet(fetcher=fetcher)
