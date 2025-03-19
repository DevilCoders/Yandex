# coding: utf-8

from __future__ import print_function, unicode_literals

from cloud.mdb.salt.salt._states.mdb_clickhouse import (
    CREATE_TABLE_QUERIES,
    INIT_TABLE_QUERIES,
    CREATE_TABLE_QUERIES_HA,
    INIT_TABLE_QUERIES_HA,
    HA_TABLE_ZK_PATH,
)
from mock import MagicMock, call

from cloud.mdb.salt.salt._states import mdb_clickhouse
from cloud.mdb.internal.python.pytest.utils import parametrize


def _build_zk_path(table_path):
    from os.path import join

    return join('/clickhouse/cid', table_path.format(shard='shard1'), 'replicas', 'hostname')


@parametrize(
    {
        'id': 'Initial sync in non-HA configuration',
        'args': {
            'has_zookeeper': False,
            'existing_tables': {
                'default': [],
                '_system': [],
            },
            'tables_have_data_parts': False,
            'args': ['ensure_mdb_tables'],
            'opts': {'test': False},
            'result': {
                'name': 'ensure_mdb_tables',
                'result': True,
                'changes': {
                    'created_tables': [
                        CREATE_TABLE_QUERIES['read_sli_part'],
                        CREATE_TABLE_QUERIES['read_sli'],
                        CREATE_TABLE_QUERIES['write_sli_part'],
                        CREATE_TABLE_QUERIES['write_sli'],
                        CREATE_TABLE_QUERIES['primary_election_part'],
                        CREATE_TABLE_QUERIES['primary_election'],
                    ],
                    'inserted_data': [
                        INIT_TABLE_QUERIES['read_sli_part'],
                        INIT_TABLE_QUERIES['primary_election_part'],
                    ],
                },
                'comment': '',
            },
            'calls': {
                'delete_table': [],
                'execute': [
                    call(CREATE_TABLE_QUERIES['read_sli_part']),
                    call(CREATE_TABLE_QUERIES['read_sli']),
                    call(CREATE_TABLE_QUERIES['write_sli_part']),
                    call(CREATE_TABLE_QUERIES['write_sli']),
                    call(CREATE_TABLE_QUERIES['primary_election_part']),
                    call(CREATE_TABLE_QUERIES['primary_election']),
                    call(INIT_TABLE_QUERIES['read_sli_part']),
                    call(INIT_TABLE_QUERIES['primary_election_part']),
                ],
            },
            'zk_nodes': [],
            'zk_auth': [None, None, None],
        },
    },
    {
        'id': 'Initial sync in non-HA configuration (dry-run)',
        'args': {
            'has_zookeeper': False,
            'existing_tables': {
                'default': [],
                '_system': [],
            },
            'tables_have_data_parts': False,
            'args': ['ensure_mdb_tables'],
            'opts': {'test': True},
            'result': {
                'name': 'ensure_mdb_tables',
                'result': None,
                'changes': {
                    'created_tables': [
                        CREATE_TABLE_QUERIES['read_sli_part'],
                        CREATE_TABLE_QUERIES['read_sli'],
                        CREATE_TABLE_QUERIES['write_sli_part'],
                        CREATE_TABLE_QUERIES['write_sli'],
                        CREATE_TABLE_QUERIES['primary_election_part'],
                        CREATE_TABLE_QUERIES['primary_election'],
                    ],
                    'inserted_data': [
                        INIT_TABLE_QUERIES['read_sli_part'],
                        INIT_TABLE_QUERIES['primary_election_part'],
                    ],
                },
                'comment': '',
            },
            'calls': {
                'delete_table': [],
                'execute': [],
            },
            'zk_nodes': [],
            'zk_auth': [None, None, None],
        },
    },
    {
        'id': 'Initial sync in HA configuration',
        'args': {
            'has_zookeeper': True,
            'existing_tables': {
                'default': [],
                '_system': [],
            },
            'tables_have_data_parts': False,
            'args': ['ensure_mdb_tables'],
            'opts': {'test': False},
            'result': {
                'name': 'ensure_mdb_tables',
                'result': True,
                'changes': {
                    'created_tables': [
                        CREATE_TABLE_QUERIES_HA['read_sli_part'],
                        CREATE_TABLE_QUERIES_HA['read_sli'],
                        CREATE_TABLE_QUERIES_HA['write_sli_part'],
                        CREATE_TABLE_QUERIES_HA['write_sli'],
                        CREATE_TABLE_QUERIES_HA['shard_primary_election'],
                        CREATE_TABLE_QUERIES_HA['cluster_primary_election'],
                    ],
                    'inserted_data': [
                        INIT_TABLE_QUERIES_HA['read_sli_part'],
                    ],
                },
                'comment': '',
            },
            'calls': {
                'delete_table': [],
                'execute': [
                    call(CREATE_TABLE_QUERIES_HA['read_sli_part']),
                    call(CREATE_TABLE_QUERIES_HA['read_sli']),
                    call(CREATE_TABLE_QUERIES_HA['write_sli_part']),
                    call(CREATE_TABLE_QUERIES_HA['write_sli']),
                    call(CREATE_TABLE_QUERIES_HA['shard_primary_election']),
                    call(CREATE_TABLE_QUERIES_HA['cluster_primary_election']),
                    call(INIT_TABLE_QUERIES_HA['read_sli_part']),
                ],
            },
            'zk_nodes': [],
            'zk_auth': [None, None, None],
        },
    },
    {
        'id': 'Initial sync in HA configuration (dry-run)',
        'args': {
            'has_zookeeper': True,
            'existing_tables': {
                'default': [],
                '_system': [],
            },
            'tables_have_data_parts': False,
            'args': ['ensure_mdb_tables'],
            'opts': {'test': True},
            'result': {
                'name': 'ensure_mdb_tables',
                'result': None,
                'changes': {
                    'created_tables': [
                        CREATE_TABLE_QUERIES_HA['read_sli_part'],
                        CREATE_TABLE_QUERIES_HA['read_sli'],
                        CREATE_TABLE_QUERIES_HA['write_sli_part'],
                        CREATE_TABLE_QUERIES_HA['write_sli'],
                        CREATE_TABLE_QUERIES_HA['shard_primary_election'],
                        CREATE_TABLE_QUERIES_HA['cluster_primary_election'],
                    ],
                    'inserted_data': [
                        INIT_TABLE_QUERIES_HA['read_sli_part'],
                    ],
                },
                'comment': '',
            },
            'calls': {
                'delete_table': [],
                'execute': [],
            },
            'zk_nodes': [],
            'zk_auth': [None, None, None],
        },
    },
    {
        'id': 'Upgrade to HA configuration',
        'args': {
            'has_zookeeper': True,
            'existing_tables': {
                'default': [],
                '_system': [
                    {
                        'name': 'read_sli',
                        'create_table_query': CREATE_TABLE_QUERIES['read_sli'],
                    },
                    {
                        'name': 'read_sli_part',
                        'create_table_query': CREATE_TABLE_QUERIES['read_sli_part'],
                    },
                    {
                        'name': 'write_sli',
                        'create_table_query': CREATE_TABLE_QUERIES['write_sli'],
                    },
                    {
                        'name': 'write_sli_part',
                        'create_table_query': CREATE_TABLE_QUERIES['write_sli_part'],
                    },
                    {
                        'name': 'primary_election',
                        'create_table_query': CREATE_TABLE_QUERIES['primary_election'],
                    },
                    {
                        'name': 'primary_election_part',
                        'create_table_query': CREATE_TABLE_QUERIES['primary_election_part'],
                    },
                ],
            },
            'tables_have_data_parts': True,
            'args': ['ensure_mdb_tables'],
            'opts': {'test': False},
            'result': {
                'name': 'ensure_mdb_tables',
                'result': True,
                'changes': {
                    'deleted_tables': [
                        CREATE_TABLE_QUERIES['read_sli_part'],
                        CREATE_TABLE_QUERIES['write_sli_part'],
                        CREATE_TABLE_QUERIES['primary_election'],
                        CREATE_TABLE_QUERIES['primary_election_part'],
                    ],
                    'created_tables': [
                        CREATE_TABLE_QUERIES_HA['read_sli_part'],
                        CREATE_TABLE_QUERIES_HA['write_sli_part'],
                        CREATE_TABLE_QUERIES_HA['shard_primary_election'],
                        CREATE_TABLE_QUERIES_HA['cluster_primary_election'],
                    ],
                    'inserted_data': [
                        INIT_TABLE_QUERIES_HA['read_sli_part'],
                    ],
                },
                'comment': '',
            },
            'calls': {
                'delete_table': [
                    call('_system', 'read_sli_part'),
                    call('_system', 'write_sli_part'),
                    call('_system', 'primary_election'),
                    call('_system', 'primary_election_part'),
                ],
                'execute': [
                    call(CREATE_TABLE_QUERIES_HA['read_sli_part']),
                    call(CREATE_TABLE_QUERIES_HA['write_sli_part']),
                    call(CREATE_TABLE_QUERIES_HA['shard_primary_election']),
                    call(CREATE_TABLE_QUERIES_HA['cluster_primary_election']),
                    call(INIT_TABLE_QUERIES_HA['read_sli_part']),
                ],
            },
            'zk_nodes': [],
            'zk_auth': [None, None, None],
        },
    },
    {
        'id': 'Upgrade to HA configuration (dry-run)',
        'args': {
            'has_zookeeper': True,
            'existing_tables': {
                'default': [],
                '_system': [
                    {
                        'name': 'read_sli',
                        'create_table_query': CREATE_TABLE_QUERIES['read_sli'],
                    },
                    {
                        'name': 'read_sli_part',
                        'create_table_query': CREATE_TABLE_QUERIES['read_sli_part'],
                    },
                    {
                        'name': 'write_sli',
                        'create_table_query': CREATE_TABLE_QUERIES['write_sli'],
                    },
                    {
                        'name': 'write_sli_part',
                        'create_table_query': CREATE_TABLE_QUERIES['write_sli_part'],
                    },
                    {
                        'name': 'primary_election',
                        'create_table_query': CREATE_TABLE_QUERIES['primary_election'],
                    },
                    {
                        'name': 'primary_election_part',
                        'create_table_query': CREATE_TABLE_QUERIES['primary_election_part'],
                    },
                ],
            },
            'tables_have_data_parts': True,
            'args': ['ensure_mdb_tables'],
            'opts': {'test': True},
            'result': {
                'name': 'ensure_mdb_tables',
                'result': None,
                'changes': {
                    'deleted_tables': [
                        CREATE_TABLE_QUERIES['read_sli_part'],
                        CREATE_TABLE_QUERIES['write_sli_part'],
                        CREATE_TABLE_QUERIES['primary_election'],
                        CREATE_TABLE_QUERIES['primary_election_part'],
                    ],
                    'created_tables': [
                        CREATE_TABLE_QUERIES_HA['read_sli_part'],
                        CREATE_TABLE_QUERIES_HA['write_sli_part'],
                        CREATE_TABLE_QUERIES_HA['shard_primary_election'],
                        CREATE_TABLE_QUERIES_HA['cluster_primary_election'],
                    ],
                    'inserted_data': [
                        INIT_TABLE_QUERIES_HA['read_sli_part'],
                    ],
                },
                'comment': '',
            },
            'calls': {
                'delete_table': [],
                'execute': [],
            },
            'zk_nodes': [],
            'zk_auth': [None, None, None],
        },
    },
    {
        'id': 'Fix uninitialized table',
        'args': {
            'has_zookeeper': False,
            'existing_tables': {
                'default': [],
                '_system': [
                    {
                        'name': 'read_sli',
                        'create_table_query': CREATE_TABLE_QUERIES['read_sli'],
                    },
                    {
                        'name': 'read_sli_part',
                        'create_table_query': CREATE_TABLE_QUERIES['read_sli_part'],
                    },
                    {
                        'name': 'write_sli',
                        'create_table_query': CREATE_TABLE_QUERIES['write_sli'],
                    },
                    {
                        'name': 'write_sli_part',
                        'create_table_query': CREATE_TABLE_QUERIES['write_sli_part'],
                    },
                    {
                        'name': 'primary_election',
                        'create_table_query': CREATE_TABLE_QUERIES['primary_election'],
                    },
                    {
                        'name': 'primary_election_part',
                        'create_table_query': CREATE_TABLE_QUERIES['primary_election_part'],
                    },
                ],
            },
            'tables_have_data_parts': False,
            'args': ['ensure_mdb_tables'],
            'opts': {'test': False},
            'result': {
                'name': 'ensure_mdb_tables',
                'result': True,
                'changes': {
                    'inserted_data': [
                        INIT_TABLE_QUERIES['read_sli_part'],
                        INIT_TABLE_QUERIES['primary_election_part'],
                    ],
                },
                'comment': '',
            },
            'calls': {
                'delete_table': [],
                'execute': [
                    call(INIT_TABLE_QUERIES['read_sli_part']),
                    call(INIT_TABLE_QUERIES['primary_election_part']),
                ],
            },
            'zk_nodes': [],
            'zk_auth': [None, None, None],
        },
    },
    {
        'id': 'Fix uninitialized table (dry-run)',
        'args': {
            'has_zookeeper': False,
            'existing_tables': {
                'default': [],
                '_system': [
                    {
                        'name': 'read_sli',
                        'create_table_query': CREATE_TABLE_QUERIES['read_sli'],
                    },
                    {
                        'name': 'read_sli_part',
                        'create_table_query': CREATE_TABLE_QUERIES['read_sli_part'],
                    },
                    {
                        'name': 'write_sli',
                        'create_table_query': CREATE_TABLE_QUERIES['write_sli'],
                    },
                    {
                        'name': 'write_sli_part',
                        'create_table_query': CREATE_TABLE_QUERIES['write_sli_part'],
                    },
                    {
                        'name': 'primary_election',
                        'create_table_query': CREATE_TABLE_QUERIES['primary_election'],
                    },
                    {
                        'name': 'primary_election_part',
                        'create_table_query': CREATE_TABLE_QUERIES['primary_election_part'],
                    },
                ],
            },
            'tables_have_data_parts': False,
            'args': ['ensure_mdb_tables'],
            'opts': {'test': True},
            'result': {
                'name': 'ensure_mdb_tables',
                'result': None,
                'changes': {
                    'inserted_data': [
                        INIT_TABLE_QUERIES['read_sli_part'],
                        INIT_TABLE_QUERIES['primary_election_part'],
                    ],
                },
                'comment': '',
            },
            'calls': {
                'delete_table': [],
                'execute': [],
            },
            'zk_nodes': [],
            'zk_auth': [None, None, None],
        },
    },
    {
        'id': 'No changes required',
        'args': {
            'has_zookeeper': False,
            'existing_tables': {
                'default': [],
                '_system': [
                    {
                        'name': 'read_sli',
                        'create_table_query': CREATE_TABLE_QUERIES['read_sli'],
                    },
                    {
                        'name': 'read_sli_part',
                        'create_table_query': CREATE_TABLE_QUERIES['read_sli_part'],
                    },
                    {
                        'name': 'write_sli',
                        'create_table_query': CREATE_TABLE_QUERIES['write_sli'],
                    },
                    {
                        'name': 'write_sli_part',
                        'create_table_query': CREATE_TABLE_QUERIES['write_sli_part'],
                    },
                    {
                        'name': 'primary_election',
                        'create_table_query': CREATE_TABLE_QUERIES['primary_election'],
                    },
                    {
                        'name': 'primary_election_part',
                        'create_table_query': CREATE_TABLE_QUERIES['primary_election_part'],
                    },
                ],
            },
            'tables_have_data_parts': True,
            'args': ['ensure_mdb_tables'],
            'opts': {'test': False},
            'result': {'name': 'ensure_mdb_tables', 'result': True, 'changes': {}, 'comment': ''},
            'calls': {
                'delete_table': [],
                'execute': [],
            },
            'zk_nodes': [],
            'zk_auth': [None, None, None],
        },
    },
    {
        'id': 'No changes required (dry-run)',
        'args': {
            'has_zookeeper': False,
            'existing_tables': {
                'default': [],
                '_system': [
                    {
                        'name': 'read_sli',
                        'create_table_query': CREATE_TABLE_QUERIES['read_sli'],
                    },
                    {
                        'name': 'read_sli_part',
                        'create_table_query': CREATE_TABLE_QUERIES['read_sli_part'],
                    },
                    {
                        'name': 'write_sli',
                        'create_table_query': CREATE_TABLE_QUERIES['write_sli'],
                    },
                    {
                        'name': 'write_sli_part',
                        'create_table_query': CREATE_TABLE_QUERIES['write_sli_part'],
                    },
                    {
                        'name': 'primary_election',
                        'create_table_query': CREATE_TABLE_QUERIES['primary_election'],
                    },
                    {
                        'name': 'primary_election_part',
                        'create_table_query': CREATE_TABLE_QUERIES['primary_election_part'],
                    },
                ],
            },
            'tables_have_data_parts': True,
            'args': ['ensure_mdb_tables'],
            'opts': {'test': True},
            'result': {'name': 'ensure_mdb_tables', 'result': True, 'changes': {}, 'comment': ''},
            'calls': {
                'delete_table': [],
                'execute': [],
            },
            'zk_nodes': [],
            'zk_auth': [None, None, None],
        },
    },
    {
        'id': 'Sync in HA configuration with dirty zk meta',
        'args': {
            'has_zookeeper': True,
            'existing_tables': {
                'default': [],
                '_system': [],
            },
            'tables_have_data_parts': False,
            'args': ['ensure_mdb_tables'],
            'opts': {'test': False},
            'result': {
                'name': 'ensure_mdb_tables',
                'result': True,
                'changes': {
                    'created_tables': [
                        CREATE_TABLE_QUERIES_HA['read_sli_part'],
                        CREATE_TABLE_QUERIES_HA['read_sli'],
                        CREATE_TABLE_QUERIES_HA['write_sli_part'],
                        CREATE_TABLE_QUERIES_HA['write_sli'],
                        CREATE_TABLE_QUERIES_HA['shard_primary_election'],
                        CREATE_TABLE_QUERIES_HA['cluster_primary_election'],
                    ],
                    'inserted_data': [
                        INIT_TABLE_QUERIES_HA['read_sli_part'],
                    ],
                    'delete_zk_nodes': [
                        _build_zk_path(HA_TABLE_ZK_PATH['read_sli_part']),
                        _build_zk_path(HA_TABLE_ZK_PATH['write_sli_part']),
                        _build_zk_path(HA_TABLE_ZK_PATH['shard_primary_election']),
                        _build_zk_path(HA_TABLE_ZK_PATH['cluster_primary_election']),
                    ],
                },
                'comment': '',
            },
            'calls': {
                'delete_table': [],
                'execute': [
                    call(CREATE_TABLE_QUERIES_HA['read_sli_part']),
                    call(CREATE_TABLE_QUERIES_HA['read_sli']),
                    call(CREATE_TABLE_QUERIES_HA['write_sli_part']),
                    call(CREATE_TABLE_QUERIES_HA['write_sli']),
                    call(CREATE_TABLE_QUERIES_HA['shard_primary_election']),
                    call(CREATE_TABLE_QUERIES_HA['cluster_primary_election']),
                    call(INIT_TABLE_QUERIES_HA['read_sli_part']),
                ],
            },
            'zk_nodes': [
                _build_zk_path(HA_TABLE_ZK_PATH['read_sli_part']),
                _build_zk_path(HA_TABLE_ZK_PATH['write_sli_part']),
                _build_zk_path(HA_TABLE_ZK_PATH['shard_primary_election']),
                _build_zk_path(HA_TABLE_ZK_PATH['cluster_primary_election']),
            ],
            'zk_auth': [None, None, None],
        },
    },
    {
        'id': 'Sync in HA configuration with dirty zk meta with ZK ACL',
        'args': {
            'has_zookeeper': True,
            'existing_tables': {
                'default': [],
                '_system': [],
            },
            'tables_have_data_parts': False,
            'args': ['ensure_mdb_tables'],
            'opts': {'test': False},
            'result': {
                'name': 'ensure_mdb_tables',
                'result': True,
                'changes': {
                    'created_tables': [
                        CREATE_TABLE_QUERIES_HA['read_sli_part'],
                        CREATE_TABLE_QUERIES_HA['read_sli'],
                        CREATE_TABLE_QUERIES_HA['write_sli_part'],
                        CREATE_TABLE_QUERIES_HA['write_sli'],
                        CREATE_TABLE_QUERIES_HA['shard_primary_election'],
                        CREATE_TABLE_QUERIES_HA['cluster_primary_election'],
                    ],
                    'inserted_data': [
                        INIT_TABLE_QUERIES_HA['read_sli_part'],
                    ],
                    'delete_zk_nodes': [
                        _build_zk_path(HA_TABLE_ZK_PATH['read_sli_part']),
                        _build_zk_path(HA_TABLE_ZK_PATH['write_sli_part']),
                        _build_zk_path(HA_TABLE_ZK_PATH['shard_primary_election']),
                        _build_zk_path(HA_TABLE_ZK_PATH['cluster_primary_election']),
                    ],
                },
                'comment': '',
            },
            'calls': {
                'delete_table': [],
                'execute': [
                    call(CREATE_TABLE_QUERIES_HA['read_sli_part']),
                    call(CREATE_TABLE_QUERIES_HA['read_sli']),
                    call(CREATE_TABLE_QUERIES_HA['write_sli_part']),
                    call(CREATE_TABLE_QUERIES_HA['write_sli']),
                    call(CREATE_TABLE_QUERIES_HA['shard_primary_election']),
                    call(CREATE_TABLE_QUERIES_HA['cluster_primary_election']),
                    call(INIT_TABLE_QUERIES_HA['read_sli_part']),
                ],
            },
            'zk_nodes': [
                _build_zk_path(HA_TABLE_ZK_PATH['read_sli_part']),
                _build_zk_path(HA_TABLE_ZK_PATH['write_sli_part']),
                _build_zk_path(HA_TABLE_ZK_PATH['shard_primary_election']),
                _build_zk_path(HA_TABLE_ZK_PATH['cluster_primary_election']),
            ],
            'zk_auth': ['digest', 'clickhouse', 'password.password.passowrd'],
        },
    },
)
def test_ensure_mdb_tables(
    has_zookeeper, existing_tables, tables_have_data_parts, args, opts, result, calls, zk_nodes, zk_auth
):
    mdb_clickhouse.__salt__['mdb_clickhouse.has_zookeeper'] = lambda: has_zookeeper
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_tables'] = lambda db: existing_tables[db]
    mdb_clickhouse.__salt__['mdb_clickhouse.has_data_parts'] = lambda db, table: tables_have_data_parts

    mdb_clickhouse.__salt__['mdb_clickhouse.zookeeper_hosts'] = lambda: ['zkhost'] if has_zookeeper else []
    mdb_clickhouse.__salt__['mdb_clickhouse.zookeeper_root'] = lambda: '/clickhouse/cid'
    mdb_clickhouse.__salt__['mdb_clickhouse.shard_name'] = lambda: 'shard1'
    mdb_clickhouse.__salt__['mdb_clickhouse.hostname'] = lambda: 'hostname'
    mdb_clickhouse.__salt__['mdb_clickhouse.zookeeper_user'] = lambda: zk_auth[1]
    mdb_clickhouse.__salt__['mdb_clickhouse.zookeeper_password'] = lambda user: zk_auth[2] if user == zk_auth[1] else ''
    mdb_clickhouse.__salt__['mdb_clickhouse.has_embedded_keeper'] = lambda: False

    mdb_clickhouse.__salt__['zookeeper.exists'] = lambda key, hosts, scheme, username, password: key in zk_nodes

    for function in calls:
        mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)] = MagicMock()
    mdb_clickhouse.__salt__['zookeeper.delete'] = MagicMock()
    mdb_clickhouse.__opts__ = opts

    assert mdb_clickhouse.ensure_mdb_tables(*args) == result
    for function, function_calls in calls.items():
        assert mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)].call_args_list == function_calls
    assert mdb_clickhouse.__salt__['zookeeper.delete'].call_args_list == [
        call(
            node,
            recursive=True,
            hosts=mdb_clickhouse.__salt__['mdb_clickhouse.zookeeper_hosts'](),
            scheme=zk_auth[0],
            username=zk_auth[1],
            password=zk_auth[2],
        )
        for node in zk_nodes
    ]
