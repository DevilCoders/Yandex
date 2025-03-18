# coding: utf-8

from __future__ import unicode_literals

from six.moves import urllib

from ids.lib import pagination
from ids.resource import wrap


class LinkedPage(pagination.Page):
    """
    Формат пагинации через ссылки на следующую/предыдущую страницы
    """

    results = None
    count = None
    previous_page_url = None
    next_page_url = None

    num = None

    def set_page_attributes(self, data):
        self.results = data.pop('results')
        self.count = data.pop('count')
        self.previous_page_url = data.pop('previous')
        self.next_page_url = data.pop('next')
        super(LinkedPage, self).set_page_attributes(data)

    def __iter__(self):
        for obj in self.results:
            yield wrap(obj)

    def __len__(self):
        return len(self.results)

    @property
    def total(self):
        return self.count

    @property
    def num(self):
        if self.previous_page_url is None:
            return 1

        parsed_url = urllib.parse.urlparse(self.previous_page_url)
        query_params = urllib.parse.parse_qs(parsed_url.query)
        if 'page' in query_params:
            previous_page = int(query_params['page'][0])
        else:
            previous_page = 1
        return previous_page + 1


class LinkedPageFetcher(pagination.Fetcher):

    page_cls = LinkedPage

    def get_request_params_for_page_after(self, current):
        request_params = dict(self.request_params)
        query_params = request_params.get('params', {})
        query_params['page'] = current.num + 1
        return request_params

    def has_page_after(self, current):
        return current.next_page_url is not None
