# -*- coding: utf-8 -*-

from __future__ import unicode_literals

from mock import call, MagicMock

from cloud.mdb.salt.salt._modules.mdb_clickhouse import (
    admin_grants,
    grant_except_databases,
    grant,
    version_cmp,
    __salt__,
)
from cloud.mdb.salt.salt._states import mdb_clickhouse
from cloud.mdb.internal.python.pytest.utils import parametrize
from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_version_cmp


@parametrize(
    {
        'id': 'User not exists',
        'args': {
            'args': ['test_user', 'password_hash', None],
            'result': {'user_created': 'test_user'},
            'existing_users': [],
            'calls': [call('test_user', 'password_hash', None)],
            'opts': {'test': False},
        },
    },
    {
        'id': 'User not exists(dry-run)',
        'args': {
            'args': ['test_user', 'password_hash', None],
            'result': {'user_created': 'test_user'},
            'existing_users': [],
            'calls': [],
            'opts': {'test': True},
        },
    },
    {
        'id': 'User exists',
        'args': {
            'args': ['test_user', 'password_hash', None],
            'result': {},
            'existing_users': ['test_user'],
            'calls': [],
            'opts': {'test': False},
        },
    },
)
def test_ensure_user_exist(args, result, existing_users, calls, opts):
    mdb_clickhouse.__opts__ = opts
    mdb_clickhouse.__salt__['mdb_clickhouse.create_user'] = MagicMock()
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_user_names'] = lambda: existing_users
    assert mdb_clickhouse._ensure_user_exist(*args) == result
    assert mdb_clickhouse.__salt__['mdb_clickhouse.create_user'].call_args_list == calls


@parametrize(
    {
        'id': 'Nothing changed',
        'args': {
            'args': ['test_user', 'password_hash'],
            'result': {},
            'existing_hash': "password_hash",
            'calls': [],
            'opts': {'test': False},
        },
    },
    {
        'id': 'Password changed',
        'args': {
            'args': ['test_user', 'password_hash'],
            'result': {'user_password_changed': 'test_user'},
            'existing_hash': "another_hash",
            'calls': [call('test_user', 'password_hash')],
            'opts': {'test': False},
        },
    },
    {
        'id': 'Password changed(dry-run)',
        'args': {
            'args': ['test_user', 'password_hash'],
            'result': {'user_password_changed': 'test_user'},
            'existing_hash': "another_hash",
            'calls': [],
            'opts': {'test': True},
        },
    },
)
def test_ensure_user_password(args, result, existing_hash, calls, opts):
    mdb_clickhouse.__opts__ = opts
    mdb_clickhouse.__salt__['mdb_clickhouse.change_user_password'] = MagicMock()
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_user_password_hash'] = lambda _: existing_hash
    assert mdb_clickhouse._ensure_user_password(*args) == result
    assert mdb_clickhouse.__salt__['mdb_clickhouse.change_user_password'].call_args_list == calls


@parametrize(
    {
        'id': 'Nothing changed for regular user',
        'args': {
            'args': ['test_user', None],
            'result': {},
            'existing_grantees': [['ANY'], []],
            'calls': [],
            'opts': {'test': False},
        },
    },
    {
        'id': 'Invalid grantees for regular user',
        'args': {
            'args': ['test_user', None],
            'result': {'grantees_created': 'ALTER USER test_user GRANTEES ANY'},
            'existing_grantees': [['ANY'], ['test_user']],
            'calls': [call('ALTER USER test_user GRANTEES ANY')],
            'opts': {'test': False},
        },
    },
    {
        'id': 'Invalid grantees for regular user(dry-run)',
        'args': {
            'args': ['test_user', None],
            'result': {'grantees_created': 'ALTER USER test_user GRANTEES ANY'},
            'existing_grantees': [['ANY'], ['test_user']],
            'calls': [],
            'opts': {'test': True},
        },
    },
    {
        'id': 'Nothing changed for admin',
        'args': {
            'args': ['admin', [['ANY'], ['admin']]],
            'result': {},
            'existing_grantees': [['ANY'], ['admin']],
            'calls': [],
            'opts': {'test': False},
        },
    },
    {
        'id': 'Invalid grantees for admin',
        'args': {
            'args': ['admin', [['ANY'], ['admin']]],
            'result': {'grantees_created': 'ALTER USER admin GRANTEES ANY EXCEPT admin'},
            'existing_grantees': [['ANY'], []],
            'calls': [call('ALTER USER admin GRANTEES ANY EXCEPT admin')],
            'opts': {'test': False},
        },
    },
    {
        'id': 'Invalid grantees for admin(dry-run)',
        'args': {
            'args': ['admin', [['ANY'], ['admin']]],
            'result': {'grantees_created': 'ALTER USER admin GRANTEES ANY EXCEPT admin'},
            'existing_grantees': [['ANY'], []],
            'calls': [],
            'opts': {'test': True},
        },
    },
)
def test_ensure_user_grantees(args, result, existing_grantees, calls, opts):
    mdb_clickhouse.__opts__ = opts
    mdb_clickhouse.__salt__['mdb_clickhouse.execute'] = MagicMock()
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_user_grantees'] = lambda _: existing_grantees
    assert mdb_clickhouse._ensure_user_grantees(*args) == result
    assert mdb_clickhouse.__salt__['mdb_clickhouse.execute'].call_args_list == calls


