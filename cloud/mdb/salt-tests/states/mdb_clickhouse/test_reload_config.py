# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.internal.python.pytest.utils import parametrize
from cloud.mdb.salt.salt._states import mdb_clickhouse
from mock import call, MagicMock


@parametrize(
    {
        'id': 'Reload config',
        'args': {
            'args': ['name'],
            'opts': {'test': False},
            'result': {
                'name': 'name',
                'result': True,
                'changes': {
                    'config': 'reloaded',
                },
                'comment': 'Config was reloaded',
            },
            'calls': {
                'reload_config': [call()],
            },
        },
    },
    {
        'id': 'Reload config (dry-run)',
        'args': {
            'args': ['name'],
            'opts': {'test': True},
            'result': {
                'name': 'name',
                'result': None,
                'changes': {
                    'config': 'reloaded',
                },
                'comment': 'Config would be reloaded',
            },
            'calls': {
                'reload_config': [],
            },
        },
    },
)
def test_reload_config(args, opts, result, calls):
    mdb_clickhouse.__opts__ = opts
    for function in calls:
        mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)] = MagicMock()
    assert mdb_clickhouse.reload_config(*args) == result
