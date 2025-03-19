# -*- coding: utf-8 -*-
"""
MDB ClickHouse module.
"""

from __future__ import division
from __future__ import unicode_literals

import json
import logging
import os.path
import re

import math
import time
from collections import OrderedDict
from datetime import timedelta
from re import search
from xml.dom import minidom
from xml.sax import saxutils

try:
    from urllib.parse import urlparse
except ImportError:
    from urlparse import urlparse

try:
    import six
except ImportError:
    from salt.ext import six

__salt__ = {}

_default = object()
log = logging.getLogger(__name__)

# The same constants is in dbaas-internal-api
ZK_ACL_USER_CLICKHOUSE = 'clickhouse'
MDB_BACKUP_ADMIN_USER = 'mdb_backup_admin'
MDB_BACKUP_ADMIN_USER_NAME = '_backup_admin'


def __virtual__():
    return True


# Path to root of cached in s3 objects
OBJECT_CACHE_ROOT = 'object_cache'

# 1k block count equal to 20GB
BLOCK_20GB = '20510288'

# Same as in s3_credentials ch tool
DEFAULT_S3_CONFIG_PATH = '/etc/clickhouse-server/config.d/s3_credentials.xml'

# User settings with system-wide effect. These settings must be specified for system profile.
SYSTEM_PROFILE_SETTINGS = [
    'background_pool_size',
    'background_fetches_pool_size',
    'background_schedule_pool_size',
    'background_move_pool_size',
    'background_distributed_schedule_pool_size',
    'background_buffer_flush_schedule_pool_size',
]

# Settings that are not mapped as is to ClickHouse settings.
CUSTOM_SETTINGS = SYSTEM_PROFILE_SETTINGS + [
    'geobase_uri',
    'log_level',
    'ssl_client_verification_mode',
    'query_log_retention_size',
    'query_log_retention_time',
    'query_thread_log_enabled',
    'query_thread_log_retention_size',
    'query_thread_log_retention_time',
    'part_log_retention_size',
    'part_log_retention_time',
    'metric_log_enabled',
    'metric_log_retention_size',
    'metric_log_retention_time',
    'trace_log_enabled',
    'trace_log_retention_size',
    'trace_log_retention_time',
    'text_log_enabled',
    'text_log_retention_size',
    'text_log_retention_time',
    'text_log_level',
    'opentelemetry_span_log_enabled',
]


class ClickhouseError(Exception):
    def __init__(self, query, response):
        message = response.text.strip() + '\n\nQuery: ' + re.sub(r'\s*\n\s*', ' ', query.strip())
        super(Exception, self).__init__(message)


class ClickhouseGrant:
    """
    Stores user grants information. Support two modes:
    is_partial_revoke == False: GRANTS to target databases without REVOKE's
    is_partial_revoke == True: GRANTS to *.* then REVOKES for target databases
    """

    def __init__(self, access_type, is_partial_revoke, grant_option, target_databases=None):
        self.access_type = access_type
        self.databases = target_databases or []
        self.is_partial_revoke = is_partial_revoke
        self.grant_option = grant_option

    def __eq__(self, other):
        if not isinstance(other, ClickhouseGrant):
            return False
        return self._as_tuple() == other._as_tuple()

    def __repr__(self):
        return 'ClickhouseGrant({0})'.format(', '.join(repr(item) for item in self._as_tuple()))

    def _as_tuple(self):
        return self.access_type, self.databases, self.is_partial_revoke, self.grant_option

    def grant_statements(self, user):
        statements = []
        if not self.databases:
            statements.append(
                "GRANT {access_type} ON *.* TO '{user}'{grant_option};".format(
                    access_type=self.access_type,
                    user=user,
                    grant_option=" WITH GRANT OPTION" if self.grant_option else "",
                )
            )
        else:
            if self.is_partial_revoke:
                statements.append(
                    "GRANT {access_type} ON *.* TO '{user}'{grant_option};".format(
                        access_type=self.access_type,
                        user=user,
                        grant_option=" WITH GRANT OPTION" if self.grant_option else "",
                    )
                )
                statements.extend(
                    "REVOKE {access_type} ON `{database}`.* FROM '{user}';".format(
                        access_type=self.access_type, database=db, user=user
                    )
                    for db in self.databases
                )
            else:
                statements.append(
                    "REVOKE {access_type} ON *.* FROM '{user}';".format(access_type=self.access_type, user=user)
                )
                statements.extend(
                    "GRANT {access_type} ON `{database}`.* TO '{user}'{grant_option};".format(
                        access_type=self.access_type,
                        database=db,
                        user=user,
                        grant_option=" WITH GRANT OPTION" if self.grant_option else "",
                    )
                    for db in self.databases
                )

        return statements

    def revoke_statement(self, user):
        return "REVOKE {access_type} ON *.* FROM '{user}';".format(access_type=self.access_type, user=user)

    def compare_with_existing(self, grants):
        """
        Compare target grants with present grants list.
        If changes needed returns reason to simplify debug.
        """
        if len(grants) == 0:
            return 'not exists'

        # we query grants with partial_revoke sort, so grants must be first
        is_partial_revoke = grants[-1]['is_partial_revoke']
        if is_partial_revoke != self.is_partial_revoke:
            return 'partial_revoke mismatch. Expected: {}, got: {}'.format(self.is_partial_revoke, is_partial_revoke)

        if is_partial_revoke:
            if any(map(lambda g: g['is_partial_revoke'] != 1, grants[1:])):
                return 'invalid grants or mixed grants/revokes'
        else:
            if any(map(lambda g: g['is_partial_revoke'] != 0, grants)):
                return 'mixed partial grants and revokes'

        for grant in grants:
            # TODO: do not ignore the case when GRANT OPTION is not expected
            if grant['is_partial_revoke'] == 0 and self.grant_option and not grant['grant_option']:
                return 'grant option mismatch. Expected: {}, got: {}'.format(
                    self.grant_option, bool(grant['grant_option'])
                )

        target_databases = self.databases
        present_databases = [grant['database'] for grant in grants if grant['database']]
        if set(target_databases) != set(present_databases):
            return 'databases mismatch. Expected: [{}], got: [{}]'.format(
                ', '.join(map(lambda v: str(v), target_databases)), ', '.join(map(lambda v: str(v), present_databases))
            )

        return None


def grant(access_type, target_databases=None, grant_option=False):
    return ClickhouseGrant(
        access_type=access_type, target_databases=target_databases, is_partial_revoke=False, grant_option=grant_option
    )


def grant_except_databases(access_type, except_databases=None, grant_option=False):
    return ClickhouseGrant(
        access_type=access_type, target_databases=except_databases, is_partial_revoke=True, grant_option=grant_option
    )


SYSTEM_DATABASES = ['system', 'information_schema', 'INFORMATION_SCHEMA']
TMP_DATABASE = '_temporary_and_external_tables'
MDB_SYSTEM_DATABASE = '_system'

# Permissions that must be granted to all users creating through API.
USER_COMMON_GRANT_LIST = [
    grant('SHOW'),
    grant('SHOW ACCESS'),
    grant('CREATE TEMPORARY TABLE'),
    grant('KILL QUERY'),
    grant('SYSTEM DROP CACHE'),
    grant('SYSTEM RELOAD'),
    grant('SYSTEM MERGES'),
    grant('SYSTEM TTL MERGES'),
    grant('SYSTEM FETCHES'),
    grant('SYSTEM MOVES'),
    grant('SYSTEM SENDS'),
    grant('SYSTEM REPLICATION QUEUES'),
    grant('SYSTEM DROP REPLICA'),
    grant('SYSTEM SYNC REPLICA'),
    grant('SYSTEM RESTART REPLICA'),
    grant('SYSTEM FLUSH'),
    grant('dictGet'),
    grant('INTROSPECTION'),
    grant('URL'),
    grant('REMOTE'),
    grant('MONGO'),
    grant('MYSQL'),
    grant('ODBC'),
    grant('JDBC'),
    grant('HDFS'),
    grant('S3'),
]

