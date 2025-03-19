# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_clickhouse, mdb_s3
from cloud.mdb.salt_tests.common.mocks import mock_grains, mock_pillar
from cloud.mdb.internal.python.pytest.utils import parametrize

TIMEOUT = 600


@parametrize(
    {
        'id': 'with defaults for optional parameters',
        'args': {
            'pillar': {
                'data': {
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                        'access_key_id': 'test_access_key_id',
                        'access_secret_key': 'test_access_secret_key',
                    },
                    's3_bucket': 'cid1_bucket',
                    'ch_backup': {
                        'encryption_key': 'test_encryption_key',
                    },
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                        'zk_hosts': ['zk01', 'zk02'],
                        'system_users': {
                            'mdb_backup_admin': {
                                'password': 'password1',
                                'hash': 'hash',
                            },
                        },
                    },
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_backup_admin',
                    'clickhouse_password': 'password1',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard1',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 30,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 7,
                    },
                    'retain_count': 7,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': False,
                    'backup_access_control': True,
                },
                'encryption': {
                    'type': 'nacl',
                    'key': 'test_encryption_key',
                },
                'storage': {
                    'type': 's3',
                    'credentials': {
                        'endpoint_url': 'https://s3.mds.yandex.net',
                        'bucket': 'cid1_bucket',
                        'access_key_id': 'test_access_key_id',
                        'secret_access_key': 'test_access_secret_key',
                        'send_metadata': 'true',
                    },
                    'boto_config': {
                        'addressing_style': 'path',
                        'region_name': None,
                    },
                    'bulk_delete_chunk_size': 100,
                },
                'zookeeper': {
                    'hosts': 'zk01:2181,zk02:2181',
                    'root_path': '/clickhouse/cid1',
                    'secure': False,
                },
                'multiprocessing': {
                    'workers': 4,
                },
                'main': {
                    'drop_privileges': True,
                },
            },
        },
    },
    {
        'id': 'with virtual addressing style',
        'args': {
            'pillar': {
                'data': {
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                        'access_key_id': 'test_access_key_id',
                        'access_secret_key': 'test_access_secret_key',
                        'virtual_addressing_style': True,
                    },
                    's3_bucket': 'cid1_bucket',
                    'ch_backup': {
                        'encryption_key': 'test_encryption_key',
                    },
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                        'zk_hosts': ['zk01', 'zk02'],
                    },
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_admin',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard1',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 30,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 7,
                    },
                    'retain_count': 7,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': False,
                    'backup_access_control': True,
                },
                'encryption': {
                    'type': 'nacl',
                    'key': 'test_encryption_key',
                },
                'storage': {
                    'type': 's3',
                    'credentials': {
                        'endpoint_url': 'https://s3.mds.yandex.net',
                        'bucket': 'cid1_bucket',
                        'access_key_id': 'test_access_key_id',
                        'secret_access_key': 'test_access_secret_key',
                        'send_metadata': 'true',
                    },
                    'boto_config': {
                        'addressing_style': 'virtual',
                        'region_name': None,
                    },
                    'bulk_delete_chunk_size': 100,
                },
                'zookeeper': {
                    'hosts': 'zk01:2181,zk02:2181',
                    'root_path': '/clickhouse/cid1',
                    'secure': False,
                },
                'multiprocessing': {
                    'workers': 4,
                },
                'main': {
                    'drop_privileges': True,
                },
            },
        },
    },
    {
        'id': 'with proxy resolver',
        'args': {
            'pillar': {
                'data': {
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                        'access_key_id': 'test_access_key_id',
                        'access_secret_key': 'test_access_secret_key',
                        'proxy_resolver': {
                            'uri': 'http://s3.mds.yandex.net/hostname',
                            'proxy_port': 4080,
                        },
                    },
                    's3_bucket': 'cid1_bucket',
                    'ch_backup': {
                        'encryption_key': 'test_encryption_key',
                    },
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                        'zk_hosts': ['zk01', 'zk02'],
                    },
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_admin',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard1',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 30,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 7,
                    },
                    'retain_count': 7,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': False,
                    'backup_access_control': True,
                },
                'encryption': {
                    'type': 'nacl',
                    'key': 'test_encryption_key',
                },
                'storage': {
                    'type': 's3',
                    'credentials': {
                        'endpoint_url': 'http://s3.mds.yandex.net',
                        'bucket': 'cid1_bucket',
                        'access_key_id': 'test_access_key_id',
                        'secret_access_key': 'test_access_secret_key',
                        'send_metadata': 'true',
                    },
                    'boto_config': {
                        'addressing_style': 'path',
                        'region_name': None,
                    },
                    'proxy_resolver': {
                        'uri': 'http://s3.mds.yandex.net/hostname',
                        'proxy_port': 4080,
                    },
                    'bulk_delete_chunk_size': 100,
                },
                'zookeeper': {
                    'hosts': 'zk01:2181,zk02:2181',
                    'root_path': '/clickhouse/cid1',
                    'secure': False,
                },
                'multiprocessing': {
                    'workers': 4,
                },
                'main': {
                    'drop_privileges': True,
                },
            },
        },
    },
    {
        'id': 'minimal config required to support ch-resetup',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '21.8.14.5',
                        'zk_hosts': ['zk01', 'zk02'],
                    }
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_admin',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard1',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 30,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 7,
                    },
                    'retain_count': 7,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': False,
                    'backup_access_control': True,
                },
                'zookeeper': {
                    'hosts': 'zk01:2181,zk02:2181',
                    'root_path': '/clickhouse/cid1',
                    'secure': False,
                },
                'multiprocessing': {
                    'workers': 4,
                },
                'main': {
                    'drop_privileges': True,
                },
            },
        },
    },
    {
        'id': 'with sql user management',
        'args': {
            'pillar': {
                'data': {
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                        'access_key_id': 'test_access_key_id',
                        'access_secret_key': 'test_access_secret_key',
                    },
                    's3_bucket': 'cid1_bucket',
                    'ch_backup': {
                        'encryption_key': 'test_encryption_key',
                    },
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                        'zk_hosts': ['zk01', 'zk02'],
                    },
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_admin',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                    'clickhouse_user': '_admin',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard1',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 30,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 7,
                    },
                    'retain_count': 7,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': False,
                    'backup_access_control': True,
                },
                'encryption': {
                    'type': 'nacl',
                    'key': 'test_encryption_key',
                },
                'storage': {
                    'type': 's3',
                    'credentials': {
                        'endpoint_url': 'https://s3.mds.yandex.net',
                        'bucket': 'cid1_bucket',
                        'access_key_id': 'test_access_key_id',
                        'secret_access_key': 'test_access_secret_key',
                        'send_metadata': 'true',
                    },
                    'boto_config': {
                        'addressing_style': 'path',
                        'region_name': None,
                    },
                    'bulk_delete_chunk_size': 100,
                },
                'zookeeper': {
                    'hosts': 'zk01:2181,zk02:2181',
                    'root_path': '/clickhouse/cid1',
                    'secure': False,
                },
                'multiprocessing': {
                    'workers': 4,
                },
                'main': {
                    'drop_privileges': True,
                },
            },
        },
    },
    {
        'id': 'with custom backup parameters',
        'args': {
            'pillar': {
                'data': {
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                        'access_key_id': 'test_access_key_id',
                        'access_secret_key': 'test_access_secret_key',
                    },
                    's3_bucket': 'cid1_bucket',
                    'ch_backup': {
                        'encryption_key': 'test_encryption_key',
                        'workers': 10,
                        'deduplication_age_limit_days': 10,
                        'retain_time_days': 30,
                        'retain_count': 30,
                    },
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                        'zk_hosts': ['zk01', 'zk02'],
                        'system_users': {
                            'mdb_backup_admin': {
                                'password': 'password1',
                                'hash': 'hash',
                            },
                        },
                    },
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_backup_admin',
                    'clickhouse_password': 'password1',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard1',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 10,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 30,
                    },
                    'retain_count': 30,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': False,
                    'backup_access_control': True,
                },
                'encryption': {
                    'type': 'nacl',
                    'key': 'test_encryption_key',
                },
                'storage': {
                    'type': 's3',
                    'credentials': {
                        'endpoint_url': 'https://s3.mds.yandex.net',
                        'bucket': 'cid1_bucket',
                        'access_key_id': 'test_access_key_id',
                        'secret_access_key': 'test_access_secret_key',
                        'send_metadata': 'true',
                    },
                    'boto_config': {
                        'addressing_style': 'path',
                        'region_name': None,
                    },
                    'bulk_delete_chunk_size': 100,
                },
                'zookeeper': {
                    'hosts': 'zk01:2181,zk02:2181',
                    'root_path': '/clickhouse/cid1',
                    'secure': False,
                },
                'multiprocessing': {
                    'workers': 10,
                },
                'main': {
                    'drop_privileges': True,
                },
            },
        },
    },
)
def test_backup_config(pillar, result):
    mock_grains(mdb_clickhouse.__salt__, {'id': 'man-1.db.yandex.net'})
    pillar['data']['dbaas'] = {
        'cluster_id': 'cid1',
        'shard_name': 'shard1',
        'geo': 'man',
    }
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    mock_pillar(mdb_s3.__salt__, pillar)
    mdb_clickhouse.__salt__['mdb_s3.endpoint'] = mdb_s3.endpoint
    mdb_clickhouse.__salt__['dbaas.is_public_ca'] = lambda: False
    mdb_clickhouse.__salt__['dbaas.is_aws'] = lambda: False
    assert mdb_clickhouse.backup_config() == result


