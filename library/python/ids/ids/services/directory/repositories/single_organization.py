# coding: utf-8

from __future__ import unicode_literals

from ids.registry import registry
from .base import DirectoryStaticRepository


@registry.add_simple
class SingleOrganizationRepository(DirectoryStaticRepository):
    RESOURCES = 'single_organization'

    def getiter_from_service(self, lookup):
        uid = lookup.pop('id')
        return self.connector.get(
            resource=self.RESOURCES,
            params=lookup,
            url_vars={'id': uid}
        )
