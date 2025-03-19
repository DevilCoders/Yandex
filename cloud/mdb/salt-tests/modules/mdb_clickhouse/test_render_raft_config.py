# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_clickhouse
from cloud.mdb.salt_tests.common.assertion import assert_xml_equals
from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_grains
from cloud.mdb.internal.python.pytest.utils import parametrize

DEFAULT_PILLAR = {
    'data': {
        'dbaas': {
            'cluster_id': 'cid1',
            'shard_name': 'shard1',
            'geo': 'man',
            'cluster_hosts': [
                'man-2.db.yandex.net',
                'sas-2.db.yandex.net',
                'man-3.db.yandex.net',
                'sas-3.db.yandex.net',
            ],
            'cluster': {
                'subclusters': {
                    'subcid1': {
                        'roles': ['clickhouse_cluster'],
                        'shards': {
                            'shard_id1': {
                                'name': 'shard1',
                                'hosts': {
                                    'man-2.db.yandex.net': {'geo': 'man'},
                                    'sas-2.db.yandex.net': {'geo': 'sas'},
                                },
                            },
                            'shard_id2': {
                                'name': 'shard2',
                                'hosts': {
                                    'man-3.db.yandex.net': {'geo': 'man'},
                                    'sas-3.db.yandex.net': {'geo': 'sas'},
                                },
                            },
                        },
                    },
                },
            },
        },
        'clickhouse': {
            'embedded_keeper': True,
            'shards': {
                'shard_id1': {
                    'weight': 100,
                }
            },
            'zk_hosts': ['man-2.db.yandex.net', 'sas-2.db.yandex.net', 'man-3.db.yandex.net'],
        },
    },
    'cert.key': 'noop key',
}


