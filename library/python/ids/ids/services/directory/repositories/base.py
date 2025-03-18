# coding: utf-8

from __future__ import unicode_literals

from ids.lib.paging_api import PagingApiRepository, PagingApiPage, PagingApiFetcher
from ids.repositories.base import RepositoryBase
from ..connector import DirectoryConnector


class DirectoryPage(PagingApiPage):

    data_key_limit = 'per_page'


class DirectoryPagingFetcher(PagingApiFetcher):

    page_cls = DirectoryPage

    query_param_page = 'page'

    default_per_page_param_value = 1000

    def __init__(self, connector, **request_params):
        lookup = request_params.get('params', None)
        if lookup is None:
            request_params['params'] = lookup = {}

        if DirectoryPage.data_key_limit not in lookup:
            lookup[DirectoryPage.data_key_limit] = self.default_per_page_param_value

        super(DirectoryPagingFetcher, self).__init__(connector, **request_params)


class DirectoryRepository(RepositoryBase):

    SERVICE = 'directory'

    connector_cls = DirectoryConnector


class DirectoryPagingRepository(DirectoryRepository, PagingApiRepository):

    fetcher_cls = DirectoryPagingFetcher


class DirectoryStaticRepository(DirectoryRepository):

    def getiter_from_service(self, lookup):
        return self.connector.get(resource=self.RESOURCES, params=lookup)
