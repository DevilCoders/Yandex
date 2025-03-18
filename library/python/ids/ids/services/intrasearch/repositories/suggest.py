# coding: utf-8

from __future__ import unicode_literals

from ids.registry import registry
from ids.repositories.base import RepositoryBase
from ids.services.intrasearch import connector


@registry.add_simple
class SuggestRepository(RepositoryBase):
    SERVICE = 'intrasearch'
    RESOURCES = 'suggest'

    connector_cls = connector.IntrasearchConnector

    def getiter_from_service(self, lookup):
        if 'version' not in lookup:
            lookup['version'] = '1'

        return self.connector.get(
            resource='suggest',
            params=lookup,
        )
