# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.internal.python.pytest.utils import parametrize
from cloud.mdb.salt.salt._states import mdb_clickhouse
from mock import call, MagicMock


@parametrize(
    {
        'id': 'Reload dictionaries',
        'args': {
            'args': ['name'],
            'opts': {'test': False},
            'result': {
                'name': 'name',
                'result': True,
                'changes': {
                    'dictionaries': 'reloaded',
                },
                'comment': 'Dictionaries was reloaded',
            },
            'calls': {
                'reload_dictionaries': [call()],
            },
        },
    },
    {
        'id': 'Reload dictionaries (dry-run)',
        'args': {
            'args': ['name'],
            'opts': {'test': True},
            'result': {
                'name': 'name',
                'result': None,
                'changes': {
                    'dictionaries': 'reloaded',
                },
                'comment': 'Dictionaries would be reloaded',
            },
            'calls': {
                'reload_dictionaries': [],
            },
        },
    },
)
def test_reload_config(args, opts, result, calls):
    mdb_clickhouse.__opts__ = opts
    for function in calls:
        mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)] = MagicMock()
    assert mdb_clickhouse.reload_dictionaries(*args) == result
