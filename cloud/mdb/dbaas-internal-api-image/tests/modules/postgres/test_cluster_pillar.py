"""
PostgresqlCluster pillar tests
"""

from copy import deepcopy

import pytest
from hamcrest import (
    assert_that,
    contains,
    contains_inanyorder,
    has_entries,
    has_entry,
    has_item,
    has_key,
    has_properties,
    is_not,
    not_none,
)

from dbaas_internal_api.core.crypto import gen_encrypted_password
from dbaas_internal_api.core.exceptions import (
    DatabaseAccessExistsError,
    DatabaseAccessNotExistsError,
    DatabaseExistsError,
    DatabaseNotExistsError,
    PreconditionFailedError,
    UserExistsError,
    UserInvalidError,
    UserNotExistsError,
    IncorrectTemplateError,
)
from dbaas_internal_api.modules.postgres.pillar import PostgresqlClusterPillar
from dbaas_internal_api.modules.postgres.types import Database, UserWithPassword
from dbaas_internal_api.utils.zk import choice_zk_hosts

# flake8: noqa
from ...fixtures import app_config
from ...matchers import at_path

PILLAR = {
    'data': {
        'pg': {
            'version': {
                'major_num': '9600',
                'major_human': '9.6',
            },
        },
        's3': {
            'gpg_key': {
                'data': '<CENSORED>',
                'encryption_version': 1,
            },
            'gpg_key_id': '<CENSORED>',
        },
        'config': {
            'pgusers': {
                'no_login': {
                    'create': True,
                    'bouncer': True,
                    'allow_db': '*',
                    'password': {
                        'data': '<CENSORED>',
                        'encryption_version': 1,
                    },
                    'login': False,
                    'superuser': False,
                    'allow_port': 6432,
                    'conn_limit': 50,
                    'connect_dbs': ['xdb_test', 'xdb_corp'],
                    'replication': False,
                    'settings': {},
                    'grants': [],
                },
                'fast': {
                    'create': True,
                    'bouncer': True,
                    'allow_db': '*',
                    'password': {
                        'data': '<CENSORED>',
                        'encryption_version': 1,
                    },
                    'superuser': False,
                    'allow_port': 6432,
                    'conn_limit': 50,
                    'connect_dbs': ['xdb_test', 'xdb_corp'],
                    'replication': False,
                    'settings': {},
                    'grants': [],
                },
                'fast2': {
                    'create': True,
                    'bouncer': True,
                    'allow_db': '*',
                    'password': {
                        'data': '<CENSORED>',
                        'encryption_version': 1,
                    },
                    'superuser': False,
                    'allow_port': 6432,
                    'conn_limit': 50,
                    'connect_dbs': ['xdb_test'],
                    'replication': False,
                    'settings': {},
                    'grants': ['writer'],
                },
                'repl': {
                    'create': True,
                    'bouncer': False,
                    'allow_db': '*',
                    'password': {
                        'data': '<CENSORED>',
                        'encryption_version': 1,
                    },
                    'superuser': False,
                    'allow_port': '*',
                    'replication': True,
                    'settings': {},
                },
                'admin': {
                    'create': True,
                    'bouncer': False,
                    'allow_db': '*',
                    'password': {
                        'data': '<CENSORED>',
                        'encryption_version': 1,
                    },
                    'superuser': True,
                    'allow_port': '*',
                    'connect_dbs': ['*'],
                    'replication': False,
                    'settings': {
                        "default_transaction_isolation": "read committed",
                        "log_statement": "mod",
                    },
                },
                'mdb_admin': {
                    'create': True,
                    'bouncer': False,
                    'allow_db': '*',
                    'login': False,
                    'password': {
                        'data': '<CENSORED>',
                        'encryption_version': 1,
                    },
                    'superuser': False,
                    'allow_port': '*',
                    'connect_dbs': ['*'],
                    'replication': False,
                    'settings': {
                        "default_transaction_isolation": "read committed",
                        "log_statement": "mod",
                    },
                },
                'mdb_monitor': {
                    'create': True,
                    'bouncer': False,
                    'allow_db': '*',
                    'login': False,
                    'password': {
                        'data': '<CENSORED>',
                        'encryption_version': 1,
                    },
                    'superuser': False,
                    'allow_port': '*',
                    'connect_dbs': ['*'],
                    'replication': False,
                    'settings': {
                        "default_transaction_isolation": "read committed",
                        "log_statement": "mod",
                    },
                },
                'mdb_replication': {
                    'create': True,
                    'bouncer': False,
                    'allow_db': '*',
                    'login': False,
                    'password': {
                        'data': '<CENSORED>',
                        'encryption_version': 1,
                    },
                    'superuser': False,
                    'allow_port': '*',
                    'connect_dbs': ['*'],
                    'replication': False,
                    'settings': {
                        "default_transaction_isolation": "read committed",
                        "log_statement": "mod",
                    },
                },
            },
            'pgbouncer': {
                'override_pool_mode': {},
            },
            'pool_mode': 'transaction',
            'log_min_duration_statement': '100ms',
        },
        'pgsync': {
            'zk_hosts': 'zk1.test:2181,zk2.test:2181,zk3.test:2181',
            'zk_id': 'test_zk_id',
        },
        'use_wale': False,
        'use_walg': True,
        'sox_audit': True,
        'pgbouncer': {
            'custom_user_pool': True,
            'custom_user_params': ['auth-xdb_test = pool_mode=statement'],
        },
        's3_bucket': 'xdb-bucket',
        'unmanaged_dbs': [
            {
                'xdb_test': {
                    'user': 'fast',
                    'lc_ctype': 'C',
                    'extensions': [],
                    'lc_collate': 'C',
                },
            },
            {
                'xdb_corp': {
                    'user': 'fast',
                    'lc_ctype': 'C',
                    'extensions': [],
                    'lc_collate': 'C',
                },
            },
        ],
        'cluster_private_key': {
            'data': '<CENSORED>',
            'encryption_version': 1,
        },
    },
}