@parametrize(
    {
        'id': 'restore to cluster without zookeeper',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                    },
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                        'access_key_id': 'test_access_key_id',
                        'access_secret_key': 'test_access_secret_key',
                    },
                    'restore-from-pillar-data': {
                        'ch_backup': {
                            'encryption_key': 'test_encryption_key',
                        },
                        's3_bucket': 'cid1_bucket',
                    },
                },
                'restore-from': {
                    'cid': 'cid1',
                    's3-path': 'ch_backup/cid1/shard1/backup1/',
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_admin',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard1',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 30,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 7,
                    },
                    'retain_count': 7,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': True,
                    'backup_access_control': True,
                },
                'encryption': {
                    'type': 'nacl',
                    'key': 'test_encryption_key',
                },
                'storage': {
                    'type': 's3',
                    'credentials': {
                        'endpoint_url': 'https://s3.mds.yandex.net',
                        'bucket': 'cid1_bucket',
                        'access_key_id': 'test_access_key_id',
                        'secret_access_key': 'test_access_secret_key',
                        'send_metadata': 'true',
                    },
                    'boto_config': {
                        'addressing_style': 'path',
                        'region_name': None,
                    },
                    'bulk_delete_chunk_size': 100,
                },
                'zookeeper': {
                    'hosts': '',
                    'root_path': '/clickhouse/cid2',
                    'secure': False,
                },
                'multiprocessing': {
                    'workers': 4,
                },
                'main': {
                    'drop_privileges': True,
                },
                'restore_from': {
                    'cid': 'cid1',
                    'shard_name': 'shard1',
                },
            },
        },
    },
    {
        'id': 'restore to cluster with zookeeper',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {'ch_version': '20.8.14.4', 'zk_hosts': {'zkhost'}},
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                        'access_key_id': 'test_access_key_id',
                        'access_secret_key': 'test_access_secret_key',
                    },
                    'restore-from-pillar-data': {
                        'ch_backup': {
                            'encryption_key': 'test_encryption_key',
                        },
                        's3_bucket': 'cid1_bucket',
                    },
                },
                'restore-from': {
                    'cid': 'cid1',
                    's3-path': 'ch_backup/cid1/shard1/backup1/',
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_admin',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard1',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 30,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 7,
                    },
                    'retain_count': 7,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': False,
                    'backup_access_control': True,
                },
                'encryption': {
                    'type': 'nacl',
                    'key': 'test_encryption_key',
                },
                'storage': {
                    'type': 's3',
                    'credentials': {
                        'endpoint_url': 'https://s3.mds.yandex.net',
                        'bucket': 'cid1_bucket',
                        'access_key_id': 'test_access_key_id',
                        'secret_access_key': 'test_access_secret_key',
                        'send_metadata': 'true',
                    },
                    'boto_config': {
                        'addressing_style': 'path',
                        'region_name': None,
                    },
                    'bulk_delete_chunk_size': 100,
                },
                'zookeeper': {
                    'hosts': 'zkhost:2181',
                    'root_path': '/clickhouse/cid2',
                    'secure': False,
                },
                'multiprocessing': {
                    'workers': 4,
                },
                'main': {
                    'drop_privileges': True,
                },
                'restore_from': {
                    'cid': 'cid1',
                    'shard_name': 'shard1',
                },
            },
        },
    },
    {
        'id': 'restore to cluster with zookeeper with ACL',
        'args': {
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                        'zk_hosts': {'zkhost'},
                        'zk_users': {
                            'clickhouse': {
                                'password': 'password_is_good',
                            }
                        },
                    },
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                        'access_key_id': 'test_access_key_id',
                        'access_secret_key': 'test_access_secret_key',
                    },
                    'restore-from-pillar-data': {
                        'ch_backup': {
                            'encryption_key': 'test_encryption_key',
                        },
                        's3_bucket': 'cid1_bucket',
                    },
                    'unmanaged': {
                        'enable_zk_tls': True,
                    },
                },
                'restore-from': {
                    'cid': 'cid1',
                    's3-path': 'ch_backup/cid1/shard1/backup1/',
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_admin',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard1',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 30,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 7,
                    },
                    'retain_count': 7,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': False,
                    'backup_access_control': True,
                },
                'encryption': {
                    'type': 'nacl',
                    'key': 'test_encryption_key',
                },
                'storage': {
                    'type': 's3',
                    'credentials': {
                        'endpoint_url': 'https://s3.mds.yandex.net',
                        'bucket': 'cid1_bucket',
                        'access_key_id': 'test_access_key_id',
                        'secret_access_key': 'test_access_secret_key',
                        'send_metadata': 'true',
                    },
                    'boto_config': {
                        'addressing_style': 'path',
                        'region_name': None,
                    },
                    'bulk_delete_chunk_size': 100,
                },
                'zookeeper': {
                    'hosts': 'zkhost:2281',
                    'root_path': '/clickhouse/cid2',
                    'cert': '/etc/clickhouse-server/ssl/server.crt',
                    'key': '/etc/clickhouse-server/ssl/server.key',
                    'ca': '/etc/clickhouse-server/ssl/allCAs.pem',
                    'secure': True,
                    'user': 'clickhouse',
                    'password': 'password_is_good',
                },
                'multiprocessing': {
                    'workers': 4,
                },
                'main': {
                    'drop_privileges': True,
                },
                'restore_from': {
                    'cid': 'cid1',
                    'shard_name': 'shard1',
                },
            },
        },
    },
)
def test_backup_config_for_restore(pillar, result):
    mock_grains(mdb_clickhouse.__salt__, {'id': 'man-1.db.yandex.net'})
    pillar['data']['dbaas'] = {
        'cluster_id': 'cid2',
        'shard_name': 'shard1',
        'geo': 'man',
    }
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    mock_pillar(mdb_s3.__salt__, pillar)
    mdb_clickhouse.__salt__['mdb_s3.endpoint'] = mdb_s3.endpoint
    mdb_clickhouse.__salt__['dbaas.is_public_ca'] = lambda: False
    mdb_clickhouse.__salt__['dbaas.is_aws'] = lambda: False
    assert mdb_clickhouse.backup_config_for_restore() == result


