#!/usr/bin/python
# coding: utf-8

from __future__ import print_function, unicode_literals
import pytest
import mock_mongodb
import test_helpers

TEST_DATA = [  # initial_data, service, pillar, expected result
    (  # 1
        {'users': {}},
        'mongod',
        {
            'admin': {
                'password': 'admin_password',
                'internal': True,
                'dbs': {'admin': ['root', 'dbOwner'], 'local': ['dbOwner'], 'config': ['dbOwner']},
                'services': ['mongod', 'mongocfg', 'mongos'],
            }
        },
        {
            'users': {
                'admin': {
                    'dbs': {'admin': ['dbOwner', 'root'], 'config': ['dbOwner'], 'local': ['dbOwner']},
                    'authdb': ['admin'],
                    'password': 'admin_password',
                }
            }
        },
    ),
    (  # 2
        {
            'users': {
                'admin': {
                    'dbs': {'admin': ['dbOwner', 'root'], 'config': ['dbOwner'], 'local': ['dbOwner']},
                    'authdb': ['admin'],
                    'password': 'admin_password',
                }
            }
        },
        'mongod',
        {
            'admin': {
                'password': 'admin_password',
                'internal': True,
                'dbs': {'admin': ['root', 'dbOwner'], 'local': ['dbOwner'], 'config': ['dbOwner']},
                'services': ['mongod', 'mongocfg', 'mongos'],
            },
            'monitor1': {
                'password': 'monitor_password',
                'internal': True,
                'dbs': {
                    'admin': ['mdbMonitor'],
                },
                'services': ['mongod'],
            },
        },
        {
            'users': {
                'admin': {
                    'dbs': {'admin': ['dbOwner', 'root'], 'config': ['dbOwner'], 'local': ['dbOwner']},
                    'authdb': ['admin'],
                    'password': 'admin_password',
                },
                'monitor1': {'dbs': {'admin': ['mdbMonitor']}, 'authdb': ['admin'], 'password': 'monitor_password'},
            }
        },
    ),
    (  # 3
        {
            'users': {
                'admin': {
                    'dbs': {'admin': ['dbOwner', 'root'], 'config': ['dbOwner'], 'local': ['dbOwner']},
                    'authdb': ['admin'],
                    'password': 'admin_password',
                },
                'monitor1': {'dbs': {'admin': ['mdbMonitor']}, 'authdb': ['admin'], 'password': 'monitor_password'},
            }
        },
        'mongod',
        {
            'admin': {
                'password': '123',
                'internal': True,
                'dbs': {'admin': ['root', 'dbOwner'], 'local': ['dbOwner'], 'config': ['dbOwner']},
                'services': ['mongod', 'mongocfg', 'mongos'],
            },
            'monitor': {
                'password': 'monitor_password',
                'internal': True,
                'dbs': {
                    'admin': ['mdbMonitor'],
                },
                'services': ['mongod', 'mongos', 'mongocfg'],
            },
            'user1': {
                'password': '1234',
                'internal': False,
                'dbs': {'userdb': ['readWrite'], 'userdb2': ['read'], 'admin': ['mdbMonitor']},
                'services': ['mongod', 'mongos'],
            },
            'fake_user': {
                'password': '1234',
                'internal': False,
                'dbs': {'admin': ['mdbMonitor']},
                'services': ['mongocfg'],
            },
        },
        {
            'users': {
                'admin': {
                    'dbs': {'admin': ['dbOwner', 'root'], 'config': ['dbOwner'], 'local': ['dbOwner']},
                    'authdb': ['admin'],
                    'password': '123',
                },
                'monitor': {'dbs': {'admin': ['mdbMonitor']}, 'authdb': ['admin'], 'password': 'monitor_password'},
                'user1': {
                    'password': '1234',
                    'dbs': {'userdb': ['readWrite'], 'userdb2': ['read'], 'admin': ['mdbMonitor']},
                    'authdb': ['admin', 'userdb', 'userdb2'],
                },
            }
        },
    ),
    (  # 4
        {
            'users': {
                'admin': {
                    'dbs': {'admin': ['dbOwner', 'root'], 'config': ['dbOwner'], 'local': ['dbOwner']},
                    'authdb': ['admin'],
                    'password': '123',
                },
                'monitor': {'dbs': {'admin': ['mdbMonitor']}, 'authdb': ['admin'], 'password': 'monitor_password'},
                'user1': {
                    'password': '1234',
                    'dbs': {'userdb': ['readWrite'], 'userdb2': ['read'], 'admin': ['mdbMonitor']},
                    'authdb': ['admin', 'userdb', 'userdb2'],
                },
            }
        },
        'mongod',
        {
            'admin': {
                'password': '123',
                'internal': True,
                'dbs': {'admin': ['root'], 'local': ['dbOwner'], 'config': ['dbOwner']},
                'services': ['mongod', 'mongocfg', 'mongos'],
            },
            'monitor': {
                'password': 'monitor_password',
                'internal': True,
                'dbs': {
                    'admin': ['mdbMonitor'],
                },
                'services': ['mongod', 'mongos', 'mongocfg'],
            },
            'user1': {
                'password': '1234',
                'internal': False,
                'dbs': {'userdb': ['readWrite'], 'admin': ['mdbMonitor']},
                'services': ['mongod', 'mongos'],
            },
        },
        {
            'users': {
                'admin': {
                    'dbs': {'admin': ['root'], 'config': ['dbOwner'], 'local': ['dbOwner']},
                    'authdb': ['admin'],
                    'password': '123',
                },
                'monitor': {'dbs': {'admin': ['mdbMonitor']}, 'authdb': ['admin'], 'password': 'monitor_password'},
                'user1': {
                    'password': '1234',
                    'dbs': {'userdb': ['readWrite'], 'admin': ['mdbMonitor']},
                    'authdb': ['admin', 'userdb'],
                },
            }
        },
    ),
    (  # 5
        {
            'users': {
                'admin': {
                    'dbs': {'admin': ['dbOwner', 'root'], 'config': ['dbOwner'], 'local': ['dbOwner']},
                    'authdb': ['admin'],
                    'password': '123',
                },
                'monitor': {'dbs': {'admin': ['mdbMonitor']}, 'authdb': ['admin'], 'password': 'monitor_password'},
                'user1': {
                    'password': '1234',
                    'dbs': {'userdb': ['readWrite'], 'userdb2': ['read'], 'admin': ['mdbMonitor']},
                    'authdb': ['admin', 'userdb', 'userdb2'],
                },
            }
        },
        'mongod',
        {
            'admin': {
                'password': '123',
                'internal': True,
                'dbs': {'admin': ['root'], 'local': ['dbOwner'], 'config': ['dbOwner']},
                'services': ['mongod', 'mongocfg', 'mongos'],
            },
            'monitor': {
                'password': 'monitor_password',
                'internal': True,
                'dbs': {
                    'admin': ['mdbMonitor'],
                },
                'services': ['mongod', 'mongos', 'mongocfg'],
            },
            'user1': {
                'password': '1234',
                'internal': False,
                'dbs': {'userdb': ['readWrite'], 'admin': ['mdbMonitor']},
                'services': ['mongod', 'mongos'],
            },
            'noop': {
                'password': '123',
                'internal': False,
                'services': ['mongod', 'mongos'],
            },
            'noop2': {
                'password': '123',
                'internal': False,
                'dbs': {},
                'services': ['mongod', 'mongos'],
            },
        },
        {
            'users': {
                'admin': {
                    'dbs': {'admin': ['root'], 'config': ['dbOwner'], 'local': ['dbOwner']},
                    'authdb': ['admin'],
                    'password': '123',
                },
                'monitor': {'dbs': {'admin': ['mdbMonitor']}, 'authdb': ['admin'], 'password': 'monitor_password'},
                'user1': {
                    'password': '1234',
                    'dbs': {'userdb': ['readWrite'], 'admin': ['mdbMonitor']},
                    'authdb': ['admin', 'userdb'],
                },
            }
        },
    ),
    (  # 6
        {'users': {}},
        'mongod',
        {
            'admin': {
                'password': 'password-password-password-пароль',
                'internal': True,
                'dbs': {'admin': ['root', 'dbOwner'], 'local': ['dbOwner'], 'config': ['dbOwner']},
                'services': ['mongod', 'mongocfg', 'mongos'],
            }
        },
        {
            'users': {
                'admin': {
                    'dbs': {'admin': ['dbOwner', 'root'], 'config': ['dbOwner'], 'local': ['dbOwner']},
                    'authdb': ['admin'],
                    'password': 'password-password-password-пароль',
                }
            }
        },
    ),
]


@pytest.mark.parametrize('initial_data, service, pillar, expected', TEST_DATA)
def test_ensure_users(initial_data, service, pillar, expected):
    return test_helpers.check_some_state(
        initial_data,
        {'data': {'mongodb': {'users': pillar}}},
        expected,
        'ensure_users',
        name='mongodb',
        service=service,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, service, pillar, expected', TEST_DATA)
def test_ensure_users_test_true_make_no_changes(initial_data, service, pillar, expected):
    return test_helpers.check_some_state_test_true_make_no_changes(
        initial_data,
        {'data': {'mongodb': {'users': pillar}}},
        expected,
        'ensure_users',
        name='mongodb',
        service=service,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )


@pytest.mark.parametrize('initial_data, service, pillar, expected', TEST_DATA)
def test_ensure_users_no_more_pending_changes(initial_data, service, pillar, expected):
    return test_helpers.check_some_state_no_more_pending_changes(
        initial_data,
        {'data': {'mongodb': {'users': pillar}}},
        expected,
        'ensure_users',
        name='mongodb',
        service=service,
        **mock_mongodb.MongoSaltMock.get_auth_data()
    )
