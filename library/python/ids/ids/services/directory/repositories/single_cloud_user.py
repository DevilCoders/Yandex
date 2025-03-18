# coding: utf-8

from __future__ import unicode_literals

from ids.registry import registry
from .base import DirectoryStaticRepository


@registry.add_simple
class SingleCloudUserRepository(DirectoryStaticRepository):

    RESOURCES = 'single_cloud_user'

    def getiter_from_service(self, lookup):
        uid = lookup.pop('uid')
        return self.connector.get(
            resource=self.RESOURCES,
            params=lookup,
            url_vars={'uid': uid}
        )
