#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import mock_mongodb
import test_helpers

TEST_DATA = [  # initial_data, roles, expected result
    (  # 1
        {'roles': []},
        {
            'mdbMonitor': {
                'database': 'admin',
                'privileges': [
                    {
                        'resource': {
                            'db': '',
                            'collection': 'system.profile',
                        },
                        'actions': ['find'],
                    },
                    {
                        'resource': {
                            'db': '',
                            'collection': '',
                        },
                        'actions': [
                            'collStats',
                            'dbStats',
                            'getShardVersion',
                            'indexStats',
                            'useUUID',
                        ],
                    },
                ],
            },
        },
        {
            'roles': [
                {
                    '_id': u'admin.mdbMonitor',
                    'role': 'mdbMonitor',
                    'db': 'admin',
                    'privileges': [
                        {
                            'resource': {'db': '', 'collection': ''},
                            'actions': [
                                'collStats',
                                'dbStats',
                                'getShardVersion',
                                'indexStats',
                                'useUUID',
                            ],
                        },
                        {'resource': {'db': '', 'collection': 'system.profile'}, 'actions': ['find']},
                    ],
                    'roles': [],
                    'authenticationRestrictions': [],
                }
            ]
        },
    ),
    (  # 2
        {
            'roles': [
                {
                    '_id': u'admin.mdbMonitor',
                    'role': 'mdbMonitor',
                    'db': 'admin',
                    'privileges': [
                        {
                            'resource': {'db': '', 'collection': ''},
                            'actions': [
                                'collStats',
                                'dbStats',
                                'getShardVersion',
                                'indexStats',
                                'useUUID',
                            ],
                        },
                        {'resource': {'db': '', 'collection': 'system.profile'}, 'actions': ['find']},
                    ],
                    'roles': [],
                    'authenticationRestrictions': [],
                },
                {
                    '_id': u'admin.mdbMonitor1',
                    'role': 'mdbMonitor1',
                    'db': 'admin',
                    'privileges': [
                        {
                            'resource': {'db': '', 'collection': ''},
                            'actions': [
                                'collStats',
                                'dbStats',
                                'getShardVersion',
                                'indexStats',
                                'useUUID',
                            ],
                        },
                    ],
                    'roles': [{'db': 'admin', 'role': 'root'}],
                    'authenticationRestrictions': [],
                },
            ]
        },
        {
            'mdbMonitor2': {
                'database': 'admin',
                'privileges': [
                    {
                        'resource': {
                            'db': '',
                            'collection': 'system.profile',
                        },
                        'actions': ['find'],
                    },
                    {
                        'resource': {
                            'db': '',
                            'collection': '',
                        },
                        'actions': [
                            'collStats',
                            'dbStats',
                            'getShardVersion',
                            'indexStats',
                            'useUUID',
                        ],
                    },
                ],
            },
            'mdbMonitor1': {
                'database': 'admin',
                'privileges': [
                    {
                        'resource': {
                            'db': '',
                            'collection': 'system.profile',
                        },
                        'actions': ['find', 'update'],
                    },
                    {
                        'resource': {
                            'db': '',
                            'collection': '',
                        },
                        'actions': [
                            'collStats',
                            'dbStats',
                            'getShardVersion',
                            'indexStats',
                            'useUUID',
                        ],
                    },
                ],
                'roles': ['root'],
            },
        },
        {
            'roles': [
                {
                    '_id': u'admin.mdbMonitor2',
                    'role': 'mdbMonitor2',
                    'db': 'admin',
                    'privileges': [
                        {
                            'resource': {'db': '', 'collection': ''},
                            'actions': [
                                'collStats',
                                'dbStats',
                                'getShardVersion',
                                'indexStats',
                                'useUUID',
                            ],
                        },
                        {'resource': {'db': '', 'collection': 'system.profile'}, 'actions': ['find']},
                    ],
                    'roles': [],
                    'authenticationRestrictions': [],
                },
                {
                    '_id': u'admin.mdbMonitor1',
                    'role': 'mdbMonitor1',
                    'db': 'admin',
                    'privileges': [
                        {
                            'resource': {'db': '', 'collection': ''},
                            'actions': [
                                'collStats',
                                'dbStats',
                                'getShardVersion',
                                'indexStats',
                                'useUUID',
                            ],
                        },
                        {'resource': {'db': '', 'collection': 'system.profile'}, 'actions': ['find', 'update']},
                    ],
                    'roles': [{'db': 'admin', 'role': 'root'}],
                    'authenticationRestrictions': [],
                },
            ]
        },
    ),
]


@pytest.mark.parametrize('initial_data, roles, expected', TEST_DATA)
def test_ensure_roles(initial_data, roles, expected):
    return test_helpers.check_some_state(
        initial_data,
        {'data': {'mongodb': {'databases': {}}}},
        expected,
        'ensure_roles',
        name='mongodb',
        roles=roles,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, roles, expected', TEST_DATA)
def test_ensure_roles_test_true_make_no_changes(initial_data, roles, expected):
    return test_helpers.check_some_state_test_true_make_no_changes(
        initial_data,
        {'data': {'mongodb': {'databases': {}}}},
        expected,
        'ensure_roles',
        name='mongodb',
        roles=roles,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, roles, expected', TEST_DATA)
def test_ensure_roles_no_more_pending_changes(initial_data, roles, expected):
    return test_helpers.check_some_state_no_more_pending_changes(
        initial_data,
        {'data': {'mongodb': {'databases': {}}}},
        expected,
        'ensure_roles',
        name='mongodb',
        roles=roles,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )
