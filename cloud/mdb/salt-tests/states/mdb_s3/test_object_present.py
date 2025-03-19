# coding: utf-8

from __future__ import print_function, unicode_literals

from cloud.mdb.salt.salt._states import mdb_s3
from cloud.mdb.internal.python.pytest.utils import parametrize


def mock_s3(salt, contents=None):
    store = {} if contents is None else contents

    def _client():
        return None

    def _object_exists(s3_client, key):
        return key in store

    def _create_object(s3_client, key):
        store[key] = b''

    salt['mdb_s3.client'] = _client
    salt['mdb_s3.object_exists'] = _object_exists
    salt['mdb_s3.create_object'] = _create_object


@parametrize(
    {
        'id': 'object already exists',
        'args': {
            's3_contents': {'test_object': b''},
            'args': ['test_object'],
            'opts': {'test': False},
            'result': {
                'name': 'test_object',
                'result': True,
                'changes': {},
                'comment': 'Object "test_object" already exists',
            },
        },
    },
    {
        'id': 'object already exists (dry-run)',
        'args': {
            's3_contents': {'test_object': b''},
            'args': ['test_object'],
            'opts': {'test': True},
            'result': {
                'name': 'test_object',
                'result': True,
                'changes': {},
                'comment': 'Object "test_object" already exists',
            },
        },
    },
    {
        'id': 'new object created',
        'args': {
            's3_contents': {},
            'args': ['test_object'],
            'opts': {'test': False},
            'result': {
                'name': 'test_object',
                'result': True,
                'changes': {'created': 'test_object'},
                'comment': 'Object "test_object" was created',
            },
        },
    },
    {
        'id': 'new object created (dry-run)',
        'args': {
            's3_contents': {},
            'args': ['test_object'],
            'opts': {'test': True},
            'result': {
                'name': 'test_object',
                'result': None,
                'changes': {'created': 'test_object'},
                'comment': 'Object "test_object" will be created',
            },
        },
    },
)
def test_object_present(s3_contents, args, opts, result):
    mock_s3(mdb_s3.__salt__, s3_contents)
    mdb_s3.__opts__ = opts
    assert mdb_s3.object_present(*args) == result