# Admin user permissions that must be granted in all configurations with enabled SQL user management.
ADMIN_COMMON_GRANT_LIST = [
    grant('SHOW', grant_option=True),
    grant('SELECT', grant_option=True),
    grant('KILL QUERY', grant_option=True),
    grant('SYSTEM DROP CACHE', grant_option=True),
    grant('SYSTEM RELOAD', grant_option=True),
    grant('SYSTEM MERGES', grant_option=True),
    grant('SYSTEM TTL MERGES', grant_option=True),
    grant('SYSTEM FETCHES', grant_option=True),
    grant('SYSTEM MOVES', grant_option=True),
    grant('SYSTEM SENDS', grant_option=True),
    grant('SYSTEM REPLICATION QUEUES', grant_option=True),
    grant('SYSTEM DROP REPLICA', grant_option=True),
    grant('SYSTEM SYNC REPLICA', grant_option=True),
    grant('SYSTEM RESTART REPLICA', grant_option=True),
    grant('SYSTEM FLUSH', grant_option=True),
    grant('dictGet', grant_option=True),
    grant('INTROSPECTION', grant_option=True),
    grant('URL', grant_option=True),
    grant('REMOTE', grant_option=True),
    grant('MONGO', grant_option=True),
    grant('MYSQL', grant_option=True),
    grant('ODBC', grant_option=True),
    grant('JDBC', grant_option=True),
    grant('HDFS', grant_option=True),
    grant('S3', grant_option=True),
    grant_except_databases('INSERT', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('TRUNCATE', [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('OPTIMIZE', [MDB_SYSTEM_DATABASE], grant_option=True),
    # ALTER permissions.
    # - Allow ALTER for all databases and its objects except the databases system and _system.
    # - Allow ALTER DELETE, ALTER UPDATE and ALTER MATERIALIZE TTL for all databases without exceptions. These grants
    # are required for executing KILL MUTATION ON CLUSTER.
    grant('ALTER', grant_option=True),
    grant_except_databases('ALTER COLUMN', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('ALTER ORDER BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('ALTER SAMPLE BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('ALTER ADD INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('ALTER DROP INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('ALTER CLEAR INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('ALTER CONSTRAINT', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('ALTER TTL', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('ALTER SETTINGS', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('ALTER MOVE PARTITION', [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('ALTER FETCH PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('ALTER FREEZE PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('ALTER VIEW', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
]

# Additional admin user permissions for the case when SQL database management is enabled.
ADMIN_DATABASE_MANAGEMENT_ENABLED_GRANT_LIST = [
    grant('CREATE', grant_option=True),
    grant_except_databases('CREATE DATABASE', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('CREATE TABLE', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('CREATE VIEW', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('CREATE DICTIONARY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
]

# Additional admin user permissions for the case when SQL database management is disabled.
ADMIN_DATABASE_MANAGEMENT_DISABLED_GRANT_LIST = [
    grant('CREATE TEMPORARY TABLE', grant_option=True),
    grant_except_databases('CREATE TABLE', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('CREATE VIEW', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('CREATE DICTIONARY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('DROP TABLE', [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('DROP VIEW', [MDB_SYSTEM_DATABASE], grant_option=True),
    grant_except_databases('DROP DICTIONARY', [MDB_SYSTEM_DATABASE], grant_option=True),
]


def pillar(key, default=_default):
    """
    Like __salt__['pillar.get'], but when the key is missing and no default value provided,
    a KeyError exception will be raised regardless of 'pillar_raise_on_missing' option.
    """
    value = __salt__['pillar.get'](key, default=default)
    if value is _default:
        raise KeyError('Pillar key not found: {0}'.format(key))

    return value


def grains(key, default=_default, **kwargs):
    """
    Like __salt__['grains.get'], but when the key is missing and no default value provided,
    a KeyError exception will be raised.
    """
    value = __salt__['grains.get'](key, default=default, **kwargs)
    if value is _default:
        raise KeyError('Grains key not found: {0}'.format(key))

    return value


def hostname():
    """
    Return hostname.
    """
    return grains('id')


def cluster_id():
    """
    Return cluster ID.
    """
    return pillar('data:dbaas:cluster_id', None) or pillar('data:clickhouse:cluster_name')


def cluster_name():
    """
    Return cluster name.
    """
    return pillar('data:clickhouse:cluster_name', None) or pillar('data:dbaas:cluster_id')


def shard_id():
    """
    Return shard id.
    """
    pillar_value = pillar('data:dbaas:shard_id', None)
    if pillar_value:
        return pillar_value

    return shard_name()


def shard_name():
    """
    Return shard name.
    """
    pillar_value = pillar('data:dbaas:shard_name', None)
    if pillar_value:
        return pillar_value

    h = hostname()
    for name, opts in pillar('data:clickhouse:shards').items():
        if h in opts['replicas']:
            return name

    raise RuntimeError('Shard name not found in pillar')


def has_zookeeper():
    """
    Return True if ClickHouse cluster is configured with ZooKeeper or ClickHouse Keeper.
    """
    if pillar('data:clickhouse:zk_hosts', None) or pillar('data:clickhouse:keeper_hosts', None):
        return True

    for subcluster in pillar('data:dbaas:cluster:subclusters', {}).values():
        if 'zk' in subcluster['roles']:
            return True

    return False


def has_embedded_keeper():
    """
    Return True if ClickHouse cluster is configured to use embedded Keeper.
    """
    return pillar('data:clickhouse:embedded_keeper', False)


def has_separated_keeper():
    """
    Return True if current ClickHouse host should start Keeper in separated process.
    """
    if not has_embedded_keeper() or not pillar('data:unmanaged:separated_keeper', False):
        return False
    host = hostname()
    return host in zookeeper_hosts()


def zookeeper_root():
    """
    Return ZooKeeper root path for ClickHouse cluster.
    """
    try:
        return pillar('data:clickhouse:zk_path')
    except KeyError:
        pass

    if has_embedded_keeper():
        return '/'

    return '/clickhouse/' + cluster_id()


def zookeeper_user():
    """
    Return ZooKeeper user for ClickHouse cluster.
    """
    return ZK_ACL_USER_CLICKHOUSE


def zookeeper_password(user):
    """
    Return ZooKeeper user for ClickHouse cluster.
    """
    return pillar('data:clickhouse:zk_users:{user}:password'.format(user=user), '')


def zookeeper_hosts():
    """
    Return list of ZooKeeper or Clickhouse Keeper hosts.
    For newly created clusters - from `keeper_hosts` pillar data.
    For early created clusters - fallback to `zk_hosts` pillar data.
    If cluster is configured without ZooKeeper or ClickHouse Keeper, an empty list is returned.
    """
    k_hosts = keeper_hosts_list()
    if k_hosts:
        return k_hosts

    subclusters = list(pillar('data:dbaas:cluster:subclusters', {}).values())
    zk_host_map = dict((host, {}) for host in pillar('data:clickhouse:zk_hosts', []))
    if zk_host_map:
        # populate host properties in zk_host_map
        for subcluster in subclusters:
            for shard in subcluster.get('shards', {}).values():
                for host, opts in shard['hosts'].items():
                    if host in zk_host_map:
                        zk_host_map[host] = opts
            for host, opts in subcluster.get('hosts', {}).items():
                if host in zk_host_map:
                    zk_host_map[host] = opts
    else:
        for subcluster in subclusters:
            if 'zk' in subcluster['roles']:
                zk_host_map = subcluster['hosts']

    return _to_hostnames_ordered_by_geo(zk_host_map)


def zookeeper_is_secure():
    return (
        pillar('data:unmanaged:enable_zk_tls', False)
        and pillar('data:clickhouse:zk_users', False)
        and not pillar('data:unmanaged:tmp_disable_zk_tls_mdb_12035', False)
        and not has_embedded_keeper()
    )


def zookeeper_port():
    return 2281 if zookeeper_is_secure() else 2181


def zookeeper_hosts_port(zk_hosts=None):
    port = zookeeper_port()
    if not zk_hosts:
        zk_hosts = zookeeper_hosts()
    return ','.join(map(lambda host: host + ':' + str(port), zk_hosts))


def keeper_hosts():
    """
    Return dict "Keeper host -> server_id". If cluster is configured without Keeper, an empty dict is returned.
    """
    return pillar('data:clickhouse:keeper_hosts', {})


def keeper_hosts_list():
    """
    Return list of keeper hosts. If cluster is configured without Keeper, an empty list is returned.
    Sorting only for determined order.
    """
    return sorted(keeper_hosts().keys())


def user_replication_enabled():
    """
    Return True if replication of users, roles, grants and other access control objects is enabled.
    """
    return has_zookeeper() and pillar('data:clickhouse:user_replication_enabled', False)


def version():
    """
    Return ClickHouse version.
    """
    return pillar('data:clickhouse:ch_version')


def version_cmp(comparing_version):
    """
    Compare the ClickHouse version from pillar with the version passed in as an argument.
    """
    return __salt__['pkg.version_cmp'](version(), comparing_version)


def version_ge(comparing_version):
    """
    Perform version comparison using version_cmp, but return a boolean:
     - If the current version is lesser to the provided in the arg, then False;
     - If the current version is greater or equal to the provided, then True;
    """
    if version_cmp(comparing_version) in (0, 1):
        return True
    return False


def version_lt(comparing_version):
    """
    Perform version comparison using version_cmp, but return a boolean.
    Return True if current version is less than the provided in the arg.
    """
    if version_cmp(comparing_version) == -1:
        return True
    return False


def databases():
    """
    Return databases metadata.
    """
    return pillar('data:clickhouse:databases', None)


def user_names():
    """
    Return user names.
    """
    return list(pillar('data:clickhouse:users').keys())


def user_settings(user_name):
    """
    Return settings for the specified user.
    """
    user = pillar('data:clickhouse:users')[user_name]
    settings = user.get('settings') or {}

    result_settings = _default_user_settings()
    result_settings.update(_filter_user_settings(settings))
    return result_settings


def user_grants(user_name):
    """
    Returns grants for the specified user.
    """
    user_data = pillar('data:clickhouse:users')[user_name]
    user_databases = list(user_data['databases'].keys())

    grants = USER_COMMON_GRANT_LIST + [
        grant('SELECT', SYSTEM_DATABASES + [TMP_DATABASE] + user_databases),
        grant('TRUNCATE', SYSTEM_DATABASES + user_databases),
        grant('OPTIMIZE', SYSTEM_DATABASES + user_databases),
    ]
    if version_cmp('21.11') >= 0:
        grants.extend(
            [
                grant('DROP DATABASE', SYSTEM_DATABASES + user_databases),
                grant('DROP TABLE', SYSTEM_DATABASES + user_databases),
                grant('DROP VIEW', SYSTEM_DATABASES + user_databases),
                grant('DROP DICTIONARY', SYSTEM_DATABASES + user_databases),
                grant('CREATE FUNCTION'),
                grant('DROP FUNCTION'),
            ]
        )
    else:
        grants.extend(
            [
                grant('DROP', SYSTEM_DATABASES + user_databases),
            ]
        )
    if user_databases:
        grants.extend(
            [
                grant('INSERT', user_databases),
                grant('CREATE DATABASE', user_databases),
                grant('CREATE DICTIONARY', user_databases),
                grant('CREATE VIEW', user_databases),
                grant('CREATE TABLE', user_databases),
            ]
        )

    # ALTER permissions:
    # - Allow all ALTER commands for databases specified in user settings.
    # - Allow ALTER DELETE, ALTER UPDATE and ALTER MATERIALIZE TTL for all databases without exceptions. These grants
    # are required for executing KILL MUTATION ON CLUSTER.
    except_databases = [db for db in databases() if db not in user_databases]
    grants.extend(
        [
            grant('ALTER'),
            grant_except_databases('ALTER COLUMN', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE] + except_databases),
            grant_except_databases('ALTER ORDER BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE] + except_databases),
            grant_except_databases('ALTER SAMPLE BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE] + except_databases),
            grant_except_databases('ALTER ADD INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE] + except_databases),
            grant_except_databases('ALTER DROP INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE] + except_databases),
            grant_except_databases('ALTER CLEAR INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE] + except_databases),
            grant_except_databases('ALTER CONSTRAINT', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE] + except_databases),
            grant_except_databases('ALTER TTL', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE] + except_databases),
            grant_except_databases('ALTER SETTINGS', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE] + except_databases),
            grant_except_databases('ALTER MOVE PARTITION', [MDB_SYSTEM_DATABASE] + except_databases),
            grant_except_databases(
                'ALTER FETCH PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE] + except_databases
            ),
            grant_except_databases(
                'ALTER FREEZE PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE] + except_databases
            ),
            grant_except_databases('ALTER VIEW', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE] + except_databases),
        ]
    )

    if version_cmp('21.2') >= 0:
        grants.append(grant('POSTGRES'))
    if version_cmp('21.8') >= 0:
        grants.append(grant('SYSTEM RESTORE REPLICA'))

    return grants


def user_password_hash(user_name):
    """
    Return password hash for the specified user.
    """
    return pillar('data:clickhouse:users')[user_name]['hash']


def user_quota_mode(user_name):
    """
    Return quota mode for the specified user.
    """
    return pillar('data:clickhouse:users')[user_name].get('quota_mode') or 'default'


def user_quotas(user_name):
    """
    Return quotas for the specified user.
    """
    return pillar('data:clickhouse:users')[user_name].get('quotas') or []


def admin_grants():
    """
    Returns grants for the admin user.
    """
    target_grants = [] + ADMIN_COMMON_GRANT_LIST

    if pillar('data:clickhouse:sql_database_management', False):
        target_grants.extend(ADMIN_DATABASE_MANAGEMENT_ENABLED_GRANT_LIST)
        if version_cmp('21.11') >= 0:
            target_grants.extend(
                [
                    grant('DROP', grant_option=True),
                    grant_except_databases('DROP DATABASE', [MDB_SYSTEM_DATABASE], grant_option=True),
                    grant_except_databases('DROP TABLE', [MDB_SYSTEM_DATABASE], grant_option=True),
                    grant_except_databases('DROP VIEW', [MDB_SYSTEM_DATABASE], grant_option=True),
                    grant_except_databases('DROP DICTIONARY', [MDB_SYSTEM_DATABASE], grant_option=True),
                ]
            )
        else:
            target_grants.extend(
                [
                    grant_except_databases('DROP', [MDB_SYSTEM_DATABASE], grant_option=True),
                ]
            )
    else:
        target_grants.extend(ADMIN_DATABASE_MANAGEMENT_DISABLED_GRANT_LIST)
        if version_cmp('21.11') >= 0:
            target_grants.extend(
                [
                    grant('CREATE FUNCTION', grant_option=True),
                    grant('DROP FUNCTION', grant_option=True),
                ]
            )

    if version_cmp('21.2') >= 0:
        target_grants.append(grant('POSTGRES', grant_option=True))

    if version_cmp('21.8') >= 0:
        target_grants.append(grant('SYSTEM RESTORE REPLICA', grant_option=True))

    target_grants.append(grant('ACCESS MANAGEMENT', grant_option=True))
    if version_cmp('22.2') >= 0:
        target_grants.extend(
            [
                grant_except_databases(
                    'CREATE ROW POLICY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True
                ),
                grant_except_databases('ALTER ROW POLICY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
                grant_except_databases('DROP ROW POLICY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
            ]
        )

    return target_grants


def admin_password_hash():
    """
    Return password hash for the admin user.
    """
    return pillar('data:clickhouse:admin_password:hash')


def ssl_enabled():
    """
    Return True if SSL support is enabled.
    """
    return pillar('data:clickhouse:use_ssl', True) and bool(pillar('cert.key', False))


def ssl_enforced():
    """
    Return True if non-encrypted connections are forbidden.
    """
    if not ssl_enabled():
        return False

    if pillar('data:dbaas:vtype', None) == 'compute' and not pillar('data:dbaas:assign_public_ip', False):
        return False

    return True


def disabled_ssl_protocols():
    """
    Return comma-separated list of disabled SSL protocols for ClickHouse server config.
    """
    protocols = ['sslv2', 'sslv3']
    if not pillar('data:clickhouse:allow_tlsv1_1', False):
        protocols += ['tlsv1', 'tlsv1_1']

    return ','.join(protocols)


def ca_path():
    """
    Return file path to CA bundle.
    """
    return '/etc/clickhouse-server/ssl/allCAs.pem'


def tcp_port():
    """
    Return ClickHouse tcp port number.
    """
    pillar_value = pillar('data:clickhouse:tcp_port', None)
    if pillar_value:
        return pillar_value

    return 9000


def port_settings():
    """
    Return ClickHouse port settings.
    """
    is_ssl_enabled = ssl_enabled()
    is_ssl_enforced = ssl_enforced()

    result = {'tcp_port': tcp_port()}

    if pillar('data:clickhouse:mysql_protocol', False):
        result['mysql_port'] = 3306

    if pillar('data:clickhouse:postgresql_protocol', False):
        result['postgresql_port'] = 5433

    if not is_ssl_enforced or pillar('data:dbaas:vtype', None) == 'compute':
        result['http_port'] = 8123
    if is_ssl_enabled:
        result['interserver_https_port'] = 9010
        result['tcp_port_secure'] = 9440
        result['https_port'] = 8443
    else:
        result['interserver_http_port'] = 9009

    return result


def open_ports():
    """
    Return ports open by firewall.
    """
    ports = [8443, 9440]
    if pillar('data:clickhouse:mysql_protocol', False):
        ports.append(3306)
    if pillar('data:clickhouse:postgresql_protocol', False):
        ports.append(5433)
    if prometheus_integration_enabled():
        ports.append(9363)
    if not ssl_enforced():
        ports += [8123, 9000]

    return ports


def cloud_storage_enabled():
    """
    Return True if cloud storage is enabled, or False otherwise.
    """
    return pillar('data:cloud_storage:enabled', False)


def server_settings():
    """
    Return top-level server settings.
    """
    default_settings = {
        'default_database': 'default',
        'default_profile': 'default',
        'max_connections': 4096,
        'keep_alive_timeout': 3,
        'max_concurrent_queries': 500,
        'max_server_memory_usage': clickhouse_memory_limit(),
        'uncompressed_cache_size': 8589934592,
        'mark_cache_size': int(pillar('data:dbaas:flavor:memory_guarantee', 20 * 2**30) / 4),
        'builtin_dictionaries_reload_interval': 3600,
        'custom_settings_prefixes': 'custom_',
        'allow_no_password': 0,
    }

    if version_cmp('22.5') >= 0:
        default_settings['dictionaries_lazy_load'] = 0

    # Settings that must not be present in server settings. It includes custom settings and settings not supported for
    # using ClickHouse version.
    excluded_settings = list(CUSTOM_SETTINGS)
    if version_cmp('22.3') < 0:
        excluded_settings.append('allow_plaintext_password')
        excluded_settings.append('allow_no_password')

    settings = default_settings
    settings.update(pillar('data:clickhouse:config', {}))

    return dict(
        (name, value)
        for name, value in settings.items()
        if isinstance(value, (six.string_types, int, float)) and (name not in excluded_settings)
    )


def clickhouse_memory_limit():
    max_server_memory_usage = pillar('data:clickhouse:config:max_server_memory_usage', None)
    if max_server_memory_usage:
        return max_server_memory_usage

    memory_guarantee = pillar('data:dbaas:flavor:memory_guarantee')
    if memory_guarantee >= 40 * 1024**3:
        return memory_guarantee - 4 * 1024**3
    elif memory_guarantee >= 10 * 1024**3:
        return int(0.9 * memory_guarantee)
    elif memory_guarantee >= 2 * 1024**3:
        return memory_guarantee - 1024**3
    else:
        return memory_guarantee - 400 * 1024**2


def merge_tree_settings():
    """
    Return settings of MergeTree tables.
    """
    settings = {
        'max_suspicious_broken_parts': 1000000,
        'replicated_max_ratio_of_wrong_parts': 1,
        'max_files_to_modify_in_alter_columns': 1000000,
        'max_files_to_remove_in_alter_columns': 1000000,
        'use_minimalistic_part_header_in_zookeeper': 1,
    }
    if version_cmp('21.4') >= 0:
        if version_cmp('21.8') < 0:
            settings['allow_s3_zero_copy_replication'] = 1
        else:
            settings['allow_remote_fs_zero_copy_replication'] = 1
    if version_cmp('21.11') >= 0:
        settings['max_suspicious_broken_parts_bytes'] = 107374182400

    cpu_guarantee = pillar('data:dbaas:flavor:cpu_guarantee', None)
    if cpu_guarantee:
        settings['max_part_loading_threads'] = max(int(cpu_guarantee * 2), 1)
        settings['max_part_removal_threads'] = max(int(cpu_guarantee * 2), 1)

    settings.update(pillar('data:clickhouse:config:merge_tree', {}))

    excluded_settings = []
    if version_cmp('21.3') < 0:
        excluded_settings.append('inactive_parts_to_delay_insert')
        excluded_settings.append('inactive_parts_to_throw_insert')

    return dict((k, v) for k, v in settings.items() if k not in excluded_settings and v not in (None, ''))


def kafka_settings():
    """
    Return Kafka settings.
    """
    return pillar('data:clickhouse:config:kafka', {})


def kafka_topics():
    """
    Return per-topic Kafka settings.
    """
    return pillar('data:clickhouse:config:kafka_topics', [])


def rabbitmq_settings():
    """
    Return RabbitMQ settings.
    """
    return pillar('data:clickhouse:config:rabbitmq', {})


def system_tables():
    """
    Return configuration of system tables.
    """
    ch_config = pillar('data:clickhouse:config', {})
    database = 'system'
    default_flush_interval_milliseconds = 7500

    storage_policy = ''
    if version_ge('22.3'):
        storage_policy = ' SETTINGS storage_policy = \'local\''

    tables = {
        'query_log': {
            'enabled': True,
            'config': {
                'database': database,
                'flush_interval_milliseconds': default_flush_interval_milliseconds,
            },
        },
        'part_log': {
            'enabled': True,
            'config': {
                'database': database,
                'flush_interval_milliseconds': default_flush_interval_milliseconds,
            },
        },
        'query_thread_log': {
            'enabled': ch_config.get('query_thread_log_enabled', True),
            'config': {
                'database': database,
                'flush_interval_milliseconds': default_flush_interval_milliseconds,
            },
        },
        'metric_log': {
            'enabled': ch_config.get('metric_log_enabled', True),
            'config': {
                'database': database,
                'flush_interval_milliseconds': default_flush_interval_milliseconds,
                'collect_interval_milliseconds': 1000,
            },
        },
        'text_log': {
            'enabled': ch_config.get('text_log_enabled', False),
            'config': {
                'database': database,
                'level': ch_config.get('text_log_level', 'trace'),
                'flush_interval_milliseconds': default_flush_interval_milliseconds,
            },
        },
        'trace_log': {
            'enabled': ch_config.get('trace_log_enabled', True),
            'config': {
                'database': database,
                'flush_interval_milliseconds': default_flush_interval_milliseconds,
            },
        },
        'crash_log': {
            'enabled': True,
            'config': {
                'database': database,
                'partition_by': '',
                'flush_interval_milliseconds': 1000,
            },
        },
        'opentelemetry_span_log': {
            'enabled': ch_config.get('opentelemetry_span_log_enabled', False),
            'config': {
                'database': database,
                'engine': 'ENGINE = MergeTree PARTITION BY finish_date ORDER BY (finish_date, finish_time_us, trace_id)'
                + storage_policy,
                'flush_interval_milliseconds': default_flush_interval_milliseconds,
            },
        },
        'session_log': {
            'enabled': version_ge('21.11') and version_lt('22.6'),
            'config': {
                'database': database,
                'flush_interval_milliseconds': default_flush_interval_milliseconds,
            },
        },
    }

    for name, settings in tables.items():
        config = settings['config']
        config['table'] = name
        if config.get('partition_by') is None and config.get('engine') is None:
            config['engine'] = (
                'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)' + storage_policy
            )

    return tables


def geobase_path():
    """
    Return filesystem path to active geobase. It references to either preinstalled or custom geobase.
    """
    if custom_geobase_uri():
        return custom_geobase_path()

    return '/opt/yandex/clickhouse-geodb'


def custom_geobase_uri():
    """
    Return URI of custom geobase.
    """
    return pillar('data:clickhouse:config:geobase_uri', None)


def custom_geobase_path():
    """
    Return filesystem path to custom geobase.
    """
    return '/var/lib/clickhouse/geodb'


def custom_geobase_archive_dir():
    """
    Return filesystem path to the directory containing custom geobase archive.
    """
    return '/var/lib/clickhouse/geodb_archive'


def custom_geobase_archive_path():
    """
    Return filesystem path to custom geobase archive.
    """
    remote_path = custom_geobase_uri()
    if remote_path is None:
        return None

    archive_ext = _archive_ext(remote_path)
    if archive_ext is None:
        return None

    return os.path.join(custom_geobase_archive_dir(), 'geodb.{0}'.format(archive_ext))


def _archive_ext(uri):
    path = urlparse(uri).path
    for ending in ('tar', 'tar.gz', 'tgz', 'tar.bz2', 'tbz2', 'tbz', 'tar.xz', 'txz', 'tar.lzma', 'tlz', 'zip', 'rar'):
        if path.endswith('.' + ending):
            return ending

    return None


def backup_zkflock_id():
    """
    Return zkflock_id for ClickHouse backup tool.
    """
    return os.path.join('ch_backup', cluster_id(), shard_id())


def create_zkflock_id(zk_hosts, wait_timeout=600):
    """
    Wait for zookeeper to start up and create zkflock id.
    """
    from kazoo.client import KazooClient

    deadline = time.time() + wait_timeout
    zkflock_path = "/" + backup_zkflock_id()
    while time.time() <= deadline:
        try:
            zk_conn = KazooClient(zk_hosts)
            zk_conn.start()
            if not zk_conn.exists(zkflock_path):
                zk_conn.create(path=zkflock_path, makepath=True)
        except Exception as e:
            log.debug('Failed to connect to ZK: %s', repr(e))
            time.sleep(5)
        else:
            zk_conn.stop()
            return

    raise RuntimeError('Timeout while waiting for Zookeeper to start up')


def cleaner_config():
    """
    Return ClickHouse cleaner config.
    """
    ch_config = pillar('data:clickhouse:config', {})

    day = 24 * 3600
    default_retention_time = 30 * day

    retention_policies = [
        {
            'database': 'system',
            'table': 'query_log',
            'max_partition_count': retention_time_to_max_partition_count(
                ch_config.get('query_log_retention_time', default_retention_time), day
            ),
            'max_table_size': ch_config.get('query_log_retention_size', 1073741824),
            'table_rotation_suffix': '_[0-9]',
        },
        {
            'database': 'system',
            'table': 'query_thread_log',
            'max_partition_count': retention_time_to_max_partition_count(
                ch_config.get('query_thread_log_retention_time', default_retention_time), day
            ),
            'max_table_size': ch_config.get('query_thread_log_retention_size', 536870912),
            'table_rotation_suffix': '_[0-9]',
        },
        {
            'database': 'system',
            'table': 'part_log',
            'max_partition_count': retention_time_to_max_partition_count(
                ch_config.get('part_log_retention_time', default_retention_time), day
            ),
            'max_table_size': ch_config.get('part_log_retention_size', 536870912),
            'table_rotation_suffix': '_[0-9]',
        },
        {
            'database': 'system',
            'table': 'metric_log',
            'max_partition_count': retention_time_to_max_partition_count(
                ch_config.get('metric_log_retention_time', default_retention_time), day
            ),
            'max_table_size': ch_config.get('metric_log_retention_size', 536870912),
            'table_rotation_suffix': '_[0-9]',
        },
        {
            'database': 'system',
            'table': 'trace_log',
            'max_partition_count': retention_time_to_max_partition_count(
                ch_config.get('trace_log_retention_time', default_retention_time), day
            ),
            'max_table_size': ch_config.get('trace_log_retention_size', 536870912),
            'table_rotation_suffix': '_[0-9]',
        },
        {
            'database': 'system',
            'table': 'text_log',
            'max_partition_count': retention_time_to_max_partition_count(
                ch_config.get('text_log_retention_time', default_retention_time), day
            ),
            'max_table_size': ch_config.get('text_log_retention_size', 536870912),
            'table_rotation_suffix': '_[0-9]',
        },
        {
            'database': 'system',
            'table': 'opentelemetry_span_log',
            'max_partition_count': retention_time_to_max_partition_count(default_retention_time, day),
            'table_rotation_suffix': '_[0-9]',
        },
        {
            'database': 'system',
            'table': 'session_log',
            'max_partition_count': retention_time_to_max_partition_count(default_retention_time, day),
            'table_rotation_suffix': '_[0-9]',
        },
    ]

    retention_policies.extend(pillar('data:clickhouse:cleaner:config:retention_policies', []))

    return {
        'retention_policies': retention_policies,
    }


def retention_time_to_max_partition_count(retention_time, seconds_in_day):
    return int(math.ceil(retention_time / seconds_in_day))


def backups_enabled():
    """
    Return True if backups are enabled.
    """
    if not pillar('data:use_ch_backup', True):
        return False

    if not pillar('data:clickhouse:periodic_backups', True):
        return False

    if pillar('data:backup:use_backup_service', False):
        return False

    if cloud_storage_enabled() and version_lt('21.6'):
        return False

    return True


def backup_config():
    """
    Return config for ClickHouse backup tool.
    """
    return _backup_config()


def backup_config_for_restore():
    """
    Return restore config for ClickHouse backup tool.
    """
    encryption_key = pillar('data:restore-from-pillar-data:ch_backup:encryption_key')
    s3_bucket = pillar('data:restore-from-pillar-data:s3_bucket')
    s3_path_root = pillar('restore-from:s3-path').rstrip('/').rsplit('/', 1)[0]
    force_non_replicated = not has_zookeeper()
    config = _backup_config(encryption_key, s3_bucket, s3_path_root, force_non_replicated)

    config['restore_from'] = {
        'cid': pillar('restore-from:cid'),
        'shard_name': shard_name(),
    }

    if s3_bucket and cloud_storage_enabled():
        s3_endpoint = config['storage']['credentials']['endpoint_url']
        addressing_style = config['storage']['boto_config']['addressing_style']
        config['cloud_storage'] = {
            'type': 's3',
            'credentials': {
                'endpoint_url': s3_endpoint,
                'bucket': 'cloud-storage-' + pillar('restore-from:cid'),
                'access_key_id': pillar('data:cloud_storage:s3:access_key_id', None),
                'secret_access_key': pillar('data:s3:cloud_storage:access_secret_key', None),
                'send_metadata': 'true',
            },
            'boto_config': {'addressing_style': addressing_style, 'region_name': pillar('data:s3:region', None)},
            'bulk_delete_chunk_size': 100,
        }

        cloud_storage_proxy_resolver = pillar('data:cloud_storage:s3:proxy_resolver', {})
        if cloud_storage_proxy_resolver:
            config['cloud_storage']['proxy_resolver'] = cloud_storage_proxy_resolver

    return config


def backup_config_for_schema_copy():
    """
    Return config for ClickHouse backup tool, from another shard.
    """
    s3_path_root = 'ch_backup/' + cluster_id() + '/' + pillar('schema_backup_shard', shard_name())

    config = _backup_config(s3_path_root=s3_path_root)

    zk_hosts_with_data = pillar("zk_hosts_with_data", None)
    if zk_hosts_with_data:
        config['zookeeper']['hosts'] = zookeeper_hosts_port(zk_hosts_with_data)
    return config


def _backup_config(encryption_key=None, s3_bucket=None, s3_path_root=None, force_non_replicated=False):
    encryption_key = encryption_key or pillar('data:ch_backup:encryption_key', None)
    s3_bucket = s3_bucket or pillar('data:s3_bucket', None)
    s3_path_root = (
        s3_path_root or pillar('data:ch_backup:s3_path_prefix', 'ch_backup/' + cluster_id()) + '/' + shard_name()
    )

    is_zk_secure = zookeeper_is_secure()

    certfile = '/etc/clickhouse-server/ssl/server.crt'
    keyfile = '/etc/clickhouse-server/ssl/server.key'
    cafile = '/etc/clickhouse-server/ssl/allCAs.pem'

    config = {
        'clickhouse': {
            'clickhouse_user': '_admin',
            'timeout': 600,
        },
        'backup': {
            'exclude_dbs': [MDB_SYSTEM_DATABASE],
            'path_root': s3_path_root,
            'deduplicate_parts': True,
            'deduplication_age_limit': {
                'days': pillar('data:ch_backup:deduplication_age_limit_days', 30),
            },
            'min_interval': {
                'minutes': 30,
            },
            'retain_time': {
                'days': pillar('data:ch_backup:retain_time_days', 7),
            },
            'retain_count': pillar('data:ch_backup:retain_count', 7),
            'time_format': '%Y-%m-%d %H:%M:%S %z',
            'labels': {
                'shard_name': shard_name(),
            },
            'keep_freezed_data_on_failure': False,
            'override_replica_name': '{replica}',
            'force_non_replicated': force_non_replicated,
            'backup_access_control': True,
        },
        'zookeeper': {
            'hosts': zookeeper_hosts_port(),
            'root_path': zookeeper_root(),
            'secure': is_zk_secure,
        },
        'multiprocessing': {
            'workers': pillar('data:ch_backup:workers', 4),
        },
        'main': {
            'drop_privileges': not __salt__['dbaas.is_aws'](),
        },
    }

    mdb_backup_admin_password = pillar('data:clickhouse:system_users:{}:password'.format(MDB_BACKUP_ADMIN_USER), None)
    if mdb_backup_admin_password:
        config['clickhouse']['clickhouse_user'] = MDB_BACKUP_ADMIN_USER_NAME
        config['clickhouse']['clickhouse_password'] = mdb_backup_admin_password

    zk_user, zk_password = get_zk_acl_credentials()
    if zk_user and zk_password:
        config['zookeeper']['user'] = zk_user
        config['zookeeper']['password'] = zk_password
    if is_zk_secure:
        config['zookeeper']['cert'] = certfile
        config['zookeeper']['key'] = keyfile
        if not __salt__['dbaas.is_public_ca']():
            config['zookeeper']['ca'] = cafile

    if encryption_key:
        config['encryption'] = {
            'type': 'nacl',
            'key': encryption_key,
        }

    if s3_bucket:
        proxy_resolver = pillar('data:s3:proxy_resolver', {})

        if proxy_resolver:
            # force use of HTTP as S3 backends do not support HTTPS
            s3_endpoint = __salt__['mdb_s3.endpoint']('http')
        else:
            s3_endpoint = __salt__['mdb_s3.endpoint']()

        if pillar('data:s3:virtual_addressing_style', False):
            addressing_style = 'virtual'
        else:
            addressing_style = 'path'

        config['storage'] = {
            'type': 's3',
            'credentials': {
                'endpoint_url': s3_endpoint,
                'bucket': s3_bucket,
                'access_key_id': pillar('data:s3:access_key_id', None),
                'secret_access_key': pillar('data:s3:access_secret_key', None),
                'send_metadata': 'true',
            },
            'boto_config': {'addressing_style': addressing_style, 'region_name': pillar('data:s3:region', None)},
            'bulk_delete_chunk_size': 100,
        }

        if proxy_resolver:
            config['storage']['proxy_resolver'] = proxy_resolver

    if ssl_enabled():
        config['clickhouse']['protocol'] = 'https'
        config['clickhouse']['ca_path'] = ca_path()

    return config


def backup_command():
    """
    On-demand backup creation command.
    """
    return _backup_command()


def initial_backup_command():
    """
    Return backup creation command.
    """
    return _backup_command(initial=True)


def backup_cron_command():
    """
    Return cron command for ClickHouse backup tool.
    """
    return _backup_command(cron=True)


def _backup_command(cron=False, initial=False):
    default_timeout = 60 * 60 * 60
    if pillar('data:dbaas:vtype', None) == 'compute':
        default_sleep = 30 * 60
    else:
        default_sleep = 2 * 60 * 60

    timeout = pillar('data:backup:timeout', default_timeout)

    command = 'flock -n -o /tmp/ch-backup.lock /etc/cron.yandex/ch-backup.sh --timeout {0}'.format(timeout)

    backup_id = pillar('backup_id', None)
    if backup_id:
        command += ' --backup-name {0}'.format(backup_id)

    if cron:
        command += ' --sleep {0}'.format(pillar('data:backup:sleep', default_sleep))
        if pillar('data:ch_backup:purge_enabled', True):
            command += ' --purge'
    else:
        command += ' --force'

    for name, value in pillar("labels", {}).items():
        command += ' --label {0}={1}'.format(name, value)

    if has_zookeeper() and not initial:
        # ensure zk-flock-id is present
        command = (
            '/etc/cron.yandex/create-zkflock-id.sh '
            '&& zk-flock -c /etc/yandex/ch-backup/zk-flock.json lock "{0}"'.format(command)
        )

    return command


def is_resetup_restore():
    """
    Returns True if restore is actually host resetup from backup.
    """
    return cluster_id() == pillar('restore-from:cid')


def restore_command():
    command = (
        'flock -n -o /tmp/ch-backup.lock /usr/bin/ch-backup -c ' '/etc/yandex/ch-backup/ch-backup-restore.conf restore '
    )
    if pillar('restore-from:schema-only', False):
        command += '--schema-only '
    if cloud_storage_enabled():
        command += '--cloud-storage-source-bucket cloud-storage-' + pillar('restore-from:cid')
        command += ' --cloud-storage-source-path cloud_storage/' + pillar('restore-from:cid') + '/' + shard_name()
        if is_resetup_restore():
            command += ' --cloud-storage-latest'
        command += ' '
    command += pillar('restore-from:backup-id')

    return command


def restore_schema_command():
    schema_source = pillar('schema_backup_host', None)
    resetup_shard_name = pillar('schema_backup_shard', shard_name())

    subcluster_id = pillar('data:dbaas:subcluster_id')
    if not schema_source:
        shards = pillar('data:dbaas:cluster:subclusters:{subcid}:shards'.format(subcid=subcluster_id)).values()
        for shard in shards:
            if shard['name'] == resetup_shard_name:
                schema_source = next(
                    iter(
                        sorted(
                            filter(lambda fqdn: fqdn != hostname(), shard['hosts']), reverse=True  # Push MAN to the end
                        )
                    ),
                    None,
                )
                break

    if not schema_source:
        raise RuntimeError('Couldn`t find resetup source host')

    use_zk_hosts = ""
    zk_hosts_with_data = pillar("zk_hosts_with_data", None)
    if zk_hosts_with_data:
        use_zk_hosts = "--zk-hosts {zk_hosts_with_data}".format(
            zk_hosts_with_data=zookeeper_hosts_port(zk_hosts_with_data)
        )
    cmd = (
        '/usr/bin/ch-backup --port {port} --protocol https {use_zk_hosts} restore-schema '
        '--source-host {schema_source} '
        '--exclude-dbs system'.format(
            port=resetup_ports_config()['https_port'], schema_source=schema_source, use_zk_hosts=use_zk_hosts
        )
    )
    return cmd


def render_cluster_config():
    """
    Render cluster.xml configuration file.
    """

    doc = minidom.Document()

    root = doc.createElement('yandex')
    doc.appendChild(root)

    _append_xml_element(doc, root, 'remote-servers', _build_remote_servers_xml())

    zk_servers = _build_zookeeper_servers_xml(doc)
    if zk_servers:
        root.appendChild(zk_servers)

    root.appendChild(_build_macros_xml(doc))

    if prometheus_integration_enabled():
        root.appendChild(_build_prometheus_xml(doc))

    return _dump_xml(doc)


def _build_remote_servers_xml():
    is_ssl_enabled = ssl_enabled()
    if is_ssl_enabled:
        port = 9440
    else:
        port = tcp_port()

    internal_replication = pillar('data:clickhouse:unmanaged:internal_replication', True)

    remote_servers = {
        shard_group['name']: {
            'shard': [
                OrderedDict(
                    (
                        ('weight', shard.get('weight')),
                        ('internal_replication', 'true' if internal_replication else 'false'),
                        (
                            'replica',
                            [
                                OrderedDict(
                                    (
                                        ('host', host),
                                        ('port', port),
                                        ('secure', 1 if is_ssl_enabled else 0),
                                    )
                                )
                                for host in shard['replicas']
                            ],
                        ),
                    )
                )
                for shard in shard_group['shards']
            ]
        }
        for shard_group in _shard_groups()
    }

    return remote_servers


def _shard_groups():
    shards = _shards()

    shard_groups = [
        {
            'name': cluster_name(),
            'shards': shards,
        }
    ]

    shard_names = set(shard['name'] for shard in shards)
    for group_name, group in sorted(pillar('data:clickhouse:shard_groups', {}).items()):
        if any([name not in shard_names for name in group['shard_names']]):
            raise RuntimeError(
                'Invalid shard in shard group {}. Present {}. Shard group {}'.format(
                    group_name, repr(shard_names), repr(group['shard_names'])
                )
            )

        shard_groups.append(
            {
                'name': group_name,
                'shards': [shard for shard in shards if shard['name'] in group['shard_names']],
            }
        )

    return shard_groups


def _shards():
    shards = []

    shard_settings = pillar('data:clickhouse:shards', {})
    subclusters = pillar('data:dbaas:cluster:subclusters')
    for subcluster in subclusters.values():
        if 'clickhouse_cluster' in subcluster['roles']:
            for shard_id, opts in subcluster['shards'].items():
                weight = shard_settings.get(shard_id, {}).get('weight', 100)
                shards.append(
                    {
                        'id': shard_id,
                        'name': opts['name'],
                        'weight': weight,
                        'replicas': _to_hostnames_ordered_by_geo(opts['hosts']),
                    }
                )

    if not shards:
        raise RuntimeError('Shards data not found in pillar')

    return sorted(shards, key=lambda shard: shard['name'])


def _to_hostnames_ordered_by_geo(hosts):
    """
    Convert hosts in dict representation to hostnames ordered by geo.
    """
    current_geo = pillar('data:dbaas:geo')

    def _sort_key(host):
        name, opts = host
        return current_geo != opts.get('geo'), name

    return [name for name, opts in sorted(hosts.items(), key=_sort_key)]


def get_zk_acl_credentials():
    if pillar('data:unmanaged:enable_zk_tls', False) and pillar('data:clickhouse:zk_users', False):
        zk_user = pillar('data:clickhouse:zk_users:{user}'.format(user=ZK_ACL_USER_CLICKHOUSE), {})
        if zk_user and ('password' in zk_user):
            return ZK_ACL_USER_CLICKHOUSE, zk_user['password']
    return None, None


def _build_zookeeper_servers_xml(doc):
    zk_hosts = zookeeper_hosts()
    if not zk_hosts:
        return None

    result = doc.createElement('zookeeper-servers')
    for i, host in enumerate(zk_hosts):
        node = _append_xml_element(doc, result, 'node')
        node.setAttribute('index', six.text_type(i + 1))
        _append_xml_element(doc, node, 'host', host)
        if zookeeper_is_secure():
            _append_xml_element(doc, node, 'port', 2281)
            _append_xml_element(doc, node, 'secure', 1)
        else:
            _append_xml_element(doc, node, 'port', 2181)

    zk_user, zk_password = get_zk_acl_credentials()
    if zk_user and zk_password:
        _append_xml_element(doc, result, 'identity', '{user}:{password}'.format(user=zk_user, password=zk_password))

    zk_path = zookeeper_root()
    if zk_path:
        _append_xml_element(doc, result, 'root', zk_path)

    return result


def _build_macros_xml(doc):
    macros = doc.createElement('macros')

    _append_xml_element(doc, macros, 'cluster', cluster_name())
    _append_xml_element(doc, macros, 'shard', shard_name())
    _append_xml_element(doc, macros, 'replica', hostname())

    return macros


def _build_prometheus_xml(doc):
    prometheus = doc.createElement('prometheus')

    _append_xml_element(doc, prometheus, 'endpoint', '/metrics')
    _append_xml_element(doc, prometheus, 'port', 9363)
    _append_xml_element(doc, prometheus, 'metrics', True)
    _append_xml_element(doc, prometheus, 'events', True)
    _append_xml_element(doc, prometheus, 'asynchronous_metrics', True)
    _append_xml_element(doc, prometheus, 'status_info', True)

    return prometheus


def render_users_config():
    """
    Render users configuration file.
    """
    doc = minidom.Document()

    root = doc.createElement('yandex')
    doc.appendChild(root)

    _append_xml_element(doc, root, 'users', _users())

    _append_xml_element(doc, root, 'profiles', _profiles())

    _append_xml_element(doc, root, 'quotas', _quotas())

    return _dump_xml(doc)


def _users():
    users = OrderedDict()

    users['default'] = {
        'password': '',
        'profile': 'system',
        'quota': 'default',
        'networks': {
            'ip': ['::1', '127.0.0.1'],
            'host': [],
        },
    }
    cluster_hosts_network = users['default']['networks']
    for shard in _shards():
        cluster_hosts_network['host'].append(shard['replicas'])

    users['_metrics'] = {
        'password': '',
        'profile': '_metrics',
        'quota': 'default',
        'networks': {
            'ip': ['::1', '127.0.0.1'],
            'host': [hostname()],
        },
    }

    users['_monitor'] = {
        'password': '',
        'profile': '_monitor',
        'quota': 'default',
        'networks': {
            'ip': ['::1', '127.0.0.1'],
            'host': [hostname()],
        },
    }

    users['_dns'] = {
        'password': '',
        'profile': '_dns',
        'quota': 'default',
        'networks': {
            'ip': ['::1', '127.0.0.1'],
            'host': [hostname()],
        },
    }

    users['_admin'] = {
        'password': '',
        'profile': '_admin',
        'quota': 'default',
        'networks': cluster_hosts_network,
        'access_management': 1,
    }

    mdb_backup_admin = pillar('data:clickhouse:system_users:{}'.format(MDB_BACKUP_ADMIN_USER), None)
    if mdb_backup_admin:
        users[MDB_BACKUP_ADMIN_USER_NAME] = {
            'password_sha256_hex': mdb_backup_admin['hash'],
            'profile': MDB_BACKUP_ADMIN_USER_NAME,
            'quota': 'default',
            'networks': {
                'ip': ['::/0'],
            },
            'access_management': 1,
        }

    if not pillar('data:clickhouse:user_management_v2', False):
        for user_name, opts in sorted(pillar('data:clickhouse:users').items()):
            user = {
                'password_sha256_hex': opts['hash'],
                'profile': opts.get('profile', user_name + '_profile'),
                'networks': {
                    'ip': ['::/0'],
                },
            }

            if not opts.get('quotas'):
                user['quota'] = 'default'
            else:
                user['quota'] = user_name + '_quota'

            user_databases = opts.get('databases') or {}

            user['allow_databases'] = {'database': ['system'] + sorted(user_databases.keys())}

            if user_databases:
                user['databases'] = {}
                for db_name, db_opts in sorted(user_databases.items()):
                    tables = {}
                    for table_name, table_opts in sorted(db_opts.get('tables', {}).items()):
                        table = {}
                        if 'filter' in table_opts:
                            table['filter'] = table_opts['filter']

                        tables[table_name] = table

                    if tables:
                        user['databases'][db_name] = tables

            users[user_name] = user

    return users


def _profiles():
    # Combine system and custom profiles.
    profile_settings = OrderedDict(
        (
            (
                'system',
                {
                    'log_queries': 0,
                },
            ),
            (
                '_metrics',
                {
                    'log_queries': 0,
                    'max_execution_time': 10,
                    'max_concurrent_queries_for_user': 30,
                },
            ),
            (
                '_monitor',
                {
                    'log_queries': 0,
                    'max_execution_time': 10,
                    'max_concurrent_queries_for_user': 10,
                },
            ),
            (
                '_dns',
                {
                    'log_queries': 0,
                    'max_execution_time': 5,
                    'max_concurrent_queries_for_user': 10,
                    'skip_unavailable_shards': 1,
                },
            ),
            ('default', _system_profile_settings()),
            (
                '_admin',
                {
                    'log_queries': 0,
                    'allow_introspection_functions': 1,
                },
            ),
        )
    )

    mdb_backup_admin = pillar('data:clickhouse:system_users:{}'.format(MDB_BACKUP_ADMIN_USER), None)
    if mdb_backup_admin:
        profile_settings[MDB_BACKUP_ADMIN_USER_NAME] = {
            'log_queries': 0,
            'allow_experimental_live_view': 1,
            'allow_experimental_geo_types': 1,
        }

    for profile, settings in sorted((pillar('data:clickhouse:profiles', None) or {}).items()):
        if profile in profile_settings:
            profile_settings[profile].update(settings)
        else:
            profile_settings[profile] = settings
    if not pillar('data:clickhouse:user_management_v2', False):
        for user_name, opts in sorted(pillar('data:clickhouse:users').items()):
            if 'profile' not in opts:
                profile_settings[user_name + '_profile'] = opts.get('settings') or {}

    # Format result set of profiles with settings.
    profiles = OrderedDict()
    for profile_name, settings in profile_settings.items():
        result_settings = _default_user_settings()
        result_settings.update(_filter_user_settings(settings))
        profiles[profile_name] = result_settings

    return profiles


def _default_user_settings():
    default_settings = {
        'log_queries': 1,
        'log_queries_cut_to_length': 10000000,
        'insert_distributed_sync': 1,
        'distributed_directory_monitor_batch_inserts': 1,
        'max_concurrent_queries_for_user': server_settings()['max_concurrent_queries'] - 50,
        'join_algorithm': 'auto',
        'allow_drop_detached': 1,
    }

    if version_cmp('21.12') < 0:
        default_settings['partial_merge_join_optimizations'] = 0

    if version_cmp('22.5') < 0:
        default_settings['max_memory_usage'] = min(10000000000, clickhouse_memory_limit())

    if version_cmp('21.8') >= 0:
        default_settings['force_remove_data_recursively_on_drop'] = 1

    if version_cmp('22.5') >= 0:
        default_settings['database_atomic_wait_for_drop_and_detach_synchronously'] = 1

    if cloud_storage_enabled():
        # Avoid premature throttling when select data from S3.
        default_settings['timeout_before_checking_execution_speed'] = 300
        default_settings['s3_min_upload_part_size'] = 32 * 1024 * 1024
        if version_cmp('21.1') >= 0:  # Introduced in 21.1 version
            default_settings['s3_max_single_part_upload_size'] = 32 * 1024 * 1024

    if __salt__['dbaas.is_aws']():
        default_settings['load_balancing'] = 'first_or_random'

    return default_settings


def _system_profile_settings():
    ch_config = pillar('data:clickhouse:config', {})
    return dict((k, v) for k, v in ch_config.items() if k in SYSTEM_PROFILE_SETTINGS)


def _filter_user_settings(settings):
    if not settings:
        return {}

    # User settings that are not mapped as is to ClickHouse settings.
    custom_settings = [
        'quota_mode',
    ]

    # Settings that must not be present in profile settings. It includes custom settings and settings not supported for
    # using ClickHouse version.
    excluded_settings = custom_settings
    excluded_settings.append('compile')
    excluded_settings.append('min_count_to_compile')
    if version_cmp('20.10') < 0:
        excluded_settings.append('insert_quorum_parallel')
        excluded_settings.append('format_regexp')
        excluded_settings.append('format_regexp_escaping_rule')
        excluded_settings.append('format_regexp_skip_unmatched')
    if version_cmp('20.11') < 0:
        excluded_settings.append('date_time_output_format')
    if version_cmp('21.11') < 0:
        excluded_settings.append('local_filesystem_read_method')
        excluded_settings.append('remote_filesystem_read_method')
    if version_cmp('22.3') < 0:
        excluded_settings.append('cast_ipv4_ipv6_default_on_conversion_error')

    return dict((k, v) for k, v in settings.items() if k not in excluded_settings and v not in (None, ''))


def _quotas():
    quotas = OrderedDict()

    quotas['default'] = {
        'interval': [
            OrderedDict(
                (
                    ('duration', 3600),
                    ('queries', 0),
                    ('errors', 0),
                    ('result_rows', 0),
                    ('read_rows', 0),
                    ('execution_time', 0),
                )
            ),
        ],
    }

    if not pillar('data:clickhouse:user_management_v2', False):
        for user, opts in sorted(pillar('data:clickhouse:users').items()):
            intervals = []
            for quota_interval in opts.get('quotas') or []:
                intervals.append(
                    OrderedDict(
                        (
                            ('duration', quota_interval['interval_duration']),
                            ('queries', quota_interval.get('queries', 0)),
                            ('errors', quota_interval.get('errors', 0)),
                            ('result_rows', quota_interval.get('result_rows', 0)),
                            ('read_rows', quota_interval.get('read_rows', 0)),
                            ('execution_time', quota_interval.get('execution_time', 0)),
                        )
                    )
                )

            if not intervals:
                continue

            quota = {
                'interval': intervals,
            }

            quota_mode = opts.get('settings', {}).get('quota_mode')
            if quota_mode and quota_mode != 'default':
                quota[quota_mode] = ''

            quotas[user + '_quota'] = quota

    return quotas


def render_dictionary_config(dictionary=None, dictionary_id=None):
    """
    Render configuration file of external dictionary.
    """
    if dictionary_id is not None:
        dictionary = pillar('data:clickhouse:config:dictionaries:{0}'.format(dictionary_id))

    doc = minidom.Document()

    root = doc.createElement('yandex')
    doc.appendChild(root)

    dictionary_xml = _append_xml_element(doc, root, 'dictionary')

    _append_xml_element(doc, dictionary_xml, 'name', dictionary['name'])

    dictionary_xml.appendChild(_build_dictionary_source_xml(doc, dictionary))

    dictionary_xml.appendChild(_build_dictionary_structure_xml(doc, dictionary['structure']))

    dictionary_xml.appendChild(_build_dictionary_layout_xml(doc, dictionary['layout']))

    dictionary_xml.appendChild(_build_dictionary_lifetime_xml(doc, dictionary))

    return _dump_xml(doc)


def _build_dictionary_source_xml(doc, dictionary):
    result = doc.createElement('source')

    source = dictionary.get('http_source')
    if source:
        result.appendChild(_build_dictionary_http_source_xml(doc, source))
        return result

    source = dictionary.get('mysql_source')
    if source:
        result.appendChild(_build_dictionary_mysql_source_xml(doc, source))
        return result

    source = dictionary.get('clickhouse_source')
    if source:
        result.appendChild(_build_dictionary_clickhouse_source_xml(doc, source))
        return result

    source = dictionary.get('mongodb_source')
    if source:
        result.appendChild(_build_dictionary_mongodb_source_xml(doc, source))
        return result

    source = dictionary.get('postgresql_source')
    if source:
        result.appendChild(_build_dictionary_postgresql_source_xml(doc, source, dictionary['name']))
        return result

    source = dictionary['yt_source']
    result.appendChild(_build_dictionary_yt_source_xml(doc, source, dictionary['structure']))
    return result


def _build_dictionary_http_source_xml(doc, source):
    result = doc.createElement('http')

    _append_xml_element(doc, result, 'url', source['url'])
    _append_xml_element(doc, result, 'format', source['format'])

    return result


def _build_dictionary_mysql_source_xml(doc, source):
    result = doc.createElement('mysql')

    _append_xml_element(doc, result, 'db', source['db'])
    _append_xml_element(doc, result, 'table', source['table'])

    _append_optional_xml_element(doc, result, 'port', source.get('port'))
    _append_optional_xml_element(doc, result, 'user', source.get('user'))
    _append_optional_xml_element(doc, result, 'password', source.get('password'))

    for replica in source['replicas']:
        replica_xml = _append_xml_element(doc, result, 'replica')
        _append_xml_element(doc, replica_xml, 'host', replica['host'])
        _append_xml_element(doc, replica_xml, 'priority', replica['priority'])
        _append_optional_xml_element(doc, replica_xml, 'port', replica.get('port'))
        _append_optional_xml_element(doc, replica_xml, 'user', replica.get('user'))
        _append_optional_xml_element(doc, replica_xml, 'password', replica.get('password'))

    _append_optional_xml_element(doc, result, 'where', source.get('where'))
    _append_optional_xml_element(doc, result, 'invalidate_query', source.get('invalidate_query'))
    _append_optional_xml_element(doc, result, 'close_connection', source.get('close_connection'))

    return result


def _build_dictionary_clickhouse_source_xml(doc, source):
    result = doc.createElement('clickhouse')

    _append_xml_element(doc, result, 'host', source['host'])
    _append_xml_element(doc, result, 'port', source['port'])
    _append_xml_element(doc, result, 'user', source['user'])
    _append_xml_element(doc, result, 'password', source.get('password', ''))
    _append_xml_element(doc, result, 'db', source['db'])
    _append_xml_element(doc, result, 'table', source['table'])
    _append_optional_xml_element(doc, result, 'where', source.get('where'))

    return result


def _build_dictionary_mongodb_source_xml(doc, source):
    result = doc.createElement('mongodb')

    _append_xml_element(doc, result, 'host', source['host'])
    _append_xml_element(doc, result, 'port', source['port'])
    _append_xml_element(doc, result, 'user', source['user'])
    _append_xml_element(doc, result, 'password', source['password'])
    _append_xml_element(doc, result, 'db', source['db'])
    _append_xml_element(doc, result, 'collection', source['collection'])

    return result


def _build_dictionary_postgresql_source_xml(doc, source, dictionary_name):
    result = doc.createElement('odbc')

    connection_string = 'DSN={0}_dsn'.format(dictionary_name)
    _append_xml_element(doc, result, 'connection_string', connection_string)
    _append_xml_element(doc, result, 'table', source['table'])
    _append_optional_xml_element(doc, result, 'invalidate_query', source.get('invalidate_query'))

    return result


def _build_dictionary_yt_source_xml(doc, source, structure):
    def _append_field(parent, attribute_name, attribute_type):
        if attribute_type == 'Date':
            node_name = 'date_field'
        elif attribute_type == 'DateTime':
            node_name = 'datetime_field'
        else:
            node_name = 'field'

        _append_xml_element(doc, parent, node_name, attribute_name)

    result = doc.createElement('library')

    _append_xml_element(doc, result, 'path', '/usr/lib/libclickhouse_dictionary_yt.so')

    settings_xml = _append_xml_element(doc, result, 'settings')

    for cluster in source['clusters']:
        _append_xml_element(doc, settings_xml, 'cluster', cluster)

    _append_xml_element(doc, settings_xml, 'table', source['table'])

    if 'keys' in source:
        for key in source['keys']:
            _append_xml_element(doc, settings_xml, 'key', key)
    else:
        if 'id' in structure:
            _append_xml_element(doc, settings_xml, 'key', structure['id']['name'])
        else:
            for attribute in structure['key']['attributes']:
                _append_xml_element(doc, settings_xml, 'key', attribute['name'])

    query = source.get('query')
    if query:
        _append_xml_element(doc, settings_xml, 'query', query)
    else:
        if ('fields' in source) or ('date_fields' in source) or ('datetime_fields' in source):
            for field in source.get('fields', []):
                _append_xml_element(doc, settings_xml, 'field', field)

            for date_field in source.get('date_fields', []):
                _append_xml_element(doc, settings_xml, 'date_field', date_field)

            for datetime_field in source.get('datetime_fields', []):
                _append_xml_element(doc, settings_xml, 'datetime_field', datetime_field)
        else:
            for range_attribute_name in ('range_min', 'range_max'):
                if range_attribute_name in structure:
                    attribute = structure[range_attribute_name]
                    _append_field(settings_xml, attribute['name'], attribute.get('type', 'Date'))

            for attribute in structure['attributes']:
                _append_field(settings_xml, attribute['name'], attribute['type'])

    _append_xml_element(doc, settings_xml, 'user', source['user'])
    _append_xml_element(doc, settings_xml, 'token', source['token'])

    for setting in [
        'cluster_selection',
        'use_query_for_cache',
        'force_read_table',
        'range_expansion_limit',
        'input_row_limit',
        'yt_socket_timeout_msec',
        'yt_connection_timeout_msec',
        'yt_lookup_timeout_msec',
        'yt_select_timeout_msec',
        'output_row_limit',
        'yt_retry_count',
    ]:
        if setting in source:
            _append_optional_xml_element(doc, settings_xml, setting, source.get(setting))
    return result


def _build_dictionary_structure_xml(doc, structure):
    result = doc.createElement('structure')

    if 'id' in structure:
        id_xml = _append_xml_element(doc, result, 'id')
        _append_xml_element(doc, id_xml, 'name', structure['id']['name'])
    else:
        key_xml = _append_xml_element(doc, result, 'key')
        for attribute in structure['key']['attributes']:
            attribute_xml = _append_xml_element(doc, key_xml, 'attribute')
            _append_xml_element(doc, attribute_xml, 'name', attribute['name'])
            _append_xml_element(doc, attribute_xml, 'type', attribute['type'])
            _append_optional_xml_element(doc, attribute_xml, 'expression', attribute.get('expression'))

    for range_attribute_name in ('range_min', 'range_max'):
        if range_attribute_name in structure:
            attribute = structure[range_attribute_name]
            attribute_xml = _append_xml_element(doc, result, range_attribute_name)
            _append_xml_element(doc, attribute_xml, 'name', attribute['name'])
            _append_optional_xml_element(doc, attribute_xml, 'type', attribute.get('type'))

    for attribute in structure['attributes']:
        attribute_xml = _append_xml_element(doc, result, 'attribute')
        _append_xml_element(doc, attribute_xml, 'name', attribute['name'])
        _append_xml_element(doc, attribute_xml, 'type', attribute['type'])
        _append_xml_element(doc, attribute_xml, 'null_value', attribute.get('null_value', ''))
        _append_optional_xml_element(doc, attribute_xml, 'expression', attribute.get('expression'))
        _append_optional_xml_element(doc, attribute_xml, 'hierarchical', attribute.get('hierarchical'))
        _append_optional_xml_element(doc, attribute_xml, 'injective', attribute.get('injective'))

    return result


def _build_dictionary_layout_xml(doc, layout):
    result = doc.createElement('layout')

    layout_type = layout['type']
    layout_type_xml = _append_xml_element(doc, result, layout_type)

    if layout_type in ['cache', 'complex_key_cache']:
        _append_xml_element(doc, layout_type_xml, 'size_in_cells', layout['size_in_cells'])

    return result


def _build_dictionary_lifetime_xml(doc, dictionary):
    result = doc.createElement('lifetime')

    lifetime_range = dictionary.get('lifetime_range')
    if lifetime_range:
        _append_xml_element(doc, result, 'min', lifetime_range['min'])
        _append_xml_element(doc, result, 'max', lifetime_range['max'])
    else:
        result.appendChild(doc.createTextNode(six.text_type(dictionary['fixed_lifetime'])))

    return result


def ml_models():
    """
    Return ML models.
    """
    models = pillar('data:clickhouse:models', None)
    return models if models else {}


def ml_model(name):
    """
    Return ML model.
    """
    return pillar('data:clickhouse:models:{0}'.format(name))


def render_ml_model_config(name):
    """
    Render configuration file of ML model.
    """
    model = ml_model(name)

    doc = minidom.Document()

    root = doc.createElement('models')
    doc.appendChild(root)

    model_xml = _append_xml_element(doc, root, 'model')

    _append_xml_element(doc, model_xml, 'name', name)

    _append_xml_element(doc, model_xml, 'type', model['type'])

    _append_xml_element(doc, model_xml, 'path', '/var/lib/clickhouse/models/{0}.bin'.format(name))

    _append_xml_element(doc, model_xml, 'lifetime', 0)

    return _dump_xml(doc)


def format_schemas():
    """
    Return format schemas.
    """
    format_schemas = pillar('data:clickhouse:format_schemas', None)
    return format_schemas if format_schemas else {}


def _append_xml_element(doc, parent, name, data=None):
    if isinstance(data, list):
        return [_append_xml_element(doc, parent, name, item) for item in data]

    child = doc.createElement(name)

    if data is not None:
        if isinstance(data, OrderedDict):
            for item_name, item_data in data.items():
                _append_xml_element(doc, child, item_name, item_data)
        elif isinstance(data, dict):
            for item_name, item_data in sorted(data.items()):
                _append_xml_element(doc, child, item_name, item_data)
        else:
            # unescape value to avoid double escaping
            text = saxutils.unescape(_ensure_text(data))
            child.appendChild(doc.createTextNode(text))

    parent.appendChild(child)
    return child


def _append_optional_xml_element(doc, parent, name, data):
    if data is None:
        return

    if isinstance(data, six.string_types) and not data:
        return

    _append_xml_element(doc, parent, name, data)


def _dump_xml(doc):
    return six.text_type(doc.toprettyxml(indent=4 * ' ', encoding='utf-8'), encoding='utf-8')


def _ensure_text(value):
    """
    Take a value of an arbitrary type and return its string representation (six.text_type).
    """
    if isinstance(value, six.text_type):
        return value

    # Cast string types to six.text_type with specifying encoding.
    if isinstance(value, six.string_types):
        return six.text_type(value, encoding='utf-8')

    if isinstance(value, bool):
        return 'true' if value else 'false'

    return six.text_type(value)


def _request(query, params=None, method='GET', timeout=10, retry=10):
    """
    Send request to ClickHouse.
    """
    import requests
    from requests.adapters import HTTPAdapter
    from requests.packages.urllib3.util.retry import Retry

    ssl = ssl_enabled()
    ca_bundle = ca_path()
    protocol = 'https' if ssl else 'http'
    verify = ca_bundle if ca_bundle else ssl
    port = 8443 if ssl else 8123
    if not params:
        params = {'user': '_admin'}
    params['query'] = query
    adapter = HTTPAdapter(
        max_retries=Retry(total=retry, status_forcelist=[500, 502, 503, 504], method_whitelist=["GET"])
    )
    http = requests.Session()
    http.mount("https://", adapter)
    http.mount("http://", adapter)

    try:
        r = http.request(
            method, '{0}://{1}:{2}'.format(protocol, hostname(), port), params=params, timeout=timeout, verify=verify
        )
        r.raise_for_status()
        return r.text.strip()
    except requests.exceptions.HTTPError as e:
        raise ClickhouseError(query, e.response)


def execute(query, params=None, timeout=30):
    """
    Execute query.
    """
    if params is None:
        params = {'user': '_admin'}
    return _request(query, params=params, method='POST', timeout=timeout)


def create_database(database):
    """
    Create database if it does not exist.
    """
    sql = 'CREATE DATABASE IF NOT EXISTS `{}`'.format(database)
    _request(sql, method='POST')


def delete_database(database):
    """
    Delete database if it exists.
    """
    try:
        enable_force_drop_table()
        sql = 'DROP DATABASE IF EXISTS `{}`'.format(database)
        _request(sql, method='POST', timeout=600)
    finally:
        disable_force_drop_table()


def get_existing_database_names():
    """
    Get names of existing databases.
    """
    sql = """
        SELECT name FROM system.databases
        WHERE name NOT IN ('default', 'system', '_temporary_and_external_tables', 'information_schema', 'INFORMATION_SCHEMA')
        FORMAT JSON
    """
    res = json.loads(_request(sql))
    return [row['name'] for row in res['data']]


def get_existing_tables(database=None):
    """
    Get existing tables.
    """
    sql = 'SELECT database, name, create_table_query FROM system.tables'
    if database:
        sql += ' WHERE database = \'{0}\''.format(database)
    sql += ' FORMAT JSON'

    return json.loads(_request(sql))['data']


def delete_table(database, table):
    """
    Drop table.
    """
    query = 'DROP TABLE `{0}`.`{1}` NO DELAY'.format(database, table)
    _request(query, method='POST', timeout=120)


def enable_force_drop_table():
    """
    Set force_drop_table flag.
    """
    open('/var/lib/clickhouse/flags/force_drop_table', 'a').close()
    os.chmod('/var/lib/clickhouse/flags/force_drop_table', 0o666)


def disable_force_drop_table():
    """
    Remove force_drop_table flag.
    """
    try:
        os.remove('/var/lib/clickhouse/flags/force_drop_table')
    except OSError:
        pass


def has_data_parts(database, table):
    """
    Return True if table has data parts.
    """
    sql = "SELECT count() > 0 FROM system.parts WHERE database = '{0}' AND table = '{1}'".format(database, table)
    return _request(sql) == '1'


def ensure_system_query_log(test=False):
    """
    Ensure that system.query_log has been initialized.
    """
    sql_check = """
        SELECT 1
        FROM system.tables
        WHERE database = 'system' AND name = 'query_log'
    """
    exists = _request(sql_check, {'log_queries': 0})
    if exists:
        return {}

    if not test:
        _request('SYSTEM FLUSH LOGS', method='POST', timeout=60)

    return {'system.query_log': 'initialized'}


def reload_dictionaries():
    """
    Reload external dictionaries.
    """
    _request('SYSTEM RELOAD DICTIONARIES', method='POST', timeout=300)


def reload_config():
    """
    Reload ClickHouse configs.
    """
    _request('SYSTEM RELOAD CONFIG', method='POST', timeout=300)


def initialized_flag_name():
    return 'initialized/{}'.format(hostname())


def resetup_ports_config():
    """
    Return ports for clickhouse-server during resetup
    """
    return {
        'tcp_port': 29000,
        'http_port': 28123,
        'tcp_port_secure': 29440,
        'https_port': 29443,
    }


def resetup_possible():
    """
    Return possibility of automatic resetup
    """

    # used for schema copy from another shard in clusters without zookeeper.
    if pillar('replica_schema_backup', False):
        return True

    if has_zookeeper():
        return False

    for shard in _shards():
        replicas = shard['replicas']
        if hostname() in replicas:
            return len(replicas) > 1

    return False


def resetup_required():
    """
    Detect if clickhouse resetup tool should be executed
    """
    if pillar('replica_schema_backup', False):
        return True

    if not has_zookeeper():
        return False

    s3_client = __salt__['mdb_s3.client']()

    if not __salt__['mdb_s3.object_exists'](s3_client, initialized_flag_name()):
        return False

    system_prefix = r'/var/lib/clickhouse/metadata/system'
    sql_suffix = r'.sql'
    ch_data = list(
        filter(
            lambda path: not path.startswith(system_prefix) and path.endswith(sql_suffix),
            __salt__['file.find']('/var/lib/clickhouse/metadata'),
        )
    )
    if len(ch_data) > 0:
        return False

    return True


def render_server_resetup_config():
    """
    Render clickhouse-server/conf.d/resetup_config.xml configuration file.
    """
    doc = minidom.Document()

    merge_tree_config = {}
    if version_cmp('21.9') >= 0:
        merge_tree_config['check_sample_column_is_correct'] = 0

    config = {
        'distributed_ddl': {
            'path': '/clickhouse/task_queue/fake_ddl',
        },
        'dictionaries_lazy_load': 1,
    }
    config.update(resetup_ports_config())
    if merge_tree_config:
        config['merge_tree'] = merge_tree_config

    _append_xml_element(doc, doc, 'yandex', config)

    return _dump_xml(doc)


def render_client_resetup_config():
    """
    Render clickhouse-client/conf.d/resetup_config.xml configuration file.
    """
    doc = minidom.Document()
    config = {'tcp_port': resetup_ports_config()['tcp_port']}
    _append_xml_element(doc, doc, 'config', config)

    return _dump_xml(doc)


def render_storage_config():
    """
    Render config.d/storage_policy.xml configuration file.
    The config file contains:
    - disk configurations:
        - default
        - object storage
    - policies:
        - YC:
            - local
            - object storage
            - default (local + object storage)
        - DC:
            - local
            - object_storage
            - hybrid_storage (local + object storage)
            - default (local)

    Object storage related stuff is only being rendered if it's enabled.
    """
    doc = minidom.Document()

    root = doc.createElement('yandex')
    doc.appendChild(root)

    _append_xml_element(doc, root, 'storage_configuration', _storage_config())

    return _dump_xml(doc)


def _storage_config():
    config = _default_storage_config()

    if cloud_storage_enabled():
        config = _add_cloud_storage_config(disks=config['disks'], policies=config['policies'])

    return config


def _default_storage_config():
    disks = {
        'default': {},
    }

    default_volume = {
        'disk': 'default',
    }

    policies = {
        'default': {
            'volumes': OrderedDict((('default', default_volume),)),
        },
        'local': {
            'volumes': OrderedDict((('default', default_volume),)),
        },
    }

    return {
        'disks': disks,
        'policies': policies,
    }


def _add_cloud_storage_config(disks, policies):
    disks.update(
        {
            'object_storage': _object_storage_disk_config(),
        }
    )

    default_volume = {
        'disk': 'default',
    }

    object_storage_volume = {
        'disk': 'object_storage',
    }

    if version_cmp('20.10') >= 0:
        object_storage_volume['perform_ttl_move_on_insert'] = 'false'

    if version_cmp('20.11') >= 0:
        object_storage_volume['prefer_not_to_merge'] = pillar('data:cloud_storage:settings', {}).get(
            'prefer_not_to_merge', 'true'
        )

    policies.update(
        {
            'object_storage': {
                'volumes': OrderedDict((('object_storage', object_storage_volume),)),
            },
        }
    )

    if not __salt__['dbaas.is_aws']():
        policies['default'].update(
            {
                # Move parts to s3 when local disk filled more than 99%
                'move_factor': pillar('data:cloud_storage:settings', {}).get('move_factor', 0.01),
            }
        )
        policies['default']['volumes'].update(
            {
                'object_storage': object_storage_volume,
            }
        )
    else:
        policies.update(
            {
                'hybrid_storage': {
                    'volumes': OrderedDict(
                        (
                            ('default', default_volume),
                            ('object_storage', object_storage_volume),
                        )
                    ),
                    'move_factor': pillar('data:cloud_storage:settings', {}).get('move_factor', 0.01),
                },
            }
        )

    return {
        'disks': disks,
        'policies': policies,
    }


def _object_storage_disk_config():
    s3 = pillar('data:cloud_storage:s3')

    disk_s3 = OrderedDict()

    disk_s3['type'] = 's3'
    disk_s3['endpoint'] = _s3_endpoint(s3)
    disk_s3['access_key_id'] = s3['access_key_id']
    disk_s3['secret_access_key'] = s3['access_secret_key']
    # Using pooled connections causes deadlock in CH, we shouldn't limit it.
    disk_s3['max_connections'] = 10000
    disk_s3['request_timeout_ms'] = 3600000
    disk_s3['send_metadata'] = True

    proxy_resolver = s3.get('proxy_resolver', {})
    if proxy_resolver:
        resolver_cfg = OrderedDict()

        resolver_cfg['endpoint'] = proxy_resolver['uri']
        resolver_cfg['proxy_scheme'] = s3['scheme']
        resolver_cfg['proxy_port'] = proxy_resolver['proxy_port'][s3['scheme']]

        disk_s3['proxy'] = {'resolver': resolver_cfg}

    # Low-level settings to configure s3 disk. Can be manually set in pillar.
    # May be useful for hot-fixes on running clusters.
    # ClickHouse should be restarted to apply changes.
    settings_names = [
        'connect_timeout_ms',
        'request_timeout_ms',
        'metadata_path',
        'cache_enabled',
        'min_multi_part_upload_size',
        'min_bytes_for_seek',
        'skip_access_check',
        'max_connections',
        'send_metadata',
        'thread_pool_size',
        'list_object_keys_size',
        'data_cache_enabled',
        'data_cache_max_size',
    ]

    settings_values = pillar('data:cloud_storage:settings', {})

    for setting_name in settings_names:
        setting_value = settings_values.get(setting_name, None)
        if setting_value is not None:
            disk_s3[setting_name] = setting_value

    return disk_s3


def _s3_endpoint(s3):
    s3_path = 'cloud_storage/' + cluster_id() + '/' + shard_name() + '/'

    if s3.get('virtual_hosted_style', False):
        return s3['scheme'] + '://' + s3['bucket'] + '.' + s3['endpoint'] + '/' + s3_path
    else:
        return s3['scheme'] + '://' + s3['endpoint'] + '/' + s3['bucket'] + '/' + s3_path


def render_raft_config():
    """
    Render "keeper_server" configuration tag for config.xml in case it is required.
    """
    if not has_embedded_keeper():
        return ""

    host = hostname()
    # If there are no 'keeper_hosts' in pillar, we will have fallback to 'zk_hosts' ordered by geo in AZ-order.
    # We will fully migrate from 'zk_hosts' to 'keeper_hosts' later.
    zk_hosts = keeper_hosts() or {host: index + 1 for index, host in enumerate(sorted(zookeeper_hosts()))}

    my_server_id = zk_hosts.get(host, 0)
    if my_server_id <= 0:
        return ""

    doc = minidom.Document()
    root = doc.createElement('keeper_server')

    _append_xml_element(doc, root, "tcp_port", 2181)
    _append_xml_element(doc, root, "server_id", my_server_id)
    _append_xml_element(doc, root, "log_storage_path", "/var/lib/clickhouse/coordination/log")
    _append_xml_element(doc, root, "snapshot_storage_path", "/var/lib/clickhouse/coordination/snapshots")

    _append_xml_element(
        doc,
        root,
        'coordination_settings',
        {
            "operation_timeout_ms": 5000,
            "session_timeout_ms": 10000,
            "raft_logs_level": "trace",
        },
    )

    def items_sort_key(item):
        return item[1]

    raft_config = _append_xml_element(doc, root, 'raft_configuration')
    _append_xml_element(
        doc,
        raft_config,
        'server',
        [
            {
                "id": server_id,
                "hostname": zk_host,
                "port": 2888,
            }
            for zk_host, server_id in sorted(zk_hosts.items(), key=items_sort_key)
        ],
    )

    return _dump_xml(root)


def kafka_ca_path():
    """
    Return file path to custom Kafka CA bundle.
    """
    return '/etc/clickhouse-server/ssl/kafkaCAs.pem'


def kafka_topic_ca_path(name):
    """
    Return file path to user Kafka CA bundle for specific topic.
    """
    return '/etc/clickhouse-server/ssl/kafkaCAs_{}.pem'.format(name)


def render_kafka_config():
    """
    Render kafka config in config.xml configuration file.
    """

    def _format_settings(data, topic_name=None):
        result = {name: data[name] for name, value in data.items() if name != 'ca_cert'}
        if 'ca_cert' not in data:
            result['ssl_ca_location'] = ca_path()
        elif data['ca_cert']:
            result['ssl_ca_location'] = kafka_topic_ca_path(topic_name) if topic_name else kafka_ca_path()
        return result

    result = {'kafka_' + topic['name']: _format_settings(topic['settings'], topic['name']) for topic in kafka_topics()}
    result['kafka'] = _format_settings(kafka_settings())
    return result


def get_existing_user_names():
    """
    Get names of existing users.
    """
    query = "SELECT name FROM system.users WHERE storage != 'users.xml' FORMAT JSON"
    return [row['name'] for row in json.loads(_request(query=query, timeout=200))['data']]


def create_user(user_name, password_hash, grantees=None):
    """
    Create ClickHouse user.
    """
    grantees_req = ""
    if grantees:
        grantees_req = " GRANTEES {}".format(", ".join(grantees[0]) if grantees[0] else "ANY")
        if grantees[1]:
            grantees_req += " EXCEPT {}".format(", ".join(grantees[1]))
    query = """CREATE USER OR REPLACE '{user_name}'
        IDENTIFIED WITH sha256_hash BY '{password_hash}'{grantees_req};""".format(
        user_name=user_name, password_hash=password_hash, grantees_req=grantees_req
    )
    execute(query)


def delete_user(user_name):
    """
    Drop ClickHouse user.
    """
    execute("""DROP USER IF EXISTS '{user_name}'""".format(user_name=user_name))


def change_user_password(user_name, password_hash):
    """
    Alter ClickHouse user password.
    """
    query = """ALTER USER '{user_name}' IDENTIFIED WITH sha256_hash BY '{password_hash}';""".format(
        user_name=user_name, password_hash=password_hash
    )
    execute(query)


def get_existing_user_password_hash(user_name):
    """
    Get current password hash for the specified user.
    """
    user_id = _request(query="SELECT id FROM system.users WHERE name='{0}'".format(user_name), timeout=200)
    raw_str = __salt__['file.read']('/var/lib/clickhouse/access/{0}.sql'.format(user_id))
    match = search(r"IDENTIFIED WITH sha256_hash BY '(?P<pass>[0-9A-F]+)'", raw_str)
    return match.group('pass') if match else 'invalid'


def get_existing_user_grants(user_name):
    """
    Get current grants for the specified user.
    """
    query = """
        SELECT access_type, database, is_partial_revoke, grant_option
        FROM system.grants WHERE user_name = '{user_name}'
        ORDER BY access_type, is_partial_revoke, database
        FORMAT JSON""".format(
        user_name=user_name
    )
    response = json.loads(_request(query))
    grants_by_type = {}
    for item in response.get('data', []):
        if item['access_type'] in grants_by_type:
            grants_by_type[item['access_type']].append(item)
        else:
            grants_by_type[item['access_type']] = [item]

    return grants_by_type


def get_existing_user_grantees(user_name):
    """
    Get current user grantees.
    """
    query = "SHOW CREATE USER '{user_name}' FORMAT Values".format(user_name=user_name)
    response = _request(query)
    r = search(r"GRANTEES (\w+(, \w+)*)( EXCEPT (\w+(, \w+)*))?", response)
    if r:
        grantees = r.group(1).split(", ") if r.group(1) else []
        ungrantees = r.group(4).split(", ") if r.group(4) else []
    else:
        grantees = ['ANY']
        ungrantees = []
    return [grantees, ungrantees]


def get_existing_profile_names():
    """
    Get names of existing user profiles
    """
    query = "SELECT name FROM system.settings_profiles WHERE storage != 'users.xml' FORMAT JSON"
    response = json.loads(_request(query))
    return {profile['name'] for profile in response.get('data', [])}


def get_existing_user_settings(user_name):
    """
    Get current settings for the specified user.
    """
    query = """
        SELECT
            apply_to_list
        FROM system.settings_profiles
        WHERE name='{user_name}_profile'
        FORMAT JSON""".format(
        user_name=user_name
    )
    response = json.loads(_request(query))
    user_in_list = False
    for list in response.get('data', []):
        if user_name in list.get('apply_to_list', []):
            user_in_list = True
    if not user_in_list:
        return {}
    query = """
        SELECT
            setting_name,
            value
        FROM system.settings_profile_elements
        WHERE profile_name='{user_name}_profile'
        FORMAT JSON""".format(
        user_name=user_name
    )
    response = json.loads(_request(query))
    return {item['setting_name']: item['value'] for item in response.get('data', [])}


def get_existing_quota_names():
    """
    Get names of existing user quota objects.
    """
    query = "SELECT name FROM system.quotas WHERE storage != 'users.xml' FORMAT JSON"
    response = json.loads(_request(query))
    return {quota['name'] for quota in response.get('data', [])}


def get_existing_user_quotas(user_name):
    """
    Get current quotas for the specified user.
    """
    query = """
        SELECT
            quota_name name,
            duration interval_duration,
            max_result_rows result_rows,
            max_read_rows read_rows,
            max_queries queries,
            max_execution_time execution_time,
            max_errors errors,
            keys,
            apply_to_list
        FROM system.quota_limits JOIN system.quotas ON quota_limits.quota_name == quotas.name
        WHERE match(quota_name, '{user_name}_quota_[0-9]+') != 0 FORMAT JSON;""".format(
        user_name=user_name
    )
    response = json.loads(_request(query))
    return {quota['name']: quota for quota in response.get('data', [])}


def do_install_debug_symbols():
    """
    Returns true if rootfs size already extended to 20G or more.
    """
    disks_info = __salt__['disk.usage']()
    assert '/' in disks_info, 'Could not obtain root disk size info'
    return disks_info['/']['1K-blocks'] >= BLOCK_20GB


def s3_cache_path(object_type, name):
    return os.path.join(OBJECT_CACHE_ROOT, object_type, name)


def cache_user_object(object_type, name):
    type_map = {
        'format_schema': 'data:clickhouse:format_schemas:{name}:uri',
        'ml_model': 'data:clickhouse:models:{name}:uri',
        'geobase': 'data:clickhouse:config:geobase_uri',
    }

    if object_type not in type_map:
        raise RuntimeError('Invalid object type ' + object_type)

    object_uri = pillar(type_map[object_type].format(name=name), None)
    if not object_uri:
        raise RuntimeError(object_type + ' uri not found: ' + name)

    data = __salt__['fs.download_object'](object_uri, bool(pillar('data:service_account_id', None)))
    client = __salt__['mdb_s3.client']()
    __salt__['mdb_s3.create_object'](client, s3_cache_path(object_type, name), data)
    return object_uri


def check_keeper_service():
    return os.path.exists('/etc/systemd/system/clickhouse-keeper.service')


def clear_user_object_cache(object_type, name):
    client = __salt__['mdb_s3.client']()
    __salt__['mdb_s3.object_absent'](client, s3_cache_path(object_type, name))
    return True


def restore_user_object_cache(backup_bucket, restore_geobase):
    """
    Copy user object cache from original cluster bucket.
    """

    client = __salt__['mdb_s3.client']()
    restored = []

    def clone_data(key):
        if __salt__['mdb_s3.object_exists'](client, key, s3_bucket=backup_bucket):
            data = __salt__['mdb_s3.get_object'](client, key, s3_bucket=backup_bucket)
            __salt__['mdb_s3.create_object'](client, key, data)
            restored.append(key)

    if custom_geobase_uri() and restore_geobase:
        clone_data(s3_cache_path('geobase', 'custom_clickhouse_geobase'))

    for schema_name in format_schemas().keys():
        clone_data(s3_cache_path('format_schema', schema_name))

    for model_name in ml_models().keys():
        clone_data(s3_cache_path('ml_model', model_name))

    return restored


def s3_credentials_config_recreate_required():
    """
    Returns whether default s3 credentials should be recreated.
    """
    if not os.path.exists(DEFAULT_S3_CONFIG_PATH):
        return True
    timediff = timedelta(seconds=time.time() - os.path.getmtime(DEFAULT_S3_CONFIG_PATH))
    return not (timedelta(hours=0) < timediff < timedelta(hours=2))


def render_limits_config():
    """
    Renders json file that used for collecting static metrics from host by telegraf.
    """
    data = {
        'name': 'ch_limits',
        'cpu_limit': pillar('data:dbaas:flavor:cpu_limit', 0),
        'parts_to_delay_insert': pillar('data:clickhouse:config:merge_tree:parts_to_delay_insert', 150),
        'parts_to_throw_insert': pillar('data:clickhouse:config:merge_tree:parts_to_throw_insert', 300),
    }
    return json.dumps(data, indent=4, sort_keys=True)


def prometheus_integration_enabled():
    """
    Returns true if CH cluster is in DoubleCloud
    """
    return __salt__['dbaas.is_aws']()
