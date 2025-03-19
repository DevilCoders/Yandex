# -*- coding: utf-8 -*-

from __future__ import unicode_literals

import pytest
import re

from cloud.mdb.salt.salt._modules import mdb_clickhouse
from cloud.mdb.salt.salt._modules.mdb_clickhouse import (
    ADMIN_COMMON_GRANT_LIST,
    SYSTEM_DATABASES,
    TMP_DATABASE,
    USER_COMMON_GRANT_LIST,
    MDB_SYSTEM_DATABASE,
    ADMIN_DATABASE_MANAGEMENT_DISABLED_GRANT_LIST,
    ADMIN_DATABASE_MANAGEMENT_ENABLED_GRANT_LIST,
    grant,
    grant_except_databases,
)
from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_grains, mock_version_cmp, mock_vtype
from cloud.mdb.internal.python.pytest.utils import parametrize


def test_hostname_succeeded():
    hostname = 'man-1.db.yandex.net'
    mock_grains(mdb_clickhouse.__salt__, {'id': hostname})
    assert mdb_clickhouse.hostname() == hostname


def test_hostname_failed():
    mock_grains(mdb_clickhouse.__salt__, {})
    with pytest.raises(KeyError):
        mdb_clickhouse.hostname()


@parametrize(
    {
        'id': 'MDB cluster',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                    },
                },
            },
            'result': 'cid1',
        },
    },
    {
        'id': 'MDB cluster with overridden name',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                    },
                    'clickhouse': {
                        'cluster_name': 'cluster1',
                    },
                },
            },
            'result': 'cid1',
        },
    },
    {
        'id': 'Non-MDB cluster',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'cluster_name': 'cluster1',
                    },
                },
            },
            'result': 'cluster1',
        },
    },
)
def test_cluster_id_succeeded(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.cluster_id() == result


def test_cluster_id_failed():
    mock_pillar(mdb_clickhouse.__salt__, {})
    with pytest.raises(KeyError):
        mdb_clickhouse.cluster_id()


@parametrize(
    {
        'id': 'MDB cluster',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                    },
                },
            },
            'result': 'cid1',
        },
    },
    {
        'id': 'MDB cluster with overridden name',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                    },
                    'clickhouse': {
                        'cluster_name': 'cluster1',
                    },
                },
            },
            'result': 'cluster1',
        },
    },
    {
        'id': 'Non-MDB cluster',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'cluster_name': 'cluster1',
                    },
                },
            },
            'result': 'cluster1',
        },
    },
)
def test_cluster_name_succeeded(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.cluster_name() == result


def test_cluster_name_failed():
    mock_pillar(mdb_clickhouse.__salt__, {})
    with pytest.raises(KeyError):
        mdb_clickhouse.cluster_name()


@parametrize(
    {
        'id': 'MDB cluster',
        'args': {
            'grains': {
                'id': 'man-1.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'shard_id': 'shard_id1',
                    },
                },
            },
            'result': 'shard_id1',
        },
    },
    {
        'id': 'Non-MDB cluster',
        'args': {
            'grains': {
                'id': 'man-1.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'clickhouse': {
                        'shards': {
                            'shard1': {
                                'replicas': [
                                    'man-1.db.yandex.net',
                                    'sas-1.db.yandex.net',
                                ],
                            },
                        },
                    },
                },
            },
            'result': 'shard1',
        },
    },
)
def test_shard_id_succeeded(grains, pillar, result):
    mock_grains(mdb_clickhouse.__salt__, grains)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.shard_id() == result


def test_shard_id_failed():
    mock_grains(mdb_clickhouse.__salt__, {'id': 'man-1.db.yandex.net'})
    mock_pillar(mdb_clickhouse.__salt__, {})
    with pytest.raises(KeyError):
        mdb_clickhouse.shard_id()


@parametrize(
    {
        'id': 'MDB cluster',
        'args': {
            'grains': {
                'id': 'man-1.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'shard_name': 'shard1',
                    },
                },
            },
            'result': 'shard1',
        },
    },
    {
        'id': 'Non-MDB cluster',
        'args': {
            'grains': {
                'id': 'man-1.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'clickhouse': {
                        'shards': {
                            'shard1': {
                                'replicas': [
                                    'man-1.db.yandex.net',
                                    'sas-1.db.yandex.net',
                                ],
                            },
                        },
                    },
                },
            },
            'result': 'shard1',
        },
    },
)
def test_shard_name_succeeded(grains, pillar, result):
    mock_grains(mdb_clickhouse.__salt__, grains)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.shard_name() == result


def test_shard_name_failed():
    mock_grains(mdb_clickhouse.__salt__, {'id': 'man-1.db.yandex.net'})
    mock_pillar(mdb_clickhouse.__salt__, {})
    with pytest.raises(KeyError):
        mdb_clickhouse.shard_name()


@parametrize(
    {
        'id': 'default ZooKeeper root',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                    },
                },
            },
            'result': '/clickhouse/cid1',
        },
    },
    {
        'id': 'custom ZooKeeper root',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'zk_path': '/cluster1',
                    },
                },
            },
            'result': '/cluster1',
        },
    },
    {
        'id': 'no ZooKeeper root',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'zk_path': None,
                    },
                },
            },
            'result': None,
        },
    },
    {
        'id': 'default root for Embedded Keeper',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'embedded_keeper': True,
                    },
                },
            },
            'result': '/',
        },
    },
    {
        'id': 'custom root for Embedded Keeper',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'embedded_keeper': True,
                        'zk_path': '/cluster1',
                    },
                },
            },
            'result': '/cluster1',
        },
    },
)
def test_zookeeper_root(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.zookeeper_root() == result


@parametrize(
    {
        'id': 'Cluster with shared ZooKeeper (legacy)',
        'args': {
            'grains': {'id': 'sas-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'sas',
                    },
                    'clickhouse': {
                        'zk_hosts': [
                            'zk-1.db.yandex.net',
                            'zk-2.db.yandex.net',
                            'zk-3.db.yandex.net',
                        ],
                    },
                },
            },
            'result': ['zk-1.db.yandex.net', 'zk-2.db.yandex.net', 'zk-3.db.yandex.net'],
        },
    },
    {
        'id': 'Cluster with dedicated ZooKeeper',
        'args': {
            'grains': {'id': 'sas-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'sas',
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'hosts': {
                                        'sas-1.db.yandex.net': {'geo': 'sas'},
                                        'vla-1.db.yandex.net': {'geo': 'vla'},
                                    },
                                },
                                'subcid2': {
                                    'roles': ['zk'],
                                    'hosts': {
                                        'man-2.db.yandex.net': {'geo': 'man'},
                                        'sas-2.db.yandex.net': {'geo': 'sas'},
                                        'vla-2.db.yandex.net': {'geo': 'vla'},
                                    },
                                },
                            },
                        },
                    },
                },
            },
            'result': ['sas-2.db.yandex.net', 'man-2.db.yandex.net', 'vla-2.db.yandex.net'],
        },
    },
    {
        'id': 'Cluster with ClickHouse Keeper',
        'args': {
            'grains': {'id': 'sas-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'sas',
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'hosts': {
                                        'man-1.db.yandex.net': {'geo': 'man'},
                                        'sas-1.db.yandex.net': {'geo': 'sas'},
                                        'vla-1.db.yandex.net': {'geo': 'vla'},
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'zk_hosts': [
                            'vla-1.db.yandex.net',
                            'man-1.db.yandex.net',
                            'sas-1.db.yandex.net',
                        ],
                    },
                },
            },
            'result': ['sas-1.db.yandex.net', 'man-1.db.yandex.net', 'vla-1.db.yandex.net'],
        },
    },
    {
        'id': 'Cluster with Keeper and pillar data with `keeper_hosts`',
        'args': {
            'grains': {'id': 'sas-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'sas',
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'hosts': {
                                        'man-1.db.yandex.net': {'geo': 'man'},
                                        'sas-1.db.yandex.net': {'geo': 'sas'},
                                        'vla-1.db.yandex.net': {'geo': 'vla'},
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'keeper_hosts': {
                            'vla-1.db.yandex.net': 1,
                            'man-1.db.yandex.net': 2,
                            'sas-1.db.yandex.net': 3,
                        },
                    },
                },
            },
            'result': [
                'man-1.db.yandex.net',
                'sas-1.db.yandex.net',
                'vla-1.db.yandex.net',
            ]
        },
    },
    {
        'id': 'Cluster without ZooKeeper and ClickHouse Keeper',
        'args': {
            'grains': {'id': 'sas-1.db.yandex.net'},
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'sas',
                    },
                },
            },
            'result': [],
        },
    },
)
def test_zookeeper_hosts(grains, pillar, result):
    mock_grains(mdb_clickhouse.__salt__, grains)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.zookeeper_hosts() == result


@parametrize(
    {
        'id': 'Empty pillar',
        'args': {
            'pillar': {},
            'result': False,
        },
    },
    {
        'id': 'Pillar with user_replication_enabled=True',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'zk_hosts': ['sas-1.db.yandex.net'],
                        'user_replication_enabled': True,
                    },
                },
            },
            'result': True,
        },
    },
    {
        'id': 'Pillar with user_replication_enabled=False',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'zk_hosts': ['sas-1.db.yandex.net'],
                        'user_replication_enabled': False,
                    },
                },
            },
            'result': False,
        },
    },
)
def test_user_replication_enabled(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.user_replication_enabled() == result


@parametrize(
    {
        'id': 'pillar with use_ssl=True',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'use_ssl': True,
                    },
                },
                'cert.key': 'noop key',
            },
            'result': True,
        },
    },
    {
        'id': 'pillar with use_ssl=False',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'use_ssl': False,
                    },
                },
            },
            'result': False,
        },
    },
    {
        'id': 'default with key',
        'args': {
            'pillar': {'cert.key': 'noop key'},
            'result': True,
        },
    },
    {
        'id': 'default',
        'args': {
            'pillar': {},
            'result': False,
        },
    },
)
def test_ssl_enabled(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.ssl_enabled() == result


@parametrize(
    {
        'id': 'SSL is enforced by default',
        'args': {
            'pillar': {'cert.key': 'noop key'},
            'result': True,
        },
    },
    {
        'id': 'SSL is not enforced when SSL is disabled (use_ssl=False)',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'use_ssl': False,
                    },
                },
            },
            'result': False,
        },
    },
    {
        'id': 'SSL is enforced when SSL is enabled (use_ssl=True)',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'use_ssl': True,
                    },
                },
                'cert.key': 'noop key',
            },
            'result': True,
        },
    },
    {
        'id': 'SSL is not enforced when assign_public_ip is False (compute)',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                        'assign_public_ip': False,
                    },
                },
                'cert.key': 'noop key',
            },
            'result': False,
        },
    },
    {
        'id': 'SSL is enforced when assign_public_ip is False (porto)',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'porto',
                        'assign_public_ip': False,
                    },
                },
                'cert.key': 'noop key',
            },
            'result': True,
        },
    },
)
def test_ssl_enforced(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.ssl_enforced() == result


@parametrize(
    {
        'id': 'default',
        'args': {
            'pillar': {},
            'result': 'sslv2,sslv3,tlsv1,tlsv1_1',
        },
    },
    {
        'id': 'enabled TLS 1.0 and TLS 1.1',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'allow_tlsv1_1': True,
                    },
                },
            },
            'result': 'sslv2,sslv3',
        },
    },
)
def test_disabled_ssl_protocols(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.disabled_ssl_protocols() == result


@parametrize(
    {
        'id': 'empty pillar',
        'args': {
            'pillar': {},
            'result': 9000,
        },
    },
    {
        'id': 'pillar with explicit tcp_port',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'tcp_port': 9001,
                    },
                },
            },
            'result': 9001,
        },
    },
)
def test_tcp_port(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.tcp_port() == result


@parametrize(
    {
        'id': 'SSL is disabled',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'use_ssl': False,
                    },
                },
            },
            'result': {
                'tcp_port': 9000,
                'http_port': 8123,
                'interserver_http_port': 9009,
            },
        },
    },
    {
        'id': 'SSL is enabled (porto)',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'porto',
                    },
                    'clickhouse': {
                        'tcp_port': 9001,
                        'use_ssl': True,
                    },
                },
                'cert.key': 'noop key',
            },
            'result': {
                'tcp_port': 9001,
                'tcp_port_secure': 9440,
                'https_port': 8443,
                'interserver_https_port': 9010,
            },
        },
    },
    {
        'id': 'SSL is enabled (compute)',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                    },
                    'clickhouse': {
                        'use_ssl': True,
                    },
                },
                'cert.key': 'noop key',
            },
            'result': {
                'tcp_port': 9000,
                'http_port': 8123,
                'tcp_port_secure': 9440,
                'https_port': 8443,
                'interserver_https_port': 9010,
            },
        },
    },
)
def test_port_settings(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert sorted(mdb_clickhouse.port_settings()) == sorted(result)


@parametrize(
    {
        'id': 'SSL is enforced',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                        'assign_public_ip': True,
                    },
                    'clickhouse': {
                        'use_ssl': True,
                    },
                },
                'cert.key': 'noop key',
            },
            'result': [8443, 9440],
        },
    },
    {
        'id': 'SSL is not enforced',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                        'assign_public_ip': False,
                    },
                    'clickhouse': {
                        'use_ssl': True,
                    },
                },
                'cert.key': 'noop key',
            },
            'result': [8123, 8443, 9000, 9440],
        },
    },
    {
        'id': 'SSL is enforced and MySQL protocol is enabled',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                        'assign_public_ip': True,
                    },
                    'clickhouse': {
                        'mysql_protocol': True,
                        'use_ssl': True,
                    },
                },
                'cert.key': 'noop key',
            },
            'result': [3306, 8443, 9440],
        },
    },
    {
        'id': 'SSL is not enforced and MySQL & PostrgeSQL protocols are enabled',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                        'assign_public_ip': False,
                    },
                    'clickhouse': {
                        'mysql_protocol': True,
                        'postgresql_protocol': True,
                        'use_ssl': True,
                    },
                },
                'cert.key': 'noop key',
            },
            'result': [3306, 5433, 8123, 8443, 9000, 9440],
        },
    },
    {
        'id': 'DoubleCloud Prometheus port opened',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'aws',
                        'assign_public_ip': True,
                    },
                    'clickhouse': {
                        'use_ssl': True,
                    },
                },
                'cert.key': 'noop key',
            },
            'result': [8443, 9363, 9440],
        },
    },
)
def test_open_ports(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    mock_vtype(mdb_clickhouse.__salt__, pillar['data']['dbaas']['vtype'])
    assert sorted(mdb_clickhouse.open_ports()) == sorted(result)


@parametrize(
    {
        'id': 'default settings, db1.nano, ClickHouse 21.8',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 1073741824,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.8.15.7',
                    },
                },
            },
            'result': {
                'default_database': 'default',
                'default_profile': 'default',
                'max_connections': 4096,
                'keep_alive_timeout': 3,
                'max_concurrent_queries': 500,
                'max_server_memory_usage': 654311424,
                'uncompressed_cache_size': 8589934592,
                'mark_cache_size': 268435456,
                'builtin_dictionaries_reload_interval': 3600,
                'custom_settings_prefixes': 'custom_',
            },
        },
    },
    {
        'id': 'default settings, s3.medium, ClickHouse 21.8',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 8589934592,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.8.15.7',
                    },
                },
            },
            'result': {
                'default_database': 'default',
                'default_profile': 'default',
                'max_connections': 4096,
                'keep_alive_timeout': 3,
                'max_concurrent_queries': 500,
                'max_server_memory_usage': 7516192768,
                'uncompressed_cache_size': 8589934592,
                'mark_cache_size': 2147483648,
                'builtin_dictionaries_reload_interval': 3600,
                'custom_settings_prefixes': 'custom_',
            },
        },
    },
    {
        'id': 'custom settings, ClickHouse 21.8',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 4294967296,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.8.15.7',
                        'config': {
                            'default_database': 'db1',
                            'default_profile': 'profile1',
                            'max_connections': 1024,
                            'keep_alive_timeout': 5,
                            'max_concurrent_queries': 200,
                            'uncompressed_cache_size': 1073741824,
                            'mark_cache_size': 1073741824,
                            'background_pool_size': 32,
                            'background_schedule_pool_size': 32,
                            'builtin_dictionaries_reload_interval': 7200,
                            'log_level': 'trace',
                            'ssl_client_verification_mode': 'none',
                            'geobase_uri': 'https://storage.yandexcloud.net/test_bucket/geobase.tar.gz',
                        },
                    },
                },
            },
            'result': {
                'default_database': 'db1',
                'default_profile': 'profile1',
                'max_connections': 1024,
                'keep_alive_timeout': 5,
                'max_concurrent_queries': 200,
                'uncompressed_cache_size': 1073741824,
                'mark_cache_size': 1073741824,
                'builtin_dictionaries_reload_interval': 7200,
                'custom_settings_prefixes': 'custom_',
                'max_server_memory_usage': 3221225472,
            },
        },
    },
    {
        'id': 'default settings, db1.nano, ClickHouse 22.3',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 1073741824,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '22.3.2.2',
                    },
                },
            },
            'result': {
                'default_database': 'default',
                'default_profile': 'default',
                'max_connections': 4096,
                'keep_alive_timeout': 3,
                'max_concurrent_queries': 500,
                'max_server_memory_usage': 654311424,
                'uncompressed_cache_size': 8589934592,
                'mark_cache_size': 268435456,
                'builtin_dictionaries_reload_interval': 3600,
                'custom_settings_prefixes': 'custom_',
                'allow_no_password': 0,
            },
        },
    },
    {
        'id': 'default settings, s3.medium, ClickHouse 22.3',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 8589934592,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '22.3.2.2',
                    },
                },
            },
            'result': {
                'default_database': 'default',
                'default_profile': 'default',
                'max_connections': 4096,
                'keep_alive_timeout': 3,
                'max_concurrent_queries': 500,
                'max_server_memory_usage': 7516192768,
                'uncompressed_cache_size': 8589934592,
                'mark_cache_size': 2147483648,
                'builtin_dictionaries_reload_interval': 3600,
                'custom_settings_prefixes': 'custom_',
                'allow_no_password': 0,
            },
        },
    },
    {
        'id': 'custom settings, ClickHouse 22.3',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 4294967296,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '22.3.2.2',
                        'config': {
                            'default_database': 'db1',
                            'default_profile': 'profile1',
                            'max_connections': 1024,
                            'keep_alive_timeout': 5,
                            'max_concurrent_queries': 200,
                            'uncompressed_cache_size': 1073741824,
                            'mark_cache_size': 1073741824,
                            'background_pool_size': 32,
                            'background_schedule_pool_size': 32,
                            'builtin_dictionaries_reload_interval': 7200,
                            'log_level': 'trace',
                            'ssl_client_verification_mode': 'none',
                            'geobase_uri': 'https://storage.yandexcloud.net/test_bucket/geobase.tar.gz',
                            'allow_plaintext_password': 1,
                            'allow_no_password': 1,
                        },
                    },
                },
            },
            'result': {
                'default_database': 'db1',
                'default_profile': 'profile1',
                'max_connections': 1024,
                'keep_alive_timeout': 5,
                'max_concurrent_queries': 200,
                'uncompressed_cache_size': 1073741824,
                'mark_cache_size': 1073741824,
                'builtin_dictionaries_reload_interval': 7200,
                'custom_settings_prefixes': 'custom_',
                'max_server_memory_usage': 3221225472,
                'allow_plaintext_password': 1,
                'allow_no_password': 1,
            },
        },
    },
    {
        'id': 'default settings, db1.nano, ClickHouse 22.5',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 1073741824,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '22.5.1.2079',
                    },
                },
            },
            'result': {
                'default_database': 'default',
                'default_profile': 'default',
                'max_connections': 4096,
                'keep_alive_timeout': 3,
                'max_concurrent_queries': 500,
                'max_server_memory_usage': 654311424,
                'uncompressed_cache_size': 8589934592,
                'mark_cache_size': 268435456,
                'builtin_dictionaries_reload_interval': 3600,
                'custom_settings_prefixes': 'custom_',
                'allow_no_password': 0,
                'dictionaries_lazy_load': 0,
            },
        },
    },
    {
        'id': 'default settings, s3.medium, ClickHouse 22.5',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 8589934592,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '22.5.1.2079',
                    },
                },
            },
            'result': {
                'default_database': 'default',
                'default_profile': 'default',
                'max_connections': 4096,
                'keep_alive_timeout': 3,
                'max_concurrent_queries': 500,
                'max_server_memory_usage': 7516192768,
                'uncompressed_cache_size': 8589934592,
                'mark_cache_size': 2147483648,
                'builtin_dictionaries_reload_interval': 3600,
                'custom_settings_prefixes': 'custom_',
                'allow_no_password': 0,
                'dictionaries_lazy_load': 0,
            },
        },
    },
    {
        'id': 'custom settings, ClickHouse 22.5',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 4294967296,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '22.5.1.2079',
                        'config': {
                            'default_database': 'db1',
                            'default_profile': 'profile1',
                            'max_connections': 1024,
                            'keep_alive_timeout': 5,
                            'max_concurrent_queries': 200,
                            'uncompressed_cache_size': 1073741824,
                            'mark_cache_size': 1073741824,
                            'background_pool_size': 32,
                            'background_schedule_pool_size': 32,
                            'builtin_dictionaries_reload_interval': 7200,
                            'log_level': 'trace',
                            'ssl_client_verification_mode': 'none',
                            'geobase_uri': 'https://storage.yandexcloud.net/test_bucket/geobase.tar.gz',
                            'allow_plaintext_password': 1,
                            'allow_no_password': 1,
                        },
                    },
                },
            },
            'result': {
                'default_database': 'db1',
                'default_profile': 'profile1',
                'max_connections': 1024,
                'keep_alive_timeout': 5,
                'max_concurrent_queries': 200,
                'uncompressed_cache_size': 1073741824,
                'mark_cache_size': 1073741824,
                'builtin_dictionaries_reload_interval': 7200,
                'custom_settings_prefixes': 'custom_',
                'max_server_memory_usage': 3221225472,
                'allow_plaintext_password': 1,
                'allow_no_password': 1,
                'dictionaries_lazy_load': 0,
            },
        },
    },
)
def test_server_settings(pillar, result):
    mock_version_cmp(mdb_clickhouse.__salt__)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.server_settings() == result