@parametrize(
    {
        'id': 'restore from another shard with zk',
        'args': {
            'pillar': {
                'schema_backup_shard': 'shard2',
                'data': {
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                        'access_key_id': 'test_access_key_id',
                        'access_secret_key': 'test_access_secret_key',
                    },
                    's3_bucket': 'cid1_bucket',
                    'ch_backup': {
                        'encryption_key': 'test_encryption_key',
                    },
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                        'zk_hosts': ['zk01', 'zk02'],
                    },
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_admin',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard2',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 30,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 7,
                    },
                    'retain_count': 7,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': False,
                    'backup_access_control': True,
                },
                'encryption': {
                    'type': 'nacl',
                    'key': 'test_encryption_key',
                },
                'storage': {
                    'type': 's3',
                    'credentials': {
                        'endpoint_url': 'https://s3.mds.yandex.net',
                        'bucket': 'cid1_bucket',
                        'access_key_id': 'test_access_key_id',
                        'secret_access_key': 'test_access_secret_key',
                        'send_metadata': 'true',
                    },
                    'boto_config': {
                        'addressing_style': 'path',
                        'region_name': None,
                    },
                    'bulk_delete_chunk_size': 100,
                },
                'zookeeper': {
                    'hosts': 'zk01:2181,zk02:2181',
                    'root_path': '/clickhouse/cid1',
                    'secure': False,
                },
                'multiprocessing': {
                    'workers': 4,
                },
                'main': {
                    'drop_privileges': True,
                },
            },
        },
    },
    {
        'id': 'restore from another shard without zk',
        'args': {
            'pillar': {
                'schema_backup_shard': 'shard2',
                'data': {
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                        'access_key_id': 'test_access_key_id',
                        'access_secret_key': 'test_access_secret_key',
                    },
                    's3_bucket': 'cid1_bucket',
                    'ch_backup': {
                        'encryption_key': 'test_encryption_key',
                    },
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                    },
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_admin',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard2',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 30,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 7,
                    },
                    'retain_count': 7,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': False,
                    'backup_access_control': True,
                },
                'encryption': {
                    'type': 'nacl',
                    'key': 'test_encryption_key',
                },
                'storage': {
                    'type': 's3',
                    'credentials': {
                        'endpoint_url': 'https://s3.mds.yandex.net',
                        'bucket': 'cid1_bucket',
                        'access_key_id': 'test_access_key_id',
                        'secret_access_key': 'test_access_secret_key',
                        'send_metadata': 'true',
                    },
                    'boto_config': {
                        'addressing_style': 'path',
                        'region_name': None,
                    },
                    'bulk_delete_chunk_size': 100,
                },
                'zookeeper': {
                    'hosts': '',
                    'root_path': '/clickhouse/cid1',
                    'secure': False,
                },
                'multiprocessing': {
                    'workers': 4,
                },
                'main': {
                    'drop_privileges': True,
                },
            },
        },
    },
    {
        'id': 'restore from the same shard without zk',
        'args': {
            'pillar': {
                'data': {
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                        'access_key_id': 'test_access_key_id',
                        'access_secret_key': 'test_access_secret_key',
                    },
                    's3_bucket': 'cid1_bucket',
                    'ch_backup': {
                        'encryption_key': 'test_encryption_key',
                    },
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                    },
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_admin',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard1',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 30,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 7,
                    },
                    'retain_count': 7,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': False,
                    'backup_access_control': True,
                },
                'encryption': {
                    'type': 'nacl',
                    'key': 'test_encryption_key',
                },
                'storage': {
                    'type': 's3',
                    'credentials': {
                        'endpoint_url': 'https://s3.mds.yandex.net',
                        'bucket': 'cid1_bucket',
                        'access_key_id': 'test_access_key_id',
                        'secret_access_key': 'test_access_secret_key',
                        'send_metadata': 'true',
                    },
                    'boto_config': {
                        'addressing_style': 'path',
                        'region_name': None,
                    },
                    'bulk_delete_chunk_size': 100,
                },
                'zookeeper': {
                    'hosts': '',
                    'root_path': '/clickhouse/cid1',
                    'secure': False,
                },
                'multiprocessing': {
                    'workers': 4,
                },
                'main': {
                    'drop_privileges': True,
                },
            },
        },
    },
    {
        'id': 'with custom backup lifetime',
        'args': {
            'pillar': {
                'data': {
                    's3': {
                        'endpoint': 'https+path://s3.mds.yandex.net',
                        'access_key_id': 'test_access_key_id',
                        'access_secret_key': 'test_access_secret_key',
                    },
                    's3_bucket': 'cid1_bucket',
                    'ch_backup': {
                        'encryption_key': 'test_encryption_key',
                        'retain_time_days': 30,
                        'retain_count': 30,
                    },
                    'clickhouse': {
                        'ch_version': '20.8.14.4',
                        'zk_hosts': ['zk01', 'zk02'],
                        'system_users': {
                            'mdb_backup_admin': {
                                'password': 'password1',
                                'hash': 'hash',
                            },
                        },
                    },
                },
                'cert.key': 'noop key',
            },
            'result': {
                'clickhouse': {
                    'clickhouse_user': '_backup_admin',
                    'clickhouse_password': 'password1',
                    'timeout': TIMEOUT,
                    'protocol': 'https',
                    'ca_path': '/etc/clickhouse-server/ssl/allCAs.pem',
                },
                'backup': {
                    'exclude_dbs': ['_system'],
                    'path_root': 'ch_backup/cid1/shard1',
                    'deduplicate_parts': True,
                    'deduplication_age_limit': {
                        'days': 30,
                    },
                    'min_interval': {
                        'minutes': 30,
                    },
                    'retain_time': {
                        'days': 30,
                    },
                    'retain_count': 30,
                    'time_format': '%Y-%m-%d %H:%M:%S %z',
                    'labels': {
                        'shard_name': 'shard1',
                    },
                    'keep_freezed_data_on_failure': False,
                    'override_replica_name': '{replica}',
                    'force_non_replicated': False,
                    'backup_access_control': True,
                },
                'encryption': {
                    'type': 'nacl',
                    'key': 'test_encryption_key',
                },
                'storage': {
                    'type': 's3',
                    'credentials': {
                        'endpoint_url': 'https://s3.mds.yandex.net',
                        'bucket': 'cid1_bucket',
                        'access_key_id': 'test_access_key_id',
                        'secret_access_key': 'test_access_secret_key',
                        'send_metadata': 'true',
                    },
                    'boto_config': {
                        'addressing_style': 'path',
                        'region_name': None,
                    },
                    'bulk_delete_chunk_size': 100,
                },
                'zookeeper': {
                    'hosts': 'zk01:2181,zk02:2181',
                    'root_path': '/clickhouse/cid1',
                    'secure': False,
                },
                'multiprocessing': {
                    'workers': 4,
                },
                'main': {
                    'drop_privileges': True,
                },
            },
        },
    },
)
def test_backup_config_for_schema_copy(pillar, result):
    mock_grains(mdb_clickhouse.__salt__, {'id': 'man-1.db.yandex.net'})
    pillar['data']['dbaas'] = {
        'cluster_id': 'cid1',
        'shard_name': 'shard1',
        'geo': 'man',
    }
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    mock_pillar(mdb_s3.__salt__, pillar)
    mdb_clickhouse.__salt__['mdb_s3.endpoint'] = mdb_s3.endpoint
    mdb_clickhouse.__salt__['dbaas.is_public_ca'] = lambda: False
    mdb_clickhouse.__salt__['dbaas.is_aws'] = lambda: False
    assert mdb_clickhouse.backup_config_for_schema_copy() == result
