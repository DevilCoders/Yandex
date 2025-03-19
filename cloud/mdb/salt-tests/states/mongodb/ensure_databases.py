#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import mock_mongodb
import test_helpers
import cloud.mdb.salt.salt._states.mdb_mongodb as s_mongodb

TEST_DATA = [  # initial_data, pillar, target-database, expected result
    (  # 1
        {
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
        {'x1': None, 'x2': None},
        None,
        {
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
    ),
    (  # 2
        {
            'dbs': {
                'mdb_internal': {
                    'sharded': True,
                    'collections': {},
                },
                'db1': {
                    'sharded': False,
                    'collections': {},
                },
                'db2': {
                    'sharded': False,
                    'collections': {},
                },
            },
        },
        {
            'db1': None,
        },
        'db2',
        {
            'dbs': {
                'mdb_internal': {
                    'sharded': True,
                    'collections': {},
                },
                'db1': {
                    'sharded': False,
                    'collections': {},
                },
            },
        },
    ),
    (  # 3
        {
            'dbs': {
                'mdb_internal': {
                    'sharded': True,
                    'collections': {},
                },
                'db1': {
                    'sharded': False,
                    'collections': {},
                },
            },
        },
        {},
        'db1',
        {
            'dbs': {
                'mdb_internal': {
                    'sharded': True,
                    'collections': {},
                },
            },
        },
    ),
]


@pytest.mark.parametrize('initial_data, pillar, target_database, expected', TEST_DATA)
def test_ensure_databases(initial_data, pillar, target_database, expected):
    return test_helpers.check_some_state(
        initial_data,
        {'data': {'mongodb': {'databases': pillar}}, 'target-database': target_database},
        expected,
        'ensure_databases',
        name='sync-databases',
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, pillar, target_database, expected', TEST_DATA)
def test_ensure_databases_test_true_make_no_changes(initial_data, pillar, target_database, expected):
    return test_helpers.check_some_state_test_true_make_no_changes(
        initial_data,
        {'data': {'mongodb': {'databases': pillar}}, 'target-database': target_database},
        expected,
        'ensure_databases',
        name='sync-databases',
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, pillar, target_database, expected', TEST_DATA)
def test_ensure_databases_no_more_pending_changes(initial_data, pillar, target_database, expected):
    return test_helpers.check_some_state_no_more_pending_changes(
        initial_data,
        {'data': {'mongodb': {'databases': pillar}}, 'target-database': target_database},
        expected,
        'ensure_databases',
        name='sync-databases',
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, pillar, target_database, expected', TEST_DATA)
def test_ensure_databases_Fails_on_empty_pillar_and_more_than_one_db(
    initial_data,
    pillar,
    target_database,
    expected,
):
    mocked_salt = mock_mongodb.MongoSaltMock(initial_data)
    s_mongodb.__salt__ = mocked_salt
    if pillar is not None:
        mocked_salt.pillar_set({'data': {'mongodb': {'databases': {}}}, 'target-database': target_database})
    s_mongodb.__opts__ = {'test': False}

    # Perform changes
    ret = s_mongodb.ensure_databases(name='sync-databases', **mock_mongodb.MongoSaltMock.get_auth_data())

    expected_result = len([db for db in initial_data['dbs'].keys() if db not in s_mongodb.INTERNAL_DBS]) == 1
    assert ret['result'] == expected_result, 'ret[result] is not {}: {}'.format(
        expected_result,
        ret,
    )
    if expected_result:
        mocked_salt.assert_data_is_equal(expected)
    else:
        mocked_salt.assert_data_is_equal(initial_data)


@pytest.mark.parametrize('initial_data, pillar, target_database, expected', TEST_DATA)
def test_ensure_databases_Fails_on_empty_mongodb(initial_data, pillar, target_database, expected):
    mocked_salt = mock_mongodb.MongoSaltMock({})
    s_mongodb.__salt__ = mocked_salt
    if pillar is not None:
        mocked_salt.pillar_set({'data': {'mongodb': {'databases': pillar}}, 'target-database': target_database})
    s_mongodb.__opts__ = {'test': False}

    # Perform changes
    ret = s_mongodb.ensure_databases(name='sync-databases', **mock_mongodb.MongoSaltMock.get_auth_data())

    assert ret['result'] is False, 'ret[result] is not false: {}'.format(ret)
    mocked_salt.assert_data_is_equal({})


@pytest.mark.parametrize('initial_data, pillar, target_database, expected', TEST_DATA)
def test_ensure_databases_Fails_to_delete_db_if_target_is_empty(
    initial_data,
    pillar,
    target_database,
    expected,
):
    if target_database is None:
        # Nothing to check if we aren't trying to delete anything
        return

    mocked_salt = mock_mongodb.MongoSaltMock(initial_data)
    s_mongodb.__salt__ = mocked_salt
    if pillar is not None:
        mocked_salt.pillar_set({'data': {'mongodb': {'databases': {}}}})
    s_mongodb.__opts__ = {'test': False}

    # Perform changes
    ret = s_mongodb.ensure_databases(name='sync-databases', **mock_mongodb.MongoSaltMock.get_auth_data())

    assert ret['result'] is False, 'ret[result] is not False'
    mocked_salt.assert_data_is_equal(initial_data)
