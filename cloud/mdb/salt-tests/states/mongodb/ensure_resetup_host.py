#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import mock_mongodb
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb

TEST_DATA = [  # initial_data, resetup_id, expected_result, result, changes
    (  # 1
        {
            'resetup_id': None,
            'mongodb': {
                'is_primary': True,
                'nodes': 3,
            },
        },
        None,
        {
            'resetup_id': None,
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        True,
        {
            'resetup': True,
            'stepdown': True,
        },
    ),
    (  # 2
        {
            'resetup_id': None,
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        None,
        {
            'resetup_id': None,
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        True,
        {
            'resetup': True,
        },
    ),
    (  # 3
        {
            'resetup_id': None,
            'mongodb': {
                'is_primary': True,
                'nodes': 1,
            },
        },
        None,
        {
            'resetup_id': None,
            'mongodb': {
                'is_primary': True,
                'nodes': 1,
            },
        },
        False,
        {},
    ),
    (  # 4
        {
            'resetup_id': None,
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        '123',
        {
            'resetup_id': '123',
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        True,
        {
            'resetup': True,
        },
    ),
    (  # 5
        {
            'resetup_id': '123',
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        '123',
        {
            'resetup_id': '123',
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        True,
        {},
    ),
    (  # 6
        {
            'resetup_id': '123',
            'mongodb': {
                'is_primary': True,
                'nodes': 3,
            },
        },
        '123',
        {
            'resetup_id': '123',
            'mongodb': {
                'is_primary': True,
                'nodes': 3,
            },
        },
        True,
        {},
    ),
    (  # 7
        {
            'resetup_id': '123',
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        '1234',
        {
            'resetup_id': '1234',
            'mongodb': {
                'is_primary': False,
                'nodes': 3,
            },
        },
        True,
        {
            'resetup': True,
        },
    ),
    (  # 8
        {
            'resetup_id': None,
            'mongodb': {
                'is_primary': True,
                'nodes': 2,
            },
        },
        None,
        {
            'resetup_id': None,
            'mongodb': {
                'is_primary': False,
                'nodes': 2,
            },
        },
        True,
        {
            'resetup': True,
            'stepdown': True,
        },
    ),
]


@pytest.mark.parametrize('initial_data, resetup_id, expected, result, changes', TEST_DATA)
def test_ensure_resetup_host(initial_data, resetup_id, expected, result, changes):
    mocked_salt = mock_mongodb.MongoSaltMock(initial_data)
    s_mongodb.__salt__ = mocked_salt
    s_mongodb.__opts__ = {'test': False}

    # Perform changes
    ret = s_mongodb.ensure_resetup_host(
        name='resetup', resetup_id=resetup_id, **mock_mongodb.MongoSaltMock.get_auth_data()
    )

    assert result == ret['result'], ret
    assert changes == ret['changes'], initial_data
    mocked_salt.assert_data_is_equal(expected)