@parametrize(
    {
        'id': 'ClickHouse 21.3, default settings, s2.medium flavor',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'cpu_guarantee': 4.0,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.3.20.1',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'max_part_loading_threads': 8,
                'max_part_removal_threads': 8,
            },
        },
    },
    {
        'id': 'ClickHouse 21.3, default settings, b2.micro flavor',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'cpu_guarantee': 0.4,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.3.20.1',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'max_part_loading_threads': 1,
                'max_part_removal_threads': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.3, default settings, no flavor',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '21.3.20.1',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.3, default settings, empty flavor',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {},
                    },
                    'clickhouse': {
                        'ch_version': '21.3.20.1',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.3, custom settings',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'cpu_guarantee': 4.0,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.3.20.1',
                        'config': {
                            'merge_tree': {
                                'replicated_deduplication_window': 100,
                                'replicated_deduplication_window_seconds': 604800,
                                'parts_to_delay_insert': 300,
                                'parts_to_throw_insert': 600,
                                'max_replicated_merges_in_queue': 6,
                                'number_of_free_entries_in_pool_to_lower_max_size_of_merge': 5,
                                'max_bytes_to_merge_at_min_space_in_pool': 1073741824,
                                'max_part_loading_threads': 10,
                                'max_part_removal_threads': 10,
                            },
                        },
                    },
                },
            },
            'result': {
                'replicated_deduplication_window': 100,
                'replicated_deduplication_window_seconds': 604800,
                'parts_to_delay_insert': 300,
                'parts_to_throw_insert': 600,
                'max_replicated_merges_in_queue': 6,
                'number_of_free_entries_in_pool_to_lower_max_size_of_merge': 5,
                'max_bytes_to_merge_at_min_space_in_pool': 1073741824,
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'max_part_loading_threads': 10,
                'max_part_removal_threads': 10,
            },
        },
    },
    {
        'id': 'ClickHouse 21.4, default settings, s2.medium flavor',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'cpu_guarantee': 4.0,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.4.1.6414',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'max_part_loading_threads': 8,
                'max_part_removal_threads': 8,
                'allow_s3_zero_copy_replication': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.4, default settings, b2.micro flavor',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'cpu_guarantee': 0.4,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.4.1.6414',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'max_part_loading_threads': 1,
                'max_part_removal_threads': 1,
                'allow_s3_zero_copy_replication': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.4, default settings, no flavor',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '21.4.1.6414',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'allow_s3_zero_copy_replication': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.4, default settings, empty flavor',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {},
                    },
                    'clickhouse': {
                        'ch_version': '21.4.1.6414',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'allow_s3_zero_copy_replication': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.4, custom settings',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'cpu_guarantee': 4.0,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.4.1.6414',
                        'config': {
                            'merge_tree': {
                                'replicated_deduplication_window': 100,
                                'replicated_deduplication_window_seconds': 604800,
                                'parts_to_delay_insert': 300,
                                'parts_to_throw_insert': 600,
                                'max_replicated_merges_in_queue': 6,
                                'number_of_free_entries_in_pool_to_lower_max_size_of_merge': 5,
                                'max_bytes_to_merge_at_min_space_in_pool': 1073741824,
                                'max_part_loading_threads': 10,
                                'max_part_removal_threads': 10,
                            },
                        },
                    },
                },
            },
            'result': {
                'replicated_deduplication_window': 100,
                'replicated_deduplication_window_seconds': 604800,
                'parts_to_delay_insert': 300,
                'parts_to_throw_insert': 600,
                'max_replicated_merges_in_queue': 6,
                'number_of_free_entries_in_pool_to_lower_max_size_of_merge': 5,
                'max_bytes_to_merge_at_min_space_in_pool': 1073741824,
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'max_part_loading_threads': 10,
                'max_part_removal_threads': 10,
                'allow_s3_zero_copy_replication': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.8, default settings, s2.medium flavor',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'cpu_guarantee': 4.0,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.8.3.44',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'max_part_loading_threads': 8,
                'max_part_removal_threads': 8,
                'allow_remote_fs_zero_copy_replication': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.8, default settings, b2.micro flavor',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'cpu_guarantee': 0.4,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.8.3.44',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'max_part_loading_threads': 1,
                'max_part_removal_threads': 1,
                'allow_remote_fs_zero_copy_replication': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.8, default settings, no flavor',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '21.8.3.44',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'allow_remote_fs_zero_copy_replication': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.8, default settings, empty flavor',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {},
                    },
                    'clickhouse': {
                        'ch_version': '21.8.3.44',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'allow_remote_fs_zero_copy_replication': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.8, custom settings',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'cpu_guarantee': 4.0,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.8.3.44',
                        'config': {
                            'merge_tree': {
                                'replicated_deduplication_window': 100,
                                'replicated_deduplication_window_seconds': 604800,
                                'parts_to_delay_insert': 300,
                                'parts_to_throw_insert': 600,
                                'max_replicated_merges_in_queue': 6,
                                'number_of_free_entries_in_pool_to_lower_max_size_of_merge': 5,
                                'max_bytes_to_merge_at_min_space_in_pool': 1073741824,
                                'max_part_loading_threads': 10,
                                'max_part_removal_threads': 10,
                            },
                        },
                    },
                },
            },
            'result': {
                'replicated_deduplication_window': 100,
                'replicated_deduplication_window_seconds': 604800,
                'parts_to_delay_insert': 300,
                'parts_to_throw_insert': 600,
                'max_replicated_merges_in_queue': 6,
                'number_of_free_entries_in_pool_to_lower_max_size_of_merge': 5,
                'max_bytes_to_merge_at_min_space_in_pool': 1073741824,
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'max_part_loading_threads': 10,
                'max_part_removal_threads': 10,
                'allow_remote_fs_zero_copy_replication': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.11, default settings, s2.medium flavor',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'cpu_guarantee': 4.0,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.11.5.33',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'max_part_loading_threads': 8,
                'max_part_removal_threads': 8,
                'allow_remote_fs_zero_copy_replication': 1,
                'max_suspicious_broken_parts_bytes': 107374182400,
            },
        },
    },
    {
        'id': 'ClickHouse 21.11, default settings, b2.micro flavor',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'cpu_guarantee': 0.4,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.11.5.33',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'max_part_loading_threads': 1,
                'max_part_removal_threads': 1,
                'allow_remote_fs_zero_copy_replication': 1,
                'max_suspicious_broken_parts_bytes': 107374182400,
            },
        },
    },
    {
        'id': 'ClickHouse 21.11, default settings, no flavor',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '21.11.5.33',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'allow_remote_fs_zero_copy_replication': 1,
                'max_suspicious_broken_parts_bytes': 107374182400,
            },
        },
    },
    {
        'id': 'ClickHouse 21.11, default settings, empty flavor',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {},
                    },
                    'clickhouse': {
                        'ch_version': '21.11.5.33',
                    },
                },
            },
            'result': {
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'allow_remote_fs_zero_copy_replication': 1,
                'max_suspicious_broken_parts_bytes': 107374182400,
            },
        },
    },
    {
        'id': 'ClickHouse 21.11, custom settings',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'cpu_guarantee': 4.0,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.11.5.33',
                        'config': {
                            'merge_tree': {
                                'replicated_deduplication_window': 100,
                                'replicated_deduplication_window_seconds': 604800,
                                'parts_to_delay_insert': 300,
                                'parts_to_throw_insert': 600,
                                'max_replicated_merges_in_queue': 6,
                                'number_of_free_entries_in_pool_to_lower_max_size_of_merge': 5,
                                'max_bytes_to_merge_at_min_space_in_pool': 1073741824,
                                'max_part_loading_threads': 10,
                                'max_part_removal_threads': 10,
                            },
                        },
                    },
                },
            },
            'result': {
                'replicated_deduplication_window': 100,
                'replicated_deduplication_window_seconds': 604800,
                'parts_to_delay_insert': 300,
                'parts_to_throw_insert': 600,
                'max_replicated_merges_in_queue': 6,
                'number_of_free_entries_in_pool_to_lower_max_size_of_merge': 5,
                'max_bytes_to_merge_at_min_space_in_pool': 1073741824,
                'max_suspicious_broken_parts': 1000000,
                'replicated_max_ratio_of_wrong_parts': 1,
                'max_files_to_modify_in_alter_columns': 1000000,
                'max_files_to_remove_in_alter_columns': 1000000,
                'use_minimalistic_part_header_in_zookeeper': 1,
                'max_part_loading_threads': 10,
                'max_part_removal_threads': 10,
                'allow_remote_fs_zero_copy_replication': 1,
                'max_suspicious_broken_parts_bytes': 107374182400,
            },
        },
    },
)
def test_merge_tree_settings(pillar, result):
    mock_version_cmp(mdb_clickhouse.__salt__)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.merge_tree_settings() == result


def test_version_succeeded():
    version = '19.14.7.17'
    mock_pillar(
        mdb_clickhouse.__salt__,
        {
            'data': {
                'clickhouse': {
                    'ch_version': version,
                },
            },
        },
    )
    assert mdb_clickhouse.version() == version


def test_version_failed():
    mock_pillar(mdb_clickhouse.__salt__, {})
    with pytest.raises(Exception):
        mdb_clickhouse.version()


def test_version_greater_or_equal():
    version = '19.14.7.17'
    mock_version_cmp(mdb_clickhouse.__salt__)
    mock_pillar(
        mdb_clickhouse.__salt__,
        {
            'data': {
                'clickhouse': {
                    'ch_version': version,
                },
            },
        },
    )
    assert mdb_clickhouse.version_ge(version)
    assert mdb_clickhouse.version_ge('18.1.1')
    assert not mdb_clickhouse.version_ge('20.1.1')


def test_version_less_than():
    version = '19.14.7.17'
    mock_version_cmp(mdb_clickhouse.__salt__)
    mock_pillar(
        mdb_clickhouse.__salt__,
        {
            'data': {
                'clickhouse': {
                    'ch_version': version,
                },
            },
        },
    )
    assert not mdb_clickhouse.version_lt(version)
    assert mdb_clickhouse.version_lt('20.1.1.1')


def test_databases():
    mock_pillar(
        mdb_clickhouse.__salt__,
        {
            'data': {
                'clickhouse': {
                    'databases': ['db1'],
                },
            },
        },
    )
    assert mdb_clickhouse.databases() == ['db1']


@parametrize(
    {
        'id': 'ClickHouse 20.8, default settings',
        'args': {
            'vtype': 'compute',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.8.12.2',
                        'users': {
                            'test_user': {},
                        },
                    },
                },
            },
            'result': {
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_concurrent_queries_for_user': 450,
                'insert_distributed_sync': 1,
                'distributed_directory_monitor_batch_inserts': 1,
                'join_algorithm': 'auto',
                'partial_merge_join_optimizations': 0,
                'allow_drop_detached': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 20.8, customized settings',
        'args': {
            'vtype': 'compute',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.8.12.2',
                        'users': {
                            'test_user': {
                                'settings': {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'max_memory_usage_for_user': 21474836480,
                                    'max_concurrent_queries_for_user': 50,
                                    'load_balancing': 'random',
                                    'insert_distributed_sync': 0,
                                    'insert_quorum_parallel': 1,
                                    'deduplicate_blocks_in_dependent_materialized_views': 1,
                                    'use_uncompressed_cache': 1,
                                    'force_primary_key': 1,
                                    'force_index_by_date': 1,
                                    'join_algorithm': 'hash',
                                    'transform_null_in': 1,
                                    'compile': 1,
                                    'min_count_to_compile': 3,
                                    'format_regexp': '^(.+?) separator (.+?)$',
                                    'format_regexp_escaping_rule': 'Raw',
                                    'format_regexp_skip_unmatched': 1,
                                    'date_time_output_format': 'iso',
                                    'quota_mode': 'keyed',
                                },
                            },
                        },
                    },
                },
            },
            'result': {
                'readonly': 1,
                'allow_dll': 0,
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_memory_usage_for_user': 21474836480,
                'max_concurrent_queries_for_user': 50,
                'load_balancing': 'random',
                'insert_distributed_sync': 0,
                'distributed_directory_monitor_batch_inserts': 1,
                'deduplicate_blocks_in_dependent_materialized_views': 1,
                'use_uncompressed_cache': 1,
                'force_primary_key': 1,
                'force_index_by_date': 1,
                'join_algorithm': 'hash',
                'partial_merge_join_optimizations': 0,
                'transform_null_in': 1,
                'allow_drop_detached': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 20.10, default settings',
        'args': {
            'vtype': 'compute',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.10.7.4',
                        'users': {
                            'test_user': {},
                        },
                    },
                },
            },
            'result': {
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_concurrent_queries_for_user': 450,
                'insert_distributed_sync': 1,
                'distributed_directory_monitor_batch_inserts': 1,
                'join_algorithm': 'auto',
                'partial_merge_join_optimizations': 0,
                'allow_drop_detached': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 20.10, customized settings',
        'args': {
            'vtype': 'compute',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.10.7.4',
                        'users': {
                            'test_user': {
                                'settings': {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'max_memory_usage_for_user': 21474836480,
                                    'max_concurrent_queries_for_user': 50,
                                    'load_balancing': 'random',
                                    'insert_distributed_sync': 0,
                                    'insert_quorum_parallel': 1,
                                    'deduplicate_blocks_in_dependent_materialized_views': 1,
                                    'use_uncompressed_cache': 1,
                                    'force_primary_key': 1,
                                    'force_index_by_date': 1,
                                    'join_algorithm': 'hash',
                                    'transform_null_in': 1,
                                    'compile': 1,
                                    'min_count_to_compile': 3,
                                    'format_regexp': '^(.+?) separator (.+?)$',
                                    'format_regexp_escaping_rule': 'Raw',
                                    'format_regexp_skip_unmatched': 1,
                                    'date_time_output_format': 'iso',
                                    'quota_mode': 'keyed',
                                },
                            },
                        },
                    },
                },
            },
            'result': {
                'readonly': 1,
                'allow_dll': 0,
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_memory_usage_for_user': 21474836480,
                'max_concurrent_queries_for_user': 50,
                'load_balancing': 'random',
                'insert_distributed_sync': 0,
                'distributed_directory_monitor_batch_inserts': 1,
                'insert_quorum_parallel': 1,
                'deduplicate_blocks_in_dependent_materialized_views': 1,
                'use_uncompressed_cache': 1,
                'force_primary_key': 1,
                'force_index_by_date': 1,
                'join_algorithm': 'hash',
                'partial_merge_join_optimizations': 0,
                'transform_null_in': 1,
                'format_regexp': '^(.+?) separator (.+?)$',
                'format_regexp_escaping_rule': 'Raw',
                'format_regexp_skip_unmatched': 1,
                'allow_drop_detached': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 20.11, default settings',
        'args': {
            'vtype': 'compute',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.11.7.16',
                        'users': {
                            'test_user': {},
                        },
                    },
                },
            },
            'result': {
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_concurrent_queries_for_user': 450,
                'insert_distributed_sync': 1,
                'distributed_directory_monitor_batch_inserts': 1,
                'join_algorithm': 'auto',
                'partial_merge_join_optimizations': 0,
                'allow_drop_detached': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 20.11, customized settings',
        'args': {
            'vtype': 'compute',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.11.7.16',
                        'users': {
                            'test_user': {
                                'settings': {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'max_memory_usage_for_user': 21474836480,
                                    'max_concurrent_queries_for_user': 50,
                                    'load_balancing': 'random',
                                    'insert_distributed_sync': 0,
                                    'insert_quorum_parallel': 1,
                                    'deduplicate_blocks_in_dependent_materialized_views': 1,
                                    'use_uncompressed_cache': 1,
                                    'force_primary_key': 1,
                                    'force_index_by_date': 1,
                                    'join_algorithm': 'hash',
                                    'transform_null_in': 1,
                                    'compile': 1,
                                    'min_count_to_compile': 3,
                                    'format_regexp': '^(.+?) separator (.+?)$',
                                    'format_regexp_escaping_rule': 'Raw',
                                    'format_regexp_skip_unmatched': 1,
                                    'date_time_output_format': 'iso',
                                    'quota_mode': 'keyed',
                                },
                            },
                        },
                    },
                },
            },
            'result': {
                'readonly': 1,
                'allow_dll': 0,
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_memory_usage_for_user': 21474836480,
                'max_concurrent_queries_for_user': 50,
                'load_balancing': 'random',
                'insert_distributed_sync': 0,
                'distributed_directory_monitor_batch_inserts': 1,
                'insert_quorum_parallel': 1,
                'deduplicate_blocks_in_dependent_materialized_views': 1,
                'use_uncompressed_cache': 1,
                'force_primary_key': 1,
                'force_index_by_date': 1,
                'join_algorithm': 'hash',
                'partial_merge_join_optimizations': 0,
                'transform_null_in': 1,
                'format_regexp': '^(.+?) separator (.+?)$',
                'format_regexp_escaping_rule': 'Raw',
                'format_regexp_skip_unmatched': 1,
                'date_time_output_format': 'iso',
                'allow_drop_detached': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 20.12, default settings',
        'args': {
            'vtype': 'compute',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.12.6.29',
                        'users': {
                            'test_user': {},
                        },
                    },
                },
            },
            'result': {
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_concurrent_queries_for_user': 450,
                'insert_distributed_sync': 1,
                'distributed_directory_monitor_batch_inserts': 1,
                'join_algorithm': 'auto',
                'partial_merge_join_optimizations': 0,
                'allow_drop_detached': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 20.12, customized settings',
        'args': {
            'vtype': 'compute',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '20.12.6.29',
                        'users': {
                            'test_user': {
                                'settings': {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'max_memory_usage_for_user': 21474836480,
                                    'max_concurrent_queries_for_user': 50,
                                    'load_balancing': 'random',
                                    'insert_distributed_sync': 0,
                                    'insert_quorum_parallel': 1,
                                    'deduplicate_blocks_in_dependent_materialized_views': 1,
                                    'use_uncompressed_cache': 1,
                                    'force_primary_key': 1,
                                    'force_index_by_date': 1,
                                    'join_algorithm': 'hash',
                                    'transform_null_in': 1,
                                    'compile': 1,
                                    'min_count_to_compile': 3,
                                    'format_regexp': '^(.+?) separator (.+?)$',
                                    'format_regexp_escaping_rule': 'Raw',
                                    'format_regexp_skip_unmatched': 1,
                                    'date_time_output_format': 'iso',
                                    'quota_mode': 'keyed',
                                    'local_filesystem_read_method': 'read',
                                    'remote_filesystem_read_method': 'read',
                                },
                            },
                        },
                    },
                },
            },
            'result': {
                'readonly': 1,
                'allow_dll': 0,
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_memory_usage_for_user': 21474836480,
                'max_concurrent_queries_for_user': 50,
                'load_balancing': 'random',
                'insert_distributed_sync': 0,
                'distributed_directory_monitor_batch_inserts': 1,
                'insert_quorum_parallel': 1,
                'deduplicate_blocks_in_dependent_materialized_views': 1,
                'use_uncompressed_cache': 1,
                'force_primary_key': 1,
                'force_index_by_date': 1,
                'join_algorithm': 'hash',
                'partial_merge_join_optimizations': 0,
                'transform_null_in': 1,
                'format_regexp': '^(.+?) separator (.+?)$',
                'format_regexp_escaping_rule': 'Raw',
                'format_regexp_skip_unmatched': 1,
                'date_time_output_format': 'iso',
                'allow_drop_detached': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.12, default settings, Yandex Cloud',
        'args': {
            'vtype': 'compute',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.12.4.1',
                        'users': {
                            'test_user': {},
                        },
                    },
                },
            },
            'result': {
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_concurrent_queries_for_user': 450,
                'insert_distributed_sync': 1,
                'distributed_directory_monitor_batch_inserts': 1,
                'join_algorithm': 'auto',
                'allow_drop_detached': 1,
                'force_remove_data_recursively_on_drop': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.12, customized settings, Yandex Cloud',
        'args': {
            'vtype': 'compute',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.12.4.1',
                        'users': {
                            'test_user': {
                                'settings': {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'max_memory_usage_for_user': 21474836480,
                                    'max_concurrent_queries_for_user': 50,
                                    'load_balancing': 'random',
                                    'insert_distributed_sync': 0,
                                    'insert_quorum_parallel': 1,
                                    'deduplicate_blocks_in_dependent_materialized_views': 1,
                                    'use_uncompressed_cache': 1,
                                    'force_primary_key': 1,
                                    'force_index_by_date': 1,
                                    'join_algorithm': 'hash',
                                    'transform_null_in': 1,
                                    'compile': 1,
                                    'min_count_to_compile': 3,
                                    'format_regexp': '^(.+?) separator (.+?)$',
                                    'format_regexp_escaping_rule': 'Raw',
                                    'format_regexp_skip_unmatched': 1,
                                    'date_time_output_format': 'iso',
                                    'quota_mode': 'keyed',
                                    'local_filesystem_read_method': 'read',
                                    'remote_filesystem_read_method': 'read',
                                },
                            },
                        },
                    },
                },
            },
            'result': {
                'readonly': 1,
                'allow_dll': 0,
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_memory_usage_for_user': 21474836480,
                'max_concurrent_queries_for_user': 50,
                'load_balancing': 'random',
                'insert_distributed_sync': 0,
                'distributed_directory_monitor_batch_inserts': 1,
                'insert_quorum_parallel': 1,
                'deduplicate_blocks_in_dependent_materialized_views': 1,
                'use_uncompressed_cache': 1,
                'force_primary_key': 1,
                'force_index_by_date': 1,
                'join_algorithm': 'hash',
                'transform_null_in': 1,
                'format_regexp': '^(.+?) separator (.+?)$',
                'format_regexp_escaping_rule': 'Raw',
                'format_regexp_skip_unmatched': 1,
                'date_time_output_format': 'iso',
                'allow_drop_detached': 1,
                'force_remove_data_recursively_on_drop': 1,
                'local_filesystem_read_method': 'read',
                'remote_filesystem_read_method': 'read',
            },
        },
    },
    {
        'id': 'ClickHouse 21.12, default settings, Double Cloud',
        'args': {
            'vtype': 'aws',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.12.4.1',
                        'users': {
                            'test_user': {},
                        },
                    },
                },
            },
            'result': {
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_concurrent_queries_for_user': 450,
                'load_balancing': 'first_or_random',
                'insert_distributed_sync': 1,
                'distributed_directory_monitor_batch_inserts': 1,
                'join_algorithm': 'auto',
                'allow_drop_detached': 1,
                'force_remove_data_recursively_on_drop': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 21.12, customized settings, Double Cloud',
        'args': {
            'vtype': 'aws',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '21.12.4.1',
                        'users': {
                            'test_user': {
                                'settings': {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'max_memory_usage_for_user': 21474836480,
                                    'max_concurrent_queries_for_user': 50,
                                    'load_balancing': 'random',
                                    'insert_distributed_sync': 0,
                                    'insert_quorum_parallel': 1,
                                    'deduplicate_blocks_in_dependent_materialized_views': 1,
                                    'use_uncompressed_cache': 1,
                                    'force_primary_key': 1,
                                    'force_index_by_date': 1,
                                    'join_algorithm': 'hash',
                                    'transform_null_in': 1,
                                    'compile': 1,
                                    'min_count_to_compile': 3,
                                    'format_regexp': '^(.+?) separator (.+?)$',
                                    'format_regexp_escaping_rule': 'Raw',
                                    'format_regexp_skip_unmatched': 1,
                                    'date_time_output_format': 'iso',
                                    'quota_mode': 'keyed',
                                    'local_filesystem_read_method': 'read',
                                    'remote_filesystem_read_method': 'read',
                                },
                            },
                        },
                    },
                },
            },
            'result': {
                'readonly': 1,
                'allow_dll': 0,
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_memory_usage_for_user': 21474836480,
                'max_concurrent_queries_for_user': 50,
                'load_balancing': 'random',
                'insert_distributed_sync': 0,
                'distributed_directory_monitor_batch_inserts': 1,
                'insert_quorum_parallel': 1,
                'deduplicate_blocks_in_dependent_materialized_views': 1,
                'use_uncompressed_cache': 1,
                'force_primary_key': 1,
                'force_index_by_date': 1,
                'join_algorithm': 'hash',
                'transform_null_in': 1,
                'format_regexp': '^(.+?) separator (.+?)$',
                'format_regexp_escaping_rule': 'Raw',
                'format_regexp_skip_unmatched': 1,
                'date_time_output_format': 'iso',
                'allow_drop_detached': 1,
                'force_remove_data_recursively_on_drop': 1,
                'local_filesystem_read_method': 'read',
                'remote_filesystem_read_method': 'read',
            },
        },
    },
    {
        'id': 'ClickHouse 22.5, default settings, Double Cloud',
        'args': {
            'vtype': 'aws',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '22.5.1.2079',
                        'users': {
                            'test_user': {},
                        },
                    },
                },
            },
            'result': {
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_concurrent_queries_for_user': 450,
                'load_balancing': 'first_or_random',
                'insert_distributed_sync': 1,
                'distributed_directory_monitor_batch_inserts': 1,
                'join_algorithm': 'auto',
                'allow_drop_detached': 1,
                'force_remove_data_recursively_on_drop': 1,
                'database_atomic_wait_for_drop_and_detach_synchronously': 1,
            },
        },
    },
    {
        'id': 'ClickHouse 22.5, customized settings, Double Cloud',
        'args': {
            'vtype': 'aws',
            'pillar': {
                'data': {
                    'dbaas': {
                        'flavor': {
                            'memory_guarantee': 34359738368,
                        },
                    },
                    'clickhouse': {
                        'ch_version': '22.5.1.2079',
                        'users': {
                            'test_user': {
                                'settings': {
                                    'readonly': 1,
                                    'allow_dll': 0,
                                    'max_memory_usage': 10000000000,
                                    'max_memory_usage_for_user': 21474836480,
                                    'max_concurrent_queries_for_user': 50,
                                    'load_balancing': 'random',
                                    'insert_distributed_sync': 0,
                                    'insert_quorum_parallel': 1,
                                    'deduplicate_blocks_in_dependent_materialized_views': 1,
                                    'use_uncompressed_cache': 1,
                                    'force_primary_key': 1,
                                    'force_index_by_date': 1,
                                    'join_algorithm': 'hash',
                                    'transform_null_in': 1,
                                    'compile': 1,
                                    'min_count_to_compile': 3,
                                    'format_regexp': '^(.+?) separator (.+?)$',
                                    'format_regexp_escaping_rule': 'Raw',
                                    'format_regexp_skip_unmatched': 1,
                                    'date_time_output_format': 'iso',
                                    'quota_mode': 'keyed',
                                    'local_filesystem_read_method': 'read',
                                    'remote_filesystem_read_method': 'read',
                                },
                            },
                        },
                    },
                },
            },
            'result': {
                'readonly': 1,
                'allow_dll': 0,
                'log_queries': 1,
                'log_queries_cut_to_length': 10000000,
                'max_memory_usage': 10000000000,
                'max_memory_usage_for_user': 21474836480,
                'max_concurrent_queries_for_user': 50,
                'load_balancing': 'random',
                'insert_distributed_sync': 0,
                'distributed_directory_monitor_batch_inserts': 1,
                'insert_quorum_parallel': 1,
                'deduplicate_blocks_in_dependent_materialized_views': 1,
                'use_uncompressed_cache': 1,
                'force_primary_key': 1,
                'force_index_by_date': 1,
                'join_algorithm': 'hash',
                'transform_null_in': 1,
                'format_regexp': '^(.+?) separator (.+?)$',
                'format_regexp_escaping_rule': 'Raw',
                'format_regexp_skip_unmatched': 1,
                'date_time_output_format': 'iso',
                'allow_drop_detached': 1,
                'force_remove_data_recursively_on_drop': 1,
                'database_atomic_wait_for_drop_and_detach_synchronously': 1,
                'local_filesystem_read_method': 'read',
                'remote_filesystem_read_method': 'read',
            },
        },
    },
)
def test_user_settings(vtype, pillar, result):
    mock_vtype(mdb_clickhouse.__salt__, vtype)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.user_settings('test_user') == result


