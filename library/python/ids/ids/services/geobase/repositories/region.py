# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from ids.registry import registry

from .base import GeobaseRepository


@registry.add_simple
class GeobaseRegionRepository(GeobaseRepository):
    SERVICE = 'geobase'
    RESOURCES = 'region'

    def region_by_id(self, region_id, **options):
        lookup = {
            'id': region_id,
        }
        url_vars = {
            'func': 'region_by_id',
        }
        return self.wrap(self.get(lookup, url_vars=url_vars, **options))

    def region_by_ip(self, ip, **options):
        lookup = {
            'ip': ip,
        }
        url_vars = {
            'func': 'region_by_ip',
        }
        return self.wrap(self.get(lookup, url_vars=url_vars, **options))

    def getiter_from_service(self, lookup):
        url_vars = {
            'func': 'regions_by_type',
        }
        for region in self.get(lookup, url_vars=url_vars):
            yield self.wrap(region)

    def get_one(self, lookup, **options):
        if 'type' in lookup:
            return next(self.getiter_from_service(lookup))
        if 'ip' in lookup:
            return self.region_by_ip(lookup.get('ip'), **options)
        return self.region_by_id(lookup.get('id'), **options)

    def get_all(self, lookup, **options):
        return list(self.getiter_from_service(lookup))