TEMPLATE_PILLAR = {
    'data': {
        'pgbouncer': {
            'custom_user_pool': True,
            'custom_user_params': [],
        },
        'config': {
            'pgusers': {
                'repl': {
                    'create': True,
                    'bouncer': False,
                    'allow_db': '*',
                    'superuser': False,
                    'allow_port': '*',
                    'replication': True,
                },
                'admin': {
                    'create': True,
                    'bouncer': False,
                    'allow_db': '*',
                    'superuser': True,
                    'allow_port': '*',
                    'connect_dbs': ['*'],
                    'replication': False,
                },
                'monitor': {
                    'create': True,
                    'bouncer': True,
                    'allow_db': '*',
                    'superuser': False,
                    'allow_port': '*',
                    'connect_dbs': ['*'],
                    'replication': False,
                },
            },
            'pgbouncer': {
                'override_pool_mode': {},
            },
        },
        'pgsync': {},
        'unmanaged_dbs': [],
        'use_wale': False,
        'use_walg': True,
    },
}

# pylint: disable=missing-docstring, invalid-name


def mk(pillar_dict: dict = None) -> PostgresqlClusterPillar:
    return PostgresqlClusterPillar(deepcopy(pillar_dict or PILLAR))


class TestPostgresqlClusterPillar:
    """
    Test PostgresqlClusterPillar
    """

    # pylint: disable=expression-not-assigned

    def test_get_s3_bucket(self):
        assert mk().s3_bucket == 'xdb-bucket'

    def test_set_s3_bucket(self):
        p = mk()
        p.s3_bucket = 'new-s3-bucket'
        assert_that(p.as_dict(), at_path('$.data.s3_bucket', 'new-s3-bucket'))


class TestConfigPillar:
    def test_update_config(self):
        p = mk()
        p.config.update_config(
            {
                'max_connections': 100500,
                'make_it_fast': True,
            }
        )
        assert_that(
            p.as_dict(),
            at_path(
                '$.data.config',
                has_entries(
                    max_connections=100500,
                    make_it_fast=True,
                ),
            ),
        )

    def test_max_connections(self):
        """
        Should return None, cause they not defined in sample
        """
        p = mk()
        assert p.config.max_connections is None
        p.config.update_config({'max_connections': 42})
        assert p.config.max_connections == 42