@parametrize(
    {
        'id': 'Nothing changed',
        'args': {
            'opts': {'test': False},
            'target_grants': [
                grant('SELECT'),
                grant('INSERT', grant_option=True),
                grant('CREATE', ['db1']),
                grant_except_databases('ALTER', ['db2']),
            ],
            'existing_grants': {
                'SELECT': [{'access_type': 'SELECT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0}],
                'INSERT': [{'access_type': 'INSERT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 1}],
                'CREATE': [{'access_type': 'CREATE', 'database': 'db1', 'is_partial_revoke': 0, 'grant_option': 0}],
                'ALTER': [
                    {'access_type': 'ALTER', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'ALTER', 'database': 'db2', 'is_partial_revoke': 1, 'grant_option': 0},
                ],
            },
            'calls': {},
            'result': {},
        },
    },
    {
        'id': 'Nothing changed (dry-run)',
        'args': {
            'opts': {'test': True},
            'target_grants': [
                grant('SELECT'),
                grant('INSERT', grant_option=True),
                grant('CREATE', ['db1']),
                grant_except_databases('ALTER', ['db2']),
            ],
            'existing_grants': {
                'SELECT': [{'access_type': 'SELECT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0}],
                'INSERT': [{'access_type': 'INSERT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 1}],
                'CREATE': [{'access_type': 'CREATE', 'database': 'db1', 'is_partial_revoke': 0, 'grant_option': 0}],
                'ALTER': [
                    {'access_type': 'ALTER', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'ALTER', 'database': 'db2', 'is_partial_revoke': 1, 'grant_option': 0},
                ],
            },
            'calls': {},
            'result': {},
        },
    },
    {
        'id': 'Add new user',
        'args': {
            'opts': {'test': False},
            'target_grants': [
                grant('SELECT'),
                grant('INSERT', grant_option=True),
                grant('CREATE', ['db1']),
                grant_except_databases('ALTER', ['db2']),
            ],
            'existing_grants': {},
            'calls': {
                'execute': [
                    call("GRANT SELECT ON *.* TO 'test_user';"),
                    call("GRANT INSERT ON *.* TO 'test_user' WITH GRANT OPTION;"),
                    call("REVOKE CREATE ON *.* FROM 'test_user';"),
                    call("GRANT CREATE ON `db1`.* TO 'test_user';"),
                    call("GRANT ALTER ON *.* TO 'test_user';"),
                    call("REVOKE ALTER ON `db2`.* FROM 'test_user';"),
                ],
            },
            'result': {
                'grants_changed': {
                    'SELECT': {
                        'reason': 'not exists',
                        'actions': ["GRANT SELECT ON *.* TO 'test_user';"],
                    },
                    'INSERT': {
                        'reason': 'not exists',
                        'actions': ["GRANT INSERT ON *.* TO 'test_user' WITH GRANT OPTION;"],
                    },
                    'CREATE': {
                        'reason': 'not exists',
                        'actions': [
                            "REVOKE CREATE ON *.* FROM 'test_user';",
                            "GRANT CREATE ON `db1`.* TO 'test_user';",
                        ],
                    },
                    'ALTER': {
                        'reason': 'not exists',
                        'actions': [
                            "GRANT ALTER ON *.* TO 'test_user';",
                            "REVOKE ALTER ON `db2`.* FROM 'test_user';",
                        ],
                    },
                }
            },
        },
    },
    {
        'id': 'Add new user (dry-run)',
        'args': {
            'opts': {'test': True},
            'target_grants': [
                grant('SELECT'),
                grant('INSERT', grant_option=True),
                grant('CREATE', ['db1']),
                grant_except_databases('ALTER', ['db2']),
            ],
            'existing_grants': {},
            'calls': {
                'execute': [],
            },
            'result': {
                'grants_changed': {
                    'SELECT': {
                        'reason': 'not exists',
                        'actions': ["GRANT SELECT ON *.* TO 'test_user';"],
                    },
                    'INSERT': {
                        'reason': 'not exists',
                        'actions': ["GRANT INSERT ON *.* TO 'test_user' WITH GRANT OPTION;"],
                    },
                    'CREATE': {
                        'reason': 'not exists',
                        'actions': [
                            "REVOKE CREATE ON *.* FROM 'test_user';",
                            "GRANT CREATE ON `db1`.* TO 'test_user';",
                        ],
                    },
                    'ALTER': {
                        'reason': 'not exists',
                        'actions': [
                            "GRANT ALTER ON *.* TO 'test_user';",
                            "REVOKE ALTER ON `db2`.* FROM 'test_user';",
                        ],
                    },
                }
            },
        },
    },
    {
        'id': 'Grant missing permissions',
        'args': {
            'opts': {'test': False},
            'target_grants': [
                grant('SELECT'),
                grant('INSERT', grant_option=True),
                grant('CREATE', ['db1', 'db2']),
                grant_except_databases('ALTER', ['db3']),
                grant('DROP'),
            ],
            'existing_grants': {
                'SELECT': [{'access_type': 'SELECT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0}],
                'CREATE': [{'access_type': 'CREATE', 'database': 'db1', 'is_partial_revoke': 0, 'grant_option': 0}],
                'ALTER': [
                    {'access_type': 'ALTER', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'ALTER', 'database': 'db3', 'is_partial_revoke': 1, 'grant_option': 0},
                    {'access_type': 'ALTER', 'database': 'db4', 'is_partial_revoke': 1, 'grant_option': 0},
                ],
                'DROP': [
                    {'access_type': 'DROP', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'DROP', 'database': 'db5', 'is_partial_revoke': 1, 'grant_option': 0},
                ],
            },
            'calls': {
                'execute': [
                    call("GRANT INSERT ON *.* TO 'test_user' WITH GRANT OPTION;"),
                    call("REVOKE CREATE ON *.* FROM 'test_user';"),
                    call("GRANT CREATE ON `db1`.* TO 'test_user';"),
                    call("GRANT CREATE ON `db2`.* TO 'test_user';"),
                    call("GRANT ALTER ON *.* TO 'test_user';"),
                    call("REVOKE ALTER ON `db3`.* FROM 'test_user';"),
                    call("GRANT DROP ON *.* TO 'test_user';"),
                ],
            },
            'result': {
                'grants_changed': {
                    'INSERT': {
                        'reason': 'not exists',
                        'actions': ["GRANT INSERT ON *.* TO 'test_user' WITH GRANT OPTION;"],
                    },
                    'CREATE': {
                        'reason': 'databases mismatch. Expected: [db1, db2], got: [db1]',
                        'actions': [
                            "REVOKE CREATE ON *.* FROM 'test_user';",
                            "GRANT CREATE ON `db1`.* TO 'test_user';",
                            "GRANT CREATE ON `db2`.* TO 'test_user';",
                        ],
                    },
                    'ALTER': {
                        'reason': 'databases mismatch. Expected: [db3], got: [db3, db4]',
                        'actions': [
                            "GRANT ALTER ON *.* TO 'test_user';",
                            "REVOKE ALTER ON `db3`.* FROM 'test_user';",
                        ],
                    },
                    'DROP': {
                        'reason': 'partial_revoke mismatch. Expected: False, got: 1',
                        'actions': [
                            "GRANT DROP ON *.* TO 'test_user';",
                        ],
                    },
                }
            },
        },
    },
    {
        'id': 'Grant missing permissions (dry-run)',
        'args': {
            'opts': {'test': True},
            'target_grants': [
                grant('SELECT'),
                grant('INSERT', grant_option=True),
                grant('CREATE', ['db1', 'db2']),
                grant_except_databases('ALTER', ['db3']),
                grant('DROP'),
            ],
            'existing_grants': {
                'SELECT': [{'access_type': 'SELECT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0}],
                'CREATE': [{'access_type': 'CREATE', 'database': 'db1', 'is_partial_revoke': 0, 'grant_option': 0}],
                'ALTER': [
                    {'access_type': 'ALTER', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'ALTER', 'database': 'db3', 'is_partial_revoke': 1, 'grant_option': 0},
                    {'access_type': 'ALTER', 'database': 'db4', 'is_partial_revoke': 1, 'grant_option': 0},
                ],
                'DROP': [
                    {'access_type': 'DROP', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'DROP', 'database': 'db5', 'is_partial_revoke': 1, 'grant_option': 0},
                ],
            },
            'calls': {
                'execute': [],
            },
            'result': {
                'grants_changed': {
                    'INSERT': {
                        'reason': 'not exists',
                        'actions': ["GRANT INSERT ON *.* TO 'test_user' WITH GRANT OPTION;"],
                    },
                    'CREATE': {
                        'reason': 'databases mismatch. Expected: [db1, db2], got: [db1]',
                        'actions': [
                            "REVOKE CREATE ON *.* FROM 'test_user';",
                            "GRANT CREATE ON `db1`.* TO 'test_user';",
                            "GRANT CREATE ON `db2`.* TO 'test_user';",
                        ],
                    },
                    'ALTER': {
                        'reason': 'databases mismatch. Expected: [db3], got: [db3, db4]',
                        'actions': [
                            "GRANT ALTER ON *.* TO 'test_user';",
                            "REVOKE ALTER ON `db3`.* FROM 'test_user';",
                        ],
                    },
                    'DROP': {
                        'reason': 'partial_revoke mismatch. Expected: False, got: 1',
                        'actions': [
                            "GRANT DROP ON *.* TO 'test_user';",
                        ],
                    },
                }
            },
        },
    },
    {
        'id': 'Revoke excessive permissions',
        'args': {
            'opts': {'test': False},
            'target_grants': [
                grant('SELECT'),
                grant('CREATE', ['db1']),
                grant_except_databases('ALTER', ['db3', 'db4']),
                grant_except_databases('DROP', ['db5']),
            ],
            'existing_grants': {
                'SELECT': [{'access_type': 'SELECT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0}],
                'INSERT': [{'access_type': 'INSERT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 1}],
                'CREATE': [
                    {'access_type': 'CREATE', 'database': 'db1', 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'CREATE', 'database': 'db2', 'is_partial_revoke': 0, 'grant_option': 0},
                ],
                'ALTER': [
                    {'access_type': 'ALTER', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'ALTER', 'database': 'db3', 'is_partial_revoke': 1, 'grant_option': 0},
                ],
                'DROP': [
                    {'access_type': 'DROP', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0},
                ],
            },
            'calls': {
                'execute': [
                    call("REVOKE INSERT ON *.* FROM 'test_user';"),
                    call("REVOKE CREATE ON *.* FROM 'test_user';"),
                    call("GRANT CREATE ON `db1`.* TO 'test_user';"),
                    call("GRANT ALTER ON *.* TO 'test_user';"),
                    call("REVOKE ALTER ON `db3`.* FROM 'test_user';"),
                    call("REVOKE ALTER ON `db4`.* FROM 'test_user';"),
                    call("GRANT DROP ON *.* TO 'test_user';"),
                    call("REVOKE DROP ON `db5`.* FROM 'test_user';"),
                ],
            },
            'result': {
                'grants_changed': {
                    'INSERT': {
                        'reason': 'removing, not in config',
                        'actions': ["REVOKE INSERT ON *.* FROM 'test_user';"],
                    },
                    'CREATE': {
                        'reason': 'databases mismatch. Expected: [db1], got: [db1, db2]',
                        'actions': [
                            "REVOKE CREATE ON *.* FROM 'test_user';",
                            "GRANT CREATE ON `db1`.* TO 'test_user';",
                        ],
                    },
                    'ALTER': {
                        'reason': 'databases mismatch. Expected: [db3, db4], got: [db3]',
                        'actions': [
                            "GRANT ALTER ON *.* TO 'test_user';",
                            "REVOKE ALTER ON `db3`.* FROM 'test_user';",
                            "REVOKE ALTER ON `db4`.* FROM 'test_user';",
                        ],
                    },
                    'DROP': {
                        'reason': 'partial_revoke mismatch. Expected: True, got: 0',
                        'actions': [
                            "GRANT DROP ON *.* TO 'test_user';",
                            "REVOKE DROP ON `db5`.* FROM 'test_user';",
                        ],
                    },
                }
            },
        },
    },
    {
        'id': 'Revoke excessive permissions (dry-run)',
        'args': {
            'opts': {'test': True},
            'target_grants': [
                grant('SELECT'),
                grant('CREATE', ['db1']),
                grant_except_databases('ALTER', ['db3', 'db4']),
                grant_except_databases('DROP', ['db5']),
            ],
            'existing_grants': {
                'SELECT': [{'access_type': 'SELECT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0}],
                'INSERT': [{'access_type': 'INSERT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 1}],
                'CREATE': [
                    {'access_type': 'CREATE', 'database': 'db1', 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'CREATE', 'database': 'db2', 'is_partial_revoke': 0, 'grant_option': 0},
                ],
                'ALTER': [
                    {'access_type': 'ALTER', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'ALTER', 'database': 'db3', 'is_partial_revoke': 1, 'grant_option': 0},
                ],
                'DROP': [
                    {'access_type': 'DROP', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0},
                ],
            },
            'calls': {
                'execute': [],
            },
            'result': {
                'grants_changed': {
                    'INSERT': {
                        'reason': 'removing, not in config',
                        'actions': ["REVOKE INSERT ON *.* FROM 'test_user';"],
                    },
                    'CREATE': {
                        'reason': 'databases mismatch. Expected: [db1], got: [db1, db2]',
                        'actions': [
                            "REVOKE CREATE ON *.* FROM 'test_user';",
                            "GRANT CREATE ON `db1`.* TO 'test_user';",
                        ],
                    },
                    'ALTER': {
                        'reason': 'databases mismatch. Expected: [db3, db4], got: [db3]',
                        'actions': [
                            "GRANT ALTER ON *.* TO 'test_user';",
                            "REVOKE ALTER ON `db3`.* FROM 'test_user';",
                            "REVOKE ALTER ON `db4`.* FROM 'test_user';",
                        ],
                    },
                    'DROP': {
                        'reason': 'partial_revoke mismatch. Expected: True, got: 0',
                        'actions': [
                            "GRANT DROP ON *.* TO 'test_user';",
                            "REVOKE DROP ON `db5`.* FROM 'test_user';",
                        ],
                    },
                }
            },
        },
    },
    {
        'id': 'Change list of accessible databases',
        'args': {
            'opts': {'test': False},
            'target_grants': [
                grant('SELECT', ['db1', 'db2']),
                grant_except_databases('INSERT', ['db2', 'db3']),
            ],
            'existing_grants': {
                'SELECT': [
                    {'access_type': 'SELECT', 'database': 'db2', 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'SELECT', 'database': 'db3', 'is_partial_revoke': 0, 'grant_option': 0},
                ],
                'INSERT': [
                    {'access_type': 'INSERT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'INSERT', 'database': 'db1', 'is_partial_revoke': 1, 'grant_option': 0},
                    {'access_type': 'INSERT', 'database': 'db3', 'is_partial_revoke': 1, 'grant_option': 0},
                ],
            },
            'calls': {
                'execute': [
                    call("REVOKE SELECT ON *.* FROM 'test_user';"),
                    call("GRANT SELECT ON `db1`.* TO 'test_user';"),
                    call("GRANT SELECT ON `db2`.* TO 'test_user';"),
                    call("GRANT INSERT ON *.* TO 'test_user';"),
                    call("REVOKE INSERT ON `db2`.* FROM 'test_user';"),
                    call("REVOKE INSERT ON `db3`.* FROM 'test_user';"),
                ],
            },
            'result': {
                'grants_changed': {
                    'SELECT': {
                        'reason': 'databases mismatch. Expected: [db1, db2], got: [db2, db3]',
                        'actions': [
                            "REVOKE SELECT ON *.* FROM 'test_user';",
                            "GRANT SELECT ON `db1`.* TO 'test_user';",
                            "GRANT SELECT ON `db2`.* TO 'test_user';",
                        ],
                    },
                    'INSERT': {
                        'reason': 'databases mismatch. Expected: [db2, db3], got: [db1, db3]',
                        'actions': [
                            "GRANT INSERT ON *.* TO 'test_user';",
                            "REVOKE INSERT ON `db2`.* FROM 'test_user';",
                            "REVOKE INSERT ON `db3`.* FROM 'test_user';",
                        ],
                    },
                }
            },
        },
    },
    {
        'id': 'Change list of accessible databases (dry-run)',
        'args': {
            'opts': {'test': True},
            'target_grants': [
                grant('SELECT', ['db1', 'db2']),
                grant_except_databases('INSERT', ['db2', 'db3']),
            ],
            'existing_grants': {
                'SELECT': [
                    {'access_type': 'SELECT', 'database': 'db2', 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'SELECT', 'database': 'db3', 'is_partial_revoke': 0, 'grant_option': 0},
                ],
                'INSERT': [
                    {'access_type': 'INSERT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0},
                    {'access_type': 'INSERT', 'database': 'db1', 'is_partial_revoke': 1, 'grant_option': 0},
                    {'access_type': 'INSERT', 'database': 'db3', 'is_partial_revoke': 1, 'grant_option': 0},
                ],
            },
            'calls': {
                'execute': [],
            },
            'result': {
                'grants_changed': {
                    'SELECT': {
                        'reason': 'databases mismatch. Expected: [db1, db2], got: [db2, db3]',
                        'actions': [
                            "REVOKE SELECT ON *.* FROM 'test_user';",
                            "GRANT SELECT ON `db1`.* TO 'test_user';",
                            "GRANT SELECT ON `db2`.* TO 'test_user';",
                        ],
                    },
                    'INSERT': {
                        'reason': 'databases mismatch. Expected: [db2, db3], got: [db1, db3]',
                        'actions': [
                            "GRANT INSERT ON *.* TO 'test_user';",
                            "REVOKE INSERT ON `db2`.* FROM 'test_user';",
                            "REVOKE INSERT ON `db3`.* FROM 'test_user';",
                        ],
                    },
                }
            },
        },
    },
    {
        'id': 'Change grant option',
        'args': {
            'opts': {'test': False},
            'target_grants': [
                grant('SELECT', grant_option=True),
            ],
            'existing_grants': {
                'SELECT': [{'access_type': 'SELECT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0}],
            },
            'calls': {
                'execute': [
                    call("GRANT SELECT ON *.* TO 'test_user' WITH GRANT OPTION;"),
                ],
            },
            'result': {
                'grants_changed': {
                    'SELECT': {
                        'reason': 'grant option mismatch. Expected: True, got: False',
                        'actions': [
                            "GRANT SELECT ON *.* TO 'test_user' WITH GRANT OPTION;",
                        ],
                    },
                }
            },
        },
    },
    {
        'id': 'Change grant option (dry-run)',
        'args': {
            'opts': {'test': True},
            'target_grants': [
                grant('SELECT', grant_option=True),
            ],
            'existing_grants': {
                'SELECT': [{'access_type': 'SELECT', 'database': None, 'is_partial_revoke': 0, 'grant_option': 0}],
            },
            'calls': {
                'execute': [],
            },
            'result': {
                'grants_changed': {
                    'SELECT': {
                        'reason': 'grant option mismatch. Expected: True, got: False',
                        'actions': [
                            "GRANT SELECT ON *.* TO 'test_user' WITH GRANT OPTION;",
                        ],
                    },
                }
            },
        },
    },
)
def test_ensure_user_grants(opts, target_grants, existing_grants, calls, result):
    mdb_clickhouse.__opts__ = opts
    mdb_clickhouse.__salt__['mdb_clickhouse.grant'] = grant
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_user_grants'] = lambda _: existing_grants

    for function in calls.keys():
        mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)] = MagicMock()

    assert mdb_clickhouse._ensure_user_grants('test_user', target_grants) == result

    for function, function_calls in calls.items():
        assert mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)].call_args_list == function_calls


@parametrize(
    {
        'id': 'Nothing changed, ClickHouse 21.4',
        'args': {
            'opts': {'test': False},
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '21.4',
                        'sql_user_management': True,
                        'sql_database_management': True,
                    },
                },
            },
            'target_password_hash': 'password_hash',
            'existing_users': ['admin'],
            'existing_password_hash': 'password_hash',
            'existing_grants': {
                "SYSTEM DROP REPLICA": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM DROP REPLICA"}
                ],
                "SYSTEM RELOAD": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM RELOAD"}
                ],
                "SYSTEM RESTART REPLICA": [
                    {
                        "database": None,
                        "is_partial_revoke": 0,
                        "grant_option": 1,
                        "access_type": "SYSTEM RESTART REPLICA",
                    }
                ],
                "TRUNCATE": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "TRUNCATE"},
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "TRUNCATE"},
                ],
                "S3": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "S3"}],
                "ALTER": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "ALTER"},
                ],
                "ALTER COLUMN": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER COLUMN"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER COLUMN",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER COLUMN",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER COLUMN"},
                ],
                "ALTER ORDER BY": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER ORDER BY"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER ORDER BY",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER ORDER BY",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER ORDER BY"},
                ],
                "ALTER SAMPLE BY": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER SAMPLE BY"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER SAMPLE BY",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER SAMPLE BY",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER SAMPLE BY",
                    },
                ],
                "ALTER ADD INDEX": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER ADD INDEX"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER ADD INDEX",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER ADD INDEX",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER ADD INDEX",
                    },
                ],
                "ALTER DROP INDEX": [
                    {
                        "database": "system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER DROP INDEX",
                    },
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER DROP INDEX",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER DROP INDEX",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER DROP INDEX",
                    },
                ],
                "ALTER CLEAR INDEX": [
                    {
                        "database": "system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CLEAR INDEX",
                    },
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CLEAR INDEX",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CLEAR INDEX",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CLEAR INDEX",
                    },
                ],
                "ALTER CONSTRAINT": [
                    {
                        "database": "system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CONSTRAINT",
                    },
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CONSTRAINT",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CONSTRAINT",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CONSTRAINT",
                    },
                ],
                "ALTER TTL": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER TTL"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER TTL",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER TTL",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER TTL"},
                ],
                "ALTER SETTINGS": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER SETTINGS"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER SETTINGS",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER SETTINGS",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER SETTINGS"},
                ],
                "ALTER MOVE PARTITION": [
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "MOVE PARTITION"},
                ],
                "ALTER FETCH PARTITION": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "FETCH PARTITION"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "FETCH PARTITION",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "FETCH PARTITION",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "FETCH PARTITION",
                    },
                ],
                "ALTER FREEZE PARTITION": [
                    {
                        "database": "system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER FREEZE PARTITION",
                    },
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER FREEZE PARTITION",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER FREEZE PARTITION",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER FREEZE PARTITION",
                    },
                ],
                "ALTER VIEW": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER VIEW"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER VIEW",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER VIEW",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER VIEW"},
                ],
                "dictGet": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "dictGet"}],
                "OPTIMIZE": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "OPTIMIZE"},
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "OPTIMIZE"},
                ],
                "SELECT": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SELECT"}],
                "INSERT": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "INSERT"},
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "INSERT"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "INSERT",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "INSERT",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "INSERT"},
                ],
                "DROP": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "DROP"},
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "DROP"},
                ],
                "ODBC": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "ODBC"}],
                "SYSTEM MOVES": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM MOVES"}
                ],
                "SYSTEM REPLICATION QUEUES": [
                    {
                        "database": None,
                        "is_partial_revoke": 0,
                        "grant_option": 1,
                        "access_type": "SYSTEM REPLICATION QUEUES",
                    }
                ],
                "POSTGRES": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "POSTGRES"}],
                "SHOW": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SHOW"}],
                "URL": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "URL"}],
                "INTROSPECTION": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "INTROSPECTION"}
                ],
                "CREATE": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "ALTER"},
                ],
                "CREATE DATABASE": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "CREATE DATABASE"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DATABASE",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DATABASE",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DATABASE",
                    },
                ],
                "CREATE TABLE": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "CREATE TABLE"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE TABLE",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE TABLE",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "CREATE TABLE"},
                ],
                "CREATE VIEW": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "CREATE VIEW"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE VIEW",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE VIEW",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "CREATE VIEW"},
                ],
                "CREATE DICTIONARY": [
                    {
                        "database": "system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DICTIONARY",
                    },
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DICTIONARY",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DICTIONARY",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DICTIONARY",
                    },
                ],
                "SYSTEM SYNC REPLICA": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM SYNC REPLICA"}
                ],
                "SYSTEM SENDS": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM SENDS"}
                ],
                "SYSTEM MERGES": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM MERGES"}
                ],
                "HDFS": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "HDFS"}],
                "JDBC": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "JDBC"}],
                "REMOTE": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "REMOTE"}],
                "MONGO": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "MONGO"}],
                "KILL QUERY": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "KILL QUERY"}
                ],
                "ACCESS MANAGEMENT": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "ACCESS MANAGEMENT"}
                ],
                "SYSTEM FLUSH": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM FLUSH"}
                ],
                "SYSTEM FETCHES": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM FETCHES"}
                ],
                "MYSQL": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "MYSQL"}],
                "SYSTEM DROP CACHE": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM DROP CACHE"}
                ],
                "SYSTEM TTL MERGES": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM TTL MERGES"}
                ],
            },
            'existing_grantees': [['ANY'], ['admin']],
            'grant_changes': [],
        },
    },
    {
        'id': 'Create admin user, ClickHouse 21.4',
        'args': {
            'opts': {'test': False},
            'pillar': {
                'data': {
                    'clickhouse': {'ch_version': '21.4', 'sql_user_management': True, 'sql_database_management': True},
                },
            },
            'target_password_hash': 'password_hash',
            'existing_users': [],
            'existing_password_hash': 'password_hash',
            'existing_grants': {},
            'existing_grantees': [],
            'grant_changes': [
                {
                    "name": "SHOW",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SHOW ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SELECT",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SELECT ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "KILL QUERY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT KILL QUERY ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM DROP CACHE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM DROP CACHE ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM RELOAD",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM RELOAD ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM MERGES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM MERGES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM TTL MERGES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM TTL MERGES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM FETCHES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM FETCHES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM MOVES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM MOVES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM SENDS",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM SENDS ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM REPLICATION QUEUES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM REPLICATION QUEUES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM DROP REPLICA",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM DROP REPLICA ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM SYNC REPLICA",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM SYNC REPLICA ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM RESTART REPLICA",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM RESTART REPLICA ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM FLUSH",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM FLUSH ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "dictGet",
                    "reason": "not exists",
                    "actions": [
                        "GRANT dictGet ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "INTROSPECTION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT INTROSPECTION ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "URL",
                    "reason": "not exists",
                    "actions": [
                        "GRANT URL ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "REMOTE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT REMOTE ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "MONGO",
                    "reason": "not exists",
                    "actions": [
                        "GRANT MONGO ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "MYSQL",
                    "reason": "not exists",
                    "actions": [
                        "GRANT MYSQL ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "ODBC",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ODBC ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "JDBC",
                    "reason": "not exists",
                    "actions": [
                        "GRANT JDBC ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "HDFS",
                    "reason": "not exists",
                    "actions": [
                        "GRANT HDFS ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "S3",
                    "reason": "not exists",
                    "actions": [
                        "GRANT S3 ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "INSERT",
                    "reason": "not exists",
                    "actions": [
                        "GRANT INSERT ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE INSERT ON `system`.* FROM 'admin';",
                        "REVOKE INSERT ON `information_schema`.* FROM 'admin';",
                        "REVOKE INSERT ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE INSERT ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "TRUNCATE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT TRUNCATE ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE TRUNCATE ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "OPTIMIZE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT OPTIMIZE ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE OPTIMIZE ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "ALTER COLUMN",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER COLUMN ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER COLUMN ON `system`.* FROM 'admin';",
                        "REVOKE ALTER COLUMN ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER COLUMN ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER COLUMN ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER ORDER BY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER ORDER BY ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER ORDER BY ON `system`.* FROM 'admin';",
                        "REVOKE ALTER ORDER BY ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER ORDER BY ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER ORDER BY ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER SAMPLE BY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER SAMPLE BY ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER SAMPLE BY ON `system`.* FROM 'admin';",
                        "REVOKE ALTER SAMPLE BY ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER SAMPLE BY ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER SAMPLE BY ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER ADD INDEX",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER ADD INDEX ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER ADD INDEX ON `system`.* FROM 'admin';",
                        "REVOKE ALTER ADD INDEX ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER ADD INDEX ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER ADD INDEX ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER DROP INDEX",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER DROP INDEX ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER DROP INDEX ON `system`.* FROM 'admin';",
                        "REVOKE ALTER DROP INDEX ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER DROP INDEX ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER DROP INDEX ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER CLEAR INDEX",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER CLEAR INDEX ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER CLEAR INDEX ON `system`.* FROM 'admin';",
                        "REVOKE ALTER CLEAR INDEX ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER CLEAR INDEX ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER CLEAR INDEX ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER CONSTRAINT",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER CONSTRAINT ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER CONSTRAINT ON `system`.* FROM 'admin';",
                        "REVOKE ALTER CONSTRAINT ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER CONSTRAINT ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER CONSTRAINT ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER TTL",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER TTL ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER TTL ON `system`.* FROM 'admin';",
                        "REVOKE ALTER TTL ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER TTL ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER TTL ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER SETTINGS",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER SETTINGS ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER SETTINGS ON `system`.* FROM 'admin';",
                        "REVOKE ALTER SETTINGS ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER SETTINGS ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER SETTINGS ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER MOVE PARTITION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER MOVE PARTITION ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER MOVE PARTITION ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER FETCH PARTITION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER FETCH PARTITION ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER FETCH PARTITION ON `system`.* FROM 'admin';",
                        "REVOKE ALTER FETCH PARTITION ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER FETCH PARTITION ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER FETCH PARTITION ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER FREEZE PARTITION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER FREEZE PARTITION ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER FREEZE PARTITION ON `system`.* FROM 'admin';",
                        "REVOKE ALTER FREEZE PARTITION ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER FREEZE PARTITION ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER FREEZE PARTITION ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER VIEW",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER VIEW ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER VIEW ON `system`.* FROM 'admin';",
                        "REVOKE ALTER VIEW ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER VIEW ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER VIEW ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "CREATE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "CREATE DATABASE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE DATABASE ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE CREATE DATABASE ON `system`.* FROM 'admin';",
                        "REVOKE CREATE DATABASE ON `information_schema`.* FROM 'admin';",
                        "REVOKE CREATE DATABASE ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE CREATE DATABASE ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "CREATE TABLE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE TABLE ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE CREATE TABLE ON `system`.* FROM 'admin';",
                        "REVOKE CREATE TABLE ON `information_schema`.* FROM 'admin';",
                        "REVOKE CREATE TABLE ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE CREATE TABLE ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "CREATE VIEW",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE VIEW ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE CREATE VIEW ON `system`.* FROM 'admin';",
                        "REVOKE CREATE VIEW ON `information_schema`.* FROM 'admin';",
                        "REVOKE CREATE VIEW ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE CREATE VIEW ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "CREATE DICTIONARY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE DICTIONARY ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE CREATE DICTIONARY ON `system`.* FROM 'admin';",
                        "REVOKE CREATE DICTIONARY ON `information_schema`.* FROM 'admin';",
                        "REVOKE CREATE DICTIONARY ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE CREATE DICTIONARY ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "DROP",
                    "reason": "not exists",
                    "actions": [
                        "GRANT DROP ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE DROP ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "POSTGRES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT POSTGRES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "ACCESS MANAGEMENT",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ACCESS MANAGEMENT ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
            ],
        },
    },
    {
        'id': 'Nothing changed, ClickHouse 21.8',
        'args': {
            'opts': {'test': False},
            'pillar': {
                'data': {
                    'clickhouse': {'ch_version': '21.8', 'sql_user_management': True, 'sql_database_management': True},
                },
            },
            'target_password_hash': 'password_hash',
            'existing_users': ['admin'],
            'existing_password_hash': 'password_hash',
            'existing_grants': {
                "SYSTEM DROP REPLICA": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM DROP REPLICA"}
                ],
                "SYSTEM RELOAD": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM RELOAD"}
                ],
                "SYSTEM RESTART REPLICA": [
                    {
                        "database": None,
                        "is_partial_revoke": 0,
                        "grant_option": 1,
                        "access_type": "SYSTEM RESTART REPLICA",
                    }
                ],
                "SYSTEM RESTORE REPLICA": [
                    {
                        "database": None,
                        "is_partial_revoke": 0,
                        "grant_option": 1,
                        "access_type": "SYSTEM RESTORE REPLICA",
                    },
                ],
                "TRUNCATE": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "TRUNCATE"},
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "TRUNCATE"},
                ],
                "S3": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "S3"}],
                "ALTER": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "ALTER"},
                ],
                "ALTER COLUMN": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER COLUMN"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER COLUMN",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER COLUMN",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER COLUMN"},
                ],
                "ALTER ORDER BY": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER ORDER BY"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER ORDER BY",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER ORDER BY",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER ORDER BY"},
                ],
                "ALTER SAMPLE BY": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER SAMPLE BY"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER SAMPLE BY",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER SAMPLE BY",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER SAMPLE BY",
                    },
                ],
                "ALTER ADD INDEX": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER ADD INDEX"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER ADD INDEX",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER ADD INDEX",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER ADD INDEX",
                    },
                ],
                "ALTER DROP INDEX": [
                    {
                        "database": "system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER DROP INDEX",
                    },
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER DROP INDEX",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER DROP INDEX",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER DROP INDEX",
                    },
                ],
                "ALTER CLEAR INDEX": [
                    {
                        "database": "system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CLEAR INDEX",
                    },
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CLEAR INDEX",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CLEAR INDEX",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CLEAR INDEX",
                    },
                ],
                "ALTER CONSTRAINT": [
                    {
                        "database": "system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CONSTRAINT",
                    },
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CONSTRAINT",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CONSTRAINT",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER CONSTRAINT",
                    },
                ],
                "ALTER TTL": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER TTL"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER TTL",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER TTL",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER TTL"},
                ],
                "ALTER SETTINGS": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER SETTINGS"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER SETTINGS",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER SETTINGS",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER SETTINGS"},
                ],
                "ALTER MOVE PARTITION": [
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "MOVE PARTITION"},
                ],
                "ALTER FETCH PARTITION": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "FETCH PARTITION"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "FETCH PARTITION",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "FETCH PARTITION",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "FETCH PARTITION",
                    },
                ],
                "ALTER FREEZE PARTITION": [
                    {
                        "database": "system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER FREEZE PARTITION",
                    },
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER FREEZE PARTITION",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER FREEZE PARTITION",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER FREEZE PARTITION",
                    },
                ],
                "ALTER VIEW": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER VIEW"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER VIEW",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "ALTER VIEW",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "ALTER VIEW"},
                ],
                "dictGet": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "dictGet"},
                ],
                "OPTIMIZE": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "OPTIMIZE"},
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "OPTIMIZE"},
                ],
                "SELECT": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SELECT"}],
                "INSERT": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "INSERT"},
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "INSERT"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "INSERT",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "INSERT",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "INSERT"},
                ],
                "DROP": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "DROP"},
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "DROP"},
                ],
                "ODBC": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "ODBC"},
                ],
                "SYSTEM MOVES": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM MOVES"},
                ],
                "SYSTEM REPLICATION QUEUES": [
                    {
                        "database": None,
                        "is_partial_revoke": 0,
                        "grant_option": 1,
                        "access_type": "SYSTEM REPLICATION QUEUES",
                    },
                ],
                "POSTGRES": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "POSTGRES"},
                ],
                "SHOW": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SHOW"}],
                "URL": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "URL"}],
                "INTROSPECTION": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "INTROSPECTION"}
                ],
                "CREATE": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "ALTER"},
                ],
                "CREATE DATABASE": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "CREATE DATABASE"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DATABASE",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DATABASE",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DATABASE",
                    },
                ],
                "CREATE TABLE": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "CREATE TABLE"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE TABLE",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE TABLE",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "CREATE TABLE"},
                ],
                "CREATE VIEW": [
                    {"database": "system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "CREATE VIEW"},
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE VIEW",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE VIEW",
                    },
                    {"database": "_system", "is_partial_revoke": 1, "grant_option": 0, "access_type": "CREATE VIEW"},
                ],
                "CREATE DICTIONARY": [
                    {
                        "database": "system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DICTIONARY",
                    },
                    {
                        "database": "information_schema",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DICTIONARY",
                    },
                    {
                        "database": "INFORMATION_SCHEMA",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DICTIONARY",
                    },
                    {
                        "database": "_system",
                        "is_partial_revoke": 1,
                        "grant_option": 0,
                        "access_type": "CREATE DICTIONARY",
                    },
                ],
                "SYSTEM SYNC REPLICA": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM SYNC REPLICA"}
                ],
                "SYSTEM SENDS": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM SENDS"}
                ],
                "SYSTEM MERGES": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM MERGES"}
                ],
                "HDFS": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "HDFS"}],
                "JDBC": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "JDBC"}],
                "REMOTE": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "REMOTE"}],
                "MONGO": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "MONGO"}],
                "KILL QUERY": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "KILL QUERY"}
                ],
                "ACCESS MANAGEMENT": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "ACCESS MANAGEMENT"}
                ],
                "SYSTEM FLUSH": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM FLUSH"}
                ],
                "SYSTEM FETCHES": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM FETCHES"}
                ],
                "MYSQL": [{"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "MYSQL"}],
                "SYSTEM DROP CACHE": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM DROP CACHE"}
                ],
                "SYSTEM TTL MERGES": [
                    {"database": None, "is_partial_revoke": 0, "grant_option": 1, "access_type": "SYSTEM TTL MERGES"}
                ],
            },
            'existing_grantees': [['ANY'], ['admin']],
            'grant_changes': [],
        },
    },
    {
        'id': 'Create admin user, ClickHouse 21.8',
        'args': {
            'opts': {'test': False},
            'pillar': {
                'data': {
                    'clickhouse': {'ch_version': '21.8', 'sql_user_management': True, 'sql_database_management': True},
                },
            },
            'target_password_hash': 'password_hash',
            'existing_users': [],
            'existing_password_hash': 'password_hash',
            'existing_grants': {},
            'existing_grantees': [],
            'grant_changes': [
                {
                    "name": "SHOW",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SHOW ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SELECT",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SELECT ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "KILL QUERY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT KILL QUERY ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM DROP CACHE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM DROP CACHE ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM RELOAD",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM RELOAD ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM MERGES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM MERGES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM TTL MERGES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM TTL MERGES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM FETCHES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM FETCHES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM MOVES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM MOVES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM SENDS",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM SENDS ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM REPLICATION QUEUES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM REPLICATION QUEUES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM DROP REPLICA",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM DROP REPLICA ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM SYNC REPLICA",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM SYNC REPLICA ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM RESTART REPLICA",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM RESTART REPLICA ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM FLUSH",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM FLUSH ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "dictGet",
                    "reason": "not exists",
                    "actions": [
                        "GRANT dictGet ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "INTROSPECTION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT INTROSPECTION ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "URL",
                    "reason": "not exists",
                    "actions": [
                        "GRANT URL ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "REMOTE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT REMOTE ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "MONGO",
                    "reason": "not exists",
                    "actions": [
                        "GRANT MONGO ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "MYSQL",
                    "reason": "not exists",
                    "actions": [
                        "GRANT MYSQL ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "ODBC",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ODBC ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "JDBC",
                    "reason": "not exists",
                    "actions": [
                        "GRANT JDBC ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "HDFS",
                    "reason": "not exists",
                    "actions": [
                        "GRANT HDFS ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "S3",
                    "reason": "not exists",
                    "actions": [
                        "GRANT S3 ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "INSERT",
                    "reason": "not exists",
                    "actions": [
                        "GRANT INSERT ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE INSERT ON `system`.* FROM 'admin';",
                        "REVOKE INSERT ON `information_schema`.* FROM 'admin';",
                        "REVOKE INSERT ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE INSERT ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "TRUNCATE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT TRUNCATE ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE TRUNCATE ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "OPTIMIZE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT OPTIMIZE ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE OPTIMIZE ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "ALTER COLUMN",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER COLUMN ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER COLUMN ON `system`.* FROM 'admin';",
                        "REVOKE ALTER COLUMN ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER COLUMN ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER COLUMN ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER ORDER BY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER ORDER BY ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER ORDER BY ON `system`.* FROM 'admin';",
                        "REVOKE ALTER ORDER BY ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER ORDER BY ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER ORDER BY ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER SAMPLE BY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER SAMPLE BY ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER SAMPLE BY ON `system`.* FROM 'admin';",
                        "REVOKE ALTER SAMPLE BY ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER SAMPLE BY ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER SAMPLE BY ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER ADD INDEX",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER ADD INDEX ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER ADD INDEX ON `system`.* FROM 'admin';",
                        "REVOKE ALTER ADD INDEX ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER ADD INDEX ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER ADD INDEX ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER DROP INDEX",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER DROP INDEX ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER DROP INDEX ON `system`.* FROM 'admin';",
                        "REVOKE ALTER DROP INDEX ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER DROP INDEX ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER DROP INDEX ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER CLEAR INDEX",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER CLEAR INDEX ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER CLEAR INDEX ON `system`.* FROM 'admin';",
                        "REVOKE ALTER CLEAR INDEX ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER CLEAR INDEX ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER CLEAR INDEX ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER CONSTRAINT",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER CONSTRAINT ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER CONSTRAINT ON `system`.* FROM 'admin';",
                        "REVOKE ALTER CONSTRAINT ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER CONSTRAINT ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER CONSTRAINT ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER TTL",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER TTL ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER TTL ON `system`.* FROM 'admin';",
                        "REVOKE ALTER TTL ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER TTL ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER TTL ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER SETTINGS",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER SETTINGS ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER SETTINGS ON `system`.* FROM 'admin';",
                        "REVOKE ALTER SETTINGS ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER SETTINGS ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER SETTINGS ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER MOVE PARTITION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER MOVE PARTITION ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER MOVE PARTITION ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER FETCH PARTITION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER FETCH PARTITION ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER FETCH PARTITION ON `system`.* FROM 'admin';",
                        "REVOKE ALTER FETCH PARTITION ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER FETCH PARTITION ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER FETCH PARTITION ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER FREEZE PARTITION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER FREEZE PARTITION ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER FREEZE PARTITION ON `system`.* FROM 'admin';",
                        "REVOKE ALTER FREEZE PARTITION ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER FREEZE PARTITION ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER FREEZE PARTITION ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER VIEW",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER VIEW ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER VIEW ON `system`.* FROM 'admin';",
                        "REVOKE ALTER VIEW ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER VIEW ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER VIEW ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "CREATE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "CREATE DATABASE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE DATABASE ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE CREATE DATABASE ON `system`.* FROM 'admin';",
                        "REVOKE CREATE DATABASE ON `information_schema`.* FROM 'admin';",
                        "REVOKE CREATE DATABASE ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE CREATE DATABASE ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "CREATE TABLE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE TABLE ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE CREATE TABLE ON `system`.* FROM 'admin';",
                        "REVOKE CREATE TABLE ON `information_schema`.* FROM 'admin';",
                        "REVOKE CREATE TABLE ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE CREATE TABLE ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "CREATE VIEW",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE VIEW ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE CREATE VIEW ON `system`.* FROM 'admin';",
                        "REVOKE CREATE VIEW ON `information_schema`.* FROM 'admin';",
                        "REVOKE CREATE VIEW ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE CREATE VIEW ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "CREATE DICTIONARY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE DICTIONARY ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE CREATE DICTIONARY ON `system`.* FROM 'admin';",
                        "REVOKE CREATE DICTIONARY ON `information_schema`.* FROM 'admin';",
                        "REVOKE CREATE DICTIONARY ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE CREATE DICTIONARY ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "DROP",
                    "reason": "not exists",
                    "actions": [
                        "GRANT DROP ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE DROP ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "POSTGRES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT POSTGRES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM RESTORE REPLICA",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM RESTORE REPLICA ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "ACCESS MANAGEMENT",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ACCESS MANAGEMENT ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
            ],
        },
    },
    {
        'id': 'Create admin user, without db management, ClickHouse 21.11',
        'args': {
            'opts': {'test': False},
            'pillar': {
                'data': {
                    'clickhouse': {
                        'ch_version': '21.11',
                        'sql_user_management': True,
                        'sql_database_management': False,
                    },
                },
            },
            'target_password_hash': 'password_hash',
            'existing_users': [],
            'existing_password_hash': 'password_hash',
            'existing_grants': {},
            'existing_grantees': [],
            'grant_changes': [
                {
                    "name": "SHOW",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SHOW ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SELECT",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SELECT ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "KILL QUERY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT KILL QUERY ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM DROP CACHE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM DROP CACHE ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM RELOAD",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM RELOAD ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM MERGES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM MERGES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM TTL MERGES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM TTL MERGES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM FETCHES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM FETCHES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM MOVES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM MOVES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM SENDS",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM SENDS ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM REPLICATION QUEUES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM REPLICATION QUEUES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM DROP REPLICA",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM DROP REPLICA ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM SYNC REPLICA",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM SYNC REPLICA ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM RESTART REPLICA",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM RESTART REPLICA ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM FLUSH",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM FLUSH ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "dictGet",
                    "reason": "not exists",
                    "actions": [
                        "GRANT dictGet ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "INTROSPECTION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT INTROSPECTION ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "URL",
                    "reason": "not exists",
                    "actions": [
                        "GRANT URL ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "REMOTE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT REMOTE ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "MONGO",
                    "reason": "not exists",
                    "actions": [
                        "GRANT MONGO ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "MYSQL",
                    "reason": "not exists",
                    "actions": [
                        "GRANT MYSQL ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "ODBC",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ODBC ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "JDBC",
                    "reason": "not exists",
                    "actions": [
                        "GRANT JDBC ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "HDFS",
                    "reason": "not exists",
                    "actions": [
                        "GRANT HDFS ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "S3",
                    "reason": "not exists",
                    "actions": [
                        "GRANT S3 ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "INSERT",
                    "reason": "not exists",
                    "actions": [
                        "GRANT INSERT ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE INSERT ON `system`.* FROM 'admin';",
                        "REVOKE INSERT ON `information_schema`.* FROM 'admin';",
                        "REVOKE INSERT ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE INSERT ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "TRUNCATE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT TRUNCATE ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE TRUNCATE ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "OPTIMIZE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT OPTIMIZE ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE OPTIMIZE ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "ALTER COLUMN",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER COLUMN ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER COLUMN ON `system`.* FROM 'admin';",
                        "REVOKE ALTER COLUMN ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER COLUMN ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER COLUMN ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER ORDER BY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER ORDER BY ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER ORDER BY ON `system`.* FROM 'admin';",
                        "REVOKE ALTER ORDER BY ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER ORDER BY ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER ORDER BY ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER SAMPLE BY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER SAMPLE BY ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER SAMPLE BY ON `system`.* FROM 'admin';",
                        "REVOKE ALTER SAMPLE BY ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER SAMPLE BY ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER SAMPLE BY ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER ADD INDEX",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER ADD INDEX ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER ADD INDEX ON `system`.* FROM 'admin';",
                        "REVOKE ALTER ADD INDEX ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER ADD INDEX ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER ADD INDEX ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER DROP INDEX",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER DROP INDEX ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER DROP INDEX ON `system`.* FROM 'admin';",
                        "REVOKE ALTER DROP INDEX ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER DROP INDEX ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER DROP INDEX ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER CLEAR INDEX",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER CLEAR INDEX ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER CLEAR INDEX ON `system`.* FROM 'admin';",
                        "REVOKE ALTER CLEAR INDEX ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER CLEAR INDEX ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER CLEAR INDEX ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER CONSTRAINT",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER CONSTRAINT ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER CONSTRAINT ON `system`.* FROM 'admin';",
                        "REVOKE ALTER CONSTRAINT ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER CONSTRAINT ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER CONSTRAINT ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER TTL",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER TTL ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER TTL ON `system`.* FROM 'admin';",
                        "REVOKE ALTER TTL ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER TTL ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER TTL ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER SETTINGS",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER SETTINGS ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER SETTINGS ON `system`.* FROM 'admin';",
                        "REVOKE ALTER SETTINGS ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER SETTINGS ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER SETTINGS ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER MOVE PARTITION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER MOVE PARTITION ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER MOVE PARTITION ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER FETCH PARTITION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER FETCH PARTITION ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER FETCH PARTITION ON `system`.* FROM 'admin';",
                        "REVOKE ALTER FETCH PARTITION ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER FETCH PARTITION ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER FETCH PARTITION ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER FREEZE PARTITION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER FREEZE PARTITION ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER FREEZE PARTITION ON `system`.* FROM 'admin';",
                        "REVOKE ALTER FREEZE PARTITION ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER FREEZE PARTITION ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER FREEZE PARTITION ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "ALTER VIEW",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ALTER VIEW ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE ALTER VIEW ON `system`.* FROM 'admin';",
                        "REVOKE ALTER VIEW ON `information_schema`.* FROM 'admin';",
                        "REVOKE ALTER VIEW ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE ALTER VIEW ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "CREATE TEMPORARY TABLE",
                    "reason": "not exists",
                    "actions": ["GRANT CREATE TEMPORARY TABLE ON *.* TO 'admin' WITH GRANT OPTION;"],
                },
                {
                    "name": "CREATE TABLE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE TABLE ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE CREATE TABLE ON `system`.* FROM 'admin';",
                        "REVOKE CREATE TABLE ON `information_schema`.* FROM 'admin';",
                        "REVOKE CREATE TABLE ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE CREATE TABLE ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "CREATE VIEW",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE VIEW ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE CREATE VIEW ON `system`.* FROM 'admin';",
                        "REVOKE CREATE VIEW ON `information_schema`.* FROM 'admin';",
                        "REVOKE CREATE VIEW ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE CREATE VIEW ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "CREATE DICTIONARY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE DICTIONARY ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE CREATE DICTIONARY ON `system`.* FROM 'admin';",
                        "REVOKE CREATE DICTIONARY ON `information_schema`.* FROM 'admin';",
                        "REVOKE CREATE DICTIONARY ON `INFORMATION_SCHEMA`.* FROM 'admin';",
                        "REVOKE CREATE DICTIONARY ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "DROP TABLE",
                    "reason": "not exists",
                    "actions": [
                        "GRANT DROP TABLE ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE DROP TABLE ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "DROP VIEW",
                    "reason": "not exists",
                    "actions": [
                        "GRANT DROP VIEW ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE DROP VIEW ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "DROP DICTIONARY",
                    "reason": "not exists",
                    "actions": [
                        "GRANT DROP DICTIONARY ON *.* TO 'admin' WITH GRANT OPTION;",
                        "REVOKE DROP DICTIONARY ON `_system`.* FROM 'admin';",
                    ],
                },
                {
                    "name": "CREATE FUNCTION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT CREATE FUNCTION ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "DROP FUNCTION",
                    "reason": "not exists",
                    "actions": [
                        "GRANT DROP FUNCTION ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "POSTGRES",
                    "reason": "not exists",
                    "actions": [
                        "GRANT POSTGRES ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "SYSTEM RESTORE REPLICA",
                    "reason": "not exists",
                    "actions": [
                        "GRANT SYSTEM RESTORE REPLICA ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
                {
                    "name": "ACCESS MANAGEMENT",
                    "reason": "not exists",
                    "actions": [
                        "GRANT ACCESS MANAGEMENT ON *.* TO 'admin' WITH GRANT OPTION;",
                    ],
                },
            ],
        },
    },
)
def test_ensure_admin_user(
    opts,
    pillar,
    target_password_hash,
    existing_users,
    existing_password_hash,
    existing_grants,
    existing_grantees,
    grant_changes,
):
    expected_execute_calls = []
    for change in grant_changes:
        for action in change['actions']:
            expected_execute_calls.append(call(action))

    expected_result = {
        'comment': '',
        'name': 'admin',
        'result': True,
        'changes': {},
    }
    if grant_changes:
        expected_result['changes'] = {
            'user_created': 'admin',
            'grants_changed': dict(
                (change['name'], {'reason': change['reason'], 'actions': change['actions']}) for change in grant_changes
            ),
        }

    mdb_clickhouse.__opts__ = opts
    mock_pillar(__salt__, pillar)
    mock_version_cmp(__salt__)
    mdb_clickhouse.__salt__['mdb_clickhouse.version_cmp'] = version_cmp
    mdb_clickhouse.__salt__['mdb_clickhouse.admin_password_hash'] = lambda: target_password_hash
    mdb_clickhouse.__salt__['mdb_clickhouse.admin_grants'] = admin_grants
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_user_names'] = lambda: existing_users
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_user_password_hash'] = lambda _: existing_password_hash
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_user_grants'] = lambda _: existing_grants
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_user_grantees'] = lambda _: existing_grantees
    mdb_clickhouse.__salt__['mdb_clickhouse.execute'] = MagicMock()

    assert mdb_clickhouse.ensure_admin_user('ignored') == expected_result
    assert mdb_clickhouse.__salt__['mdb_clickhouse.execute'].call_args_list == expected_execute_calls


@parametrize(
    {
        'id': 'Nothing changed',
        'args': {
            'opts': {'test': False},
            'target_quotas': [
                {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                },
                {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                },
            ],
            'target_quota_mode': 'default',
            'existing_quotas': {
                'test_user_quota_0': {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                    'keys': [],
                    'apply_to_list': ['test_user'],
                },
                'test_user_quota_1': {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                    'keys': [],
                    'apply_to_list': ['test_user'],
                },
            },
            'calls': {},
            'result': {},
        },
    },
    {
        'id': 'Nothing changed (dry run)',
        'args': {
            'opts': {'test': True},
            'target_quotas': [
                {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                },
                {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                },
            ],
            'target_quota_mode': 'default',
            'existing_quotas': {
                'test_user_quota_0': {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                    'keys': [],
                    'apply_to_list': ['test_user'],
                },
                'test_user_quota_1': {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                    'keys': [],
                    'apply_to_list': ['test_user'],
                },
            },
            'calls': {},
            'result': {},
        },
    },
    {
        'id': 'Add new user',
        'args': {
            'opts': {'test': False},
            'target_quotas': [
                {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                },
                {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                },
            ],
            'target_quota_mode': 'default',
            'existing_quotas': {},
            'calls': {
                'execute': [
                    call(
                        'CREATE QUOTA OR REPLACE `test_user_quota_0` FOR INTERVAL 0 second MAX queries = 0, '
                        'errors = 0, result_rows = 0, read_rows = 0, execution_time = 0 TO `test_user`'
                    ),
                    call(
                        'CREATE QUOTA OR REPLACE `test_user_quota_1` FOR INTERVAL 1 second MAX queries = 1, '
                        'errors = 1, result_rows = 1, read_rows = 1, execution_time = 1 TO `test_user`'
                    ),
                ],
            },
            'result': {
                'quota_created': [
                    'CREATE QUOTA OR REPLACE `test_user_quota_0` FOR INTERVAL 0 second MAX queries = 0, '
                    'errors = 0, result_rows = 0, read_rows = 0, execution_time = 0 TO `test_user`',
                    'CREATE QUOTA OR REPLACE `test_user_quota_1` FOR INTERVAL 1 second MAX queries = 1, '
                    'errors = 1, result_rows = 1, read_rows = 1, execution_time = 1 TO `test_user`',
                ]
            },
        },
    },
    {
        'id': 'Add new user (dry run)',
        'args': {
            'opts': {'test': True},
            'target_quotas': [
                {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                },
                {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                },
            ],
            'target_quota_mode': 'default',
            'existing_quotas': {},
            'calls': {},
            'result': {
                'quota_created': [
                    'CREATE QUOTA OR REPLACE `test_user_quota_0` FOR INTERVAL 0 second MAX queries = 0, '
                    'errors = 0, result_rows = 0, read_rows = 0, execution_time = 0 TO `test_user`',
                    'CREATE QUOTA OR REPLACE `test_user_quota_1` FOR INTERVAL 1 second MAX queries = 1, '
                    'errors = 1, result_rows = 1, read_rows = 1, execution_time = 1 TO `test_user`',
                ]
            },
        },
    },
    {
        'id': 'Add quota',
        'args': {
            'opts': {'test': False},
            'target_quotas': [
                {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                },
                {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                },
            ],
            'target_quota_mode': 'default',
            'existing_quotas': {
                'test_user_quota_0': {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                    'keys': [],
                    'apply_to_list': ['test_user'],
                },
            },
            'calls': {
                'execute': [
                    call(
                        'CREATE QUOTA OR REPLACE `test_user_quota_1` FOR INTERVAL 1 second MAX queries = 1, '
                        'errors = 1, result_rows = 1, read_rows = 1, execution_time = 1 TO `test_user`'
                    )
                ],
            },
            'result': {
                'quota_created': [
                    'CREATE QUOTA OR REPLACE `test_user_quota_1` FOR INTERVAL 1 second MAX queries = 1, '
                    'errors = 1, result_rows = 1, read_rows = 1, execution_time = 1 TO `test_user`'
                ]
            },
        },
    },
    {
        'id': 'Add quota (dry run)',
        'args': {
            'opts': {'test': True},
            'target_quotas': [
                {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                },
                {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                },
            ],
            'target_quota_mode': 'default',
            'existing_quotas': {
                'test_user_quota_0': {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                    'keys': [],
                    'apply_to_list': ['test_user'],
                },
            },
            'calls': {},
            'result': {
                'quota_created': [
                    'CREATE QUOTA OR REPLACE `test_user_quota_1` FOR INTERVAL 1 second MAX queries = 1, '
                    'errors = 1, result_rows = 1, read_rows = 1, execution_time = 1 TO `test_user`'
                ]
            },
        },
    },
    {
        'id': 'Update quota',
        'args': {
            'opts': {'test': False},
            'target_quotas': [
                {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 10,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                },
            ],
            'target_quota_mode': 'default',
            'existing_quotas': {
                'test_user_quota_0': {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                    'keys': [],
                    'apply_to_list': ['test_user'],
                },
            },
            'calls': {
                'execute': [
                    call(
                        'CREATE QUOTA OR REPLACE `test_user_quota_1` FOR INTERVAL 1 second MAX queries = 10, '
                        'errors = 1, result_rows = 1, read_rows = 1, execution_time = 1 TO `test_user`'
                    )
                ],
            },
            'result': {
                'quota_created': [
                    'CREATE QUOTA OR REPLACE `test_user_quota_1` FOR INTERVAL 1 second MAX queries = 10, '
                    'errors = 1, result_rows = 1, read_rows = 1, execution_time = 1 TO `test_user`'
                ]
            },
        },
    },
    {
        'id': 'Update quota (dry run)',
        'args': {
            'opts': {'test': True},
            'target_quotas': [
                {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 10,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                },
            ],
            'target_quota_mode': 'default',
            'existing_quotas': {
                'test_user_quota_0': {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                    'keys': [],
                    'apply_to_list': ['test_user'],
                },
            },
            'calls': {},
            'result': {
                'quota_created': [
                    'CREATE QUOTA OR REPLACE `test_user_quota_1` FOR INTERVAL 1 second MAX queries = 10, '
                    'errors = 1, result_rows = 1, read_rows = 1, execution_time = 1 TO `test_user`'
                ]
            },
        },
    },
    {
        'id': 'Update quota mode',
        'args': {
            'opts': {'test': False},
            'target_quotas': [
                {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                },
            ],
            'target_quota_mode': 'keyed',
            'existing_quotas': {
                'test_user_quota_0': {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                    'keys': [],
                    'apply_to_list': ['test_user'],
                },
            },
            'calls': {
                'execute': [
                    call(
                        'CREATE QUOTA OR REPLACE `test_user_quota_1` KEYED BY user_name FOR INTERVAL 1 second MAX queries = 1, '
                        'errors = 1, result_rows = 1, read_rows = 1, execution_time = 1 TO `test_user`'
                    )
                ],
            },
            'result': {
                'quota_created': [
                    'CREATE QUOTA OR REPLACE `test_user_quota_1` KEYED BY user_name FOR INTERVAL 1 second MAX queries = 1, '
                    'errors = 1, result_rows = 1, read_rows = 1, execution_time = 1 TO `test_user`'
                ]
            },
        },
    },
    {
        'id': 'Update quota mode (dry run)',
        'args': {
            'opts': {'test': True},
            'target_quotas': [
                {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                },
            ],
            'target_quota_mode': 'keyed',
            'existing_quotas': {
                'test_user_quota_0': {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                    'keys': [],
                    'apply_to_list': ['test_user'],
                },
            },
            'calls': {},
            'result': {
                'quota_created': [
                    'CREATE QUOTA OR REPLACE `test_user_quota_1` KEYED BY user_name FOR INTERVAL 1 second MAX queries = 1, '
                    'errors = 1, result_rows = 1, read_rows = 1, execution_time = 1 TO `test_user`'
                ]
            },
        },
    },
    {
        'id': 'Restore quotas when apply_to_list was corrupted',
        'args': {
            'opts': {'test': False},
            'target_quotas': [
                {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                },
                {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                },
            ],
            'target_quota_mode': 'default',
            'existing_quotas': {
                'test_user_quota_0': {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                    'keys': [],
                    'apply_to_list': [],
                },
                'test_user_quota_1': {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                    'keys': [],
                    'apply_to_list': ['test_user'],
                },
            },
            'calls': {
                'execute': [
                    call(
                        'CREATE QUOTA OR REPLACE `test_user_quota_0` FOR INTERVAL 0 second MAX queries = 0, '
                        'errors = 0, result_rows = 0, read_rows = 0, execution_time = 0 TO `test_user`'
                    )
                ],
            },
            'result': {
                'quota_created': [
                    'CREATE QUOTA OR REPLACE `test_user_quota_0` FOR INTERVAL 0 second MAX queries = 0, '
                    'errors = 0, result_rows = 0, read_rows = 0, execution_time = 0 TO `test_user`'
                ]
            },
        },
    },
    {
        'id': 'Restore quotas when apply_to_list was corrupted (dry run)',
        'args': {
            'opts': {'test': True},
            'target_quotas': [
                {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                },
                {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                },
            ],
            'target_quota_mode': 'default',
            'existing_quotas': {
                'test_user_quota_0': {
                    'result_rows': 0,
                    'read_rows': 0,
                    'queries': 0,
                    'interval_duration': 0,
                    'execution_time': 0,
                    'errors': 0,
                    'keys': [],
                    'apply_to_list': [],
                },
                'test_user_quota_1': {
                    'result_rows': 1,
                    'read_rows': 1,
                    'queries': 1,
                    'interval_duration': 1,
                    'execution_time': 1,
                    'errors': 1,
                    'keys': [],
                    'apply_to_list': ['test_user'],
                },
            },
            'calls': {},
            'result': {
                'quota_created': [
                    'CREATE QUOTA OR REPLACE `test_user_quota_0` FOR INTERVAL 0 second MAX queries = 0, '
                    'errors = 0, result_rows = 0, read_rows = 0, execution_time = 0 TO `test_user`'
                ]
            },
        },
    },
)
def test_ensure_user_quotas(opts, target_quotas, target_quota_mode, existing_quotas, calls, result):
    mdb_clickhouse.__opts__ = opts
    mdb_clickhouse.__salt__['mdb_clickhouse.user_quotas'] = lambda _: target_quotas
    mdb_clickhouse.__salt__['mdb_clickhouse.user_quota_mode'] = lambda _: target_quota_mode
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_user_quotas'] = lambda _: existing_quotas

    for function in calls.keys():
        mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)] = MagicMock()

    assert mdb_clickhouse._ensure_user_quotas('test_user') == result

    for function, function_calls in calls.items():
        assert mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)].call_args_list == function_calls


@parametrize(
    {
        'id': 'Nothing changed',
        'args': {
            'opts': {'test': False},
            'target_settings': {
                'send_timeout': 1,
                'count_distinct_implementation': 'uniqCombined',
            },
            'existing_settings': {
                'send_timeout': '1',
                'count_distinct_implementation': 'uniqCombined',
            },
            'calls': {},
            'result': {},
        },
    },
    {
        'id': 'Nothing changed (dry run)',
        'args': {
            'opts': {'test': False},
            'target_settings': {
                'send_timeout': 1,
                'count_distinct_implementation': 'uniqCombined',
            },
            'existing_settings': {
                'send_timeout': '1',
                'count_distinct_implementation': 'uniqCombined',
            },
            'calls': {},
            'result': {},
        },
    },
    {
        'id': 'Add user',
        'args': {
            'opts': {'test': False},
            'target_settings': {
                'send_timeout': 1,
                'count_distinct_implementation': 'uniqCombined',
            },
            'existing_settings': {},
            'calls': {
                'execute': [
                    call(
                        "CREATE SETTINGS PROFILE OR REPLACE `test_user_profile` SETTINGS "
                        "count_distinct_implementation = 'uniqCombined', send_timeout = 1 TO `test_user`"
                    )
                ],
            },
            'result': {
                'profile_created': "CREATE SETTINGS PROFILE OR REPLACE `test_user_profile` SETTINGS "
                "count_distinct_implementation = 'uniqCombined', "
                "send_timeout = 1 TO `test_user`",
            },
        },
    },
    {
        'id': 'Add user (dry run)',
        'args': {
            'opts': {'test': True},
            'target_settings': {
                'send_timeout': 1,
                'count_distinct_implementation': 'uniqCombined',
            },
            'existing_settings': {},
            'calls': {},
            'result': {
                'profile_created': "CREATE SETTINGS PROFILE OR REPLACE `test_user_profile` SETTINGS "
                "count_distinct_implementation = 'uniqCombined', "
                "send_timeout = 1 TO `test_user`",
            },
        },
    },
    {
        'id': 'Update setting value',
        'args': {
            'opts': {'test': False},
            'target_settings': {
                'send_timeout': 5,
                'count_distinct_implementation': 'uniqCombined',
            },
            'existing_settings': {
                'send_timeout': '1',
                'count_distinct_implementation': 'uniqCombined',
            },
            'calls': {
                'execute': [
                    call(
                        "CREATE SETTINGS PROFILE OR REPLACE `test_user_profile` SETTINGS "
                        "count_distinct_implementation = 'uniqCombined', send_timeout = 5 TO `test_user`"
                    )
                ],
            },
            'result': {
                'profile_created': "CREATE SETTINGS PROFILE OR REPLACE `test_user_profile` SETTINGS "
                "count_distinct_implementation = 'uniqCombined', "
                "send_timeout = 5 TO `test_user`",
            },
        },
    },
    {
        'id': 'Update setting value (dry run)',
        'args': {
            'opts': {'test': True},
            'target_settings': {
                'send_timeout': 5,
                'count_distinct_implementation': 'uniqCombined',
            },
            'existing_settings': {
                'send_timeout': '1',
                'count_distinct_implementation': 'uniqCombined',
            },
            'calls': {},
            'result': {
                'profile_created': "CREATE SETTINGS PROFILE OR REPLACE `test_user_profile` SETTINGS "
                "count_distinct_implementation = 'uniqCombined', "
                "send_timeout = 5 TO `test_user`",
            },
        },
    },
    {
        'id': 'Add setting',
        'args': {
            'opts': {'test': False},
            'target_settings': {
                'allow_ddl': 1,
                'send_timeout': 2,
                'count_distinct_implementation': 'uniqCombined',
            },
            'existing_settings': {
                'send_timeout': '2',
                'count_distinct_implementation': 'uniqCombined',
            },
            'calls': {
                'execute': [
                    call(
                        "CREATE SETTINGS PROFILE OR REPLACE `test_user_profile` SETTINGS "
                        "allow_ddl = 1, "
                        "count_distinct_implementation = 'uniqCombined', "
                        "send_timeout = 2 "
                        "TO `test_user`"
                    )
                ],
            },
            'result': {
                'profile_created': "CREATE SETTINGS PROFILE OR REPLACE `test_user_profile` SETTINGS "
                "allow_ddl = 1, "
                "count_distinct_implementation = 'uniqCombined', "
                "send_timeout = 2 "
                "TO `test_user`",
            },
        },
    },
)
def test_ensure_user_setting(opts, target_settings, existing_settings, calls, result):
    mdb_clickhouse.__opts__ = opts
    mdb_clickhouse.__salt__['mdb_clickhouse.user_settings'] = lambda _: target_settings
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_user_settings'] = lambda _: existing_settings

    for function in calls.keys():
        mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)] = MagicMock()

    assert mdb_clickhouse._ensure_user_settings('test_user') == result

    for function, function_calls in calls.items():
        assert mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)].call_args_list == function_calls


@parametrize(
    {
        'id': 'Nothing changed',
        'args': {
            'opts': {'test': False},
            'target_users': ['test_user'],
            'existing_users': ['test_user'],
            'calls': {},
            'result': {'name': 'users_cleanup', 'result': True, 'changes': {}, 'comment': ''},
        },
    },
    {
        'id': 'Nothing changed (dry run)',
        'args': {
            'opts': {'test': True},
            'target_users': ['test_user'],
            'existing_users': ['test_user'],
            'calls': {},
            'result': {'name': 'users_cleanup', 'result': True, 'changes': {}, 'comment': ''},
        },
    },
    {
        'id': 'Delete user',
        'args': {
            'opts': {'test': False},
            'target_users': ['test_user'],
            'existing_users': ['test_user', 'old_user'],
            'calls': {
                'delete_user': [call('old_user')],
            },
            'result': {
                'name': 'users_cleanup',
                'result': True,
                'changes': {
                    'user_deleted': ['old_user'],
                },
                'comment': '',
            },
        },
    },
    {
        'id': 'Delete user (dry run)',
        'args': {
            'opts': {'test': True},
            'target_users': ['test_user'],
            'existing_users': ['test_user', 'old_user'],
            'calls': {},
            'result': {
                'name': 'users_cleanup',
                'result': None,
                'changes': {
                    'user_deleted': ['old_user'],
                },
                'comment': '',
            },
        },
    },
)
def test_cleanup_users(opts, target_users, existing_users, calls, result):
    mdb_clickhouse.__opts__ = opts
    mdb_clickhouse.__salt__['mdb_clickhouse.user_names'] = lambda: target_users
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_user_names'] = lambda: existing_users

    for function in calls.keys():
        mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)] = MagicMock()

    assert mdb_clickhouse.cleanup_users('users_cleanup') == result

    for function, function_calls in calls.items():
        assert mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)].call_args_list == function_calls


@parametrize(
    {
        'id': 'Nothing changed',
        'args': {
            'opts': {'test': False},
            'target_users': {
                'test_user': {
                    'quotas': [
                        {
                            'result_rows': 1,
                            'read_rows': 1,
                            'queries': 1,
                            'interval_duration': 1,
                            'execution_time': 1,
                            'errors': 1,
                        }
                    ],
                },
            },
            'existing_quota_names': ['test_user_quota_1'],
            'calls': {},
            'result': {'name': 'quotas_cleanup', 'result': True, 'changes': {}, 'comment': ''},
        },
    },
    {
        'id': 'Nothing changed (dry run)',
        'args': {
            'opts': {'test': False},
            'target_users': {
                'test_user': {
                    'quotas': [
                        {
                            'result_rows': 1,
                            'read_rows': 1,
                            'queries': 1,
                            'interval_duration': 1,
                            'execution_time': 1,
                            'errors': 1,
                        }
                    ],
                },
            },
            'existing_quota_names': ['test_user_quota_1'],
            'calls': {},
            'result': {'name': 'quotas_cleanup', 'result': True, 'changes': {}, 'comment': ''},
        },
    },
    {
        'id': 'Delete quota',
        'args': {
            'opts': {'test': False},
            'target_users': {
                'test_user': {
                    'quotas': [
                        {
                            'result_rows': 1,
                            'read_rows': 1,
                            'queries': 1,
                            'interval_duration': 1,
                            'execution_time': 1,
                            'errors': 1,
                        }
                    ],
                },
            },
            'existing_quota_names': ['test_user_quota_1', 'test_user_quota_2'],
            'calls': {'execute': [call('DROP QUOTA `test_user_quota_2`;')]},
            'result': {
                'name': 'quotas_cleanup',
                'result': True,
                'changes': {
                    'quota_deleted': ['test_user_quota_2'],
                },
                'comment': '',
            },
        },
    },
    {
        'id': 'Delete quota (dry run)',
        'args': {
            'opts': {'test': True},
            'target_users': {
                'test_user': {
                    'quotas': [
                        {
                            'result_rows': 1,
                            'read_rows': 1,
                            'queries': 1,
                            'interval_duration': 1,
                            'execution_time': 1,
                            'errors': 1,
                        }
                    ],
                },
            },
            'existing_quota_names': ['test_user_quota_1', 'test_user_quota_2'],
            'calls': {},
            'result': {
                'name': 'quotas_cleanup',
                'result': None,
                'changes': {
                    'quota_deleted': ['test_user_quota_2'],
                },
                'comment': '',
            },
        },
    },
    {
        'id': 'Delete user',
        'args': {
            'opts': {'test': False},
            'target_users': {
                'test_user': {
                    'quotas': [],
                },
            },
            'existing_quota_names': ['test_user2_quota_1', 'test_user2_quota_2'],
            'calls': {
                'execute': [
                    call('DROP QUOTA `test_user2_quota_1`;'),
                    call('DROP QUOTA `test_user2_quota_2`;'),
                ]
            },
            'result': {
                'name': 'quotas_cleanup',
                'result': True,
                'changes': {
                    'quota_deleted': ['test_user2_quota_1', 'test_user2_quota_2'],
                },
                'comment': '',
            },
        },
    },
    {
        'id': 'Delete user (dry run)',
        'args': {
            'opts': {'test': True},
            'target_users': {
                'test_user': {
                    'quotas': [],
                },
            },
            'existing_quota_names': ['test_user2_quota_1', 'test_user2_quota_2'],
            'calls': {},
            'result': {
                'name': 'quotas_cleanup',
                'result': None,
                'changes': {
                    'quota_deleted': ['test_user2_quota_1', 'test_user2_quota_2'],
                },
                'comment': '',
            },
        },
    },
)
def test_cleanup_user_quotas(opts, target_users, existing_quota_names, calls, result):
    mdb_clickhouse.__opts__ = opts
    mdb_clickhouse.__salt__['mdb_clickhouse.user_names'] = lambda: list(target_users.keys())
    mdb_clickhouse.__salt__['mdb_clickhouse.user_quotas'] = lambda user_name: target_users[user_name]['quotas']
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_quota_names'] = lambda: existing_quota_names

    for function in calls.keys():
        mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)] = MagicMock()

    assert mdb_clickhouse.cleanup_user_quotas('quotas_cleanup') == result

    for function, function_calls in calls.items():
        assert mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)].call_args_list == function_calls


@parametrize(
    {
        'id': 'Nothing changed',
        'args': {
            'opts': {'test': False},
            'target_users': {
                'test_user': {
                    'settings': {
                        'send_timeout': 1,
                    },
                },
            },
            'existing_profile_names': ['test_user_profile'],
            'calls': {},
            'result': {'name': 'profiles_cleanup', 'result': True, 'changes': {}, 'comment': ''},
        },
    },
    {
        'id': 'Nothing changed (dry run)',
        'args': {
            'opts': {'test': True},
            'target_users': {
                'test_user': {
                    'settings': {
                        'send_timeout': 1,
                    },
                },
            },
            'existing_profile_names': ['test_user_profile'],
            'calls': {},
            'result': {'name': 'profiles_cleanup', 'result': True, 'changes': {}, 'comment': ''},
        },
    },
    {
        'id': 'Profile not needed',
        'args': {
            'opts': {'test': False},
            'target_users': {
                'test_user': {
                    'settings': {},
                },
            },
            'existing_profile_names': ['test_user_profile'],
            'calls': {
                'execute': [call('DROP SETTINGS PROFILE `test_user_profile`;')],
            },
            'result': {
                'name': 'profiles_cleanup',
                'result': True,
                'changes': {
                    'profile_deleted': ['test_user_profile'],
                },
                'comment': '',
            },
        },
    },
    {
        'id': 'Profile not needed (dry run)',
        'args': {
            'opts': {'test': True},
            'target_users': {
                'test_user': {
                    'settings': {},
                },
            },
            'existing_profile_names': ['test_user_profile'],
            'calls': {},
            'result': {
                'name': 'profiles_cleanup',
                'result': None,
                'changes': {
                    'profile_deleted': ['test_user_profile'],
                },
                'comment': '',
            },
        },
    },
    {
        'id': 'Delete user',
        'args': {
            'opts': {'test': False},
            'target_users': {
                'test_user': {
                    'settings': {
                        'send_timeout': 1,
                    },
                },
            },
            'existing_profile_names': ['test_user_profile', 'test_user2_profile'],
            'calls': {
                'execute': [call('DROP SETTINGS PROFILE `test_user2_profile`;')],
            },
            'result': {
                'name': 'profiles_cleanup',
                'result': True,
                'changes': {
                    'profile_deleted': ['test_user2_profile'],
                },
                'comment': '',
            },
        },
    },
    {
        'id': 'Delete user (dry run)',
        'args': {
            'opts': {'test': True},
            'target_users': {
                'test_user': {
                    'settings': {
                        'send_timeout': 1,
                    },
                },
            },
            'existing_profile_names': ['test_user_profile', 'test_user2_profile'],
            'calls': {},
            'result': {
                'name': 'profiles_cleanup',
                'result': None,
                'changes': {
                    'profile_deleted': ['test_user2_profile'],
                },
                'comment': '',
            },
        },
    },
)
def test_cleanup_user_settings_profiles(opts, target_users, existing_profile_names, calls, result):
    mdb_clickhouse.__opts__ = opts
    mdb_clickhouse.__salt__['mdb_clickhouse.user_names'] = lambda: list(target_users.keys())
    mdb_clickhouse.__salt__['mdb_clickhouse.user_settings'] = lambda user_name: target_users[user_name]['settings']
    mdb_clickhouse.__salt__['mdb_clickhouse.get_existing_profile_names'] = lambda: existing_profile_names

    for function in calls.keys():
        mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)] = MagicMock()

    assert mdb_clickhouse.cleanup_user_settings_profiles('profiles_cleanup') == result

    for function, function_calls in calls.items():
        assert mdb_clickhouse.__salt__['mdb_clickhouse.{0}'.format(function)].call_args_list == function_calls
