# coding: utf-8

from __future__ import print_function, unicode_literals

import os
import psycopg2
import pwd

from cloud.mdb.salt.salt._modules import mdb_postgresql as pg_modules
from cloud.mdb.salt.salt._states import mdb_postgresql
from cloud.mdb.internal.python.pytest.utils import parametrize
from cloud.mdb.salt_tests.common.mocks import mock_pillar, mock_get_md5_encrypted_password
from functools import partial

host = os.environ.get('SALT_POSTGRESQL_RECIPE_HOST')
user = pwd.getpwuid(os.getuid()).pw_name
dsn = 'host={host} user={user} dbname=postgres'.format(host=host, user=user)


def call_sql(sql):
    with psycopg2.connect(dsn) as conn:
        cursor = conn.cursor()
        cursor.execute(sql)
        conn.commit()


def check_acl(relname):
    with psycopg2.connect(dsn) as conn:
        cursor = conn.cursor()
        cursor.execute('SELECT relacl FROM pg_class WHERE relname = %(relname)s', {'relname': relname})
        res = cursor.fetchone()[0]
        return res.replace('"', '').replace('//', '').replace('\\', '')


@parametrize(
    {
        'id': 'Create users',
        'args': {
            'pillar': {
                'data': {
                    'config': {
                        'pgusers': {
                            user: {'password': '', 'create': True},
                            'user1': {'password': '', 'create': True},
                            'User2': {'password': '', 'create': True},
                            'User3': {'password': '', 'create': True},
                        }
                    },
                    'dbaas': {'shard_hosts': 'host1'},
                },
            },
            'args': ['sync_users'],
            'func': call_sql,
            'sql': 'CREATE TABLE "Tt" (t int); '
            'CREATE TABLE "t" (t int);'
            'GRANT ALL ON "Tt" to user1 WITH GRANT OPTION;'
            'GRANT ALL ON "t" to user1 WITH GRANT OPTION;'
            'SET SESSION AUTHORIZATION user1;'
            'CREATE TABLE "t\""T" (t int);'
            'GRANT ALL ON "t\""T" to "User2" WITH GRANT OPTION;'
            'GRANT ALL ON "Tt" to "User2" WITH GRANT OPTION;'
            'SET SESSION AUTHORIZATION "User2";'
            'GRANT ALL ON "t\""T" to "User3" WITH GRANT OPTION;'
            'GRANT ALL ON "Tt" to "User3"',
            'check_func': check_acl,
            'table_acl': {
                'Tt': '{{{user}=arwdDxt/{user},'
                'user1=a*r*w*d*D*x*t*/{user},User2=a*r*w*d*D*x*t*/user1,User3=arwdDxt/User2}}'.format(user=user),
                't': '{{{user}=arwdDxt/{user},user1=a*r*w*d*D*x*t*/{user}}}'.format(user=user),
                't\"T': '{user1=arwdDxt/user1,User2=a*r*w*d*D*x*t*/user1,User3=a*r*w*d*D*x*t*/User2}',
            },
            'opts': {'test': False},
            'result': {
                'name': 'sync_users',
                'result': True,
                'changes': {'user1': 'create', 'User2': 'create', 'User3': 'create'},
                'comment': '',
            },
        },
    },
    {
        'id': 'Drop User3',
        'args': {
            'pillar': {
                'data': {
                    'config': {
                        'pgusers': {
                            user: {'password': 'no password', 'create': True},
                            'user1': {'password': 'no password', 'create': True},
                        }
                    },
                    'dbaas': {'shard_hosts': 'host1'},
                },
                'target-user': 'User2',
            },
            'args': ['sync_users'],
            'func': None,
            'sql': None,
            'check_func': check_acl,
            'table_acl': {
                'Tt': '{{{user}=arwdDxt/{user},user1=a*r*w*d*D*x*t*/{user},User3=arwdDxt/user1}}'.format(user=user),
                't': '{{{user}=arwdDxt/{user},user1=a*r*w*d*D*x*t*/{user}}}'.format(user=user),
                't\"T': '{user1=arwdDxt/user1,User3=a*r*w*d*D*x*t*/user1}',
            },
            'opts': {'test': False},
            'result': {'name': 'sync_users', 'result': True, 'changes': {'User2': 'drop'}, 'comment': ''},
        },
    },
)
def test_sync_users(pillar, args, opts, result, func, sql, table_acl, check_func):
    mock_pillar(mdb_postgresql.__salt__, pillar)
    mock_pillar(pg_modules.__salt__, pillar)
    mdb_postgresql.__salt__['mdb_postgresql.get_connection'] = partial(
        pg_modules.get_connection, database='postgres', user=user, host=host
    )
    mdb_postgresql.__salt__['mdb_postgresql.is_in_recovery'] = pg_modules.is_in_recovery
    mdb_postgresql.__salt__['mdb_postgresql.list_users'] = pg_modules.list_users
    mdb_postgresql.__salt__['mdb_postgresql.get_md5_encrypted_password'] = mock_get_md5_encrypted_password
    mdb_postgresql.__salt__['mdb_postgresql.create_user'] = pg_modules.create_user
    mdb_postgresql.__salt__['mdb_postgresql.get_user'] = pg_modules.get_user
    mdb_postgresql.__salt__['mdb_postgresql.modify_user'] = pg_modules.modify_user
    mdb_postgresql.__salt__['mdb_postgresql.terminate_user_connections'] = pg_modules.terminate_user_connections
    mdb_postgresql.__salt__['mdb_postgresql.get_databases_with_owners'] = pg_modules.get_databases_with_owners
    mdb_postgresql.__salt__['mdb_postgresql.revoke_acl_user'] = pg_modules.revoke_acl_user
    mdb_postgresql.__salt__['mdb_postgresql.reassign_doomed_user_objects'] = pg_modules.reassign_doomed_user_objects
    mdb_postgresql.__salt__['mdb_postgresql.drop_user'] = pg_modules.drop_user

    mdb_postgresql.__opts__ = opts
    res = mdb_postgresql.sync_users(*args)
    if func:
        func(sql)
    assert res == result
    for table in table_acl:
        assert check_func(table) == table_acl[table]
