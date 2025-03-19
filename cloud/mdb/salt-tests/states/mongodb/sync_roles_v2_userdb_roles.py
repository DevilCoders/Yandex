#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import mock_mongodb
import test_helpers

TEST_DATA = [  # initial_data, databases, roles, userdb_roles, expected result
    (  # 1
        {'roles': []},
        {},
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
        {},
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
        {'roles': []},
        {'db1': {}},
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
        {},
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
    (  # 3
        {'roles': []},
        {},
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
            'mdbDbAdmin': {
                'database': '{db}',
                'privileges': [
                    {
                        'resource': {
                            'db': '{db}',
                            'collection': '',
                        },
                        'actions': [
                            'collMod',
                            'planCacheWrite',
                            'planCacheRead',
                            'bypassDocumentValidation',
                        ],
                    },
                ],
                'roles': [
                    {'role': 'readWrite', 'db': '{db}'},
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
    (  # 4
        {'roles': []},
        {'db1': {}},
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
            'mdbDbAdmin': {
                'database': '{db}',
                'privileges': [
                    {
                        'resource': {
                            'db': '{db}',
                            'collection': '',
                        },
                        'actions': [
                            'collMod',
                            'planCacheWrite',
                            'planCacheRead',
                            'bypassDocumentValidation',
                        ],
                    },
                ],
                'roles': [
                    {'role': 'readWrite', 'db': '{db}'},
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
                },
                {
                    '_id': u'db1.mdbDbAdmin',
                    'role': 'mdbDbAdmin',
                    'db': 'db1',
                    'privileges': [
                        {
                            'resource': {'db': 'db1', 'collection': ''},
                            'actions': [
                                'collMod',
                                'planCacheWrite',
                                'planCacheRead',
                                'bypassDocumentValidation',
                            ],
                        },
                    ],
                    'roles': [
                        {'role': 'readWrite', 'db': 'db1'},
                    ],
                    'authenticationRestrictions': [],
                },
            ]
        },
    ),
]


@pytest.mark.parametrize('initial_data, databases, roles, userdb_roles, expected', TEST_DATA)
def test_ensure_roles_v2(initial_data, databases, roles, userdb_roles, expected):
    return test_helpers.check_some_state(
        initial_data,
        {'data': {'mongodb': {'databases': databases}}},
        expected,
        'ensure_roles',
        name='mongodb',
        roles=roles,
        userdb_roles=userdb_roles,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, databases, roles, userdb_roles, expected', TEST_DATA)
def test_ensure_roles_test_true_make_no_changes_v2(
    initial_data,
    databases,
    roles,
    userdb_roles,
    expected,
):
    return test_helpers.check_some_state_test_true_make_no_changes(
        initial_data,
        {'data': {'mongodb': {'databases': databases}}},
        expected,
        'ensure_roles',
        name='mongodb',
        roles=roles,
        userdb_roles=userdb_roles,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, databases, roles, userdb_roles, expected', TEST_DATA)
def test_ensure_roles_no_more_pending_changes_v2(
    initial_data,
    databases,
    roles,
    userdb_roles,
    expected,
):
    return test_helpers.check_some_state_no_more_pending_changes(
        initial_data,
        {'data': {'mongodb': {'databases': databases}}},
        expected,
        'ensure_roles',
        name='mongodb',
        roles=roles,
        userdb_roles=userdb_roles,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )
