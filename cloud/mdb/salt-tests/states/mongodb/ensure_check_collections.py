#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import mock_mongodb
import test_helpers

TEST_DATA = [  # initial_data, service, pillar, expected result
    (  # 1
        {
            'shards': {'rs01': ['rs01']},
            'dbs': {},
        },
        {
            'id1': {
                'roles': ['mongodb_cluster.mongod'],
                'shards': {
                    'i1': {'name': 'rs01'},
                },
            },
        },
        'mdb_internal',
        'check_',
        {
            'shards': {'rs01': ['rs01']},
            'dbs': {
                'mdb_internal': {
                    'sharded': True,
                    'collections': {
                        'check_rs01': {
                            'sharded': True,
                            'tags': ['rs01'],
                            'capped': False,
                            'indexes': [{'expireAfterSeconds': 86400, 'key': [('d', 1)]}],
                        },
                    },
                },
            },
        },
    ),
    (  # 2
        {
            'shards': {'rs01': ['rs01'], 'rs02': ['rs02'], 'rs03': ['rs03'], 'rs04': ['rs04'], 'rs05': ['rs05', 'x1']},
            'dbs': {
                'x1': {
                    'sharded': True,
                    'collections': {},
                },
                'x2': {
                    'sharded': False,
                    'collections': {},
                },
                'mdb_internal': {
                    'sharded': True,
                    'collections': {
                        'some_collection': {
                            'sharded': False,
                            'tags': [],
                            'capped': False,
                            'indexes': [],
                        },
                        'check_rs01': {
                            'sharded': True,
                            'tags': ['rs01'],
                            'capped': False,
                            'indexes': [{'expireAfterSeconds': 86400, 'key': [('d', 1)]}],
                        },
                        'check_rs03': {
                            'sharded': True,
                            'tags': ['rs02', 'rs03'],
                            'capped': False,
                            'indexes': [{'expireAfterSeconds': 86400, 'key': [('d', 1)]}],
                        },
                        'check_rs04': {
                            'sharded': False,
                            'tags': ['rs04'],
                            'capped': False,
                            'indexes': [{'expireAfterSeconds': 86400, 'key': [('d', 1)]}],
                        },
                        'check_rs05': {
                            'sharded': True,
                            'tags': ['rs05'],
                            'capped': False,
                            'indexes': [],
                        },
                    },
                },
            },
        },
        {
            'id1': {
                'roles': ['mongodb_cluster.mongod'],
                'shards': {
                    'i1': {'name': 'rs01'},
                    'i2': {'name': 'rs02'},
                    'i3': {'name': 'rs03'},
                    'i4': {'name': 'rs04'},
                    'i5': {'name': 'rs05'},
                },
            },
        },
        'mdb_internal',
        'check_',
        {
            'shards': {'rs01': ['rs01'], 'rs02': ['rs02'], 'rs03': ['rs03'], 'rs04': ['rs04'], 'rs05': ['rs05', 'x1']},
            'dbs': {
                'x1': {
                    'sharded': True,
                    'collections': {},
                },
                'x2': {
                    'sharded': False,
                    'collections': {},
                },
                'mdb_internal': {
                    'sharded': True,
                    'collections': {
                        'some_collection': {
                            'sharded': False,
                            'tags': [],
                            'capped': False,
                            'indexes': [],
                        },
                        'check_rs01': {
                            'sharded': True,
                            'tags': ['rs01'],
                            'capped': False,
                            'indexes': [{'expireAfterSeconds': 86400, 'key': [('d', 1)]}],
                        },
                        'check_rs02': {
                            'sharded': True,
                            'tags': ['rs02'],
                            'capped': False,
                            'indexes': [{'expireAfterSeconds': 86400, 'key': [('d', 1)]}],
                        },
                        'check_rs03': {
                            'sharded': True,
                            'tags': ['rs03'],
                            'capped': False,
                            'indexes': [{'expireAfterSeconds': 86400, 'key': [('d', 1)]}],
                        },
                        'check_rs04': {
                            'sharded': True,
                            'tags': ['rs04'],
                            'capped': False,
                            'indexes': [{'expireAfterSeconds': 86400, 'key': [('d', 1)]}],
                        },
                        'check_rs05': {
                            'sharded': True,
                            'tags': ['rs05'],
                            'capped': False,
                            'indexes': [{'expireAfterSeconds': 86400, 'key': [('d', 1)]}],
                        },
                    },
                },
            },
        },
    ),
    (  # 3
        {
            'shards': {'rs01': ['rs01']},
            'dbs': {
                'mdb_internal': {
                    'sharded': True,
                    'collections': {
                        'check_rs02': {
                            'sharded': True,
                            'tags': ['rs01'],
                            'capped': False,
                            'indexes': [{'expireAfterSeconds': 86400, 'key': [('d', 1)]}],
                        },
                        'ololo': {
                            'sharded': False,
                            'tags': [],
                            'capped': False,
                            'indexes': [],
                        },
                    },
                },
            },
        },
        {
            'id1': {
                'roles': ['mongodb_cluster.mongod'],
                'shards': {
                    'i1': {'name': 'rs01'},
                },
            },
        },
        'mdb_internal',
        'check_',
        {
            'shards': {'rs01': ['rs01']},
            'dbs': {
                'mdb_internal': {
                    'sharded': True,
                    'collections': {
                        'check_rs01': {
                            'sharded': True,
                            'tags': ['rs01'],
                            'capped': False,
                            'indexes': [{'expireAfterSeconds': 86400, 'key': [('d', 1)]}],
                        },
                        'ololo': {
                            'sharded': False,
                            'tags': [],
                            'capped': False,
                            'indexes': [],
                        },
                    },
                },
            },
        },
    ),
]


@pytest.mark.parametrize('initial_data, pillar, db, prefix, expected', TEST_DATA)
def test_ensure_check_collections(initial_data, pillar, db, prefix, expected):
    return test_helpers.check_some_state(
        initial_data,
        {'data': {'dbaas': {'cluster': {'subclusters': pillar}}}},
        expected,
        'ensure_check_collections',
        name='sync-check-collections',
        db=db,
        prefix=prefix,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, pillar, db, prefix, expected', TEST_DATA)
def test_ensure_check_collections_test_true_make_no_changes(initial_data, pillar, db, prefix, expected):
    return test_helpers.check_some_state_test_true_make_no_changes(
        initial_data,
        {'data': {'dbaas': {'cluster': {'subclusters': pillar}}}},
        expected,
        'ensure_check_collections',
        name='sync-check-collections',
        db=db,
        prefix=prefix,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, pillar, db, prefix, expected', TEST_DATA)
def test_ensure_check_collections_no_more_pending_changes(initial_data, pillar, db, prefix, expected):
    return test_helpers.check_some_state_no_more_pending_changes(
        initial_data,
        {'data': {'dbaas': {'cluster': {'subclusters': pillar}}}},
        expected,
        'ensure_check_collections',
        name='sync-check-collections',
        db=db,
        prefix=prefix,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )
