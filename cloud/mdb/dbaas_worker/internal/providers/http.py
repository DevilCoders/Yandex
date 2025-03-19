"""
HTTP Client provider
"""

import json
import urllib.parse
import uuid
from copy import deepcopy
from json.decoder import JSONDecodeError

import requests

from dbaas_common import retry

from .common import BaseProvider

SECRET_FIELDS = [
    'secret',
    'secrets',
]


def _remove_secret_fields(data):
    """
    Remove secrets from http call (for safe logging)
    """
    data_copy = deepcopy(data)
    for field in SECRET_FIELDS:
        if field in data_copy:
            data_copy[field] = '***'
    return json.dumps(data_copy)


def _get_full_url(url, params):
    """
    Get curl-friendly url from url and params
    """
    if params:
        return f'{url}?{urllib.parse.urlencode(params)}'
    return url


class HTTPErrorHandler:
    """
    HTTP error handler
    """

    def __init__(self, error_type):
        self.error_type = error_type

    def handle(self, kwargs, request_id, result):
        """
        Raise exception from http result
        """
        raise self.error_type(
            f'Unexpected {kwargs["method"].upper()} '
            f'{_get_full_url(kwargs["url"], kwargs["params"])} '
            f'result: {result.status_code} {result.text} '
            f'request_id: {request_id}'
        )


class HTTPClient(BaseProvider):
    """
    Base class for http client providers
    """

    def __init__(self, config, task, queue):
        super().__init__(config, task, queue)
        self.base_url = None
        self.default_headers = {}
        self.session = None
        self.verify = True
        self.error_handler = HTTPErrorHandler(RuntimeError)
        self.internal_error_handler = HTTPErrorHandler(requests.exceptions.HTTPError)

    def _init_session(self, *base_url, default_headers=None, error_handler=None, verify=True):
        """
        Initialize requests session
        """
        self.base_url = base_url[0]
        if not self.base_url.endswith('/'):
            self.base_url = f'{self.base_url}/'
        for part in base_url[1:]:
            self.base_url = urllib.parse.urljoin(self.base_url, part.lstrip('/'))
        self.session = requests.Session()
        adapter = requests.adapters.HTTPAdapter(pool_connections=1, pool_maxsize=1)
        parsed = urllib.parse.urlparse(self.base_url)
        self.session.mount(f'{parsed.scheme}://{parsed.netloc}', adapter)
        self.verify = verify
        if default_headers:
            self.default_headers = default_headers
        if error_handler:
            self.error_handler = error_handler

    @retry.on_exception((JSONDecodeError, requests.exceptions.RequestException), factor=1, max_wait=5, max_tries=24)
    def _make_request(self, path, method='get', expect=None, data=None, params=None, headers=None, verify=None):
        """
        Make http request
        """
        if expect is None:
            expect = [200]
        if path.startswith('http'):
            url = path
        else:
            url = urllib.parse.urljoin(self.base_url, path)

        default_headers = self.default_headers() if callable(self.default_headers) else self.default_headers.copy()

        kwargs = {
            'method': method,
            'url': url,
            'headers': default_headers,
            'timeout': (1, 600),
            'verify': self.verify if verify is None else verify,
            'params': params,
        }

        if headers:
            kwargs['headers'].update(headers)

        if data is not None:
            kwargs['headers']['Accept'] = 'application/json'
            kwargs['headers']['Content-Type'] = 'application/json'
            kwargs['json'] = data

        request_id = str(uuid.uuid4())
        kwargs['headers']['X-Request-Id'] = request_id

        extra = self.logger.extra.copy()
        extra['request_id'] = request_id

        self.logger.logger.info(
            'Starting %s request to %s%s',
            method.upper(),
            _get_full_url(kwargs['url'], params),
            f' with {_remove_secret_fields(data)}' if data else '',
            extra=extra,
        )

        res = self.session.request(**kwargs)

        self.logger.logger.info(
            '%s request to %s finished with %s',
            method.upper(),
            _get_full_url(kwargs['url'], params),
            res.status_code,
            extra=extra,
        )

        if res.status_code >= 500:
            self.internal_error_handler.handle(kwargs, request_id, res)

        if res.status_code not in expect:
            self.error_handler.handle(kwargs, request_id, res)

        if 'application/json' in res.headers.get('Content-Type', '').lower():
            return res.json()

        return res
