# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.internal.python.pytest.utils import parametrize

from cloud.mdb.salt.salt._states import mdb_clickhouse


@parametrize(
    {
        'id': 'Resetup required',
        'args': {
            'resetup_required': True,
            'args': ['name'],
            'opts': {'test': False},
            'result': {
                'name': 'name',
                'result': True,
                'changes': {
                    'run': 'Resetup tool will be executed',
                },
                'comment': '',
            },
        },
    },
    {
        'id': 'Resetup required (dry-run)',
        'args': {
            'resetup_required': True,
            'args': ['name'],
            'opts': {'test': True},
            'result': {
                'name': 'name',
                'result': None,
                'changes': {
                    'run': 'Resetup tool will be executed',
                },
                'comment': '',
            },
        },
    },
    {
        'id': 'Resetup required',
        'args': {
            'resetup_required': False,
            'args': ['name'],
            'opts': {'test': False},
            'result': {'name': 'name', 'result': True, 'changes': {}, 'comment': ''},
        },
    },
    {
        'id': 'Resetup required (dry-run)',
        'args': {
            'resetup_required': False,
            'args': ['name'],
            'opts': {'test': True},
            'result': {'name': 'name', 'result': True, 'changes': {}, 'comment': ''},
        },
    },
)
def test_resetup_required(resetup_required, args, opts, result):
    mdb_clickhouse.__salt__['mdb_clickhouse.resetup_required'] = lambda: resetup_required
    mdb_clickhouse.__opts__ = opts
    assert mdb_clickhouse.resetup_required(*args) == result
