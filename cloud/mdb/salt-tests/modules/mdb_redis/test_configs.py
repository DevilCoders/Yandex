# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_redis, dbaas
from cloud.mdb.salt_tests.common.mocks import mock_grains, mock_grains_filterby, mock_pillar
from cloud.mdb.internal.python.pytest.utils import parametrize
from mock import patch

ETH0_IP_RESULT = '2a02:6b8:c0e:501:0:f806:0:283'
PILLAR_IP_RESULT = '2a02:6b8:c0e:501:0:f806:0:284'
assert ETH0_IP_RESULT != PILLAR_IP_RESULT

MASTER_NAME = 'redis_cluster_1'
DEFAULT_PORT = 6379
REDIS_PIDFILE = mdb_redis.get_redis_pidfile()


def set_pillar_config(name, value, config=None, node='redis'):
    if config is None:
        config_dict = {}
        config = {
            'data': {node: {'config': config_dict}},
        }
    else:
        config_dict = config['data'][node]['config']
    config_dict[name] = value
    return config


@parametrize(
    {
        'id': 'Return False when strings components set are equal for notify-keyspace-events',
        'args': {
            'option': 'notify-keyspace-events',
            'cur_val': 'Eltg',
            'new_val': 'gltE',
            'result': False,
        },
    },
    {
        'id': 'Return True when strings components set are not equal for notify-keyspace-events',
        'args': {
            'option': 'notify-keyspace-events',
            'cur_val': 'Eltg',
            'new_val': 'gltEm',
            'result': True,
        },
    },
    {
        'id': 'Return True when strings components set are equal if not notify-keyspace-events',
        'args': {
            'option': 'maxmemory-policy',
            'cur_val': 'Eltg',
            'new_val': 'gltE',
            'result': True,
        },
    },
    {
        'id': 'Return False when strings are equal if not notify-keyspace-events',
        'args': {
            'option': 'maxmemory-policy',
            'cur_val': 'Eltg',
            'new_val': 'Eltg',
            'result': False,
        },
    },
)
def test__option_differs(option, cur_val, new_val, result):
    assert mdb_redis._option_differs(option, cur_val, new_val) == result