@parametrize(
    {
        'id': 'user with access to one database, ClickHouse 20.8',
        'args': {
            'version': '20.8.18.32',
            'databases': ['db1', 'db2'],
            'user_databases': ['db1'],
            'result': USER_COMMON_GRANT_LIST
            + [
                grant('SELECT', SYSTEM_DATABASES + [TMP_DATABASE, 'db1']),
                grant('TRUNCATE', SYSTEM_DATABASES + ['db1']),
                grant('OPTIMIZE', SYSTEM_DATABASES + ['db1']),
                grant('DROP', SYSTEM_DATABASES + ['db1']),
                grant('INSERT', ['db1']),
                grant('CREATE DATABASE', ['db1']),
                grant('CREATE DICTIONARY', ['db1']),
                grant('CREATE VIEW', ['db1']),
                grant('CREATE TABLE', ['db1']),
                grant('ALTER'),
                grant_except_databases('ALTER COLUMN', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER ORDER BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER SAMPLE BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER ADD INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER DROP INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER CLEAR INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER CONSTRAINT', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER TTL', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER SETTINGS', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER MOVE PARTITION', [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER FETCH PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER FREEZE PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER VIEW', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
            ],
        },
    },
    {
        'id': 'user without access to multiple databases, ClickHouse 20.8',
        'args': {
            'version': '20.8.18.32',
            'databases': ['db1', 'db2'],
            'user_databases': [],
            'result': USER_COMMON_GRANT_LIST
            + [
                grant('SELECT', SYSTEM_DATABASES + [TMP_DATABASE]),
                grant('TRUNCATE', SYSTEM_DATABASES),
                grant('OPTIMIZE', SYSTEM_DATABASES),
                grant('DROP', SYSTEM_DATABASES),
                grant('ALTER'),
                grant_except_databases('ALTER COLUMN', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER ORDER BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER SAMPLE BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER ADD INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER DROP INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER CLEAR INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER CONSTRAINT', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER TTL', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER SETTINGS', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER MOVE PARTITION', [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER FETCH PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases(
                    'ALTER FREEZE PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']
                ),
                grant_except_databases('ALTER VIEW', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
            ],
        },
    },
    {
        'id': 'user with access to one database, ClickHouse 21.2',
        'args': {
            'version': '21.2.10.48',
            'databases': ['db1', 'db2'],
            'user_databases': ['db1'],
            'result': USER_COMMON_GRANT_LIST
            + [
                grant('SELECT', SYSTEM_DATABASES + [TMP_DATABASE, 'db1']),
                grant('TRUNCATE', SYSTEM_DATABASES + ['db1']),
                grant('OPTIMIZE', SYSTEM_DATABASES + ['db1']),
                grant('DROP', SYSTEM_DATABASES + ['db1']),
                grant('INSERT', ['db1']),
                grant('CREATE DATABASE', ['db1']),
                grant('CREATE DICTIONARY', ['db1']),
                grant('CREATE VIEW', ['db1']),
                grant('CREATE TABLE', ['db1']),
                grant('ALTER'),
                grant_except_databases('ALTER COLUMN', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER ORDER BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER SAMPLE BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER ADD INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER DROP INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER CLEAR INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER CONSTRAINT', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER TTL', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER SETTINGS', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER MOVE PARTITION', [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER FETCH PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER FREEZE PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER VIEW', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant('POSTGRES'),
            ],
        },
    },
    {
        'id': 'user without access to multiple databases, ClickHouse 21.2',
        'args': {
            'version': '21.2.10.48',
            'databases': ['db1', 'db2'],
            'user_databases': [],
            'result': USER_COMMON_GRANT_LIST
            + [
                grant('SELECT', SYSTEM_DATABASES + [TMP_DATABASE]),
                grant('TRUNCATE', SYSTEM_DATABASES),
                grant('OPTIMIZE', SYSTEM_DATABASES),
                grant('DROP', SYSTEM_DATABASES),
                grant('ALTER'),
                grant_except_databases('ALTER COLUMN', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER ORDER BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER SAMPLE BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER ADD INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER DROP INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER CLEAR INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER CONSTRAINT', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER TTL', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER SETTINGS', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER MOVE PARTITION', [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER FETCH PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases(
                    'ALTER FREEZE PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']
                ),
                grant_except_databases('ALTER VIEW', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant('POSTGRES'),
            ],
        },
    },
    {
        'id': 'user with access to one database, ClickHouse 21.11',
        'args': {
            'version': '21.11.5.33',
            'databases': ['db1', 'db2'],
            'user_databases': ['db1'],
            'result': USER_COMMON_GRANT_LIST
            + [
                grant('SELECT', SYSTEM_DATABASES + [TMP_DATABASE, 'db1']),
                grant('TRUNCATE', SYSTEM_DATABASES + ['db1']),
                grant('OPTIMIZE', SYSTEM_DATABASES + ['db1']),
                grant('DROP DATABASE', SYSTEM_DATABASES + ['db1']),
                grant('DROP TABLE', SYSTEM_DATABASES + ['db1']),
                grant('DROP VIEW', SYSTEM_DATABASES + ['db1']),
                grant('DROP DICTIONARY', SYSTEM_DATABASES + ['db1']),
                grant('CREATE FUNCTION'),
                grant('DROP FUNCTION'),
                grant('INSERT', ['db1']),
                grant('CREATE DATABASE', ['db1']),
                grant('CREATE DICTIONARY', ['db1']),
                grant('CREATE VIEW', ['db1']),
                grant('CREATE TABLE', ['db1']),
                grant('ALTER'),
                grant_except_databases('ALTER COLUMN', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER ORDER BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER SAMPLE BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER ADD INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER DROP INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER CLEAR INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER CONSTRAINT', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER TTL', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER SETTINGS', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER MOVE PARTITION', [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER FETCH PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER FREEZE PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant_except_databases('ALTER VIEW', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db2']),
                grant('POSTGRES'),
                grant('SYSTEM RESTORE REPLICA'),
            ],
        },
    },
    {
        'id': 'user without access to multiple databases, ClickHouse 21.11',
        'args': {
            'version': '21.11.5.33',
            'databases': ['db1', 'db2'],
            'user_databases': [],
            'result': USER_COMMON_GRANT_LIST
            + [
                grant('SELECT', SYSTEM_DATABASES + [TMP_DATABASE]),
                grant('TRUNCATE', SYSTEM_DATABASES),
                grant('OPTIMIZE', SYSTEM_DATABASES),
                grant('DROP DATABASE', SYSTEM_DATABASES),
                grant('DROP TABLE', SYSTEM_DATABASES),
                grant('DROP VIEW', SYSTEM_DATABASES),
                grant('DROP DICTIONARY', SYSTEM_DATABASES),
                grant('CREATE FUNCTION'),
                grant('DROP FUNCTION'),
                grant('ALTER'),
                grant_except_databases('ALTER COLUMN', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER ORDER BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER SAMPLE BY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER ADD INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER DROP INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER CLEAR INDEX', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER CONSTRAINT', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER TTL', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER SETTINGS', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER MOVE PARTITION', [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases('ALTER FETCH PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant_except_databases(
                    'ALTER FREEZE PARTITION', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']
                ),
                grant_except_databases('ALTER VIEW', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE, 'db1', 'db2']),
                grant('POSTGRES'),
                grant('SYSTEM RESTORE REPLICA'),
            ],
        },
    },
)
def test_user_grants(version, databases, user_databases, result):
    mock_pillar(
        mdb_clickhouse.__salt__,
        {
            'data': {
                'clickhouse': {
                    'ch_version': version,
                    'databases': databases,
                    'users': {
                        'test_user': {
                            'databases': dict((db, {}) for db in user_databases),
                        },
                    },
                },
            },
        },
    )
    assert mdb_clickhouse.user_grants('test_user') == result


@parametrize(
    {
        'id': 'ClickHouse 20.8, SQL database management enabled',
        'args': {
            'version': '20.8.18.32',
            'sql_database_management': True,
            'result': ADMIN_COMMON_GRANT_LIST
            + ADMIN_DATABASE_MANAGEMENT_ENABLED_GRANT_LIST
            + [
                grant_except_databases('DROP', [MDB_SYSTEM_DATABASE], grant_option=True),
                grant('ACCESS MANAGEMENT', grant_option=True),
            ],
        },
    },
    {
        'id': 'ClickHouse 20.8, SQL database management disabled',
        'args': {
            'version': '20.8.18.32',
            'sql_database_management': False,
            'result': ADMIN_COMMON_GRANT_LIST
            + ADMIN_DATABASE_MANAGEMENT_DISABLED_GRANT_LIST
            + [
                grant('ACCESS MANAGEMENT', grant_option=True),
            ],
        },
    },
    {
        'id': 'ClickHouse 21.2, SQL database management enabled',
        'args': {
            'version': '21.2.10.48',
            'sql_database_management': True,
            'result': ADMIN_COMMON_GRANT_LIST
            + ADMIN_DATABASE_MANAGEMENT_ENABLED_GRANT_LIST
            + [
                grant_except_databases('DROP', [MDB_SYSTEM_DATABASE], grant_option=True),
                grant('POSTGRES', grant_option=True),
                grant('ACCESS MANAGEMENT', grant_option=True),
            ],
        },
    },
    {
        'id': 'ClickHouse 21.2, SQL database management disabled',
        'args': {
            'version': '21.2.10.48',
            'sql_database_management': False,
            'result': ADMIN_COMMON_GRANT_LIST
            + ADMIN_DATABASE_MANAGEMENT_DISABLED_GRANT_LIST
            + [
                grant('POSTGRES', grant_option=True),
                grant('ACCESS MANAGEMENT', grant_option=True),
            ],
        },
    },
    {
        'id': 'ClickHouse 21.11, SQL database management enabled',
        'args': {
            'version': '21.11.10.48',
            'sql_database_management': True,
            'result': ADMIN_COMMON_GRANT_LIST
            + ADMIN_DATABASE_MANAGEMENT_ENABLED_GRANT_LIST
            + [
                grant('DROP', grant_option=True),
                grant_except_databases('DROP DATABASE', [MDB_SYSTEM_DATABASE], grant_option=True),
                grant_except_databases('DROP TABLE', [MDB_SYSTEM_DATABASE], grant_option=True),
                grant_except_databases('DROP VIEW', [MDB_SYSTEM_DATABASE], grant_option=True),
                grant_except_databases('DROP DICTIONARY', [MDB_SYSTEM_DATABASE], grant_option=True),
                grant('POSTGRES', grant_option=True),
                grant('SYSTEM RESTORE REPLICA', grant_option=True),
                grant('ACCESS MANAGEMENT', grant_option=True),
            ],
        },
    },
    {
        'id': 'ClickHouse 21.11, SQL database management disabled',
        'args': {
            'version': '21.11.10.48',
            'sql_database_management': False,
            'result': ADMIN_COMMON_GRANT_LIST
            + ADMIN_DATABASE_MANAGEMENT_DISABLED_GRANT_LIST
            + [
                grant('CREATE FUNCTION', grant_option=True),
                grant('DROP FUNCTION', grant_option=True),
                grant('POSTGRES', grant_option=True),
                grant('SYSTEM RESTORE REPLICA', grant_option=True),
                grant('ACCESS MANAGEMENT', grant_option=True),
            ],
        },
    },
    {
        'id': 'ClickHouse 22.2, SQL database management enabled',
        'args': {
            'version': '22.2.3.5',
            'sql_database_management': True,
            'result': ADMIN_COMMON_GRANT_LIST
            + ADMIN_DATABASE_MANAGEMENT_ENABLED_GRANT_LIST
            + [
                grant('DROP', grant_option=True),
                grant_except_databases('DROP DATABASE', [MDB_SYSTEM_DATABASE], grant_option=True),
                grant_except_databases('DROP TABLE', [MDB_SYSTEM_DATABASE], grant_option=True),
                grant_except_databases('DROP VIEW', [MDB_SYSTEM_DATABASE], grant_option=True),
                grant_except_databases('DROP DICTIONARY', [MDB_SYSTEM_DATABASE], grant_option=True),
                grant('POSTGRES', grant_option=True),
                grant('SYSTEM RESTORE REPLICA', grant_option=True),
                grant('ACCESS MANAGEMENT', grant_option=True),
                grant_except_databases(
                    'CREATE ROW POLICY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True
                ),
                grant_except_databases('ALTER ROW POLICY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
                grant_except_databases('DROP ROW POLICY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
            ],
        },
    },
    {
        'id': 'ClickHouse 22.2, SQL database management disabled',
        'args': {
            'version': '22.2.3.5',
            'sql_database_management': False,
            'result': ADMIN_COMMON_GRANT_LIST
            + ADMIN_DATABASE_MANAGEMENT_DISABLED_GRANT_LIST
            + [
                grant('CREATE FUNCTION', grant_option=True),
                grant('DROP FUNCTION', grant_option=True),
                grant('POSTGRES', grant_option=True),
                grant('SYSTEM RESTORE REPLICA', grant_option=True),
                grant('ACCESS MANAGEMENT', grant_option=True),
                grant_except_databases(
                    'CREATE ROW POLICY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True
                ),
                grant_except_databases('ALTER ROW POLICY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
                grant_except_databases('DROP ROW POLICY', SYSTEM_DATABASES + [MDB_SYSTEM_DATABASE], grant_option=True),
            ],
        },
    },
)
def test_admin_grants(version, sql_database_management, result):
    mock_version_cmp(mdb_clickhouse.__salt__)
    mock_pillar(
        mdb_clickhouse.__salt__,
        {
            'data': {
                'clickhouse': {
                    'ch_version': version,
                    'sql_database_management': sql_database_management,
                },
            },
        },
    )
    assert mdb_clickhouse.admin_grants() == result


@parametrize(
    {
        'id': 'Preinstalled geobase',
        'args': {
            'pillar': {},
            'result': '/opt/yandex/clickhouse-geodb',
        },
    },
    {
        'id': 'Custom geobase',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'config': {
                            'geobase_uri': 'https://storage.yandexcloud.net/bucket1/test_geobase.tar.gz',
                        },
                    },
                },
            },
            'result': '/var/lib/clickhouse/geodb',
        },
    },
)
def test_geobase_path(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.geobase_path() == result


@parametrize(
    {
        'id': 'Preinstalled geobase',
        'args': {
            'pillar': {},
            'result': None,
        },
    },
    {
        'id': 'Custom geobase with ordinary URI',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'config': {
                            'geobase_uri': 'https://storage.yandexcloud.net/bucket1/test_geobase.tar.gz',
                        },
                    },
                },
            },
            'result': '/var/lib/clickhouse/geodb_archive/geodb.tar.gz',
        },
    },
    {
        'id': 'Custom geobase with self-signed URI',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'config': {
                            'geobase_uri': 'https://storage.yandexcloud.net/bucket1/test_geobase.tar.gz?AWSAccessKeyId=X&Signature=Y&Expires=1570824200',
                        },
                    },
                },
            },
            'result': '/var/lib/clickhouse/geodb_archive/geodb.tar.gz',
        },
    },
    {
        'id': 'Invalid custom geobase',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'config': {
                            'geobase_uri': 'https://storage.yandexcloud.net/bucket1/test_geobase.bin',
                        },
                    },
                },
            },
            'result': None,
        },
    },
)
def test_custom_geobase_archive_path(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.custom_geobase_archive_path() == result


def test_backup_zkflock_id():
    mock_pillar(
        mdb_clickhouse.__salt__,
        {
            'data': {
                'dbaas': {
                    'cluster_id': 'cid1',
                    'shard_id': 'shard_id1',
                },
            },
        },
    )
    assert mdb_clickhouse.backup_zkflock_id() == 'ch_backup/cid1/shard_id1'


@parametrize(
    {
        'id': 'default config',
        'args': {
            'pillar': {},
            'result': {
                'retention_policies': [
                    {
                        'database': 'system',
                        'table': 'query_log',
                        'max_partition_count': 30,
                        'max_table_size': 1073741824,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'query_thread_log',
                        'max_partition_count': 30,
                        'max_table_size': 536870912,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'part_log',
                        'max_partition_count': 30,
                        'max_table_size': 536870912,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'metric_log',
                        'max_partition_count': 30,
                        'max_table_size': 536870912,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'trace_log',
                        'max_partition_count': 30,
                        'max_table_size': 536870912,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'text_log',
                        'max_partition_count': 30,
                        'max_table_size': 536870912,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'opentelemetry_span_log',
                        'max_partition_count': 30,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'session_log',
                        'max_partition_count': 30,
                        'table_rotation_suffix': '_[0-9]',
                    },
                ],
            },
        },
    },
    {
        'id': 'customized config',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'config': {
                            'query_log_retention_size': 1000000000,
                            'query_log_retention_time': 100 * 24 * 3600,
                            'query_thread_log_retention_size': 1000000001,
                            'query_thread_log_retention_time': 101 * 24 * 3600,
                            'part_log_retention_size': 1000000002,
                            'part_log_retention_time': 102 * 24 * 3600,
                            'metric_log_retention_size': 1000000003,
                            'metric_log_retention_time': 103 * 24 * 3600,
                            'trace_log_retention_size': 1000000004,
                            'trace_log_retention_time': 104 * 24 * 3600,
                            'text_log_retention_size': 1000000005,
                            'text_log_retention_time': 105 * 24 * 3600,
                        },
                        'cleaner': {
                            'config': {
                                'retention_policies': [
                                    {
                                        'database': 'test',
                                        'table': 'table1',
                                        'max_partition_count': 10,
                                    },
                                    {
                                        'database': 'test',
                                        'table': 'table2',
                                        'max_partition_count': 10,
                                        'max_table_size': 1073741824,
                                    },
                                ],
                            },
                        },
                    },
                },
            },
            'result': {
                'retention_policies': [
                    {
                        'database': 'system',
                        'table': 'query_log',
                        'max_partition_count': 100,
                        'max_table_size': 1000000000,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'query_thread_log',
                        'max_partition_count': 101,
                        'max_table_size': 1000000001,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'part_log',
                        'max_partition_count': 102,
                        'max_table_size': 1000000002,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'metric_log',
                        'max_partition_count': 103,
                        'max_table_size': 1000000003,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'trace_log',
                        'max_partition_count': 104,
                        'max_table_size': 1000000004,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'text_log',
                        'max_partition_count': 105,
                        'max_table_size': 1000000005,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'opentelemetry_span_log',
                        'max_partition_count': 30,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'session_log',
                        'max_partition_count': 30,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'test',
                        'table': 'table1',
                        'max_partition_count': 10,
                    },
                    {
                        'database': 'test',
                        'table': 'table2',
                        'max_partition_count': 10,
                        'max_table_size': 1073741824,
                    },
                ],
            },
        },
    },
    {
        'id': 'disabled log_retention_time config',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'config': {
                            'query_log_retention_size': 1000000000,
                            'query_log_retention_time': 0,
                            'query_thread_log_retention_size': 1000000001,
                            'query_thread_log_retention_time': 0,
                            'part_log_retention_size': 1000000002,
                            'part_log_retention_time': 0,
                            'metric_log_retention_size': 1000000003,
                            'metric_log_retention_time': 0,
                            'trace_log_retention_size': 1000000004,
                            'trace_log_retention_time': 0,
                            'text_log_retention_size': 1000000005,
                            'text_log_retention_time': 0,
                        },
                        'cleaner': {
                            'config': {
                                'retention_policies': [
                                    {
                                        'database': 'test',
                                        'table': 'table1',
                                        'max_partition_count': 10,
                                    },
                                    {
                                        'database': 'test',
                                        'table': 'table2',
                                        'max_partition_count': 10,
                                        'max_table_size': 1073741824,
                                    },
                                ],
                            },
                        },
                    },
                },
            },
            'result': {
                'retention_policies': [
                    {
                        'database': 'system',
                        'table': 'query_log',
                        'max_partition_count': 0,
                        'max_table_size': 1000000000,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'query_thread_log',
                        'max_partition_count': 0,
                        'max_table_size': 1000000001,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'part_log',
                        'max_partition_count': 0,
                        'max_table_size': 1000000002,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'metric_log',
                        'max_partition_count': 0,
                        'max_table_size': 1000000003,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'trace_log',
                        'max_partition_count': 0,
                        'max_table_size': 1000000004,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'text_log',
                        'max_partition_count': 0,
                        'max_table_size': 1000000005,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'opentelemetry_span_log',
                        'max_partition_count': 30,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'system',
                        'table': 'session_log',
                        'max_partition_count': 30,
                        'table_rotation_suffix': '_[0-9]',
                    },
                    {
                        'database': 'test',
                        'table': 'table1',
                        'max_partition_count': 10,
                    },
                    {
                        'database': 'test',
                        'table': 'table2',
                        'max_partition_count': 10,
                        'max_table_size': 1073741824,
                    },
                ],
            },
        },
    },
)
def test_cleaner_config(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.cleaner_config() == result


@parametrize(
    {
        'id': 'default settings, 21.8',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '21.8.15.7',
                    },
                },
            },
            'result': {
                'query_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'query_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'part_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'part_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'query_thread_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'query_thread_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'metric_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'metric_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                        'collect_interval_milliseconds': 1000,
                    },
                },
                'text_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'text_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                        'level': 'trace',
                    },
                },
                'trace_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'trace_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'crash_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'crash_log',
                        'partition_by': '',
                        'flush_interval_milliseconds': 1000,
                    },
                },
                'opentelemetry_span_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'opentelemetry_span_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY finish_date ORDER BY (finish_date, finish_time_us, trace_id)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'session_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'session_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
            },
        },
    },
    {
        'id': 'customized settings, 21.8',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '21.8.15.7',
                        'config': {
                            'metric_log_enabled': False,
                            'trace_log_enabled': False,
                            'text_log_enabled': True,
                            'opentelemetry_span_log_enabled': True,
                            'text_log_level': 'error',
                        },
                    },
                },
            },
            'result': {
                'query_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'query_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'part_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'part_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'query_thread_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'query_thread_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'metric_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'metric_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                        'collect_interval_milliseconds': 1000,
                    },
                },
                'text_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'text_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                        'level': 'error',
                    },
                },
                'trace_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'trace_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'crash_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'crash_log',
                        'partition_by': '',
                        'flush_interval_milliseconds': 1000,
                    },
                },
                'opentelemetry_span_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'opentelemetry_span_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY finish_date ORDER BY (finish_date, finish_time_us, trace_id)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'session_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'session_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
            },
        },
    },
    {
        'id': 'default settings, 22.2',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '22.2.3.5',
                    },
                },
            },
            'result': {
                'query_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'query_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'part_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'part_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'query_thread_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'query_thread_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'metric_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'metric_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                        'collect_interval_milliseconds': 1000,
                    },
                },
                'text_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'text_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                        'level': 'trace',
                    },
                },
                'trace_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'trace_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'crash_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'crash_log',
                        'partition_by': '',
                        'flush_interval_milliseconds': 1000,
                    },
                },
                'opentelemetry_span_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'opentelemetry_span_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY finish_date ORDER BY (finish_date, finish_time_us, trace_id)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'session_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'session_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
            },
        },
    },
    {
        'id': 'customized settings, 22.2',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '22.2.3.5',
                        'config': {
                            'metric_log_enabled': False,
                            'trace_log_enabled': False,
                            'text_log_enabled': True,
                            'opentelemetry_span_log_enabled': True,
                            'text_log_level': 'error',
                        },
                    },
                },
            },
            'result': {
                'query_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'query_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'part_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'part_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'query_thread_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'query_thread_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'metric_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'metric_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                        'collect_interval_milliseconds': 1000,
                    },
                },
                'text_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'text_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                        'level': 'error',
                    },
                },
                'trace_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'trace_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'crash_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'crash_log',
                        'partition_by': '',
                        'flush_interval_milliseconds': 1000,
                    },
                },
                'opentelemetry_span_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'opentelemetry_span_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY finish_date ORDER BY (finish_date, finish_time_us, trace_id)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'session_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'session_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time)',
                        'flush_interval_milliseconds': 7500,
                    },
                },
            },
        },
    },
    {
        'id': 'default settings, 22.3',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '22.3.3.44',
                    },
                },
            },
            'result': {
                'query_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'query_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'part_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'part_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'query_thread_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'query_thread_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'metric_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'metric_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                        'collect_interval_milliseconds': 1000,
                    },
                },
                'text_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'text_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                        'level': 'trace',
                    },
                },
                'trace_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'trace_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'crash_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'crash_log',
                        'partition_by': '',
                        'flush_interval_milliseconds': 1000,
                    },
                },
                'opentelemetry_span_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'opentelemetry_span_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY finish_date ORDER BY (finish_date, finish_time_us, trace_id) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'session_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'session_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                    },
                },
            },
        },
    },
    {
        'id': 'default settings, 22.6',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '22.6.3.35',
                    },
                },
            },
            'result': {
                'query_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'query_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'part_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'part_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'query_thread_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'query_thread_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'metric_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'metric_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                        'collect_interval_milliseconds': 1000,
                    },
                },
                'text_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'text_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                        'level': 'trace',
                    },
                },
                'trace_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'trace_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'crash_log': {
                    'enabled': True,
                    'config': {
                        'database': 'system',
                        'table': 'crash_log',
                        'partition_by': '',
                        'flush_interval_milliseconds': 1000,
                    },
                },
                'opentelemetry_span_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'opentelemetry_span_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY finish_date ORDER BY (finish_date, finish_time_us, trace_id) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                    },
                },
                'session_log': {
                    'enabled': False,
                    'config': {
                        'database': 'system',
                        'table': 'session_log',
                        'engine': 'ENGINE = MergeTree PARTITION BY event_date ORDER BY (event_date, event_time) SETTINGS storage_policy = \'local\'',
                        'flush_interval_milliseconds': 7500,
                    },
                },
            },
        },
    },
)
def test_system_tables(pillar, result):
    mock_version_cmp(mdb_clickhouse.__salt__)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.system_tables() == result


