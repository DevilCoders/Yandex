#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import mock_mongodb
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb

TEST_DATA = [  # initial_data, host, master, expected_result, result
    (  # 1
        {
            'rs_hosts': ['vla'],
        },
        'man',
        'vla',
        {
            'rs_hosts': ['vla', 'man'],
        },
        True,
    ),
    (  # 2
        {
            'rs_hosts': ['vla', 'man'],
        },
        'man',
        'vla',
        {
            'rs_hosts': ['vla', 'man'],
        },
        True,
    ),
    (  # 3
        {
            'rs_hosts': ['vla'],
        },
        'man',
        None,
        {
            'rs_hosts': ['vla'],
        },
        False,
    ),
    (  # 2
        {
            'rs_hosts': ['vla', 'man'],
        },
        'man',
        None,
        {
            'rs_hosts': ['vla', 'man'],
        },
        True,
    ),
    (  # 2
        {
            'rs_hosts': ['vla', 'man'],
        },
        'man',
        'sas',
        {
            'rs_hosts': ['vla', 'man'],
        },
        False,
    ),
]


@pytest.mark.parametrize('initial_data, host, master, expected, result', TEST_DATA)
def test_ensure_stepdown_host(initial_data, host, master, expected, result):
    mocked_salt = mock_mongodb.MongoSaltMock(initial_data)
    s_mongodb.__salt__ = mocked_salt
    s_mongodb.__opts__ = {'test': False}

    # Perform changes
    ret = s_mongodb.ensure_member_in_replicaset(
        name='rs-add', master_hostname=master, **mock_mongodb.MongoSaltMock.get_auth_data(host=host)
    )

    assert result == ret['result'], ret
    mocked_salt.assert_data_is_equal(expected)
