# -*- coding: utf-8 -*-
from __future__ import unicode_literals

from ids.repositories.base import RepositoryBase
from ids.resource import Resource

from ..connector import GeobaseConnector


class GeobaseRepository(RepositoryBase):
    def __init__(self, storage, **options):
        super(GeobaseRepository, self).__init__(storage, **options)
        self.connector = GeobaseConnector(**options)

    def get(self, lookup, url_vars=None, **options):
        return self.connector.get(resource=self.RESOURCES, params=lookup, url_vars=url_vars, **options)

    def wrap(self, obj):
        return Region(obj) if obj else None


class Region(Resource):
    @property
    def bgp_name(self):
        return ''

    @property
    def capital_id(self):
        return self.get('capital_id')

    @property
    def en_name(self):
        return self.get('en_name')

    @property
    def id(self):
        return self.get('id')

    @property
    def latitude(self):
        return self.get('latitude')

    @property
    def longitude(self):
        return self.get('longitude')

    @property
    def is_main(self):
        return self.get('is_main')

    @property
    def name(self):
        return self.get('name')

    @property
    def parent_id(self):
        return self.get('parent_id')

    @property
    def phone_code(self):
        return self.get('phone_code')

    @property
    def position(self):
        return self.get('position')

    @property
    def services(self):
        return self.get('services')

    @property
    def short_en_name(self):
        return self.get('short_en_name')

    @property
    def latitude_size(self):
        return self.get('latitude_size')

    @property
    def longitude_size(self):
        return self.get('longitude_size')

    @property
    def synonyms(self):
        return self.get('synonyms')

    @property
    def tzname(self):
        return self.get('tzname')

    @property
    def type(self):
        return self.get('type')

    @property
    def zip_code(self):
        return self.get('zip_code')

    @property
    def zoom(self):
        return self.get('zoom')
