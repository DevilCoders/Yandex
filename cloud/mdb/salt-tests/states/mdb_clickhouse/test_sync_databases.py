# coding: utf-8

from __future__ import print_function, unicode_literals

from mock import MagicMock, call

from cloud.mdb.salt.salt._states import mdb_clickhouse
from cloud.mdb.internal.python.pytest.utils import parametrize
from cloud.mdb.salt_tests.common.mocks import mock_pillar


@parametrize(
    {
        'id': 'Database added',
        'args': {
            'pillar': {'data': {'clickhouse': {'sql_database_management': False}}},
            'pillar_databases': ['db1', 'db2'],
            'existing_databases': ['db1'],
            'args': ['sync_databases'],
            'opts': {'test': False},
            'result': {
                'name': 'sync_databases',
                'result': True,
                'changes': {
                    'created': ['_system', 'db2'],
                },
                'comment': '',
            },
            'calls': {
                'create_database': [call('_system'), call('db2')],
                'delete_database': [],
            },
        },
    },
    {
        'id': 'Database added (dry-run)',
        'args': {
            'pillar': {'data': {'clickhouse': {'sql_database_management': False}}},
            'pillar_databases': ['db1', 'db2'],
            'existing_databases': ['db1'],
            'args': ['sync_databases'],
            'opts': {'test': True},
            'result': {
                'name': 'sync_databases',
                'result': None,
                'changes': {
                    'created': ['_system', 'db2'],
                },
                'comment': '',
            },
            'calls': {
                'create_database': [],
                'delete_database': [],
            },
        },
    },
    {
        'id': 'Database deleted',
        'args': {
            'pillar': {
                'data': {'clickhouse': {'sql_database_management': False}},
                'target-database': 'db2',
            },
            'pillar_databases': ['db1'],
            'existing_databases': ['db1', 'db2', '_system'],
            'args': ['sync_databases'],
            'opts': {'test': False},
            'result': {
                'name': 'sync_databases',
                'result': True,
                'changes': {
                    'deleted': ['db2'],
                },
                'comment': '',
            },
            'calls': {
                'create_database': [],
                'delete_database': [call('db2')],
            },
        },
    },
    {
        'id': 'Database deleted (dry-run)',
        'args': {
            'pillar': {
                'data': {'clickhouse': {'sql_database_management': False}},
                'target-database': 'db2',
            },
            'pillar_databases': ['db1'],
            'existing_databases': ['db1', 'db2', '_system'],
            'args': ['sync_databases'],
            'opts': {'test': True},
            'result': {
                'name': 'sync_databases',
                'result': None,
                'changes': {
                    'deleted': ['db2'],
                },
                'comment': '',
            },
            'calls': {
                'create_database': [],
                'delete_database': [],
            },
        },
    },
    {
        'id': 'All databases deleted',
        'args': {
            'pillar': {'data': {'clickhouse': {'sql_database_management': False}}},
            'pillar_databases': [],
            'existing_databases': ['db1', 'db2', '_system'],
            'args': ['sync_databases'],
            'opts': {'test': False},
            'result': {
                'name': 'sync_databases',
                'result': False,
                'changes': {},
                'comment': "Going to delete databases: [db1, db2], but target is 'None'",
            },
            'calls': {
                'create_database': [],
                'delete_database': [],
            },
        },
    },
    {
        'id': 'All databases deleted (dry-run)',
        'args': {
            'pillar': {'data': {'clickhouse': {'sql_database_management': False}}},
            'pillar_databases': [],
            'existing_databases': ['db1', 'db2', '_system'],
            'args': ['sync_databases'],
            'opts': {'test': True},
            'result': {
                'name': 'sync_databases',
                'result': False,
                'changes': {},
                'comment': "Going to delete databases: [db1, db2], but target is 'None'",
            },
            'calls': {
                'create_database': [],
                'delete_database': [],
            },
        },
    },
    {
        'id': 'Nothing changed',
        'args': {
            'pillar': {'data': {'clickhouse': {'sql_database_management': False}}},
            'pillar_databases': ['db1', 'db2'],
            'existing_databases': ['db1', 'db2', '_system'],
            'args': ['sync_databases'],
            'opts': {'test': False},
            'result': {'name': 'sync_databases', 'result': True, 'changes': {}, 'comment': ''},
            'calls': {
                'create_database': [],
                'delete_database': [],
            },
        },
    },
    {
        'id': 'Nothing changed (dry-run)',
        'args': {
            'pillar': {'data': {'clickhouse': {'sql_database_management': False}}},
            'pillar_databases': ['db1', 'db2'],
            'existing_databases': ['db1', 'db2', '_system'],
            'args': ['sync_databases'],
            'opts': {'test': True},
            'result': {'name': 'sync_databases', 'result': True, 'changes': {}, 'comment': ''},
            'calls': {
                'create_database': [],
                'delete_database': [],
            },
        },
    },
    {
        'id': 'SQL database management',
        'args': {
            'pillar': {'data': {'clickhouse': {'sql_database_management': True}}},
            'pillar_databases': [],
            'existing_databases': ['db1', 'db2', '_system'],
            'args': ['sync_databases'],
            'opts': {'test': True},
            'result': {'name': 'sync_databases', 'result': True, 'changes': {}, 'comment': ''},
            'calls': {
                'create_database': [],
                'delete_database': [],
            },
        },
    },
    {
        'id': 'SQL database management initial',
        'args': {
            'pillar': {'data': {'clickhouse': {'sql_database_management': False}}},
            'pillar_databases': [],
            'existing_databases': [],
            'args': ['sync_databases'],
            'opts': {'test': False},
            'result': {
                'name': 'sync_databases',
                'result': True,
                'changes': {
                    'created': ['_system'],
                },
                'comment': '',
            },
            'calls': {
                'create_database': [call('_system')],
                'delete_database': [],
            },
        },
    },
)
def test_sync_databases(pillar, pillar_databases, existing_databases, args, opts, result, calls):
    mock_pillar(mdb_clickhouse.__salt__, pillar)
    mdb_clickhouse.__salt__['mdb_clickhouse.databases'] = lambda: pillar_databases
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_database_names'] = lambda: existing_databases
    for function in calls:
        mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)] = MagicMock()
    mdb_clickhouse.__opts__ = opts

    assert mdb_clickhouse.sync_databases(*args) == result
    for function, function_calls in calls.items():
        assert mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)].call_args_list == function_calls
