"""
Test for MongoDBPillar helper
"""
from copy import deepcopy

import pytest
from hamcrest import assert_that, contains, contains_inanyorder, has_entry

from dbaas_internal_api.core.exceptions import (
    DatabaseAccessExistsError,
    DatabaseExistsError,
    DatabaseNotExistsError,
    UserExistsError,
    UserNotExistsError,
)
from dbaas_internal_api.modules.mongodb.pillar import MongoDBPillar

from ...matchers import at_path

# pylint: disable=missing-docstring, invalid-name

TEST_CID = 'c21cefe0-55e0-4038-989f-69bb12bd35e2'
EXISTING_DATABASE = 'existing_database'
EXISTING_USER = 'existing_user'

user_not_exists_test_cases = [
    [
        UserNotExistsError,
        'User \'new_user\' does not exist',
        ['new_user', {'database_name': 'test_db1', 'roles': ['read']}],
    ],
]

user_exists_test_cases = [
    [UserExistsError, 'User \'alice\' already exists', ['alice', {'database_name': 'test_db1', 'roles': ['read']}]],
]

database_not_exists_test_cases = [
    [
        DatabaseNotExistsError,
        'Database \'new_database\' does not exist',
        [EXISTING_USER, {'database_name': 'new_database', 'roles': ['read']}],
    ],
]

database_exists_test_cases = [
    [
        DatabaseExistsError,
        'Database \'existing_database\' already exists',
        [EXISTING_USER, {'database_name': EXISTING_DATABASE, 'roles': ['read']}],
    ],
]

permission_exists_test_cases = [
    [
        DatabaseAccessExistsError,
        'User \'alice\' already has access to the database \'test_db1\'',
        ['alice', {'database_name': 'test_db1', 'roles': ['read']}],
    ],
]


class TestMongoDBPillar:
    SAMPLE_PILLAR = {
        'data': {
            'mongodb': {
                'databases': {'test_db1': {}, 'test_db2': {}, EXISTING_DATABASE: {}},
                'users': {
                    'admin': {
                        'services': ['mongod', 'mongos', 'mongocfg'],
                        'dbs': {'admin': ['root']},
                        'password': 'admin_pswd',
                    },
                    'alice': {
                        'services': ['mongod', 'mongos'],
                        'dbs': {
                            'test_db1': ['read'],
                        },
                        'password': 'alice_pswd',
                    },
                    'bob': {
                        'services': ['mongod', 'mongos'],
                        'dbs': {'test_db2': ['readWrite']},
                        'password': 'bob_pswd',
                    },
                    EXISTING_USER: {
                        'services': ['mongod', 'mongos'],
                        'dbs': {'test_db2': ['readWrite']},
                        'password': 'cool_pswd',
                    },
                    'monitor': {
                        'services': ['mongod', 'mongos', 'mongocfg'],
                        'dbs': {'admin': ['clusterMonitor']},
                        'password': 'monitor_pswd',
                    },
                },
                'zk_hosts': [
                    'zk1.test',
                    'zk2.test',
                    'zk3.test',
                ],
                'shards': {},
                'cluster_name': 'test_cluster_name',
                'version': {'major_num': '306', 'major_human': '3.6'},
                'cluster_auth': 'keyfile',
                'config': {
                    'mongod': {
                        'net': {'maxIncomingConnections': 128},
                        'storage': {'wiredTiger': {'engineConfig': {'cacheSizeGB': 2.0}}},
                    }
                },
                'keyfile': 'keyfile_contents',
            },
        },
    }

    def mk(self) -> MongoDBPillar:
        return MongoDBPillar.load(deepcopy(self.SAMPLE_PILLAR))

    def test_get_pillar_users(self):
        assert self.mk().pillar_users == self.SAMPLE_PILLAR['data']['mongodb']['users']

    def test_get_users(self):
        assert_that(
            self.mk().users(TEST_CID),
            contains(
                {
                    'name': 'alice',
                    'cluster_id': TEST_CID,
                    'permissions': [
                        {
                            'database_name': 'test_db1',
                            'roles': ['read'],
                        },
                    ],
                },
                {
                    'name': 'bob',
                    'cluster_id': TEST_CID,
                    'permissions': [
                        {
                            'database_name': 'test_db2',
                            'roles': ['readWrite'],
                        },
                    ],
                },
                {
                    'name': EXISTING_USER,
                    'cluster_id': TEST_CID,
                    'permissions': [
                        {
                            'database_name': 'test_db2',
                            'roles': ['readWrite'],
                        },
                    ],
                },
            ),
        )

    @pytest.mark.parametrize(
        'exception, message, permission',
        [
            *user_exists_test_cases,
        ],
    )
    def test_invalid_add_user(self, mocker, exception, message, permission):
        user_name, perm_spec = permission
        if user_name == EXISTING_USER:
            user_name = 'new_user'

        mocker.patch('dbaas_internal_api.modules.mongodb.pillar.encrypt', return_value='ENCRYPTED!')

        p = self.mk()
        with pytest.raises(exception) as exc:
            p.add_user(
                {
                    'name': user_name,
                    'password': 'blah',
                    'permissions': [perm_spec],
                }
            )
        assert message in str(exc.value)

    def test_add_user(self, mocker):
        # TODO: test add_user without permission spec
        mocker.patch('dbaas_internal_api.modules.mongodb.pillar.encrypt', return_value='ENCRYPTED!')
        p = self.mk()
        p.add_user(
            {
                'name': 'mellory',
                'password': 'blah',
                'permissions': [
                    {'database_name': 'test_db1', 'roles': ['readWrite']},
                    {'database_name': 'test_db2', 'roles': ['read']},
                ],
            }
        )

        assert_that(
            p.as_dict(),
            at_path(
                '$.data.mongodb.users',
                has_entry(
                    'mellory',
                    {
                        'services': ['mongod', 'mongos'],
                        'password': 'ENCRYPTED!',
                        'dbs': {
                            'test_db1': ['readWrite'],
                            'test_db2': ['read'],
                        },
                    },
                ),
            ),
        )

    def test_upset_existed_user(self, mocker):
        mocker.patch('dbaas_internal_api.modules.mongodb.pillar.encrypt', return_value='ENCRYPTED!')
        p = self.mk()
        p.update_user(
            'alice',
            password='blah',
            permissions=[
                {'database_name': 'test_db1', 'roles': ['readWrite']},
                {'database_name': 'test_db2', 'roles': ['read']},
            ],
        )

        assert_that(
            p.as_dict(),
            at_path(
                '$.data.mongodb.users',
                has_entry(
                    'alice',
                    {
                        'services': ['mongod', 'mongos'],
                        'password': 'ENCRYPTED!',
                        'dbs': {
                            'test_db1': ['readWrite'],
                            'test_db2': ['read'],
                        },
                    },
                ),
            ),
        )

    @pytest.mark.parametrize(
        'exception, message, permission',
        [
            *user_not_exists_test_cases,
            *database_not_exists_test_cases,
        ],
    )
    def test_invalid_upset_existed_user(self, mocker, exception, message, permission):
        user_name, perm_spec = permission

        mocker.patch('dbaas_internal_api.modules.mongodb.pillar.encrypt', return_value='ENCRYPTED!')

        p = self.mk()
        with pytest.raises(exception) as exc:
            p.update_user(user_name, password='blah', permissions=[perm_spec])
        assert message in str(exc.value)

    def test_delete_user(self):
        p = self.mk()
        p.delete_user('alice')
        assert 'alice' not in p.pillar_users

    @pytest.mark.parametrize(
        'exception, message, permission',
        [
            *user_not_exists_test_cases,
        ],
    )
    def test_invalid_delete_user(self, exception, message, permission):
        user_name, _ = permission
        p = self.mk()
        with pytest.raises(exception) as exc:
            p.delete_user(user_name)
        assert message in str(exc.value)

    def test_add_permission(self):
        p = self.mk()
        p.add_user_permission('alice', {'database_name': 'test_db2', 'roles': ['readWrite', 'read']})
        assert_that(
            p.as_dict(),
            at_path(
                '$.data.mongodb.users',
                has_entry(
                    'alice',
                    {
                        'services': ['mongod', 'mongos'],
                        'password': 'alice_pswd',
                        'dbs': {
                            'test_db1': ['read'],
                            'test_db2': ['readWrite', 'read'],
                        },
                    },
                ),
            ),
        )

    @pytest.mark.parametrize(
        'exception, message, permission',
        [*user_not_exists_test_cases, *database_not_exists_test_cases, *permission_exists_test_cases],
    )
    def test_invalid_add_user_permission(self, exception, message, permission):
        p = self.mk()
        with pytest.raises(exception) as exc:
            p.add_user_permission(*permission)
        assert message in str(exc.value)

    def test_delete_permission(self):
        p = self.mk()
        p.add_user_permission('bob', {'database_name': 'test_db1', 'roles': ['readWrite', 'read']})

        p.delete_user_permission('bob', 'test_db2')
        assert_that(
            p.as_dict(),
            at_path(
                '$.data.mongodb.users',
                has_entry(
                    'bob',
                    {
                        'services': ['mongod', 'mongos'],
                        'password': 'bob_pswd',
                        'dbs': {
                            'test_db1': ['readWrite', 'read'],
                        },
                    },
                ),
            ),
        )

    @pytest.mark.parametrize(
        'exception, message, permission',
        [
            *user_not_exists_test_cases,
            *database_not_exists_test_cases,
        ],
    )
    def test_invalid_delete_permission(self, exception, message, permission):
        p = self.mk()
        user_name, perm_spec = permission
        database_name = perm_spec['database_name']
        with pytest.raises(exception) as exc:
            p.delete_user_permission(user_name, database_name)
        assert message in str(exc.value)

    def test_pillar_databases(self):
        assert self.mk()._pillar_dbs == {'test_db1': {}, 'test_db2': {}, EXISTING_DATABASE: {}}

    def test_database(self):
        p = self.mk()
        database = p.database(TEST_CID, EXISTING_DATABASE)
        assert database == {'cluster_id': TEST_CID, 'name': EXISTING_DATABASE}

    def test_databases(self):
        p = self.mk()
        expected = [
            {
                'cluster_id': TEST_CID,
                'name': name,
            }
            for name in self.SAMPLE_PILLAR['data']['mongodb']['databases']
        ]

        assert_that(p.databases(TEST_CID), contains_inanyorder(*expected))

    def test_add_database(self):
        p = self.mk()
        p.add_database({'name': 'new_db'})
        assert_that(
            p.as_dict(),
            at_path(
                '$.data.mongodb.databases',
                contains_inanyorder('new_db', 'test_db1', 'test_db2', EXISTING_DATABASE),
            ),
        )

    def test_add_databases(self):
        p = self.mk()
        p.add_databases([{'name': 'new_db'}, {'name': 'new_db2'}])
        assert_that(
            p.as_dict(),
            at_path(
                '$.data.mongodb.databases',
                contains_inanyorder('new_db', 'new_db2', 'test_db1', 'test_db2', EXISTING_DATABASE),
            ),
        )

    @pytest.mark.parametrize(
        'exception, message, permission',
        [
            *database_exists_test_cases,
        ],
    )
    def test_invalid_add_database(self, exception, message, permission):
        p = self.mk()
        _, perm_spec = permission
        database_name = perm_spec['database_name']
        with pytest.raises(exception) as exc:
            p.add_database({'name': database_name})
        assert message in str(exc.value)

    def test_delete_database(self):
        p = self.mk()
        p.delete_database(EXISTING_DATABASE)
        assert_that(
            p.as_dict(),
            at_path(
                '$.data.mongodb.databases',
                contains_inanyorder('test_db1', 'test_db2'),
            ),
        )

    @pytest.mark.parametrize(
        'exception, message, permission',
        [
            *database_not_exists_test_cases,
        ],
    )
    def test_invalid_delete_database(self, exception, message, permission):
        p = self.mk()
        _, perm_spec = permission
        database_name = perm_spec['database_name']
        with pytest.raises(exception) as exc:
            p.delete_database(database_name)
        assert message in str(exc.value)

    def test_zk_hosts(self):
        assert self.mk().zk_hosts == ['zk1.test', 'zk2.test', 'zk3.test']
