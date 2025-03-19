#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import mock_mongodb
import test_helpers

TEST_DATA = [  # initial_data, started, expected result
    (  # 1
        {},
        False,
        {'balancer': False},
    ),
    (  # 2
        {'balancer': False},
        False,
        {'balancer': False},
    ),
    (  # 3
        {'balancer': True},
        False,
        {'balancer': False},
    ),
    (  # 4
        {},
        True,
        {},
    ),
    (  # 5
        {'balancer': True},
        True,
        {'balancer': True},
    ),
    (  # 6
        {'balancer': False},
        True,
        {'balancer': True},
    ),
]


@pytest.mark.parametrize('initial_data, started, expected', TEST_DATA)
def test_ensure_balancer_state(initial_data, started, expected):
    '''Test ensure users on some data'''
    return test_helpers.check_some_state(
        initial_data,
        None,
        expected,
        'ensure_balancer_state',
        name='mongodb',
        started=started,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, started, expected', TEST_DATA)
def test_ensure_balancer_state_test_true_make_no_changes(initial_data, started, expected):
    '''Test that with test=True, ensure users won't modify data'''
    return test_helpers.check_some_state_test_true_make_no_changes(
        initial_data,
        None,
        expected,
        'ensure_balancer_state',
        name='mongodb',
        started=started,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, started, expected', TEST_DATA)
def test_ensure_balancer_state_no_more_pending_changes(initial_data, started, expected):
    '''Test that after performing changes we won't have any pending changes anymore'''
    return test_helpers.check_some_state_no_more_pending_changes(
        initial_data,
        None,
        expected,
        'ensure_balancer_state',
        name='mongodb',
        started=started,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )
