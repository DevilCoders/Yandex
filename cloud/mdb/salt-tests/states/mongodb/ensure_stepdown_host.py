#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import mock_mongodb
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb

TEST_DATA = [  # initial_data, stepdown_id, expected_result, result, changes
    (  # 1
        {
            'stepdown_id': None,
            'mongodb': {
                'is_primary': True,
                'nodes': 3,
            },
        },
        None,
        {
            'stepdown_id': None,
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        True,
        {
            'stepdown': True,
        },
    ),
    (  # 2
        {
            'stepdown_id': None,
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        None,
        {
            'stepdown_id': None,
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        True,
        {},
    ),
    (  # 3
        {
            'stepdown_id': None,
            'mongodb': {
                'is_primary': True,
                'nodes': 1,
            },
        },
        None,
        {
            'stepdown_id': None,
            'mongodb': {
                'is_primary': True,
                'nodes': 1,
            },
        },
        True,
        {},
    ),
    (  # 4
        {
            'stepdown_id': None,
            'mongodb': {
                'is_primary': True,
                'nodes': 3,
            },
        },
        '123',
        {
            'stepdown_id': '123',
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        True,
        {
            'stepdown': True,
        },
    ),
    (  # 5
        {
            'stepdown_id': '123',
            'mongodb': {
                'is_primary': True,
                'nodes': 3,
            },
        },
        '123',
        {
            'stepdown_id': '123',
            'mongodb': {
                'is_primary': True,
                'nodes': 3,
            },
        },
        True,
        {},
    ),
    (  # 6
        {
            'stepdown_id': None,
            'mongodb': {
                'is_primary': True,
                'nodes': 1,
            },
        },
        '123',
        {
            'stepdown_id': None,
            'mongodb': {
                'is_primary': True,
                'nodes': 1,
            },
        },
        True,
        {},
    ),
]


@pytest.mark.parametrize('initial_data, stepdown_id, expected, result, changes', TEST_DATA)
def test_ensure_stepdown_host(initial_data, stepdown_id, expected, result, changes):
    mocked_salt = mock_mongodb.MongoSaltMock(initial_data)
    s_mongodb.__salt__ = mocked_salt
    s_mongodb.__opts__ = {'test': False}

    # Perform changes
    ret = s_mongodb.ensure_stepdown_host(
        name='stepdown', stepdown_id=stepdown_id, **mock_mongodb.MongoSaltMock.get_auth_data()
    )

    assert result == ret['result'], ret
    assert changes == ret['changes'], initial_data
    mocked_salt.assert_data_is_equal(expected)
