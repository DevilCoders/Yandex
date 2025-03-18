# coding: utf-8
from __future__ import unicode_literals

import six
import hashlib
import hmac
from base64 import urlsafe_b64decode, urlsafe_b64encode
from time import time

from ids import connector
from ids import exceptions


class TvmConnector(connector.HttpConnector):

    service_name = 'TVM'

    url_patterns = {
        'ticket': '/ticket/'
    }

    def __init__(self, host=None, protocol=None, **kwargs):
        super(TvmConnector, self).__init__(
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


def get_ticket_by_client_credentials(tvm_client_id, tvm_secret, options=None, host=None):
    """
    Получить тикет по идентификатору приложения
    https://wiki.yandex-team.ru/passport/tvm/api/#poluchenietiketapoidentifikatoruprilozhenija
    """
    return _get_ticket(
        tvm_params={
            'client_id': tvm_client_id,
            'grant_type': 'client_credentials',
        },
        tvm_secret=tvm_secret,
        options=options,
        host=host,
    )


def _get_ticket(tvm_params, tvm_secret, options=None, host=None):
    # https://wiki.yandex-team.ru/passport/tvm/api/#primerpoluchenijapodpisi
    padded_secret = tvm_secret + '=' * (-len(tvm_secret) % 4)
    decoded_secret = urlsafe_b64decode(str(padded_secret))
    ts = int(time())
    ts_sign = urlsafe_b64encode(
        hmac.new(
            key=decoded_secret,
            msg=six.b(str(ts)),
            digestmod=hashlib.sha256,
        ).digest(),
    ).decode('utf-8').strip('=')

    tvm_params['ts'] = ts
    tvm_params['ts_sign'] = ts_sign

    options = options or {}
    connector = TvmConnector(host=host)
    response = connector.post(
        resource='ticket',
        data=tvm_params,
        **options
    )
    return response.text
