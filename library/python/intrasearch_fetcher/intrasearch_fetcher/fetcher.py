# coding: utf-8
from __future__ import unicode_literals, absolute_import

from ids import exceptions
from ids.registry import registry

from intrasearch_fetcher.exceptions import IntrasearchFetcherException


class IntrasearchFetcher(object):
    def __init__(self, layer, user_agent, oauth_token, timeout):
        self.layer = layer
        self.default_parameters = {
            'layers': layer,
            'allow_empty': True,
        }
        self.user_agent = user_agent
        self.oauth_token = oauth_token
        self.timeout = timeout

    @staticmethod
    def convert(limit, offset):
        if limit > offset:
            return 0, limit + offset, offset, limit + offset

        for per_page in range(limit, limit + offset + 1):
            total_gap = per_page - limit
            front_gap = offset % per_page
            if front_gap <= total_gap:
                page = offset // per_page
                return page, per_page, front_gap, limit + front_gap

    def get_repository(self):
        return registry.get_repository(
            'intrasearch',
            'suggest',
            user_agent=self.user_agent,
            oauth_token=self.oauth_token,
            timeout=self.timeout
        )

    def build_query(self, filters):
        return '&'.join(
            '{}:"{}"'.format(k, v)
            for k, v in sorted(filters.items())
        )

    def fetch(self, params=(), limit=None, offset=None, page=None, per_page=None):
        # Либо page и per_page, либо limit и offset
        assert (limit is None and offset is None) or (page is None and per_page is None)

        repo = self.get_repository()

        if limit or offset:
            # Находим оптимальные page и per_page и берём срез
            page, per_page, i1, i2 = self.convert(limit, offset)
        else:
            # Берём все элементы
            i1, i2 = None, None

        parameters = self.default_parameters.copy()
        parameters.update({
            '{}.page'.format(self.layer): page,
            '{}.per_page'.format(self.layer): per_page,
        })
        parameters.update(params)

        try:
            data = repo.get(parameters)
        except exceptions.BackendError as e:
            raise IntrasearchFetcherException('IDS: {}'.format(e))

        try:
            objects = data[0]['result'][i1:i2]
        except KeyError:
            raise IntrasearchFetcherException('Intrasearch response not compliant to format')

        # заменяем список fields с type и value на словарь
        for obj in objects:
            if obj.get('fields') is not None:
                fields = {field['type']: field['value'] for field in obj['fields']}
                obj['fields'] = fields

        return objects

    def search(self, text, filters, language='ru', limit=None, offset=None, page=None, per_page=None):
        parameters = {
            'language': language,
            'text': text,
        }

        query = self.build_query(filters)
        if query:
            parameters['{}.query'.format(self.layer)] = query

        return self.fetch(parameters, limit, offset, page, per_page)
