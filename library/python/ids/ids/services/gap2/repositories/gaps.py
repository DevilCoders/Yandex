# coding: utf-8
from __future__ import unicode_literals

from ids.registry import registry
from ids.services.gap2 import connector
from ids.services.gap2.repositories import base


@registry.add_simple
class GapsRepository(base.GapRepository):

    SERVICE = 'gap2'
    RESOURCES = 'gaps'

    connector_cls = connector.Gap2Connector

    def get_one(self, lookup, **options):
        return self.connector.get(
            resource='gaps_detail',
            url_vars=lookup,
        )