class TestPgSyncPillar:
    def test_get_pgsync_zk_hosts(self):
        assert mk().pgsync.get_zk_hosts() == [
            'zk1.test:2181',
            'zk2.test:2181',
            'zk3.test:2181',
        ]

    def test_set_zk_host(self):
        p = mk()
        p.pgsync.set_zk_id_and_hosts('test_abc', ['a:1', 'b:2', 'c:3'])
        assert_that(p.as_dict(), at_path('$.data.pgsync.zk_hosts', 'a:1,b:2,c:3'))

    def test_choice_zk_hosts(self):
        zk_hosts_config = {
            'test-1': ['zkeeper-1-1', 'zkeeper-1-2', 'zkeeper-1-3'],
            'test-2': ['zkeeper-2-1', 'zkeeper-2-2', 'zkeeper-2-3'],
        }
        zk_hosts_usage = [("zkeeper-1-1,zkeeper-1-2,zkeeper-1-3", 1)]
        zk_id, zk_hosts = choice_zk_hosts(zk_hosts_config, zk_hosts_usage)
        assert zk_hosts == ['zkeeper-2-1', 'zkeeper-2-2', 'zkeeper-2-3']
        assert zk_id == 'test-2'


class TestDatabasesPillar:
    def test_get_databases(self):
        p = mk()
        assert_that(
            p.databases.get_databases(),
            contains_inanyorder(
                Database(name='xdb_test', owner='fast', extensions=[], lc_ctype='C', lc_collate='C', template=''),
                Database(name='xdb_corp', owner='fast', extensions=[], lc_ctype='C', lc_collate='C', template=''),
            ),
        )

    def test_get_database(self):
        p = mk()
        assert p.databases.database('xdb_test') == Database(
            name='xdb_test', owner='fast', extensions=[], lc_ctype='C', lc_collate='C', template=''
        )

    def test_get_not_existing_database(self):
        p = mk()
        with pytest.raises(DatabaseNotExistsError):
            p.databases.database('somethingstrange')

    def test_add_database(self):
        p = mk()
        p.databases.add_database(
            Database(
                name='newdb',
                owner='fast',
                extensions=['intarray'],
                lc_ctype='C',
                lc_collate='C',
                template='',
            )
        )
        assert_that(
            p.as_dict(),
            at_path(
                '$.data.unmanaged_dbs',
                has_item(
                    has_entries(
                        newdb=has_entries(
                            user='fast',
                            extensions=['intarray'],
                            lc_ctype='C',
                            lc_collate='C',
                        )
                    )
                ),
            ),
        )

    def test_add_existing_database(self):
        p = mk()
        with pytest.raises(DatabaseExistsError):
            p.databases.add_database(
                Database(
                    name='xdb_test',
                    owner='fast',
                    extensions=['intarray'],
                    lc_ctype='C',
                    lc_collate='C',
                    template='',
                )
            )

    def test_add_database_with_unknown_template(self):
        p = mk()
        with pytest.raises(DatabaseNotExistsError):
            p.databases.add_database(
                Database(
                    name='newdb1',
                    owner='fast',
                    extensions=['intarray'],
                    lc_ctype='C',
                    lc_collate='C',
                    template='foobar',
                )
            )

    def test_add_database_with_incorrect_template(self):
        p = mk()
        with pytest.raises(IncorrectTemplateError):
            p.databases.add_database(
                Database(
                    name='newdb1',
                    owner='fast',
                    extensions=['intarray'],
                    lc_ctype='en_US.UTF-8',
                    lc_collate='en_US.UTF-8',
                    template='xdb_test',
                )
            )

    def test_add_database_with_not_existing_owner(self):
        p = mk()
        with pytest.raises(UserNotExistsError):
            p.databases.add_database(
                Database(
                    name='somenewdatabase',
                    owner='somebodyunknown',
                    extensions=['intarray'],
                    lc_ctype='C',
                    lc_collate='C',
                    template='',
                )
            )

    def test_add_database_initial(self):
        p = mk()
        p.databases.add_database_initial(
            Database(
                name='newdb',
                owner='fast',
                extensions=['intarray'],
                lc_ctype='C',
                lc_collate='C',
                template='',
            )
        )
        assert_that(
            p.as_dict(),
            at_path(
                '$.data.unmanaged_dbs',
                has_item(
                    has_entries(
                        newdb=has_entries(
                            user='fast',
                            extensions=['intarray'],
                            lc_ctype='C',
                            lc_collate='C',
                        )
                    )
                ),
            ),
        )

    def test_add_existing_initial_database(self):
        p = mk()
        with pytest.raises(DatabaseExistsError):
            p.databases.add_database_initial(
                Database(
                    name='xdb_test',
                    owner='fast',
                    extensions=['intarray'],
                    lc_ctype='C',
                    lc_collate='C',
                    template='',
                )
            )

    def test_update_database(self):
        p = mk()
        p.databases.update_database(
            Database(
                name='xdb_test',
                owner='fast',
                extensions=['btree_gist'],
                lc_ctype='de_DE.UTF-8',
                lc_collate='de_DE.UTF-8',
                template='',
            )
        )
        assert_that(
            p.as_dict(),
            at_path(
                '$.data.unmanaged_dbs',
                contains_inanyorder(
                    has_key('xdb_corp'),
                    has_entry(
                        'xdb_test',
                        has_entries(
                            user='fast',
                            extensions=['btree_gist'],
                            lc_ctype='de_DE.UTF-8',
                            lc_collate='de_DE.UTF-8',
                        ),
                    ),
                ),
            ),
        )

    def test_update_not_existing_database(self):
        p = mk()
        with pytest.raises(DatabaseNotExistsError):
            p.databases.update_database(
                Database(
                    name='sosomenewdatabaseme',
                    owner='fast',
                    extensions=['btree_gist'],
                    lc_ctype='de_DE.UTF-8',
                    lc_collate='de_DE.UTF-8',
                    template='',
                )
            )

    def test_update_database_with_not_existing_owner(self):
        p = mk()
        with pytest.raises(UserNotExistsError):
            p.databases.update_database(
                Database(
                    name='xdb_test',
                    owner='somebodyunknown',
                    extensions=['btree_gist'],
                    lc_ctype='de_DE.UTF-8',
                    lc_collate='de_DE.UTF-8',
                    template='',
                )
            )

    def test_delete_database(self):
        p = mk()
        p.databases.delete_database('xdb_corp')
        assert 'xdb_corp' not in p.databases.get_names()
        assert_that(
            p.as_dict(),
            is_not(
                at_path(
                    '$.data.unmanaged_dbs',
                    contains_inanyorder(
                        has_key(
                            'xdb_corp',
                        )
                    ),
                )
            ),
        )

    def test_delete_not_existing_database(self):
        p = mk()
        with pytest.raises(DatabaseNotExistsError):
            p.databases.delete_database('sosomenewdatabaseme')


