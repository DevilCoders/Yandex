# coding: utf-8

from __future__ import unicode_literals

from cached_property import cached_property
from django.conf import ImproperlyConfigured


class CommonCIASettings(object):

    def __init__(self, configured=None):
        self.configured = configured or {}
        self.ds = self.get_configured_setting('ds')

    # django_db
    DATABASE_WAIT_TIMEOUT = 60 * 60 * 24 * 31  # 1 месяц
    DATABASE_CONN_MAX_AGE = 60 * 15  # 15 минут
    DATABASE_INIT_COMMAND = 'SET default_storage_engine=INNODB; SET names utf8mb4; SET wait_timeout=%s;' % DATABASE_WAIT_TIMEOUT
    DATABASE_ENGINE = 'django.db.backends.mysql'
    DATABASE_ROUTERS = ['django_replicated.router.ReplicationRouter']

    def get_configured_setting(self, name):
        if name not in self.configured:
            raise ImproperlyConfigured(
                'setting %s should be set before COMMON_SETTINGS initialization'
                % name
            )
        return self.configured[name]

    @cached_property
    def db_aliases(self):
        aliases = ['default']
        possible_slave_names = (
            'slave',
            'slave1',
            'slave2',
        )
        seen_alias_data = []
        for slave_name in possible_slave_names:
            # не добавляем по второму разу тот же слейв
            alias_data = self.get_alias_data(slave_name)
            if alias_data in seen_alias_data:
                continue
            if alias_data is not None:
                aliases.append(slave_name)
                seen_alias_data.append(alias_data)
        return aliases

    def get_alias_data(self, alias):
        db_alias_in_datasources = {
            'default': 'master'
        }.get(alias, alias)
        host = getattr(self.ds, 'database_host_' + db_alias_in_datasources, None)
        port = getattr(self.ds, 'database_port_' + db_alias_in_datasources, '3306')
        if host is None:
            return
        return {
            'host': host,
            'port': port,
        }

    @property
    def DATABASES(self):
        return {
            alias: self.get_alias_config(alias)
            for alias in self.db_aliases
        }

    def get_alias_config(self, alias):
        alias_data = self.get_alias_data(alias)
        return {
            'ENGINE': self.DATABASE_ENGINE,
            'NAME': self.ds.database_db,
            'USER': self.ds.database_user,
            'PASSWORD': self.ds.database_password,
            'CONN_MAX_AGE': self.DATABASE_CONN_MAX_AGE,
            'HOST': alias_data['host'],
            'PORT': alias_data['port'],
            'TEST': {
                'CHARSET': 'utf8',
                'COLLATION': 'utf8_general_ci',
            },
            'OPTIONS': {
                'init_command': self.DATABASE_INIT_COMMAND,
                'connect_timeout': 2,
            },
        }

    # django_replicated
    REPLICATED_DATABASE_DOWNTIME = 0  # CIA-692
    REPLICATED_CHECK_STATE_ON_WRITE = False

    @property
    def REPLICATED_DATABASE_SLAVES(self):
        return [
            alias for alias in self.db_aliases
            if alias.startswith('slave')
        ]

    # django_alive
    ALIVE_CHECK_TIMEOUT = 60 + 5
    ALIVE_MIDDLEWARE_REDUCE = 'cia_stuff.common.alive.check_http_and_db_master_availability'

    @property
    def ALIVE_CONF(self):
        host = self.get_configured_setting('HOST')
        aliases = self.db_aliases
        conf = {
            'stampers': {
                'db': {
                    'class': 'django_alive.stampers.db.DBStamper',
                },
            },
            'checks': {
                'celery': {
                    'class': 'django_alive.checkers.celery.CeleryWorkerChecker',
                    'stamper': 'db',
                    'queue': 'celery@%h.dq',
                },
                'celery-beat': {
                    'class': 'django_alive.checkers.celery.CeleryBeatChecker',
                    'stamper': 'db',
                    'queue': 'celery@%h.dq',
                },
                'www': {
                    'class': 'django_alive.checkers.http.HTTPChecker',
                    'stamper': 'db',
                    'url': 'https://localhost/',
                    'headers': {
                        'Host': host,
                    },
                    'timeout': 1,
                    'ok_statuses': [200, 302],
                    'method': 'GET',
                },
            },
            'groups': {
                'front': [
                    'www',
                ],
                'gen': [
                    'celery',
                    'celery-beat',
                ],
            }
        }

        db_alias_in_alive_map = {
            'default': 'db-master',
            'slave': 'db-slave',
            'slave1': 'db-slave1',
            'slave2': 'db-slave2',
        }
        for alias in aliases:
            db_alias_in_alive = db_alias_in_alive_map.get(alias)
            conf['checks'][db_alias_in_alive] = {
                'class': 'django_alive.checkers.db.DBChecker',
                'stamper': 'db',
                'alias': alias,
            }
            conf['groups']['front'].append(db_alias_in_alive)
            conf['groups']['gen'].append(db_alias_in_alive)

        return conf


def get_settings(configured=None):
    return CommonCIASettings(configured)
