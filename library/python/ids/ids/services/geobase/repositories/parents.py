# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from ids.registry import registry

from .base import GeobaseRepository


@registry.add_simple
class GeobaseParentsRepository(GeobaseRepository):
    SERVICE = 'geobase'
    RESOURCES = 'parents'

    def getiter_from_service(self, lookup):
        for parent_id in self.get(lookup):
            yield parent_id

    def get_all(self, lookup, **options):
        return list(self.getiter_from_service(lookup))
