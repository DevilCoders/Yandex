# coding: utf-8

from __future__ import unicode_literals

import logging
import six
import abc
import blackbox

from ylog.context import log_context
from tvmauth import BlackboxEnv, BlackboxTvmId as BlackboxClientId

from .exceptions import NotAllRequiredKwargsPassed


log = logging.getLogger(__name__)

BLACKBOX_MAP = {
    BlackboxClientId.Prod: {'url': 'http://blackbox.yandex.net/blackbox',
                            'env': BlackboxEnv.Prod, },
    BlackboxClientId.Mimino: {'url': 'http://blackbox-mimino.yandex.net/blackbox',
                              'env': BlackboxEnv.Prod, },
    BlackboxClientId.Test: {'url': 'http://pass-test.yandex.ru/blackbox',
                            'env': BlackboxEnv.Test, },
    BlackboxClientId.ProdYateam: {'url': 'http://blackbox.yandex-team.ru/blackbox',
                                  'env': BlackboxEnv.ProdYateam, },
    BlackboxClientId.TestYateam: {'url': 'http://blackbox-test.yandex-team.ru/blackbox',
                                  'env': BlackboxEnv.TestYateam, },
    BlackboxClientId.Stress: {'url': 'http://pass-stress-i1.sezam.yandex.net/blackbox',
                              'env': BlackboxEnv.Stress, },
}

BLACKBOX_BY_NAME_MAP = {
    BlackboxClientId.Prod.name: {'url': 'http://blackbox.yandex.net/blackbox',
                            'env': BlackboxEnv.Prod, },
    BlackboxClientId.Mimino.name: {'url': 'http://blackbox-mimino.yandex.net/blackbox',
                              'env': BlackboxEnv.Prod, },
    BlackboxClientId.Test.name: {'url': 'http://pass-test.yandex.ru/blackbox',
                            'env': BlackboxEnv.Test, },
    BlackboxClientId.ProdYateam.name: {'url': 'http://blackbox.yandex-team.ru/blackbox',
                                  'env': BlackboxEnv.ProdYateam, },
    BlackboxClientId.TestYateam.name: {'url': 'http://blackbox-test.yandex-team.ru/blackbox',
                                  'env': BlackboxEnv.TestYateam, },
    BlackboxClientId.Stress.name: {'url': 'http://pass-stress-i1.sezam.yandex.net/blackbox',
                              'env': BlackboxEnv.Stress, },
}

TIROLE_HOST = 'https://tirole-api.yandex.net'
TIROLE_HOST_TEST = 'https://tirole-api-test.yandex.net'
TIROLE_PORT = 443
TIROLE_CHECK_SRC_DEFAULT = False
TIROLE_CHECK_UID_DEFAULT = False
TIROLE_ENV_DEFAULT = 'production'


class _Singleton(type):
    _instance = None

    def __call__(cls, *args, **kwargs):
        if cls._instance is None:
            cls._instance = super(_Singleton, cls).__call__(*args, **kwargs)
        else:
            allowed_clients = kwargs.get('allowed_clients')
            if (allowed_clients and
                        set(allowed_clients) != cls._instance.allowed_clients):
                cls._instance.allowed_clients.update(allowed_clients)
        return cls._instance


@six.add_metaclass(_Singleton)
@six.add_metaclass(abc.ABCMeta)
class BaseTVM2(object):
    REQUIRED_KWARGS = {}
    DEFAULT_TVM_API = 'https://tvm-api.yandex.net'

    def __init__(self, client_id, blackbox_client, allowed_clients=None,
                 api_url=None, retries=3, **kwargs):

        self.client_id = int(client_id)
        self.allowed_clients = set(allowed_clients or tuple())
        self.blackbox_client = blackbox_client
        blackbox_url, blackbox_env = self.get_blackbox_data(self.blackbox_client)
        self.blackbox_url = blackbox_url
        self.blackbox_env = blackbox_env
        self.api_url = self._get_api_url(api_url)
        self.retries = retries

        if any(kwargs.get(param) is None for param in self.REQUIRED_KWARGS):
            raise NotAllRequiredKwargsPassed

        self._init_data(kwargs)

    def _get_api_url(self, url):
        return url or self.DEFAULT_TVM_API

    def get_blackbox_data(self, blackbox_env):
        current_client_data = BLACKBOX_BY_NAME_MAP[blackbox_env.name]
        return current_client_data['url'], current_client_data['env']

    @property
    def blackbox(self):
        return blackbox.JsonBlackbox(url=self.blackbox_url)

    def get_service_ticket(self, destination, **kwargs):
        return self.get_service_tickets(destination, **kwargs).get(destination)

    def get_user_ticket(self, user_ip, server_host, oauth_token=None, session_id=None):
        """
        Получает в blackbox и возвращает персонализированный тикет
        """
        if not session_id and not oauth_token:
            log.warning("Either session cookies or oauth_token should be specified")
            return

        blackbox_client_id = self.blackbox_client.value
        with log_context(action='get_user_ticket', blackbox_client_value=repr(blackbox_client_id)):
            service_tickets = self.get_service_tickets(blackbox_client_id)
            ticket_for_blackbox = service_tickets.get(blackbox_client_id)
            if ticket_for_blackbox is None:
                log.error('Don\'t get service ticket for blackbox')
                return
            blackbox_instance = self.blackbox
            try:
                if session_id:
                    response = blackbox_instance.sessionid(
                        userip=user_ip,
                        sessionid=session_id,
                        host=server_host,
                        get_user_ticket='yes',
                        headers={'X-Ya-Service-Ticket': ticket_for_blackbox}
                    )
                elif oauth_token:
                    response = blackbox_instance.oauth(
                        oauth_token=oauth_token,
                        userip=user_ip,
                        get_user_ticket='yes',
                        headers={'X-Ya-Service-Ticket': ticket_for_blackbox}
                    )

                if 'error' in response and response['error'] != 'OK':
                    log.warning('Blackbox responded with error: %s. Request was from ip %s, host %s.',
                                repr(response['error']), user_ip, server_host)
                    return
            except blackbox.BlackboxError:
                log.warning('Blackbox unavailable', exc_info=True)
                return
            ticket = response.get('user_ticket')
            if ticket is None:
                log.error('Don\'t get user ticket in blackbox response')
                return
            return ticket

    def _init_data(self, data):
        raise NotImplementedError()