@parametrize(
    {
        'id': 'Return master from grains when master from pillar is None',
        'args': {
            'pillar_master': None,
            'grains': {
                'ip_interfaces': {
                    'eth0': [
                        ETH0_IP_RESULT,
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'result': ETH0_IP_RESULT,
        },
    },
    {
        'id': 'Return master from pillar when master from pillar is not None',
        'args': {
            'pillar_master': PILLAR_IP_RESULT,
            'grains': {
                'ip_interfaces': {
                    'eth0': [
                        ETH0_IP_RESULT,
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'result': PILLAR_IP_RESULT,
        },
    },
)
def test_get_master_for_sentinel(pillar_master, grains, result):
    mock_grains(mdb_redis.__salt__, grains)
    with patch('cloud.mdb.salt.salt._modules.mdb_redis.get_master_from_pillar') as get_master_from_pillar_mock:
        get_master_from_pillar_mock.return_value = pillar_master
        assert mdb_redis.get_master_for_sentinel() == result


@parametrize(
    {
        'id': 'Return None for sharded cluster',
        'args': {
            'pillar_master': PILLAR_IP_RESULT,
            'pillar': set_pillar_config('cluster-enabled', 'yes'),
            'grains': {
                'id': MASTER_NAME,
                'ip_interfaces': {
                    'eth0': [
                        ETH0_IP_RESULT,
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'port': None,
            'result': None,
        },
    },
    {
        'id': 'Return None for sentinel-style cluster when master from pillar is None',
        'args': {
            'pillar_master': None,
            'pillar': set_pillar_config('cluster-enabled', 'no'),
            'grains': {
                'id': MASTER_NAME,
                'ip_interfaces': {
                    'eth0': [
                        ETH0_IP_RESULT,
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'port': None,
            'result': None,
        },
    },
    {
        'id': 'Return None for sentinel-style cluster when master from pillar is not None but in eth0 grains',
        'args': {
            'pillar_master': ETH0_IP_RESULT,
            'pillar': set_pillar_config('cluster-enabled', 'no'),
            'grains': {
                'id': MASTER_NAME,
                'ip_interfaces': {
                    'eth0': [
                        ETH0_IP_RESULT,
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'port': None,
            'result': None,
        },
    },
    {
        'id': 'Return None for sentinel-style cluster when master from pillar is not None and not in eth0 grains, '
              'but equals to id grains',
        'args': {
            'pillar_master': PILLAR_IP_RESULT,
            'pillar': set_pillar_config('cluster-enabled', 'no'),
            'grains': {
                'id': PILLAR_IP_RESULT,
                'ip_interfaces': {
                    'eth0': [
                        ETH0_IP_RESULT,
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'port': None,
            'result': None,
        },
    },
    {
        'id': 'Return not None for sentinel-style cluster when master from pillar is not None and not in eth0 grains, '
              "and doesn't equal to id grains",
        'args': {
            'pillar_master': PILLAR_IP_RESULT,
            'pillar': set_pillar_config('cluster-enabled', 'no'),
            'grains': {
                'id': MASTER_NAME,
                'ip_interfaces': {
                    'eth0': [
                        ETH0_IP_RESULT,
                        'fe80::d20d:15ff:fe44:955d',
                    ],
                },
            },
            'port': DEFAULT_PORT,
            'result': '{} {}'.format(PILLAR_IP_RESULT, DEFAULT_PORT),
        },
    },
)
def test_get_slaveof_string(pillar_master, pillar, grains, port, result):
    mock_grains(mdb_redis.__salt__, grains)
    mock_pillar(mdb_redis.__salt__, pillar)
    with patch('cloud.mdb.salt.salt._modules.mdb_redis.get_master_from_pillar') as get_master_mock, patch(
        'cloud.mdb.salt.salt._modules.mdb_redis.get_redis_replication_port'
    ) as get_port_mock:
        get_master_mock.return_value = pillar_master
        get_port_mock.return_value = port
        assert mdb_redis.get_slaveof_string() == result


@parametrize(
    {
        'id': 'Return host IP when master from pillar is not None',
        'args': {
            'pillar': {
                'redis-master': MASTER_NAME,
            },
            'sentinel_master': PILLAR_IP_RESULT,
            'host_master': ETH0_IP_RESULT,
            'result': ETH0_IP_RESULT,
        },
    },
    {
        'id': 'Return sentinel-given IP when master from pillar is None',
        'args': {
            'pillar': {
                'redis-master': None,
            },
            'sentinel_master': PILLAR_IP_RESULT,
            'host_master': ETH0_IP_RESULT,
            'result': PILLAR_IP_RESULT,
        },
    },
)
def test_get_master_from_pillar(pillar, sentinel_master, host_master, result):
    mock_pillar(mdb_redis.__salt__, pillar)
    with patch('cloud.mdb.salt.salt._modules.mdb_redis.get_master') as get_master_mock, patch(
        'cloud.mdb.salt.salt._modules.mdb_redis.get_ip'
    ) as get_ip_mock:
        get_master_mock.return_value = sentinel_master
        get_ip_mock.return_value = host_master
        assert mdb_redis.get_master_from_pillar() == result


def get_default_version_dict():
    def_map = mdb_redis.get_default_versions_map()
    major_num = mdb_redis.get_major_num(def_map['major_version'])
    return {
        'version': {
            'major_num': major_num,
            'major_human': def_map['major_version'],
            'pkg': def_map['package_version'],
        },
    }


@parametrize(
    {
        'id': 'Versions filled with default, direct version ignored',
        'args': {
            'pillar': {},
            'conf': {
                'version': {
                    'major_num': '500',
                    'major_human': '5.0',
                    'pkg': 'CUSTOMPKG',
                },
            },
            'result': get_default_version_dict(),
        },
    },
    {
        'id': 'Versions filled with non-default, direct version ignored',
        'args': {
            'pillar': {
                'data': {
                    'versions': {
                        'redis': {
                            'package_version': '60PKGVERSION',
                            'major_version': '6.0',
                        },
                    },
                },
            },
            'conf': {
                'version': {
                    'major_num': '500',
                    'major_human': '5.0',
                },
            },
            'result': {
                'version': {
                    'major_num': '600',
                    'major_human': '6.0',
                    'pkg': '60PKGVERSION',
                },
            },
        },
    },
)
def test_update_config_version(pillar, conf, result):
    mock_pillar(mdb_redis.__salt__, pillar)
    mdb_redis.update_config_version(conf)
    assert conf == result


USER = GROUP = 'redis_test'
PKG_TEST = 'pkg_test'


def get_sentinel_result(version_dict):
    res = {
        'cli': 'sentinel',
        'config': {
            'daemonize': 'yes',
            'dir': mdb_redis.get_redis_data_folder(),
            'logfile': '/var/log/redis/redis-sentinel.log',
            'pidfile': '/var/run/sentinel/redis-sentinel.pid',
            'port': 26379,
            'protected-mode': 'no',
            'sentinel set-disable': 'yes',
        },
        'group': GROUP,
        'user': USER,
        'master_config': {'down-after-milliseconds': 30000, 'failover-timeout': 30000},
        'tls': {'cli': 'sentinel_tls', 'enabled': False, 'port': 26380},
    }
    res.update(version_dict)
    return res


@parametrize(
    {
        'id': 'Default version from redis, direct version ignored',
        'args': {
            'pillar': {
                'data': {
                    'sentinel': {
                        'version': {
                            'major_num': '500',
                            'major_human': '5.0',
                        },
                        'user': USER,
                        'group': GROUP,
                    },
                },
            },
            'user': USER,
            'group': GROUP,
            'tls_folder': '/etc/redis/tls',
            'result': get_sentinel_result(get_default_version_dict()),
        },
    },
    {
        'id': 'Non-default versions from redis, direct version ignored',
        'args': {
            'pillar': {
                'data': {
                    'sentinel': {
                        'version': {
                            'major_num': '500',
                            'major_human': '5.0',
                        },
                        'user': USER,
                        'group': GROUP,
                    },
                    'redis': {
                        'version': {
                            'major_num': '501',
                            'major_human': '5.1',
                        },
                    },
                    'versions': {
                        'redis': {
                            'package_version': '60PKGVERSION',
                            'major_version': '6.0',
                        },
                    },
                },
            },
            'user': USER,
            'group': GROUP,
            'tls_folder': '/etc/redis/tls',
            'result': get_sentinel_result(
                {
                    'version': {
                        'major_num': '600',
                        'major_human': '6.0',
                        'pkg': '60PKGVERSION',
                    }
                }
            ),
        },
    },
)
def test_get_sentinel_data(pillar, user, group, tls_folder, result):
    mock_pillar(mdb_redis.__salt__, pillar)
    mock_grains_filterby(mdb_redis.__salt__)
    sentinel = mdb_redis.get_sentinel_data(user, group, tls_folder)
    assert sentinel == result


def get_redis_result(version_dict, appendonly='yes'):
    res = {
        'tls': {'enabled': False, 'cli': 'redis_tls', 'port': 6380},
        'homedir': '/home/redis_test',
        'group': GROUP,
        'user': USER,
        'config': {
            'cluster-replica-no-failover': 'no',
            'appendonly': appendonly,
            'rdbchecksum': 'yes',
            'repl-diskless-sync': 'yes',
            'zset-max-ziplist-value': 64,
            'slave-serve-stale-data': 'yes',
            'cluster-require-full-coverage': 'no',
            'slave-priority': 100,
            'supervised': 'no',
            'client-output-buffer-limit pubsub': '64mb 32mb 60',
            'port': 6379,
            'appendfsync': 'everysec',
            'lua-time-limit': 5000,
            'cluster-config-file': 'test_folder/cluster.conf',
            'set-max-intset-entries': 512,
            'rdb-save-incremental-fsync': 'yes',
            'min-slaves-to-write': 0,
            'list-max-ziplist-size': -2,
            'slowlog-max-len': 1000,
            'maxclients': 65000,
            'repl-backlog-ttl': 3600,
            'latency-monitor-threshold': 100,
            'save': '',
            'rdbcompression': 'yes',
            'list-compress-depth': 0,
            'zset-max-ziplist-entries': 128,
            'auto-aof-rewrite-percentage': 100,
            'hz': 10,
            'repl-backlog-size': '100mb',
            'repl-disable-tcp-nodelay': 'yes',
            'aof-use-rdb-preamble': 'yes',
            'client-output-buffer-limit normal': '0 0 0',
            'dbfilename': 'dump.rdb',
            'client-output-buffer-limit replica': '0 0 0',
            'slave-read-only': 'yes',
            'auto-aof-rewrite-min-size': '64mb',
            'stop-writes-on-bgsave-error': 'yes',
            'hash-max-ziplist-value': 64,
            'logfile': '/var/log/redis/redis-server.log',
            'cluster-node-timeout': '15000',
            'tcp-keepalive': 60,
            'cluster-migration-barrier': '2',
            'no-appendfsync-on-rewrite': 'yes',
            'loglevel': 'notice',
            'hash-max-ziplist-entries': 512,
            'protected-mode': 'no',
            'hll-sparse-max-bytes': 3000,
            'appendfilename': mdb_redis.get_aof_filename(),
            'daemonize': 'yes',
            'aof-rewrite-incremental-fsync': 'yes',
            'pidfile': REDIS_PIDFILE,
            'timeout': 0,
            'databases': 16,
            'tcp-backlog': 511,
            'activerehashing': 'yes',
            'slowlog-log-slower-than': 10000,
            'aof-load-truncated': 'yes',
            'cluster-replica-validity-factor': '10',
            'dir': mdb_redis.get_redis_data_folder(),
        },
        'cli': 'redis',
    }
    res.update(version_dict)
    if version_dict['version']['major_human'] != "5.0":
        res['config']['io-threads'] = 1
        res['config']['oom-score-adj'] = 'relative'
        res['config']['oom-score-adj-values'] = '0 100 800'
    return res


@parametrize(
    {
        'id': 'Default versions, direct version ignored',
        'args': {
            'pillar': {
                'data': {
                    'redis': {
                        'version': {
                            'major_num': '600',
                            'major_human': '6.0',
                            'pkg': 'CUSTOMPKG',
                        },
                    }
                }
            },
            'config_folder': "test_folder",
            'user': USER,
            'group': GROUP,
            'tls_folder': '/etc/redis/tls',
            'result': get_redis_result(get_default_version_dict()),
        },
    },
    {
        'id': 'Set aof mode off',
        'args': {
            'pillar': {
                'data': {
                    'redis': {
                        'version': {
                            'major_num': '500',
                            'major_human': '5.0',
                        },
                        'user': USER,
                        'group': GROUP,
                        'config': {
                            'appendonly': 'no',
                        },
                    },
                },
            },
            'config_folder': "test_folder",
            'user': USER,
            'group': GROUP,
            'tls_folder': '/etc/redis/tls',
            'result': get_redis_result(get_default_version_dict(), "no"),
        },
    },
    {
        'id': 'Set aof mode on',
        'args': {
            'pillar': {
                'data': {
                    'redis': {
                        'version': {
                            'major_num': '500',
                            'major_human': '5.0',
                        },
                        'user': USER,
                        'group': GROUP,
                        'config': {
                            'appendonly': 'yes',
                        },
                    },
                },
            },
            'config_folder': "test_folder",
            'user': USER,
            'group': GROUP,
            'tls_folder': '/etc/redis/tls',
            'result': get_redis_result(get_default_version_dict(), "yes"),
        },
    },
)
def test_get_redis_data(pillar, config_folder, user, group, tls_folder, result):
    mock_pillar(mdb_redis.__salt__, pillar)
    mock_grains_filterby(mdb_redis.__salt__)
    redis = mdb_redis.get_redis_data(config_folder, user, group, tls_folder)
    sentinel = mdb_redis.get_sentinel_data(user, group, tls_folder)
    assert redis == result
    assert redis['version'] == sentinel['version']


@parametrize(
    {
        'id': "Don't set io-threads if 5.0",
        'args': {
            'pillar': {
                'redis': {'io_threads_allowed': True},
            },
            'conf': {
                'version': {
                    'major_human': '5.0',
                },
                'config': {},
            },
            'result': {
                'version': {
                    'major_human': '5.0',
                },
                'config': {},
            },
        },
    },
    {
        'id': "Don't set for 6.0 if cores < 4",
        'args': {
            'pillar': {
                'data': {'redis': {'io_threads_allowed': True}, 'dbaas': {'flavor': {'cpu_guarantee': 3}}},
            },
            'conf': {
                'version': {
                    'major_human': '6.0',
                },
                'config': {},
            },
            'result': {
                'version': {
                    'major_human': '6.0',
                },
                'config': {
                    'io-threads': 1,
                },
            },
        },
    },
    {
        'id': "Set for 6.0 if cores = 4",
        'args': {
            'pillar': {
                'data': {'redis': {'io_threads_allowed': True}, 'dbaas': {'flavor': {'cpu_guarantee': 4}}},
            },
            'conf': {
                'version': {
                    'major_human': '6.0',
                },
                'config': {
                    'io-threads': 1,
                },
            },
            'result': {
                'version': {
                    'major_human': '6.0',
                },
                'config': {
                    'io-threads': 2,
                },
            },
        },
    },
    {
        'id': "Set for 6.2 if cores >= 11",
        'args': {
            'pillar': {
                'data': {'redis': {'io_threads_allowed': True}, 'dbaas': {'flavor': {'cpu_guarantee': 11}}},
            },
            'conf': {
                'version': {
                    'major_human': '6.2',
                },
                'config': {},
            },
            'result': {
                'version': {
                    'major_human': '6.2',
                },
                'config': {
                    'io-threads': 8,
                },
            },
        },
    },
    {
        'id': "Set for 6.2 if not set in pillar (default switched to True)",
        'args': {
            'pillar': {
                'data': {'dbaas': {'flavor': {'cpu_guarantee': 11}}},
            },
            'conf': {
                'version': {
                    'major_human': '6.2',
                },
                'config': {},
            },
            'result': {
                'version': {
                    'major_human': '6.2',
                },
                'config': {
                    'io-threads': 8,
                },
            },
        },
    },
)
def test_update_config_io_threads(pillar, conf, result):
    mock_pillar(mdb_redis.__salt__, pillar)
    mdb_redis.update_config_io_threads(conf)
    assert conf == result


@parametrize(
    {
        'id': 'No data - on (by default)',
        'args': {
            'pillar': {
                'data': {
                    'redis': {
                        'config': {},
                    },
                },
            },
            'result': True,
        },
    },
    {
        'id': 'aof on - on',
        'args': {
            'pillar': {
                'data': {
                    'redis': {
                        'config': {'appendonly': 'yes'},
                    },
                },
            },
            'result': True,
        },
    },
    {
        'id': 'aof off, rdb on - off',
        'args': {
            'pillar': {
                'data': {
                    'redis': {
                        'config': {'appendonly': 'no', 'save': '900 1'},
                    },
                },
            },
            'result': False,
        },
    },
)
def test_is_aof_enabled(pillar, result):
    mock_pillar(mdb_redis.__salt__, pillar)
    assert mdb_redis.is_aof_enabled() == result


@parametrize(
    {
        'id': 'porto',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'porto',
                    },
                },
            },
            'result': [6379, 26379],
        },
    },
    {
        'id': 'porto, tls',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'porto',
                    },
                    'redis': {
                        'tls': {
                            'enabled': True,
                        },
                    },
                },
            },
            'result': [6380, 26380],
        },
    },
    {
        'id': 'porto, sharded',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'porto',
                    },
                    'redis': {
                        'config': {
                            'cluster-enabled': 'yes',
                        },
                    },
                },
            },
            'result': [6379],
        },
    },
    {
        'id': 'porto, sharded, tls',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'porto',
                    },
                    'redis': {
                        'config': {
                            'cluster-enabled': 'yes',
                        },
                        'tls': {
                            'enabled': True,
                        },
                    },
                },
            },
            'result': [6380],
        },
    },
    {
        'id': 'compute',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                    },
                },
            },
            'result': [],
        },
    },
    {
        'id': 'compute, public_ip',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'assign_public_ip': True,
                        'vtype': 'compute',
                    },
                },
            },
            'result': [],
        },
    },
    {
        'id': 'compute, tls',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'compute',
                    },
                    'redis': {
                        'tls': {
                            'enabled': True,
                        },
                    },
                },
            },
            'result': [],
        },
    },
    {
        'id': 'compute, public_ip, tls',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'assign_public_ip': True,
                        'vtype': 'compute',
                    },
                    'redis': {
                        'tls': {
                            'enabled': True,
                        },
                    },
                },
            },
            'result': [6380, 26380],
        },
    },
    {
        'id': 'compute, sharded, public_ip, tls',
        'args': {
            'pillar': {
                'data': {
                    'dbaas': {
                        'assign_public_ip': True,
                        'vtype': 'compute',
                    },
                    'redis': {
                        'config': {
                            'cluster-enabled': 'yes',
                        },
                        'tls': {
                            'enabled': True,
                        },
                    },
                },
            },
            'result': [6380],
        },
    },
)
def test_get_public_ports(pillar, result):
    mock_pillar(mdb_redis.__salt__, pillar)
    mock_pillar(dbaas.__salt__, pillar)
    mdb_redis.__salt__['dbaas.is_porto'] = lambda: dbaas.is_porto()
    assert mdb_redis.get_public_ports() == result
