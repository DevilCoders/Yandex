# encoding: utf-8

import json
import logging
from six.moves import urllib

from ids import exceptions
from ids.connector import HttpConnector
from ids.connector import plugins_lib

from startrek_client.connection import (
    bind_method, decode_response, encode_resource
)
from startrek_client.exceptions import Conflict

logger = logging.getLogger(__name__)


class StTvm(plugins_lib.Tvm):
    header_name = 'X-TVM-Ticket'


class ConflictMonster(Conflict, exceptions.BackendError):
    """Исключение Conflict которое будет поймано и startrek_client и IDS"""

    def __init__(self, *args, **kwargs):
        response = kwargs['response']
        try:
            error = response.json()
        except Exception:
            pass
        else:
            self.errors = error.get('errors')
            self.error_messages = error.get('errorMessages')
        exceptions.BackendError.__init__(self, *args, **kwargs)


class StConnector(HttpConnector):
    service_name = 'STARTREK_API'
    plugins = [
        plugins_lib.get_disjunctive_plugin_chain(
            [
                plugins_lib.OAuth, StTvm,
                plugins_lib.TVM2UserTicket, plugins_lib.TVM2ServiceTicket,
            ]
        )
    ]
    default_connect_timeout = 4

    def __init__(self, *args, **kwargs):
        timeout = kwargs.pop('timeout', None)
        if timeout is not None:
            kwargs['timeout'] = timeout

        super(StConnector, self).__init__(*args, **kwargs)

    def build_url(self, resource, url_vars):
        return '%s://%s%s' % (self.protocol, self.host, resource)

    def prepare_params(self, params):
        version = params.pop('version', None)
        data = params.get('data')

        headers = params.get('headers', {})
        org_id = getattr(self, 'org_id', None)
        if org_id is not None:
            headers['X-Org-ID'] = org_id

        if version is not None:
            headers['If-Match'] = '"{}"'.format(version)

        params['headers'] = headers

        if data is not None:
            params['data'] = json.dumps(data, default=encode_resource)

    def request(self, method, path, params=None, **kw):
        return self.execute_request(method=method, resource=path, params=params, **kw)

    def handle_response(self, response):
        if 'application/json' in response.headers.get('content-type', ''):
            return decode_response(response, self)
        return response

    def handle_bad_response(self, response):
        """
        https://beta.wiki.yandex-team.ru/tracker/api/#oshibki
        """
        status_code = response.status_code
        try:
            errors = json.loads(response.text)
        except ValueError:
            # в случае, если организация отключена, то ответ от ручки будет без
            # json, от nginx: 403 Forbidden, поэтому не стоит рассматривать это как
            # exception
            if status_code != 403:
                logger.exception('Error while deserializing startrek errors')
            errors = {}

        exc_cls = exceptions.BackendError

        if status_code == 400:
            message = 'Invalid JSON data was sent'
        elif status_code == 403:
            message = 'Lack of permissions'
        elif status_code == 409:
            # Логика обработки этого исключения на стороне startrek_client
            # лучше решения придумать не удалось
            # Тащить исключения ids в startrek_client не имеет смысла.
            message = 'Conflict'
            exc_cls = ConflictMonster
        elif status_code == 412:
            message = 'Invalid values was sent'
        else:
            message = 'Backend responded with {code} ({reason})'.format(
                code=response.status_code,
                reason=response.reason,
            )

        startrek_error_messages = ', '.join(errors.get('errorMessages', []))
        if startrek_error_messages:
            message += '. ' + startrek_error_messages

        raise exc_cls(message, response=response, extra=errors)

    get = bind_method('GET')
    put = bind_method('PUT')
    post = bind_method('POST')
    patch = bind_method('PATCH')
    create = bind_method('CREATE')
    delete = bind_method('DELETE')
    unlink = bind_method('UNLINK')

    def link(self, path, target_url, rel, params=None, version=None, **kw):
        link = '<{target_url}>; rel="{rel}"'.format(target_url=target_url, rel=rel)

        return self.execute_request(
            method='LINK',
            resource=path,
            headers={'Link': link},
            version=version,
            params=params,
            **kw
        )

    def stream(self, path, params=None, **kw):
        path = urllib.parse(path).path
        return self.execute_request(method='GET', resource=path, stream=True, params=params, **kw)
