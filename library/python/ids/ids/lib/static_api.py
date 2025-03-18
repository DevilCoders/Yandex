# coding: utf-8
from __future__ import unicode_literals

from ids.resource import wrap
from .paging_api import PagingApiRepository


class StaticApiGeneratedRepository(PagingApiRepository):

    def get_one(self, lookup, **options):
        lookup['_one'] = '1'
        raw_obj = self.connector.get(resource=self.RESOURCES, params=lookup)
        return wrap(raw_obj)

    def get_nopage(self, lookup, **options):
        lookup['_nopage'] = '1'
        raw_objects = self.connector.get(resource=self.RESOURCES, params=lookup).get('result')
        return list(map(wrap, raw_objects))
