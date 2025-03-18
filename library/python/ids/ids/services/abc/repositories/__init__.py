# coding: utf-8
from __future__ import unicode_literals

from ids.lib.paging_api import PagingApiRepository
from ids.lib.linked_pagination import LinkedPageFetcher
from ids.lib.pagination import ResultSet

from ..connector import ABCConnector
from ..pagination import LinkedPageAbcFetcher


class ABCRepository(PagingApiRepository):
    connector_cls = ABCConnector

    def getiter_from_service(self, lookup):
        fetcher = LinkedPageAbcFetcher(
            connector=self.connector,
            resource=self.RESOURCES,
            params=lookup,
        )
        return ResultSet(fetcher=fetcher)
