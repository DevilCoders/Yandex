# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from cloud.mdb.internal.python.pytest.utils import parametrize

# from cloud.mdb.salt_tests.common.mocks import mock_pillar
from cloud.mdb.salt.salt._states import mdb_elasticsearch

# from cloud.mdb.salt.salt._modules.mdb_elasticsearch import __salt__

from mock import MagicMock, call


@parametrize(
    {
        'id': 'Nothing in settings',
        'args': {
            'settings': {},
            'opts': {'test': False},
            'keystore_keys': ['keystore.seed'],
            'calls': {},
            'result': {'name': 'name', 'result': True, 'changes': {}, 'comment': 'Keystore already in sync.'},
        },
    },
    {
        'id': 'Nothing changed',
        'args': {
            'settings': {'s3.client.default.access_key': 'key'},
            'opts': {'test': False},
            'keystore_keys': ['keystore.seed', 's3.client.default.access_key'],
            'calls': {},
            'result': {'name': 'name', 'result': True, 'changes': {}, 'comment': 'Keystore already in sync.'},
        },
    },
    {
        'id': 'Test install and remove keys',
        'args': {
            'settings': {'s3.client.default.access_key': 'key', 's3.client.default.secret_key': 'secret'},
            'opts': {'test': True},
            'keystore_keys': ['s3.client.default.access_key', 's3.client.default.session_token'],
            'calls': {},
            'result': {
                'name': 'name',
                'result': None,
                'changes': {
                    'keystore_add': ['s3.client.default.secret_key'],
                    'keystore_remove': ['s3.client.default.session_token'],
                },
                'comment': '',
            },
        },
    },
    {
        'id': 'Real install and remove plugins',
        'args': {
            'settings': {'s3.client.default.access_key': 'key', 's3.client.default.secret_key': 'secret'},
            'opts': {'test': False},
            'keystore_keys': ['keystore.seed', 's3.client.default.access_key', 's3.client.default.session_token'],
            'calls': {
                'keystore_remove': [call('s3.client.default.session_token')],
                'keystore_add': [call('s3.client.default.secret_key', 'secret')],
            },
            'result': {
                'name': 'name',
                'result': True,
                'changes': {
                    'keystore_added': ['s3.client.default.secret_key'],
                    'keystore_removed': ['s3.client.default.session_token'],
                },
                'comment': '',
            },
        },
    },
)
def test_ensure_keystore(settings, opts, keystore_keys, calls, result):
    mdb_elasticsearch.__opts__ = opts

    mdb_elasticsearch.__salt__['mdb_elasticsearch.keystore_keys'] = lambda: keystore_keys

    for function in calls.keys():
        mdb_elasticsearch.__salt__['mdb_elasticsearch.{0}'.format(function)] = MagicMock()

    assert mdb_elasticsearch.ensure_keystore('name', settings) == result

    for function, function_calls in calls.items():
        assert mdb_elasticsearch.__salt__['mdb_elasticsearch.{0}'.format(function)].call_args_list == function_calls
