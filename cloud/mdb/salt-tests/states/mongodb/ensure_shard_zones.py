#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import mock_mongodb
import test_helpers

TEST_DATA = [  # initial_data, service, pillar, expected result
    (  # 1
        {'shards': {'rs01': []}},
        {
            'id1': {
                'roles': ['mongodb_cluster.mongod'],
                'shards': {
                    'i1': {'name': 'rs01'},
                },
            },
        },
        {'shards': {'rs01': ['rs01']}},
    ),
    (  # 2
        {'shards': {}},
        {
            'id1': {
                'roles': ['mongodb_cluster.mongod'],
                'shards': {
                    'i1': {'name': 'rs01'},
                },
            },
        },
        {'shards': {}},
    ),
    (  # 3
        {'shards': {'rs01': []}},
        {
            'id1': {
                'roles': ['mongodb_cluster.mongod'],
                'shards': {
                    'i1': {'name': 'rs01'},
                },
            },
            'id2': {
                'roles': ['mongodb_cluster.mongos'],
                'shards': {
                    'i1': {'name': 'rs02'},
                },
            },
        },
        {'shards': {'rs01': ['rs01']}},
    ),
    (  # 4
        {'shards': {'rs01': [], 'rs02': ['1', '2']}},
        {
            'id1': {
                'roles': ['mongodb_cluster.mongod'],
                'shards': {
                    'i1': {'name': 'rs01'},
                },
            },
        },
        {'shards': {'rs01': ['rs01'], 'rs02': ['1', '2']}},
    ),
    (  # 5
        {'shards': {'rs01': ['x']}},
        {
            'id1': {
                'roles': ['mongodb_cluster.mongod'],
                'shards': {
                    'i1': {'name': 'rs01'},
                },
            },
        },
        {'shards': {'rs01': ['rs01', 'x']}},
    ),
    (  # 6
        {'shards': {'rs01': ['x']}},
        {
            'id1': {
                'roles': ['mongodb_cluster.mongod'],
                'shards': {
                    'i1': {'name': 'rs01', 'tags': ['rs01', 'x2']},
                },
            },
        },
        {'shards': {'rs01': ['rs01', 'x2', 'x']}},
    ),
    (  # 7
        {'shards': {'rs01': ['rs01', 'x'], 'rs02': ['rs01']}},
        {
            'id1': {
                'roles': ['mongodb_cluster.mongod'],
                'shards': {
                    'i1': {'name': 'rs01'},
                    'i2': {'name': 'rs02'},
                },
            },
        },
        {'shards': {'rs01': ['rs01', 'x'], 'rs02': ['rs02']}},
    ),
]


@pytest.mark.parametrize('initial_data, pillar, expected', TEST_DATA)
def test_ensure_shard_zones(initial_data, pillar, expected):
    return test_helpers.check_some_state(
        initial_data,
        {'data': {'dbaas': {'cluster': {'subclusters': pillar}}}},
        expected,
        'ensure_shard_zones',
        name='sync-shard-zones',
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, pillar, expected', TEST_DATA)
def test_ensure_shard_zones_test_true_make_no_changes(initial_data, pillar, expected):
    return test_helpers.check_some_state_test_true_make_no_changes(
        initial_data,
        {'data': {'dbaas': {'cluster': {'subclusters': pillar}}}},
        expected,
        'ensure_shard_zones',
        name='sync-shard-zones',
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, pillar, expected', TEST_DATA)
def test_shard_zones_no_more_pending_changes(initial_data, pillar, expected):
    return test_helpers.check_some_state_no_more_pending_changes(
        initial_data,
        {'data': {'dbaas': {'cluster': {'subclusters': pillar}}}},
        expected,
        'ensure_shard_zones',
        name='sync-shard-zones',
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )
