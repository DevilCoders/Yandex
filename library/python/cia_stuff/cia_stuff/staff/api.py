import requests
import logging

logger = logging.getLogger(__name__)


class ApiWrapper(object):

    def __init__(self, base_url, token=None, page_size=10,
                 fields=None, filter_args=None, cookies=None):
        self.base_url = base_url
        self.token = token
        self.page_size = page_size
        self.fields = fields
        self.filter_args = filter_args
        self.cookies = cookies or {}
        assert not(self.token is None and self.cookies is None)

    def _request(self, url, params):
        headers = {}

        if self.token:
            headers['Authorization'] = 'OAuth {token}'.format(token=self.token)

        r = requests.get(
            url,
            headers=headers,
            params=params,
            cookies=self.cookies,
            verify=False,
            timeout=20,
        )

        if r.status_code != 200:
            raise Exception(r.content)

        logger.info(r.url)

        return r.json()

    def _prepare_initial_url(self):
        params = {}
        if self.page_size:
            params['_limit'] = self.page_size

        if self.fields:
            params['_fields'] = ','.join(self.fields)

        if self.filter_args:
            params.update(self.filter_args)
        return self.base_url, params

    def __iter__(self):
        next_url, params = self._prepare_initial_url()
        while next_url:
            data = self._request(next_url, params=params)
            next_url = data.get('links', {}).get('next', None)
            params = None
            for result in data['result']:
                yield result
