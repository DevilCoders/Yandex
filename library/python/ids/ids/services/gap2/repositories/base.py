# coding: utf-8

from __future__ import unicode_literals

from ids.lib import paging_api


class GapPage(paging_api.PagingApiPage):
    data_key_result = 'gaps'


class GapFetcher(paging_api.PagingApiFetcher):
    page_cls = GapPage
    query_param_page = 'page'


class GapRepository(paging_api.PagingApiRepository):
    fetcher_cls = GapFetcher
