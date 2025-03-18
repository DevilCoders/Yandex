# coding: utf-8
from __future__ import unicode_literals

import warnings
import logging

from ids import connector
from ids import exceptions
from ids.connector import plugins_lib


log = logging.getLogger(__name__)


class OauthConnector(connector.HttpConnector):

    service_name = 'OAUTH'

    plugins = (
        plugins_lib.JsonResponse,
    )

    url_patterns = {
        'token': '/token'
    }

    def __init__(self, host=None, protocol=None, **kwargs):
        super(OauthConnector, self).__init__(
            host=host,
            protocol=protocol,
            user_agent=kwargs.pop('user_agent', 'ids'),
            **kwargs
        )

    def handle_bad_response(self, response):
        raise exceptions.BackendError(' '.join([
            'Bad response:',
            str(response.status_code),
            response.text,
        ]))


# function for backward compatible args order
def get_token(uid, oauth_id, oauth_secret, options=None):
    warnings.warn(
        'get_token is deprecated. use get_token_by_uid instead',
        DeprecationWarning
    )
    return get_token_by_uid(
        oauth_id=oauth_id,
        oauth_secret=oauth_secret,
        uid=uid,
        options=options,
    )


def get_token_by_uid(
        oauth_id, oauth_secret,
        uid, options=None):
    """
    Токен по uid пользователя.

    Не требуется никаких подтверждений пользователем.
    oauth-приложение, которое использует этот метод необходимо, чтобы ВСЕ
    скоупы, которые оно указало при регистрации имели специальные гранты на
    хождение в  oauth.yandex-team.ru/token/ с 'grant_type': 'assertion'.

    Этот способ не рекомендуется к использованию и не описан даже во
    внутренней документации.
    """
    return _get_token(
        oauth_params={
            'client_id': oauth_id,
            'client_secret': oauth_secret,
            'grant_type': 'assertion',
            'assertion_type': 'yandex-uid',
            'assertion': uid,
        },
        options=options,
    )


def get_token_by_password(
        oauth_id, oauth_secret,
        username, password, options=None):
    """
    Токен по логину/паролю
    https://wiki.yandex-team.ru/oauth/token/#granttypepassword
    """
    return _get_token(
        oauth_params={
            'client_id': oauth_id,
            'client_secret': oauth_secret,
            'grant_type': 'password',
            'username': username,
            'password': password,
        },
        options=options,
    )


def get_token_by_sessionid(
        oauth_id, oauth_secret,
        sessionid, host=None, options=None):
    """
    Токен по паспортной куке sessionid
    https://wiki.yandex-team.ru/oauth/token/#granttypesessionid
    """
    host = host or 'yandex-team.ru'
    return _get_token(
        oauth_params={
            'client_id': oauth_id,
            'client_secret': oauth_secret,
            'grant_type': 'sessionid',
            'sessionid': sessionid,
            'host': host,
        },
        options=options,
    )


def get_token_by_token(
        oauth_id, oauth_secret,
        token, options=None):
    """
    Токен по другому токену пользователя.
    https://wiki.yandex-team.ru/oauth/token/#granttypex-token
    """
    return _get_token(
        oauth_params={
            'client_id': oauth_id,
            'client_secret': oauth_secret,
            'grant_type': 'x-token',
            'access_token': token,
        },
        options=options,
    )


def get_token_by_code(
        oauth_id, oauth_secret,
        code, options=None):
    """
    Токен по коду подтверждения
    https://tech.yandex.ru/oauth/doc/dg/reference/console-client-docpage/#get-token
    """
    return _get_token(
        oauth_params={
            'client_id': oauth_id,
            'client_secret': oauth_secret,
            'grant_type': 'authorization_code',
            'code': code,
        },
        options=options,
    )


def _get_token(oauth_params, options=None):
    options = options or {}
    connector = OauthConnector()
    response = connector.post(
        resource='token',
        data=oauth_params,
        **options
    )
    return response['access_token']