class TestPgUsersPillar:
    def test_user(self):
        p = mk()
        assert_that(
            p.pgusers.user('fast'),
            UserWithPassword(
                name='fast',
                conn_limit=50,
                encrypted_password={
                    'data': '<CENSORED>',
                    'encryption_version': 1,
                },
                connect_dbs=['xdb_test', 'xdb_corp'],
                settings={},
                login=True,
                grants=[],
            ),
        )
        assert_that(p.pgusers.user('admin'), has_properties({'name': 'admin'}))

    def test_not_existing_user(self):
        p = mk()
        with pytest.raises(UserNotExistsError):
            p.pgusers.user('someoneunknown')

    def test_public_user(self, app_config):
        p = mk()
        assert_that(p.pgusers.public_user('fast'), has_properties({'name': 'fast'}))

    def test_not_existing_public_user(self):
        p = mk()
        with pytest.raises(UserNotExistsError):
            p.pgusers.public_user('someoneunknown')

    def test_getting_system_user_as_public_1(self, app_config):
        p = mk()
        with pytest.raises(UserNotExistsError):
            p.pgusers.public_user('admin')

    def test_getting_system_user_as_public_2(self, app_config):
        p = mk()
        with pytest.raises(UserNotExistsError):
            p.pgusers.public_user('repl')

    def test_get_users(self):
        p = mk()
        assert_that(
            p.pgusers.get_users(),
            contains_inanyorder(
                has_properties({'name': 'fast'}),
                has_properties({'name': 'no_login'}),
                has_properties({'name': 'fast2'}),
                has_properties({'name': 'repl'}),
                has_properties({'name': 'admin'}),
                has_properties({'name': 'mdb_admin'}),
                has_properties({'name': 'mdb_monitor'}),
                has_properties({'name': 'mdb_replication'}),
            ),
        )

    def test_get_public_users(self, app_config):
        p = mk()
        assert_that(
            p.pgusers.get_public_users(),
            contains(
                has_properties({'name': 'fast'}),
                has_properties({'name': 'fast2'}),
                has_properties({'name': 'no_login'}),
            ),
        )

    def test_get_default_users(self, app_config):
        p = mk()
        assert_that(
            p.pgusers.get_default_users(),
            contains(
                has_properties({'name': 'mdb_admin'}),
                has_properties({'name': 'mdb_monitor'}),
                has_properties({'name': 'mdb_replication'}),
            ),
        )

    def test_add_user(self, app_config):
        p = mk()
        p.pgusers.add_user(
            UserWithPassword(
                name='blah',
                encrypted_password=gen_encrypted_password(),
                connect_dbs=['xdb_test'],
                conn_limit=20,
                settings={},
                login=False,
                grants=['fast', 'reader'],
            )
        )
        assert 'blah' in p.pgusers.get_names()
        assert_that(
            p.as_dict(),
            at_path(
                '$.data.config.pgusers',
                has_entry(
                    'blah',
                    has_entries(
                        {
                            'password': not_none(),
                            'create': True,
                            'bouncer': True,
                            'allow_db': '*',
                            'superuser': False,
                            'allow_port': 6432,
                            'conn_limit': 20,
                            'connect_dbs': ['xdb_test'],
                            'replication': False,
                            'login': False,
                            'grants': ['fast', 'reader'],
                        }
                    ),
                ),
            ),
        )

    def test_add_user_without_permissions(self, app_config):
        p = mk()
        p.pgusers.add_user(
            UserWithPassword(
                name='blah',
                encrypted_password=gen_encrypted_password(),
                connect_dbs=[],
                conn_limit=20,
                settings={},
                login=True,
                grants=[],
            )
        )
        assert_that(
            p.as_dict(),
            at_path('$.data.config.pgusers.blah.connect_dbs'),
            [],
        )

    def test_add_existing_user(self, app_config):
        p = mk()
        with pytest.raises(UserExistsError):
            p.pgusers.add_user(
                UserWithPassword(
                    name='fast',
                    encrypted_password=gen_encrypted_password(),
                    connect_dbs=['xdb_test'],
                    conn_limit=20,
                    settings={},
                    login=True,
                    grants=[],
                )
            )

    def test_user_with_not_existing_role(self, app_config):
        p = mk()
        with pytest.raises(UserNotExistsError):
            p.pgusers.add_user(
                UserWithPassword(
                    name='fast3',
                    encrypted_password=gen_encrypted_password(),
                    connect_dbs=['xdb_test'],
                    conn_limit=20,
                    settings={},
                    login=True,
                    grants=['not_idm_role'],
                )
            )

    def test_add_user_with_not_existing_db_in_permissions(self, app_config):
        p = mk()
        with pytest.raises(DatabaseNotExistsError):
            p.pgusers.add_user(
                UserWithPassword(
                    name='blah2',
                    encrypted_password=gen_encrypted_password(),
                    connect_dbs=['someunknowndb'],
                    conn_limit=20,
                    settings={},
                    login=True,
                    grants=[],
                )
            )

    def test_update_user(self, app_config):
        p = mk()
        p.pgusers.update_user(
            UserWithPassword(
                name='fast',
                encrypted_password=gen_encrypted_password(),
                connect_dbs=['xdb_test', 'xdb_corp'],
                conn_limit=20,
                settings={'default_transaction_isolation': 'read committed'},
                login=False,
                grants=[],
            )
        )
        assert_that(
            p.as_dict(),
            at_path(
                '$.data.config.pgusers',
                has_entry(
                    'fast',
                    has_entries(
                        {
                            'password': not_none(),
                            'conn_limit': 20,
                            'connect_dbs': ['xdb_test', 'xdb_corp'],
                            'settings': {'default_transaction_isolation': 'read committed'},
                            'login': False,
                            'grants': [],
                        }
                    ),
                ),
            ),
        )

    def test_update_not_existing_user(self, app_config):
        p = mk()
        with pytest.raises(UserNotExistsError):
            p.pgusers.update_user(
                UserWithPassword(
                    name='blah',
                    encrypted_password=gen_encrypted_password(),
                    connect_dbs=['xdb_test'],
                    conn_limit=20,
                    settings={},
                    login=True,
                    grants=[],
                )
            )

    def test_update_user_with_not_existing_db_in_permissions(self, app_config):
        p = mk()
        with pytest.raises(DatabaseNotExistsError):
            p.pgusers.update_user(
                UserWithPassword(
                    name='fast',
                    encrypted_password=gen_encrypted_password(),
                    connect_dbs=['someunknowndb'],
                    conn_limit=20,
                    settings={},
                    login=True,
                    grants=[],
                )
            )

    def test_delete_user(self, app_config):
        p = mk()
        p.pgusers.delete_user('fast2')
        assert 'fast2' not in p.pgusers.get_names()
        assert_that(p.as_dict(), at_path('$.data.config.pgusers', is_not(has_key('fast2'))))

    def test_not_existing_delete_user(self):
        p = mk()
        with pytest.raises(UserNotExistsError):
            p.pgusers.delete_user('someunknownuser')

    def test_delete_some_db_owner(self):
        p = mk()
        with pytest.raises(UserInvalidError):
            # impossible to delete dabatase owner
            p.pgusers.delete_user('fast')

    def test_add_user_permission(self, app_config):
        p = mk()
        p.pgusers.add_user_permission('fast2', {'database_name': 'xdb_corp'})
        assert_that(p.pgusers.user_has_permission('fast2', 'xdb_corp'))
        assert_that(
            p.as_dict(),
            at_path('$.data.config.pgusers.fast2.connect_dbs'),
            contains_inanyorder('xdb_test', 'xdb_corp'),
        )

    def test_add_existing_user_permission(self):
        p = mk()
        with pytest.raises(DatabaseAccessExistsError):
            p.pgusers.add_user_permission('fast', {'database_name': 'xdb_corp'})

    def test_add_permission_to_not_existing_user(self):
        p = mk()
        with pytest.raises(UserNotExistsError):
            p.pgusers.add_user_permission('someunknownuser', {'database_name': 'xdb_corp'})

    def test_add_permission_on_not_existing_database(self):
        p = mk()
        with pytest.raises(DatabaseNotExistsError):
            p.pgusers.add_user_permission('fast', {'database_name': 'someunknowndb'})

    def test_delete_user_permission(self, app_config):
        p = mk()
        p.pgusers.delete_user_permission('fast2', 'xdb_test')
        assert_that(
            p.as_dict(),
            at_path('$.data.config.pgusers.fast2.connect_dbs'),
            ['xdb_corp'],
        )

    def test_delete_not_existing_user_permission(self, app_config):
        p = mk()
        with pytest.raises(DatabaseAccessNotExistsError):
            p.pgusers.delete_user_permission('fast2', 'xdb_corp')

    def test_delete_permission_of_not_existing_user(self):
        p = mk()
        with pytest.raises(UserNotExistsError):
            p.pgusers.delete_user_permission('someunknownuser', 'xdb_corp')

    def test_delete_permission_on_unknown_database(self):
        p = mk()
        with pytest.raises(DatabaseNotExistsError):
            p.pgusers.delete_user_permission('fast', 'someunknowndb')

    def test_delete_owner_s_permissions(self):
        p = mk()
        with pytest.raises(UserInvalidError):
            p.pgusers.delete_user_permission('fast', 'xdb_test')


class TestPgBouncerPillar:
    def test_add_custom_user_param(self):
        p = mk()
        p.pgbouncer.add_custom_user_param('make_it_fast = yes')
        assert_that(p.as_dict(), at_path('$.data.pgbouncer.custom_user_params', has_item('make_it_fast = yes')))