@parametrize(
    {
        'id': "Embedded Keeper is not enabled",
        'args': {
            'grains': {
                'id': 'man-2.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                        'geo': 'man',
                        'cluster_hosts': [
                            'man-2.db.yandex.net',
                            'sas-2.db.yandex.net',
                            'man-3.db.yandex.net',
                            'sas-3.db.yandex.net',
                        ],
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                                'sas-2.db.yandex.net': {'geo': 'sas'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-3.db.yandex.net': {'geo': 'man'},
                                                'sas-3.db.yandex.net': {'geo': 'sas'},
                                            },
                                        },
                                    },
                                },
                                'subcid2': {
                                    'roles': ['zk'],
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
                        'shards': {
                            'shard_id1': {
                                'weight': 100,
                            }
                        },
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '',
        },
    },
    {
        'id': 'Embedded Keeper is enabled, 4th node',
        'args': {
            'grains': {
                'id': 'sas-3.db.yandex.net',
            },
            'pillar': DEFAULT_PILLAR,
            'result': '',
        },
    },
    {
        'id': 'Keeper enabled for 1 node setup',
        'args': {
            'grains': {
                'id': 'man-2.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                        'geo': 'man',
                        'cluster_hosts': ['man-2.db.yandex.net'],
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'embedded_keeper': True,
                        'shards': {
                            'shard_id1': {
                                'weight': 100,
                            }
                        },
                        'zk_hosts': ['man-2.db.yandex.net'],
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '''
                <keeper_server>
                    <tcp_port>2181</tcp_port>
                    <server_id>1</server_id>
                    <log_storage_path>/var/lib/clickhouse/coordination/log</log_storage_path>
                    <snapshot_storage_path>/var/lib/clickhouse/coordination/snapshots</snapshot_storage_path>

                    <coordination_settings>
                        <operation_timeout_ms>5000</operation_timeout_ms>
                        <raft_logs_level>trace</raft_logs_level>
                        <session_timeout_ms>10000</session_timeout_ms>
                    </coordination_settings>

                    <raft_configuration>
                        <server>
                            <hostname>man-2.db.yandex.net</hostname>
                            <id>1</id>
                            <port>2888</port>
                        </server>
                    </raft_configuration>
                </keeper_server>
                ''',
        },
    },
    {
        'id': 'Embedded Keeper is enabled, 1st node',
        'args': {
            'grains': {
                'id': 'man-2.db.yandex.net',
            },
            'pillar': DEFAULT_PILLAR,
            'result': '''
                <keeper_server>
                    <tcp_port>2181</tcp_port>
                    <server_id>1</server_id>
                    <log_storage_path>/var/lib/clickhouse/coordination/log</log_storage_path>
                    <snapshot_storage_path>/var/lib/clickhouse/coordination/snapshots</snapshot_storage_path>

                    <coordination_settings>
                        <operation_timeout_ms>5000</operation_timeout_ms>
                        <raft_logs_level>trace</raft_logs_level>
                        <session_timeout_ms>10000</session_timeout_ms>
                    </coordination_settings>

                    <raft_configuration>
                        <server>
                            <hostname>man-2.db.yandex.net</hostname>
                            <id>1</id>
                            <port>2888</port>
                        </server>
                        <server>
                            <hostname>man-3.db.yandex.net</hostname>
                            <id>2</id>
                            <port>2888</port>
                        </server>
                        <server>
                            <hostname>sas-2.db.yandex.net</hostname>
                            <id>3</id>
                            <port>2888</port>
                        </server>
                    </raft_configuration>
                </keeper_server>
                ''',
        },
    },
    {
        'id': 'Embedded Keeper is enabled, 2nd node',
        'args': {
            'grains': {
                'id': 'man-3.db.yandex.net',
            },
            'pillar': DEFAULT_PILLAR,
            'result': '''
                <keeper_server>
                    <tcp_port>2181</tcp_port>
                    <server_id>2</server_id>
                    <log_storage_path>/var/lib/clickhouse/coordination/log</log_storage_path>
                    <snapshot_storage_path>/var/lib/clickhouse/coordination/snapshots</snapshot_storage_path>

                    <coordination_settings>
                        <operation_timeout_ms>5000</operation_timeout_ms>
                        <raft_logs_level>trace</raft_logs_level>
                        <session_timeout_ms>10000</session_timeout_ms>
                    </coordination_settings>

                    <raft_configuration>
                        <server>
                            <hostname>man-2.db.yandex.net</hostname>
                            <id>1</id>
                            <port>2888</port>
                        </server>
                        <server>
                            <hostname>man-3.db.yandex.net</hostname>
                            <id>2</id>
                            <port>2888</port>
                        </server>
                        <server>
                            <hostname>sas-2.db.yandex.net</hostname>
                            <id>3</id>
                            <port>2888</port>
                        </server>
                    </raft_configuration>
                </keeper_server>
                ''',
        },
    },
    {
        'id': 'Embedded Keeper is enabled with unsorted zk_hosts, 2nd node',
        'args': {
            'grains': {
                'id': 'man-3.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                        'geo': 'man',
                        'cluster_hosts': [
                            'sas-2.db.yandex.net',
                            'iva-2.db.yandex.net',
                            'man-3.db.yandex.net',
                            'sas-3.db.yandex.net',
                        ],
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'sas-2.db.yandex.net': {'geo': 'sas'},
                                                'iva-2.db.yandex.net': {'geo': 'iva'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-3.db.yandex.net': {'geo': 'man'},
                                                'sas-3.db.yandex.net': {'geo': 'sas'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'embedded_keeper': True,
                        'shards': {
                            'shard_id1': {
                                'weight': 100,
                            }
                        },
                        'zk_hosts': ['sas-2.db.yandex.net', 'iva-2.db.yandex.net', 'man-3.db.yandex.net'],
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '''
                <keeper_server>
                    <tcp_port>2181</tcp_port>
                    <server_id>2</server_id>
                    <log_storage_path>/var/lib/clickhouse/coordination/log</log_storage_path>
                    <snapshot_storage_path>/var/lib/clickhouse/coordination/snapshots</snapshot_storage_path>

                    <coordination_settings>
                        <operation_timeout_ms>5000</operation_timeout_ms>
                        <raft_logs_level>trace</raft_logs_level>
                        <session_timeout_ms>10000</session_timeout_ms>
                    </coordination_settings>

                    <raft_configuration>
                        <server>
                            <hostname>iva-2.db.yandex.net</hostname>
                            <id>1</id>
                            <port>2888</port>
                        </server>
                        <server>
                            <hostname>man-3.db.yandex.net</hostname>
                            <id>2</id>
                            <port>2888</port>
                        </server>
                        <server>
                            <hostname>sas-2.db.yandex.net</hostname>
                            <id>3</id>
                            <port>2888</port>
                        </server>
                    </raft_configuration>
                </keeper_server>
                ''',
        },
    },
    {
        'id': 'Keeper enabled for 2 node setup with keeper_hosts in pillar',
        'args': {
            'grains': {
                'id': 'man-2.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                        'geo': 'man',
                        'cluster_hosts': [
                            'man-2.db.yandex.net',
                            'sas-2.db.yandex.net',
                            'man-3.db.yandex.net',
                        ],
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                                'sas-2.db.yandex.net': {'geo': 'sas'},
                                                'man-3.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'embedded_keeper': True,
                        'shards': {
                            'shard_id1': {
                                'weight': 100,
                            }
                        },
                        'keeper_hosts': {
                            'man-3.db.yandex.net': 3,
                            'man-2.db.yandex.net': 1,
                            'sas-2.db.yandex.net': 2,
                        },
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '''
                <keeper_server>
                    <tcp_port>2181</tcp_port>
                    <server_id>1</server_id>
                    <log_storage_path>/var/lib/clickhouse/coordination/log</log_storage_path>
                    <snapshot_storage_path>/var/lib/clickhouse/coordination/snapshots</snapshot_storage_path>

                    <coordination_settings>
                        <operation_timeout_ms>5000</operation_timeout_ms>
                        <raft_logs_level>trace</raft_logs_level>
                        <session_timeout_ms>10000</session_timeout_ms>
                    </coordination_settings>

                    <raft_configuration>
                        <server>
                            <hostname>man-2.db.yandex.net</hostname>
                            <id>1</id>
                            <port>2888</port>
                        </server>
                        <server>
                            <hostname>sas-2.db.yandex.net</hostname>
                            <id>2</id>
                            <port>2888</port>
                        </server>
                        <server>
                            <hostname>man-3.db.yandex.net</hostname>
                            <id>3</id>
                            <port>2888</port>
                        </server>
                    </raft_configuration>
                </keeper_server>
                ''',
        },
    },
)
def test_render_raft_config(grains, pillar, result):
    mock_grains(mdb_clickhouse.__salt__, grains)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    config = mdb_clickhouse.render_raft_config()
    if not result:
        assert config == result
    else:
        assert_xml_equals(config, result)
