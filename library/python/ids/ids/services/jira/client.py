# coding: utf-8
import requests
from jira import client
from requests_oauthlib import OAuth2
from oauthlib.oauth2 import WebApplicationClient
from oauthlib.oauth2.rfc6749.clients import base


def prepare_yandex_headers(token, headers=None):
    headers = headers or {}
    headers['Authorization'] = 'OAuth %s' % token

    return headers


class YandexClient(WebApplicationClient):
    @property
    def token_types(self):
        types = super(YandexClient, self).token_types

        types.update({'Yandex': self._add_yandex_token})

        return types

    def _add_yandex_token(self, uri, http_method='GET', body=None,
            headers=None, token_placement=None):
        if token_placement == base.AUTH_HEADER:
            headers = prepare_yandex_headers(self.access_token, headers)

        elif token_placement in (base.URI_QUERY, base.BODY):
            raise NotImplementedError('Unsupported token placement.')

        else:
            raise ValueError("Invalid token placement.")
        return uri, headers, body


class YandexJIRA(client.JIRA):
    def _create_oauth_session(self, oauth):
        verify = self._options['verify']
        oauth = OAuth2(
            client_id=oauth.get('client_id'),
            client=oauth.get('client'),
            token=oauth['token'],
        )
        self._session = requests.Session()
        self._session.proxies = self._options['proxies']
        self._session.verify = verify
        self._session.auth = oauth
        self._session.timeout = self._options.get('timeout')
