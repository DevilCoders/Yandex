# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.salt.salt._modules import mdb_clickhouse
from cloud.mdb.salt_tests.common.assertion import assert_xml_equals
from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_grains, mock_vtype
from cloud.mdb.internal.python.pytest.utils import parametrize


@parametrize(
    {
        'id': 'YandexCloud HA cluster, man replica',
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
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <remote-servers>
                        <cid1>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>sas-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>sas-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </cid1>
                    </remote-servers>
                    <zookeeper-servers>
                        <node index="1">
                            <host>man-1.db.yandex.net</host>
                            <port>2181</port>
                        </node>
                        <node index="2">
                            <host>sas-1.db.yandex.net</host>
                            <port>2181</port>
                        </node>
                        <node index="3">
                            <host>vla-1.db.yandex.net</host>
                            <port>2181</port>
                        </node>
                        <root>/clickhouse/cid1</root>
                    </zookeeper-servers>
                    <macros>
                        <cluster>cid1</cluster>
                        <shard>shard1</shard>
                        <replica>man-2.db.yandex.net</replica>
                    </macros>
                </yandex>
                ''',
        },
    },
    {
        'id': 'YandexCloud HA cluster, sas replica',
        'args': {
            'grains': {
                'id': 'sas-2.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                        'geo': 'sas',
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
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <remote-servers>
                        <cid1>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>sas-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>sas-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>man-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </cid1>
                    </remote-servers>
                    <zookeeper-servers>
                        <node index="1">
                            <host>sas-1.db.yandex.net</host>
                            <port>2181</port>
                        </node>
                        <node index="2">
                            <host>man-1.db.yandex.net</host>
                            <port>2181</port>
                        </node>
                        <node index="3">
                            <host>vla-1.db.yandex.net</host>
                            <port>2181</port>
                        </node>
                        <root>/clickhouse/cid1</root>
                    </zookeeper-servers>
                    <macros>
                        <cluster>cid1</cluster>
                        <shard>shard1</shard>
                        <replica>sas-2.db.yandex.net</replica>
                    </macros>
                </yandex>
                ''',
        },
    },
    {
        'id': 'YandexCloud HA cluster with unsorted keeper_hosts, man replica',
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
                        'keeper_hosts': {
                            'sas-1.db.yandex.net': 1,
                            'vla-1.db.yandex.net': 2,
                            'man-1.db.yandex.net': 3,
                        }
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <remote-servers>
                        <cid1>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>sas-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>sas-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </cid1>
                    </remote-servers>
                    <zookeeper-servers>
                        <node index="1">
                            <host>man-1.db.yandex.net</host>
                            <port>2181</port>
                        </node>
                        <node index="2">
                            <host>sas-1.db.yandex.net</host>
                            <port>2181</port>
                        </node>
                        <node index="3">
                            <host>vla-1.db.yandex.net</host>
                            <port>2181</port>
                        </node>
                        <root>/clickhouse/cid1</root>
                    </zookeeper-servers>
                    <macros>
                        <cluster>cid1</cluster>
                        <shard>shard1</shard>
                        <replica>man-2.db.yandex.net</replica>
                    </macros>
                </yandex>
                ''',
        },
    },
    {
        'id': 'DoubleCloud HA cluster, ec1a-s1-1 replica',
        'args': {
            'grains': {
                'id': 'ach-ec1a-s1-1.cid1.yadc.io',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'aws',
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                        'geo': 'eu-central-1a',
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'ach-ec1a-s1-1.cid1.yadc.io': {'geo': 'eu-central-1a'},
                                                'ach-ec1b-s1-2.cid1.yadc.io': {'geo': 'eu-central-1b'},
                                                'ach-ec1c-s1-3.cid1.yadc.io': {'geo': 'eu-central-1c'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'ach-ec1a-s2-1.cid1.yadc.io': {'geo': 'eu-central-1a'},
                                                'ach-ec1b-s2-2.cid1.yadc.io': {'geo': 'eu-central-1b'},
                                                'ach-ec1c-s2-3.cid1.yadc.io': {'geo': 'eu-central-1c'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'cluster_name': 'default',
                        'zk_hosts': [
                            'ach-ec1a-s1-1.cid1.yadc.io',
                            'ach-ec1b-s1-2.cid1.yadc.io',
                            'ach-ec1c-s1-3.cid1.yadc.io',
                        ],
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <remote-servers>
                        <default>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>ach-ec1a-s1-1.cid1.yadc.io</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>ach-ec1b-s1-2.cid1.yadc.io</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>ach-ec1c-s1-3.cid1.yadc.io</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>ach-ec1a-s2-1.cid1.yadc.io</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>ach-ec1b-s2-2.cid1.yadc.io</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>ach-ec1c-s2-3.cid1.yadc.io</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </default>
                    </remote-servers>
                    <zookeeper-servers>
                        <node index="1">
                            <host>ach-ec1a-s1-1.cid1.yadc.io</host>
                            <port>2181</port>
                        </node>
                        <node index="2">
                            <host>ach-ec1b-s1-2.cid1.yadc.io</host>
                            <port>2181</port>
                        </node>
                        <node index="3">
                            <host>ach-ec1c-s1-3.cid1.yadc.io</host>
                            <port>2181</port>
                        </node>
                        <root>/clickhouse/cid1</root>
                    </zookeeper-servers>
                    <macros>
                        <cluster>default</cluster>
                        <shard>shard1</shard>
                        <replica>ach-ec1a-s1-1.cid1.yadc.io</replica>
                    </macros>
                    <prometheus>
                        <endpoint>/metrics</endpoint>
                        <port>9363</port>
                        <metrics>true</metrics>
                        <events>true</events>
                        <asynchronous_metrics>true</asynchronous_metrics>
                        <status_info>true</status_info>
                    </prometheus>
                </yandex>
                ''',
        },
    },
    {
        'id': 'DoubleCloud HA cluster, ec1b-s1-2 replica',
        'args': {
            'grains': {
                'id': 'ach-ec1b-s1-2.cid1.yadc.io',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'vtype': 'aws',
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                        'geo': 'eu-central-1b',
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'ach-ec1a-s1-1.cid1.yadc.io': {'geo': 'eu-central-1a'},
                                                'ach-ec1b-s1-2.cid1.yadc.io': {'geo': 'eu-central-1b'},
                                                'ach-ec1c-s1-3.cid1.yadc.io': {'geo': 'eu-central-1c'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'ach-ec1a-s2-1.cid1.yadc.io': {'geo': 'eu-central-1a'},
                                                'ach-ec1b-s2-2.cid1.yadc.io': {'geo': 'eu-central-1b'},
                                                'ach-ec1c-s2-3.cid1.yadc.io': {'geo': 'eu-central-1c'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'cluster_name': 'default',
                        'zk_hosts': [
                            'ach-ec1a-s1-1.cid1.yadc.io',
                            'ach-ec1b-s1-2.cid1.yadc.io',
                            'ach-ec1c-s1-3.cid1.yadc.io',
                        ],
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <remote-servers>
                        <default>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>ach-ec1b-s1-2.cid1.yadc.io</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>ach-ec1a-s1-1.cid1.yadc.io</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>ach-ec1c-s1-3.cid1.yadc.io</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>ach-ec1b-s2-2.cid1.yadc.io</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>ach-ec1a-s2-1.cid1.yadc.io</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>ach-ec1c-s2-3.cid1.yadc.io</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </default>
                    </remote-servers>
                    <zookeeper-servers>
                        <node index="1">
                            <host>ach-ec1b-s1-2.cid1.yadc.io</host>
                            <port>2181</port>
                        </node>
                        <node index="2">
                            <host>ach-ec1a-s1-1.cid1.yadc.io</host>
                            <port>2181</port>
                        </node>
                        <node index="3">
                            <host>ach-ec1c-s1-3.cid1.yadc.io</host>
                            <port>2181</port>
                        </node>
                        <root>/clickhouse/cid1</root>
                    </zookeeper-servers>
                    <macros>
                        <cluster>default</cluster>
                        <shard>shard1</shard>
                        <replica>ach-ec1b-s1-2.cid1.yadc.io</replica>
                    </macros>
                    <prometheus>
                        <endpoint>/metrics</endpoint>
                        <port>9363</port>
                        <metrics>true</metrics>
                        <events>true</events>
                        <asynchronous_metrics>true</asynchronous_metrics>
                        <status_info>true</status_info>
                    </prometheus>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Non-HA cluster with default shard weights',
        'args': {
            'grains': {
                'id': 'man-1.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                        'geo': 'man',
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <remote-servers>
                        <cid1>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-1.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </cid1>
                    </remote-servers>
                    <macros>
                        <cluster>cid1</cluster>
                        <shard>shard1</shard>
                        <replica>man-1.db.yandex.net</replica>
                    </macros>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Non-HA cluster with explicit shard weights',
        'args': {
            'grains': {
                'id': 'man-1.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                        'geo': 'man',
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
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
                        'shards': {
                            'shard_id1': {
                                'weight': 100,
                            },
                            'shard_id2': {
                                'weight': 200,
                            },
                        },
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <remote-servers>
                        <cid1>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-1.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>200</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </cid1>
                    </remote-servers>
                    <macros>
                        <cluster>cid1</cluster>
                        <shard>shard1</shard>
                        <replica>man-1.db.yandex.net</replica>
                    </macros>
                </yandex>
                ''',
        },
    },
    {
        'id': 'HA cluster with shard groups',
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
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                                'sas-1.db.yandex.net': {'geo': 'sas'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                                'sas-2.db.yandex.net': {'geo': 'sas'},
                                            },
                                        },
                                        'shard_id3': {
                                            'name': 'shard3',
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
                        'shard_groups': {
                            'shards12': {
                                'description': 'group of [shard1, shard2]',
                                'shard_names': ['shard1', 'shard2'],
                            },
                            'shards23': {
                                'description': 'group of [shard2, shard3]',
                                'shard_names': ['shard2', 'shard3'],
                            },
                        },
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <remote-servers>
                        <cid1>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-1.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>sas-1.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>sas-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>sas-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </cid1>
                        <shards12>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-1.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>sas-1.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>sas-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </shards12>
                        <shards23>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>sas-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                                <replica>
                                    <host>sas-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </shards23>
                    </remote-servers>
                    <zookeeper-servers>
                        <node index="1">
                            <host>man-1.db.yandex.net</host>
                            <port>2181</port>
                        </node>
                        <node index="2">
                            <host>sas-1.db.yandex.net</host>
                            <port>2181</port>
                        </node>
                        <node index="3">
                            <host>vla-1.db.yandex.net</host>
                            <port>2181</port>
                        </node>
                        <root>/clickhouse/cid1</root>
                    </zookeeper-servers>
                    <macros>
                        <cluster>cid1</cluster>
                        <shard>shard1</shard>
                        <replica>man-2.db.yandex.net</replica>
                    </macros>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Non-HA cluster with shard groups',
        'args': {
            'grains': {
                'id': 'man-1.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                        'geo': 'man',
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id3': {
                                            'name': 'shard3',
                                            'hosts': {
                                                'man-3.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'shard_groups': {
                            'shards12': {
                                'description': 'group of [shard1, shard2]',
                                'shard_names': ['shard1', 'shard2'],
                            },
                            'shards23': {
                                'description': 'group of [shard2, shard3]',
                                'shard_names': ['shard2', 'shard3'],
                            },
                        },
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <remote-servers>
                        <cid1>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-1.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </cid1>
                        <shards12>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-1.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </shards12>
                        <shards23>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </shards23>
                    </remote-servers>
                    <macros>
                        <cluster>cid1</cluster>
                        <shard>shard1</shard>
                        <replica>man-1.db.yandex.net</replica>
                    </macros>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Non-HA cluster with internal_replication set to false',
        'args': {
            'grains': {
                'id': 'man-1.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                        'geo': 'man',
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id1': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id2': {
                                            'name': 'shard2',
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
                        'unmanaged': {
                            'internal_replication': False,
                        },
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <remote-servers>
                        <cid1>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>false</internal_replication>
                                <replica>
                                    <host>man-1.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>false</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </cid1>
                    </remote-servers>
                    <macros>
                        <cluster>cid1</cluster>
                        <shard>shard1</shard>
                        <replica>man-1.db.yandex.net</replica>
                    </macros>
                </yandex>
                ''',
        },
    },
    {
        'id': 'Shards is ordered by name',
        'args': {
            'grains': {
                'id': 'man-1.db.yandex.net',
            },
            'pillar': {
                'data': {
                    'dbaas': {
                        'geo': 'man',
                        'cluster_id': 'cid1',
                        'shard_name': 'shard1',
                        'cluster': {
                            'subclusters': {
                                'subcid1': {
                                    'roles': ['clickhouse_cluster'],
                                    'shards': {
                                        'shard_id_b': {
                                            'name': 'shard1',
                                            'hosts': {
                                                'man-1.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id_c': {
                                            'name': 'shard2',
                                            'hosts': {
                                                'man-2.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                        'shard_id_a': {
                                            'name': 'shard3',
                                            'hosts': {
                                                'man-3.db.yandex.net': {'geo': 'man'},
                                            },
                                        },
                                    },
                                },
                            },
                        },
                    },
                    'clickhouse': {
                        'shard_groups': {
                            'shards12': {
                                'description': 'group of [shard1, shard2]',
                                'shard_names': ['shard1', 'shard2'],
                            },
                        },
                    },
                },
                'cert.key': 'noop key',
            },
            'result': '''
                <?xml version="1.0"?>
                <yandex>
                    <remote-servers>
                        <cid1>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-1.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-3.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </cid1>
                        <shards12>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-1.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                            <shard>
                                <weight>100</weight>
                                <internal_replication>true</internal_replication>
                                <replica>
                                    <host>man-2.db.yandex.net</host>
                                    <port>9440</port>
                                    <secure>1</secure>
                                </replica>
                            </shard>
                        </shards12>
                    </remote-servers>
                    <macros>
                        <cluster>cid1</cluster>
                        <shard>shard1</shard>
                        <replica>man-1.db.yandex.net</replica>
                    </macros>
                </yandex>
                ''',
        },
    },
)
def test_render_cluster_config(grains, pillar, result):
    mock_grains(mdb_clickhouse.__salt__, grains)
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    mock_vtype(mdb_clickhouse.__salt__, pillar['data']['dbaas'].get('vtype', 'porto'))
    assert_xml_equals(mdb_clickhouse.render_cluster_config(), result)
