# -*- encoding: utf-8 -*-
from __future__ import unicode_literals

import logging
import re

import requests

from .settings import DEFAULT_HOST
from .exceptions import ConnectionError, ApiCallError


LOG = logging.getLogger(__name__)


class RefsResponse(dict):

    page_pattern = re.compile('page=([^&]+).*$')

    def __init__(self, *args, **kwargs):
        super(RefsResponse, self).__init__(*args, **kwargs)

        self._response = kwargs.get('response')  # type: requests.Response

    @property
    def last_page(self):
        pages = self.page_pattern.findall(self._response.links.get('last', {'url': ''})['url'])
        if pages:
            return pages[0]

    @property
    def next_page(self):
        pages = self.page_pattern.findall(self._response.links.get('next', {'url': ''})['url'])
        if pages:
            return pages[0]


class Connector(object):

    def __init__(self, host=None, timeout=None, lang=None):
        """
        :param str|unicode host: Имя хоста, на котором находится Справочная. Будет использован протокол HTTPS.
            Если не указан, будет использован хост по умолчанию (см. .settings.HOST_DEFAULT).

        :param int timeout: Таймаут на подключение. По умолчанию: 5 сек.

        :param str|unicode lang: Код языка, на котором должны быть возвращены результаты.

        """
        self._timeout = timeout or 5
        self._url_base = 'https://' + (host or DEFAULT_HOST) + '/api/'
        self._lang = lang

    def request(self, url, data=None, method='post', params=None):
        from . import VERSION_STR

        url = self._url_base + url
        timeout = self._timeout

        method = getattr(requests, method)

        LOG.debug('URL: %s', url)

        def do_request():

            headers = {
                'User-agent': 'refsclient/%s' % VERSION_STR,
            }

            lang = self._lang

            if lang:
                headers['Accept-Language'] = lang

            try:
                response = method(url, **{
                    'headers': headers,
                    'data': {'query': data},
                    'timeout': timeout,
                    'verify': False,
                    'params': params,
                })  # type: requests.Response

            except requests.ReadTimeout:
                raise ConnectionError('Request timed out.')

            except requests.ConnectionError:
                raise ConnectionError('Unable to connect to %s.' % url)

            try:
                response.raise_for_status()

            except requests.HTTPError as e:
                    msg = response.content
                    status_code = response.status_code
                    LOG.debug('API call error, code [%s]:\n%s', status_code, msg)
                    raise ApiCallError(msg, status_code)

            return response

        response = do_request()

        try:
            result = RefsResponse(response.json(), response=response)

        except ValueError:
            # Здесь может быть HTML страница от nginx.
            raise ApiCallError(response.content, response.status_code)

        return result
