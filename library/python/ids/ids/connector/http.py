# coding: utf-8

from __future__ import unicode_literals
import copy
import logging

from ids import exceptions
from ids.configuration import get_config
from ids.utils import https

logger = logging.getLogger(__name__)


def _method(name):
    def method(self, *args, **kws):
        return self.execute_request(name, *args, **kws)

    method.func_name = str(name.lower())
    return method


class HttpConnector(object):

    default_connect_timeout = 2

    statuses_to_retry = {500, 499}

    # name for ids.configuration lookup
    service_name = 'SERVICE_NAME'

    # something you want to add in front of every url pattern
    url_prefix = ''

    # something like {'user_list': 'users/', 'group_list': 'groups/'}
    url_patterns = {}

    # обязательные аргументы для __init__
    required_params = ()

    # классы коннекторных плагинов
    plugins = ()

    def __init__(self, host=None, protocol=None, **kwargs):
        self.session = https.get_secure_session()

        self.user_agent = kwargs.pop('user_agent', None)
        if not self.user_agent:
            raise exceptions.IDSException('"user_agent" option is required')

        self.protocol = protocol or self.config['protocol']
        self.host = host or self.config['host']
        self.timeout = kwargs.pop('timeout', self.default_connect_timeout)
        self.retries = kwargs.pop('retries', 1)
        self.retry_on_status = kwargs.pop('retry_on_status', False)
        self.statuses_to_retry = kwargs.pop(
            'statuses_to_retry',
            self.statuses_to_retry,
        )
        self._plugins = None

        self.check_required_params(kwargs)
        self.handle_init_params(kwargs)

    def handle_init_params(self, kwargs):
        """
        Если поведение с присваиванием не устраивает, можно переопределить
        метод.
        """
        for key, value in kwargs.items():
            setattr(self, key, value)

    @property
    def config(self):
        return get_config(self.service_name)

    def get_plugins(self):
        if self._plugins is None:
            self._plugins = [plugin(connector=self) for plugin in self.plugins]
        return self._plugins

    def check_required_params(self, params):
        not_presented = self.get_required_not_presented(
            required=self.collect_required_params(),
            presented=params,
        )
        if not_presented:
            msg = 'Required params %s not presented' % not_presented
            raise exceptions.ConfigurationError(msg)

        for plugin in self.get_plugins():
            if hasattr(plugin, 'check_required_params'):
                plugin.check_required_params(params)

    def collect_required_params(self):
        """
        Собрать параметры с учетом объявленных в плагинах
        """
        params = set(self.required_params)
        for plugin in self.get_plugins():
            if hasattr(plugin, 'required_params'):
                params |= set(plugin.required_params)
        return params

    @staticmethod
    def get_required_not_presented(required, presented):
        return set(required) - set(presented)

    def build_url(self, resource=None, url_vars=None):
        """
        @param resource: Имя ресурса.
        @param url_vars: Дикт для форматирования url_pattern'а
        @return: Урл для запроса.
        @todo: поддержать возможность параметризации урлпаттерна, когда будет
        нужно

        """
        if url_vars is None:
            url_vars = {}

        resource_url = self.get_resource_pattern(resource).format(**url_vars)
        query_params = self.get_query_params(resource, url_vars)
        url = '%(protocol)s://%(host)s%(resource_url)s' % {
            'protocol': self.protocol,
            'host': self.host,
            'resource_url': resource_url,
        }
        if query_params:
            if '?' not in url:
                url += '?'
            else:
                url += '&'
            url += '&'.join(
                '%s=%s' % key_val
                for key_val in query_params.items()
            )

        logger.debug(url)
        return url

    def get_resource_pattern(self, resource):
        """
        @param resource: Имя ресурса для запроса.
        @return: Паттерн для ресурса.
        """
        return self.url_prefix + self.url_patterns.get(resource, '')

    def get_query_params(self, resource, url_vars):
        """
        Хук для переопределения query-параметров запроса
        """
        return {}

    def execute_request(self, method='get', resource=None,
                        url_vars=None, **params):
        # можно переопределить таймаут для конкретного запроса
        params['timeout'] = params.get('timeout', self.timeout)
        self._add_user_agent(params)
        self._prepare_params(params)

        url = self.build_url(resource=resource, url_vars=url_vars)

        response = self._try_request(method, url, **params)
        return self._handle_response(response)

    @staticmethod
    def _dump_request_params(params):
        if 'Authorization' in params.get('headers', {}):
            params = copy.copy(params)
            params['headers'] = copy.copy(params['headers'])
            params['headers']['Authorization'] = '*****'
        return str(params)

    def _try_request(self, method, url, **params):
        retries = params.pop('retries', self.retries)
        if retries < 1:
            retries = 1
        exception = None
        for attempt in range(retries):
            try:
                response = self.session.request(method.upper(), url, **params)
                logger.debug('HTTP request: %s %s', url, self._dump_request_params(params))
                if self.retry_on_status and response.status_code in self.statuses_to_retry:
                    logger.warning(msg=response.text)
                    continue
                break
            except exceptions.REQUESTS_ERRORS as exc:
                logger.exception('HTTP error (%d): %s, "%s"', attempt, url, exc)
                exception = exc
        else:
            raise exceptions.BackendError(exception)

        if response.status_code > 299:
            logger.warning(msg=response.text)
            self.handle_bad_response(response)

        return response

    get = _method('GET')
    put = _method('PUT')
    post = _method('POST')
    patch = _method('PATCH')
    create = _method('CREATE')
    delete = _method('DELETE')

    def _prepare_params(self, params):
        for plugin in self.get_plugins():
            if hasattr(plugin, 'prepare_params'):
                plugin.prepare_params(params)

        self.prepare_params(params)

    def prepare_params(self, params):
        """
        Хук для обработки параметров перед запросом
        @param params: dict, параметры, переданые для запроса
        @return: None
        @todo: Может быть нужно передать и остальные параметры из execute.
        """
        pass

    def _add_user_agent(self, params):
        headers = params.get('headers', {})
        headers['User-Agent'] = self.user_agent
        params['headers'] = headers

    def _handle_response(self, response):
        for plugin in self.get_plugins():
            if hasattr(plugin, 'handle_response'):
                response = plugin.handle_response(response)

        response = self.handle_response(response)
        return response

    def handle_response(self, response):
        """
        Хук для обработки requests.Response
        например можно возвращать response.json
        """
        return response

    def handle_bad_response(self, response):
        """
        Бросить нужные исключения.
        Можно переопределить в наследнике для более детального
        разбора ответа
        """
        status_code = response.status_code
        if status_code == 403:
            raise exceptions.AuthError('Check auth token', response=response)

        raise exceptions.BackendError(
            'Backend responded with ' + str(status_code),
            response=response,
        )
