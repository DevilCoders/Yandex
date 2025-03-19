# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.internal.python.pytest.utils import parametrize
from cloud.mdb.salt.salt._states import mdb_clickhouse
from mock import MagicMock


@parametrize(
    {
        'id': 'Cleanup log tables',
        'args': {
            'args': ['name'],
            'existing_tables': {
                'default': [],
                'system': [
                    {'name': 'query_thread_log'},
                    {'name': 'metric_log'},
                ],
            },
            'system_tables': {
                'query_thread_log': {'enabled': False},
                'metric_log': {'enabled': True},
            },
            'opts': {'test': False},
            'result': {
                'name': 'name',
                'result': True,
                'changes': {
                    'deleted_tables': ['query_thread_log'],
                },
                'comment': '',
            },
            'calls': {
                'cleanup_log_tables': [],
                'delete_table': [],
            },
        },
    },
    {
        'id': 'Cleanup log tables (nothing to delete)',
        'args': {
            'args': ['name'],
            'existing_tables': {
                'default': [],
                'system': [
                    {'name': 'query_thread_log'},
                    {'name': 'metric_log'},
                ],
            },
            'system_tables': {
                'query_thread_log': {'enabled': True},
                'metric_log': {'enabled': True},
            },
            'opts': {'test': False},
            'result': {'name': 'name', 'result': True, 'changes': {}, 'comment': ''},
            'calls': {
                'cleanup_log_tables': [],
                'delete_table': [],
            },
        },
    },
    {
        'id': 'Cleanup log tables (dry run)',
        'args': {
            'args': ['name'],
            'existing_tables': {
                'default': [],
                'system': [
                    {'name': 'query_thread_log'},
                ],
            },
            'system_tables': {
                'query_thread_log': {'enabled': False},
            },
            'opts': {'test': True},
            'result': {
                'name': 'name',
                'result': None,
                'changes': {
                    'deleted_tables': ['query_thread_log'],
                },
                'comment': '',
            },
            'calls': {
                'cleanup_log_tables': [],
                'delete_table': [],
            },
        },
    },
)
def test_cleanup_log_tables(args, existing_tables, system_tables, opts, result, calls):
    mdb_clickhouse.__opts__ = opts
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_tables'] = lambda db: existing_tables[db]
    mdb_clickhouse.__salt__['mdb_clickhouse.enable_force_drop_table'] = lambda *arg: None
    mdb_clickhouse.__salt__['mdb_clickhouse.disable_force_drop_table'] = lambda *arg: None
    mdb_clickhouse.__salt__['mdb_clickhouse.system_tables'] = lambda: system_tables
    for function in calls:
        mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)] = MagicMock()
    assert mdb_clickhouse.cleanup_log_tables(*args) == result
