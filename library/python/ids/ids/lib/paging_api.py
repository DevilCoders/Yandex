# coding: utf-8
from __future__ import unicode_literals

from ids.resource import wrap
from ids.repositories.base import RepositoryBase
from . import pagination


class PagingApiPage(pagination.Page):

    data_key_page = 'page'
    data_key_pages = 'pages'
    data_key_result = 'result'
    data_key_total = 'total'
    data_key_limit = 'limit'

    page = None
    pages = None
    result = None
    total = None
    limit = None

    num = None

    def set_page_attributes(self, data):
        self.page = data.pop(self.data_key_page)
        self.num = self.page
        self.pages = data.pop(self.data_key_pages)
        self.result = data.pop(self.data_key_result)
        self.total = data.pop(self.data_key_total)
        self.limit = data.pop(self.data_key_limit)
        super(PagingApiPage, self).set_page_attributes(data)

    def __iter__(self):
        for obj in self.result:
            yield wrap(obj)

    def __len__(self):
        return len(self.result)


class PagingApiFetcher(pagination.Fetcher):

    page_cls = PagingApiPage

    query_param_page = '_page'

    def get_request_params_for_page_after(self, current):
        request_params = self.request_params
        query_params = request_params.get('params')
        if query_params is None:
            request_params['params'] = query_params = {}
        query_params[self.query_param_page] = current.num + 1
        return request_params

    def has_page_after(self, current):
        return current.num < current.pages


class PagingApiRepository(RepositoryBase):

    fetcher_cls = PagingApiFetcher

    def getiter_from_service(self, lookup):
        return pagination.ResultSet(fetcher=self.fetcher_cls(
            connector=self.connector,
            resource=self.RESOURCES,
            params=lookup,
        ))