@parametrize(
    {
        'id': 'default',
        'args': {
            'pillar': {},
            'result': True,
        },
    },
    {
        'id': 'pillar with use_ch_backup=False',
        'args': {
            'pillar': {
                'data': {
                    'use_ch_backup': False,
                },
            },
            'result': False,
        },
    },
    {
        'id': 'pillar with periodic_backups=False',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'periodic_backups': False,
                    },
                },
            },
            'result': False,
        },
    },
    {
        'id': 'pillar with cloud_storage:enabled=True',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '20.1.2.3',
                    },
                    'cloud_storage': {
                        'enabled': True,
                    },
                },
            },
            'result': False,
        },
    },
    {
        'id': 'pillar with cloud_storage:enabled=True',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '21.6.7.8',
                    },
                    'cloud_storage': {
                        'enabled': True,
                    },
                },
            },
            'result': True,
        },
    },
)
def test_backups_enabled(pillar, result):
    mock_version_cmp(mdb_clickhouse.__salt__)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.backups_enabled() == result


@parametrize(
    {
        'id': 'Non-HA cluster, default settings',
        'args': {
            'pillar': {},
            'result': 'flock -n -o /tmp/ch-backup.lock ' '/etc/cron.yandex/ch-backup.sh --timeout 216000 --force',
        },
    },
    {
        'id': 'Non-HA cluster, custom settings',
        'args': {
            'pillar': {
                'data': {
                    'backup': {
                        'timeout': 20 * 60 * 60,
                    },
                },
            },
            'result': 'flock -n -o /tmp/ch-backup.lock ' '/etc/cron.yandex/ch-backup.sh --timeout 72000 --force',
        },
    },
    {
        'id': 'HA cluster, default settings',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'zk_hosts': [
                            'vla-1.db.yandex.net',
                            'man-1.db.yandex.net',
                            'sas-1.db.yandex.net',
                        ],
                    },
                },
            },
            'result': "flock -n -o /tmp/ch-backup.lock /etc/cron.yandex/ch-backup.sh --timeout 216000 --force",
        },
    },
    {
        'id': 'HA cluster, custom settings',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'zk_hosts': [
                            'vla-1.db.yandex.net',
                            'man-1.db.yandex.net',
                            'sas-1.db.yandex.net',
                        ],
                    },
                    'backup': {
                        'timeout': 20 * 60 * 60,
                    },
                },
            },
            'result': "flock -n -o /tmp/ch-backup.lock /etc/cron.yandex/ch-backup.sh --timeout 72000 --force",
        },
    },
)
def test_initial_backup_command(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.initial_backup_command() == result


@parametrize(
    {
        'id': 'Non-HA cluster, default settings (porto)',
        'args': {
            'pillar': {},
            'result': 'flock -n -o /tmp/ch-backup.lock '
            '/etc/cron.yandex/ch-backup.sh --timeout 216000 --sleep 7200 --purge',
        },
    },
    {
        'id': 'Non-HA cluster, default settings (compute)',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                    },
                },
            },
            'result': 'flock -n -o /tmp/ch-backup.lock '
            '/etc/cron.yandex/ch-backup.sh --timeout 216000 --sleep 1800 --purge',
        },
    },
    {
        'id': 'Non-HA cluster, custom settings',
        'args': {
            'pillar': {
                'data': {
                    'backup': {
                        'sleep': 60 * 60,
                        'timeout': 20 * 60 * 60,
                    },
                    'ch_backup': {
                        'purge_enabled': False,
                    },
                },
            },
            'result': 'flock -n -o /tmp/ch-backup.lock ' '/etc/cron.yandex/ch-backup.sh --timeout 72000 --sleep 3600',
        },
    },
    {
        'id': 'HA cluster, default settings (porto)',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'zk_hosts': [
                            'vla-1.db.yandex.net',
                            'man-1.db.yandex.net',
                            'sas-1.db.yandex.net',
                        ],
                    },
                },
            },
            'result': '/etc/cron.yandex/create-zkflock-id.sh && '
            'zk-flock -c /etc/yandex/ch-backup/zk-flock.json lock '
            '"flock -n -o /tmp/ch-backup.lock '
            '/etc/cron.yandex/ch-backup.sh --timeout 216000 --sleep 7200 --purge"',
        },
    },
    {
        'id': 'HA cluster, default settings (compute)',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                    },
                    'clickhouse': {
                        'zk_hosts': [
                            'vla-1.db.yandex.net',
                            'man-1.db.yandex.net',
                            'sas-1.db.yandex.net',
                        ],
                    },
                },
            },
            'result': '/etc/cron.yandex/create-zkflock-id.sh && '
            'zk-flock -c /etc/yandex/ch-backup/zk-flock.json lock '
            '"flock -n -o /tmp/ch-backup.lock '
            '/etc/cron.yandex/ch-backup.sh --timeout 216000 --sleep 1800 --purge"',
        },
    },
    {
        'id': 'HA cluster, custom settings',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'zk_hosts': [
                            'vla-1.db.yandex.net',
                            'man-1.db.yandex.net',
                            'sas-1.db.yandex.net',
                        ],
                    },
                    'backup': {
                        'sleep': 60 * 60,
                        'timeout': 20 * 60 * 60,
                    },
                    'ch_backup': {
                        'purge_enabled': False,
                    },
                },
            },
            'result': '/etc/cron.yandex/create-zkflock-id.sh && '
            'zk-flock -c /etc/yandex/ch-backup/zk-flock.json lock '
            '"flock -n -o /tmp/ch-backup.lock /etc/cron.yandex/ch-backup.sh --timeout 72000 --sleep 3600"',
        },
    },
)
def test_backup_cron_command(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.backup_cron_command() == result


@parametrize(
    {
        'id': 'empty pillar',
        'args': {
            'pillar': {},
            'result': {},
        },
    },
    {
        'id': 'null value',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'models': None,
                    },
                },
            },
            'result': {},
        },
    },
    {
        'id': 'non-null value',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'models': {
                            'model1': {
                                'type': 'catboost',
                                'uri': 'uri1',
                            },
                            'model2': {
                                'type': 'catboost',
                                'uri': 'uri2',
                            },
                        },
                    },
                },
            },
            'result': {
                'model1': {
                    'type': 'catboost',
                    'uri': 'uri1',
                },
                'model2': {
                    'type': 'catboost',
                    'uri': 'uri2',
                },
            },
        },
    },
)
def test_ml_models(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.ml_models() == result


@parametrize(
    {
        'id': 'empty pillar',
        'args': {
            'pillar': {},
            'result': {},
        },
    },
    {
        'id': 'null value',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'format_schemas': None,
                    },
                },
            },
            'result': {},
        },
    },
    {
        'id': 'non-null value',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'format_schemas': {
                            'schema1': {
                                'type': 'protobuf',
                                'uri': 'uri1',
                            },
                            'schema2': {
                                'type': 'capnproto',
                                'uri': 'uri2',
                            },
                        },
                    },
                },
            },
            'result': {
                'schema1': {
                    'type': 'protobuf',
                    'uri': 'uri1',
                },
                'schema2': {
                    'type': 'capnproto',
                    'uri': 'uri2',
                },
            },
        },
    },
)
def test_format_schemas(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.format_schemas() == result


@parametrize(
    {
        'id': 'restore data without zk',
        'args': {
            'pillar': {
                'data': {'clickhouse': {'zk_hosts': []}},
                'restore-from': {
                    'backup-id': 'backup_id',
                },
            },
            'result': 'flock -n -o /tmp/ch-backup.lock /usr/bin/ch-backup -c '
            '/etc/yandex/ch-backup/ch-backup-restore.conf restore backup_id',
        },
    },
    {
        'id': 'restore only metadata with zk',
        'args': {
            'pillar': {
                'data': {'clickhouse': {'zk_hosts': ['zk_host']}},
                'restore-from': {
                    'backup-id': 'backup_id',
                    'schema-only': True,
                },
            },
            'result': 'flock -n -o /tmp/ch-backup.lock /usr/bin/ch-backup -c '
            '/etc/yandex/ch-backup/ch-backup-restore.conf restore --schema-only backup_id',
        },
    },
)
def test_restore_command(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    assert mdb_clickhouse.restore_command() == result


@parametrize(
    {
        'id': 'resetup from replica',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'subcluster_id': 'subcid1',
                        'shard_name': 'shard1',
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-2.db.yandex.net': {},
                                                'sas-2.db.yandex.net': {},
                                            },
                                        },
                                    },
                                },
                                'subcid2': {
                                    'roles': ['zk'],
                                    'hosts': {
                                        'man-1.db.yandex.net': {},
                                        'sas-1.db.yandex.net': {},
                                        'vla-1.db.yandex.net': {},
                                    },
                                },
                            },
                        },
                    },
                },
            },
            'result': '/usr/bin/ch-backup --port 29443 --protocol https restore-schema '
            '--source-host sas-2.db.yandex.net --exclude-dbs system',
        },
    },
    {
        'id': 'copy schema from another shard',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'subcluster_id': 'subcid1',
                        'shard_name': 'shard1',
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-2.db.yandex.net': {},
                                                'sas-2.db.yandex.net': {},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-3.db.yandex.net': {},
                                                'sas-3.db.yandex.net': {},
                                            },
                                        },
                                    },
                                },
                                'subcid2': {
                                    'roles': ['zk'],
                                    'hosts': {
                                        'man-1.db.yandex.net': {},
                                        'sas-1.db.yandex.net': {},
                                        'vla-1.db.yandex.net': {},
                                    },
                                },
                            },
                        },
                    },
                },
                'schema_backup_shard': 'shard2',
            },
            'result': '/usr/bin/ch-backup --port 29443 --protocol https restore-schema '
            '--source-host sas-3.db.yandex.net --exclude-dbs system',
        },
    },
)
def test_restore_schema_command(pillar, result):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    mock_grains(mdb_clickhouse.__salt__, {'id': 'man-2.db.yandex.net'})
    command_without_extra_whitespaces = re.sub(' +', ' ', mdb_clickhouse.restore_schema_command())
    assert command_without_extra_whitespaces == result
