
import logging

import requests

from django.conf import settings
from django.core.cache import cache


log = logging.getLogger(__name__)


class FakeCIA(object):
    def __init__(self, base_url, cert, verify):
        self.base_url = base_url
        self.cert = cert
        self.verify = verify

    def get_roles(self, user):
        return []


class CIA(object):

    CACHE_PREFIX = 'cia_roles_'
    CACHE_TIMEOUT = 60 * 10

    def __init__(self, base_url, headers):
        self.base_url = base_url
        self.headers = headers

    def _cache_key(self, login):
        return self.CACHE_PREFIX + login

    def _merge_roles(self, roles):
        for role, data in roles:
            role.update(data)
            yield role

    def get_roles(self, user):
        cache_key = self._cache_key(user.login)
        roles = cache.get(cache_key)
        if roles is not None:
            return roles

        url = self.base_url + 'api/get-user-roles/'
        try:
            response = requests.get(
                url,
                params={'login': user.login},
                headers=self.headers,
                timeout=3,
                verify=False,
            )
        except requests.RequestException:
            log.exception('CIA request failed')
            return []
        if not 200 <= response.status_code < 300:
            log.exception('CIA bad response code %s', response.status_code)
            return []
        data = response.json()

        roles = list(self._merge_roles(data['roles']))
        cache.set(cache_key, roles, self.CACHE_TIMEOUT)
        return roles


def get_cia_init_params():
    return {
        'base_url': settings.CIA_BASE_URL,
        'headers': {'Authorization': 'OAuth {token}'.format(token=settings.ROBOT_TOKEN)},
    }


def get_cia_api():
    if settings.USE_FAKE_CIA:
        return FakeCIA('', None, None)
    else:
        return CIA(**get_cia_init_params())
