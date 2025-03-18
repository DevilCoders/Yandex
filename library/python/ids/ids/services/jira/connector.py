# -*- coding: utf-8 -*-

from jira.exceptions import JIRAError

from ids.exceptions import AuthError, BackendError, REQUESTS_ERRORS
from .client import YandexClient, YandexJIRA


class JiraConnector(object):
    backend_exceptions = (
        JIRAError,
    ) + REQUESTS_ERRORS

    def _check_auth_params(self, server, access_token):
        if server is None:
            raise AuthError('server url is not specified')
        if access_token is None:
            raise AuthError('supports only oauth2 auth system')

    def __init__(self, server, access_token, timeout=None):
        self._check_auth_params(server, access_token)

        options = {'server': server, 'timeout': timeout, 'verify': False}

        client_id = None
        token = {'access_token': access_token, 'token_type': 'Yandex'}

        client = YandexClient(client_id=client_id, token=token)

        self.connection = YandexJIRA(options=options, oauth={'token': token,
                                                             'client_id': client_id,
                                                             'client': client})

    def get_all_issues(self, jql_query, fields=None, chunk_size=50):
        if fields is None:
            fields = []

        count = 0
        while True:
            try:
                rs = self.connection.search_issues(jql_query,
                        startAt=count,
                        maxResults=chunk_size,
                        fields=fields,
                    )
            except self.backend_exceptions as e:
                raise BackendError(e)
            for r in rs:
                yield r

            L = len(rs)
            count += L
            if L < chunk_size:
                break
