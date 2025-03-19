import logging
import re
import socket
from datetime import timedelta
from typing import MutableMapping

import requests
from copy import deepcopy

import sqlparse

from cloud.mdb.clickhouse.tools.common.utils import version_ge
from jinja2 import Environment

import tenacity
import xmltodict


def retry(exception_types, max_attempts=5, max_interval=5):
    """
    Function decorator that retries wrapped function on failures.
    """
    return tenacity.retry(
        retry=tenacity.retry_if_exception_type(exception_types),
        wait=tenacity.wait_random_exponential(multiplier=0.5, max=max_interval),
        stop=tenacity.stop_after_attempt(max_attempts),
        reraise=True,
    )


class ClickhouseError(Exception):
    """
    ClickHouse interaction error.
    """

    def __init__(self, query, response):
        self.query = re.sub(r'\s*\n\s*', ' ', query.strip())
        self.response = response
        super().__init__(f'{self.response.text.strip()}\n\nQuery: {self.query}')


class ClickhouseClient:
    """
    ClickHouse client.
    """

    def __init__(self, *, host=socket.getfqdn(), port=8443, user='_admin', insecure=False):
        self._session = requests.Session()
        self._session.verify = False if insecure else '/etc/clickhouse-server/ssl/allCAs.pem'
        if user:
            self._session.headers['X-ClickHouse-User'] = user
        self._url = f'https://{host}:{port}'
        self._timeout = 60
        self._ch_version = None

    def get_clickhouse_version(self):
        """
        Get ClickHouse server version.
        """
        if self._ch_version is None:
            self._ch_version = self.query('SELECT version()')

        return self._ch_version

    def get_uptime(self):
        """
        Get uptime of ClickHouse server.
        """
        seconds = int(self.query('SELECT uptime()'))
        return timedelta(seconds=seconds)

    @retry(requests.exceptions.ConnectionError)
    def query(self, query, query_args=None, format=None, post_data=None, timeout=None, echo=False, dry_run=False):
        """
        Execute query.
        """
        if query_args:
            query = self.render_query(query, **query_args)

        if format:
            query += f' FORMAT {format}'

        if timeout is None:
            timeout = self._timeout

        if echo:
            print(sqlparse.format(query, reindent=True), '\n')

        if dry_run:
            return None

        logging.debug('Executing query: %s', query)
        try:
            response = self._session.post(
                self._url,
                params={
                    'query': query,
                },
                json=post_data,
                timeout=timeout,
            )

            response.raise_for_status()

            if format in ('JSON', 'JSONCompact'):
                return response.json()

            return response.text.strip()
        except requests.exceptions.HTTPError as e:
            raise ClickhouseError(query, e.response) from None

    def render_query(self, query, **kwargs):
        env = Environment()

        env.globals['version_ge'] = lambda version: version_ge(self.get_clickhouse_version(), version)
        env.globals['format_str_match'] = _format_str_match
        env.globals['format_str_imatch'] = _format_str_imatch

        template = env.from_string(query)
        return template.render(kwargs)


class ClickhouseZookeeperConfig:
    """
    ZooKeeper section of ClickHouse server config.
    """

    def __init__(self, config):
        self._config = config

    @property
    def nodes(self):
        value = self._config['node']
        if isinstance(value, list):
            return value

        return [value]

    @property
    def root(self):
        return self._config.get('root')

    @property
    def identity(self):
        return self._config.get('identity')


class ClickhouseConfig:
    """
    ClickHouse server config (config.xml).
    """

    def __init__(self, config):
        self._config = config

    @property
    def macros(self):
        """
        ClickHouse macros.
        """
        macros = self._config['yandex'].get('macros', {})
        return {key: value for key, value in macros.items() if not key.startswith('@')}

    @property
    def cluster_name(self):
        return self.macros['cluster']

    @property
    def zookeeper(self) -> ClickhouseZookeeperConfig:
        """
        ZooKeeper configuration.
        """
        return ClickhouseZookeeperConfig(self._config['yandex'].get('zookeeper', {}))

    def has_disk(self, name):
        storage_configuration = self._config['yandex'].get('storage_configuration', {})
        return name in storage_configuration.get('disks', {})

    def dump(self, mask_secrets=True):
        config = deepcopy(self._config)
        if mask_secrets:
            _mask_secrets(config)

        return xmltodict.unparse(config, pretty=True)

    def keeper_port(self):
        return self._config.get('clickhouse', self._config.get('yandex', {})).get('keeper_server', {}).get('tcp_port')

    @staticmethod
    def load():
        return ClickhouseConfig(_load_config('/var/lib/clickhouse/preprocessed_configs/config.xml'))


class ClickhouseUsersConfig:
    """
    ClickHouse users config (users.xml).
    """

    def __init__(self, config):
        self._config = config

    def dump(self, mask_secrets=True):
        config = deepcopy(self._config)
        if mask_secrets:
            _mask_secrets(config)

        return xmltodict.unparse(config, pretty=True)

    @staticmethod
    def load():
        return ClickhouseConfig(_load_config('/var/lib/clickhouse/preprocessed_configs/users.xml'))


def _load_config(config_path):
    with open(config_path, 'r') as file:
        return xmltodict.parse(file.read())


def _mask_secrets(config):
    if isinstance(config, MutableMapping):
        for key, value in list(config.items()):
            if isinstance(value, MutableMapping):
                _mask_secrets(config[key])
            elif key in ('password', 'secret_access_key', 'header', 'identity'):
                config[key] = '*****'


def _format_str_match(value):
    if value is None:
        return None

    if value.find(',') < 0:
        return "LIKE '{0}'".format(value)

    return "IN ({0})".format(','.join("'{0}'".format(item.strip()) for item in value.split(',')))


def _format_str_imatch(value):
    if value is None:
        return None

    return _format_str_match(value.lower())
