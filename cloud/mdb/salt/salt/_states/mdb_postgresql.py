# -*- coding: utf-8 -*-
'''
PostgreSQL state for salt
'''
from __future__ import absolute_import, print_function, unicode_literals

from ast import literal_eval
from collections import defaultdict
from contextlib import closing
from copy import deepcopy
from traceback import format_exc

import re

# Initialize salt built-ins here (will be overwritten on lazy-import)
__opts__ = {}
__salt__ = {}


def __virtual__():
    '''
    Only load if the mdb_postgresql module is present
    '''
    if 'mdb_postgresql.get_connection' not in __salt__:
        return (False, 'Unable to load mdb_postgresql module.')
    return True


_1C_EXTENSIONS = ['mchar', 'fulleq', 'fasttrun', 'yndx_1c_aux']
_HIDDEN_EXTENSIONS = ['logerrors', 'mdb_perf_diag', 'lwaldump', 'plpgsql']

_EXTENSIONS_DEFAULT_ACLS = {
    'pg_stat_kcache': [
        {
            'grant': 'GRANT EXECUTE ON FUNCTION pg_stat_kcache_reset() TO mdb_admin',
            'check': "SELECT 1 FROM pg_proc WHERE proname='pg_stat_kcache_reset' AND proacl::text~'mdb_admin'",
        }
    ],
    'pg_stat_statements': [
        {
            'grant': 'GRANT EXECUTE ON FUNCTION pg_stat_statements_reset TO mdb_admin',
            'check': "SELECT 1 FROM pg_proc WHERE proname='pg_stat_statements_reset' AND proacl::text~'mdb_admin'",
        }
    ],
    'pg_repack': [
        {
            'grant': 'GRANT ALL ON SCHEMA repack TO mdb_admin',
            'check': 'SELECT 1 FROM pg_namespace WHERE '
            "nspname = 'repack' AND nspacl::text LIKE '%mdb_admin=UC/postgres%'",
        },
        {
            'grant': 'GRANT SELECT ON ALL tables IN SCHEMA repack TO mdb_admin',
            'check': "SELECT 1 FROM information_schema.role_table_grants WHERE table_schema='repack' AND "
            "grantee = 'mdb_admin' AND privilege_type = 'SELECT'",
        },
    ],
    'postgres_fdw': [
        {
            'grant': 'GRANT USAGE ON FOREIGN DATA WRAPPER postgres_fdw TO mdb_admin',
            'check': "SELECT 1 FROM pg_foreign_data_wrapper WHERE fdwname='postgres_fdw' AND fdwacl::text~'mdb_admin'",
        }
    ],
    'clickhouse_fdw': [
        {
            'grant': 'GRANT USAGE ON FOREIGN DATA WRAPPER clickhouse_fdw TO mdb_admin',
            'check': "SELECT 1 FROM pg_foreign_data_wrapper WHERE fdwname='clickhouse_fdw' AND fdwacl::text~'mdb_admin'",
            'versions': [11, 12, 13],
        }
    ],
    'oracle_fdw': [
        {
            'grant': 'GRANT USAGE ON FOREIGN DATA WRAPPER oracle_fdw TO mdb_admin',
            'check': "SELECT 1 FROM pg_foreign_data_wrapper WHERE fdwname='oracle_fdw' AND fdwacl::text~'mdb_admin'",
        }
    ],
    'dblink': [
        {
            'grant': 'GRANT USAGE ON FOREIGN DATA WRAPPER dblink_fdw TO mdb_admin',
            'check': "SELECT 1 FROM pg_foreign_data_wrapper WHERE fdwname='dblink_fdw' AND fdwacl::text~'mdb_admin'",
        }
    ],
    'postgis_topology': [
        {
            'grant': 'ALTER SCHEMA topology OWNER TO mdb_admin',
            'check': "SELECT 1 FROM pg_namespace n JOIN pg_roles r ON (n.nspowner=r.oid) WHERE "
            "nspname = 'topology' AND rolname='mdb_admin'",
        },
        {
            'grant': 'GRANT ALL ON ALL TABLES IN SCHEMA topology TO mdb_admin WITH GRANT OPTION',
            'check': "SELECT 1 FROM information_schema.role_table_grants WHERE table_schema='topology' AND "
            "grantee = 'mdb_admin' AND privilege_type = 'SELECT'",
        },
        {
            'grant': 'GRANT USAGE, SELECT ON ALL SEQUENCES IN SCHEMA topology TO mdb_admin WITH GRANT OPTION',
            'check': "SELECT 1 FROM pg_class WHERE relkind = 'S' AND relname = 'topology_id_seq' AND "
            "relacl::text LIKE '%postgres=rwU/postgres,mdb_admin=r*U*/postgres%'",
        },
    ],
    'pg_cron': [
        {
            'grant': 'GRANT USAGE ON SCHEMA cron TO mdb_admin',
            'check': 'SELECT 1 FROM pg_namespace WHERE '
            "nspname = 'cron' AND nspacl::text LIKE '%mdb_admin=U/postgres%'",
        },
        {
            'grant': 'GRANT EXECUTE ON FUNCTION cron.schedule_in_database to mdb_admin',
            'check': 'SELECT 1 FROM pg_proc p LEFT JOIN pg_namespace n ON n.oid = p.pronamespace '
            "WHERE n.nspname = 'cron' AND proname = 'schedule_in_database' AND proacl::text LIKE '%mdb_admin=X/postgres%'",
        },
        {
            'grant': 'GRANT EXECUTE ON FUNCTION cron.alter_job to mdb_admin',
            'check': 'SELECT 1 FROM pg_proc p LEFT JOIN pg_namespace n ON n.oid = p.pronamespace '
            "WHERE n.nspname = 'cron' AND proname = 'alter_job' AND proacl::text LIKE '%mdb_admin=X/postgres%'",
        },
        {
            'grant': "ALTER TABLE cron.job ADD CONSTRAINT cron_db_check CHECK (database not in ('postgres', 'template0', 'template1'))",
            'check': "SELECT 1 FROM pg_constraint c INNER JOIN pg_class r ON r.oid = c.conrelid INNER JOIN pg_namespace n ON n.oid = connamespace "
            "WHERE n.nspname = 'cron' AND r.relname = 'job' and c.conname = 'cron_db_check'",
        },
    ],
}

_DEFAULT_ACLS = [
    {
        'grant': 'ALTER DEFAULT PRIVILEGES IN SCHEMA public GRANT select, insert, update, delete ON TABLES TO %(owner)s WITH GRANT OPTION',
        'check': "SELECT 1 FROM pg_catalog.pg_default_acl "
        "WHERE pg_default_acl.defaclacl::text LIKE '%%' || replace(quote_ident(%(owner)s), '\"', '\\\"') || '=a*r*w*d*/postgres%%'",
    },
    {
        'grant': 'GRANT EXECUTE ON FUNCTION pg_stat_reset() TO mdb_admin',
        'check': "SELECT 1 FROM pg_proc WHERE proname='pg_stat_reset' AND proacl::text~'mdb_admin'",
    },
    {
        'grant': 'GRANT EXECUTE ON FUNCTION pg_get_backend_memory_contexts() TO mdb_admin WITH GRANT OPTION',
        'check': "SELECT 1 FROM pg_proc WHERE proname='pg_get_backend_memory_contexts' AND proacl::text~'mdb_admin'",
        'versions': [14],
    },
    {
        'grant': 'GRANT SELECT ON pg_backend_memory_contexts TO mdb_admin WITH GRANT OPTION',
        'check': "SELECT 1 FROM pg_class WHERE relname='pg_backend_memory_contexts' AND relacl::text~'mdb_admin'",
        'versions': [14],
    },
]


def sync_physical_replication_slots(name):
    """
    Sync physical replication slots with dbaas shard hosts
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}

    try:
        with closing(__salt__['mdb_postgresql.get_connection']()) as conn:
            with conn as txn:
                cursor = txn.cursor()
                existing = __salt__['mdb_postgresql.list_physical_replication_slots'](cursor)
                target = __salt__['mdb_postgresql.get_required_replication_slots'](cursor)
                for slot in existing:
                    if slot not in target:
                        if not __opts__.get('test'):
                            __salt__['mdb_postgresql.drop_replication_slot'](slot, cursor)
                        ret['changes'][slot] = 'drop'
                for slot in target:
                    if slot not in existing:
                        if not __opts__.get('test'):
                            __salt__['mdb_postgresql.create_physical_replication_slot'](slot, cursor)
                        ret['changes'][slot] = 'create'
    except Exception:
        ret['result'] = False
        ret['comment'] += format_exc()

    if ret['changes'] and __opts__.get('test') and ret['result']:
        ret['result'] = None
    return ret


def _set_system_users_settings(users):
    # We need to set some settings for "our" users (for example search_path)
    for user in ['admin', 'postgres', 'monitor', 'repl']:
        if user in users:
            if 'settings' not in users[user]:
                users[user]['settings'] = {}
            users[user]['settings']['search_path'] = 'public'
            users[user]['settings']['default_transaction_read_only'] = 'off'
            users[user]['settings']['pg_hint_plan.enable_hint'] = 'off'
            users[user]['settings']['standard_conforming_strings'] = 'on'
            users[user]['settings']['pg_hint_plan.enable_hint'] = 'off'

            if user == 'monitor':
                # MDB-6999 limit monitor conn_limit
                if 'conn_limit' not in users[user]:
                    users[user]['conn_limit'] = 14
                # do not log anything for monitor
                users[user]['settings']['log_min_messages'] = 'panic'
                users[user]['settings']['log_statement'] = 'none'
                users[user]['settings']['log_min_error_statement'] = 'panic'
                users[user]['settings']['log_min_duration_statement'] = '-1'


def _get_expected_users():
    # Due to some implicit user creations we can't just get pillar values
    users = deepcopy(
        {key: value for key, value in __salt__['pillar.get']('data:config:pgusers').items() if value.get('create')}
    )
    for options in users.values():
        if 'login' not in options:
            options['login'] = True
        # Set log_statement = all for all users with roles created by IDM
        if options.get('origin') == 'idm' and set({'reader', 'writer'}) & set(options.get('grants', [])):
            if 'settings' not in options:
                options['settings'] = {}
            options['settings']['log_statement'] = 'all'
    if __salt__['pillar.get']('data:dbaas:shard_hosts'):
        if 'mdb_admin' in users:
            if 'grants' not in users['mdb_admin']:
                users['mdb_admin']['grants'] = []
            if 'pg_monitor' not in users['mdb_admin']['grants']:
                users['mdb_admin']['grants'].append('pg_monitor')
            if 'pg_signal_backend' not in users['mdb_admin']['grants']:
                users['mdb_admin']['grants'].append('pg_signal_backend')
        if 'mdb_monitor' in users:
            if 'grants' not in users['mdb_monitor']:
                users['mdb_monitor']['grants'] = []
            if 'pg_monitor' not in users['mdb_monitor']['grants']:
                users['mdb_monitor']['grants'].append('pg_monitor')

    if 'monitor' in users and 'pg_monitor' not in users['monitor'].get('grants', []):
        if 'grants' not in users['monitor']:
            users['monitor']['grants'] = []
        users['monitor']['grants'].append('pg_monitor')
    _set_system_users_settings(users)
    if __salt__['pillar.get']('data:sox_audit', False):
        users.update(
            {
                'reader': {
                    'login': False,
                    'conn_limit': -1,
                    'settings': {},
                },
                'writer': {
                    'login': False,
                    'conn_limit': -1,
                    'settings': {
                        'log_statement': 'mod',
                    },
                },
            }
        )
        users['postgres']['settings']['log_statement'] = 'all'
    return users


def _drop_doomed_users(ret, doomed, name_filter, cursor):
    for user in doomed:
        if not __opts__.get('test') and user == name_filter:
            __salt__['mdb_postgresql.terminate_user_connections'](user, cursor)
            for database, datallowconn, owner in __salt__['mdb_postgresql.get_databases_with_owners'](cursor):
                if not datallowconn:
                    cursor.execute(
                        __salt__['mdb_postgresql.cmd_obj_escape'](
                            cursor.mogrify(
                                'ALTER DATABASE %(database)s WITH ALLOW_CONNECTIONS true', {'database': database}
                            ),
                            database,
                        )
                    )

                with closing(__salt__['mdb_postgresql.get_connection'](database=database)) as conn:
                    with conn as txn:
                        db_cursor = txn.cursor()
                        __salt__['mdb_postgresql.revoke_acl_user'](user, db_cursor)
                        __salt__['mdb_postgresql.reassign_doomed_user_objects'](user, owner, db_cursor)
            __salt__['mdb_postgresql.drop_user'](user, cursor)
        ret['changes'][user] = 'drop'


def _create_new_users(ret, new_users, expected, cursor):
    for user in new_users:
        if not __opts__.get('test'):
            __salt__['mdb_postgresql.create_user'](user, expected[user], cursor)
        ret['changes'][user] = 'create'


_USER_SETTINGS_RESET_WHITELIST = [
    'default_transaction_isolation',
    'lock_timeout',
    'log_min_duration_statement',
    'log_statement',
    'synchronous_commit',
    'temp_file_limit',
]

_ODYSSEY_USER_SETTINGS = [
    'pool_mode',
    'usr_pool_reserve_prepaped_statements',
    'catchup_timeout',
]


def _sync_existing_users(ret, users, expected, cursor):
    for user in users:
        changes = {}
        in_db = __salt__['mdb_postgresql.get_user'](user, cursor)
        for key in ['superuser', 'login', 'conn_limit', 'replication', 'encrypted_password']:
            if key not in expected[user]:
                continue
            if expected[user][key] != in_db[key]:
                changes[key] = expected[user][key]
        for setting, value in expected[user].get('settings', {}).items():
            if setting in _ODYSSEY_USER_SETTINGS:
                continue
            if str(value) != in_db['settings'].get(setting):
                if 'settings' not in changes:
                    changes['settings'] = {}
                changes['settings'][setting] = value
        for setting in in_db['settings']:
            if setting not in expected[user].get('settings', {}) and setting in _USER_SETTINGS_RESET_WHITELIST:
                if 'settings_removals' not in changes:
                    changes['settings_removals'] = []
                changes['settings_removals'].append(setting)
        if changes:
            if not __opts__.get('test'):
                __salt__['mdb_postgresql.modify_user'](user, changes, cursor)
            ret['changes'][user] = changes


def sync_users(name):
    """
    Sync postgresql users with pillar data
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    name_filter = __salt__['pillar.get']('target-user', None)

    try:
        with closing(__salt__['mdb_postgresql.get_connection']()) as conn:
            conn.autocommit = True
            cursor = conn.cursor()
            if __salt__['mdb_postgresql.is_in_recovery'](cursor):
                ret['comment'] = 'Skipping on replica'
                return ret
            existing = __salt__['mdb_postgresql.list_users'](cursor)
            expected = _get_expected_users()
            for user, options in expected.items():
                if options['login'] and options.get('password'):
                    options['encrypted_password'] = __salt__['mdb_postgresql.get_md5_encrypted_password'](
                        user, options['password']
                    )
            doomed = set(existing).difference(set(expected.keys()))
            if name_filter:
                doomed = [x for x in doomed if x == name_filter]
            _drop_doomed_users(ret, doomed, name_filter, cursor)
            new_users = set(expected.keys()).difference(set(existing))
            if name_filter:
                new_users = {x for x in new_users if x == name_filter}
            _create_new_users(ret, new_users, expected, cursor)
            other = set(expected.keys()) - new_users
            if name_filter:
                other = [x for x in other if x == name_filter]
            _sync_existing_users(ret, other, expected, cursor)
    except Exception:
        ret['result'] = False
        ret['comment'] += format_exc()

    if ret['changes'] and __opts__.get('test') and ret['result']:
        ret['result'] = None
    return ret


def sync_user_grants(name):
    """
    Sync postgresql grants for all users with pillar data
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    name_filter = __salt__['pillar.get']('target-user', None)

    try:
        with closing(__salt__['mdb_postgresql.get_connection']()) as conn:
            with conn as txn:
                cursor = txn.cursor()
                if __salt__['mdb_postgresql.is_in_recovery'](cursor):
                    ret['comment'] = 'Skipping on replica'
                    return ret
                users = _get_expected_users()
                for user, options in users.items():
                    if name_filter and user != name_filter:
                        continue
                    in_db = __salt__['mdb_postgresql.get_user_role_membership'](user, cursor)
                    new_roles = set(options.get('grants', [])).difference(set(in_db))
                    for role in new_roles:
                        if not __opts__.get('test'):
                            __salt__['mdb_postgresql.assign_role_to_user'](user, role, cursor)
                        if user not in ret['changes']:
                            ret['changes'][user] = {}
                        ret['changes'][user][role] = 'assign'
                    stale_roles = set(in_db).difference(set(options.get('grants', [])))
                    for role in stale_roles:
                        if not __opts__.get('test'):
                            __salt__['mdb_postgresql.revoke_role_from_user'](user, role, cursor)
                        if user not in ret['changes']:
                            ret['changes'][user] = {}
                        ret['changes'][user][role] = 'revoke'
    except Exception:
        ret['result'] = False
        ret['comment'] += format_exc()

    if ret['changes'] and __opts__.get('test') and ret['result']:
        ret['result'] = None
    return ret


def _drop_doomed_databases(ret, doomed, name_filter, allow_conn_map, cursor):
    for database in doomed:
        if not __opts__.get('test') and name_filter == database:
            if allow_conn_map[database]:
                with closing(__salt__['mdb_postgresql.get_connection'](database=database)) as conn:
                    with conn as txn:
                        db_cursor = txn.cursor()
                        __salt__['mdb_postgresql.drop_database_subscriptions'](db_cursor)
                __salt__['mdb_postgresql.terminate_database_connections'](database, cursor)
            __salt__['mdb_postgresql.drop_database'](database, cursor)
        ret['changes'][database] = 'drop'


def _owner_format_query(cursor, query, owner):
    if '(owner)s' in query:
        return cursor.mogrify(query, {'owner': owner})
    return query


def _owner_format_grant(cursor, query, owner):
    if '(owner)s' in query:
        return __salt__['mdb_postgresql.cmd_obj_escape'](cursor.mogrify(query, {'owner': owner}), owner)
    return query


def _sync_default_acl(owner, cursor):
    """
    Sync "default" ACLs (Access Control List)
    """
    pg_version = int(__salt__['pillar.get']('data:versions:postgres:major_version', 10))
    changes = {}
    for acl in _DEFAULT_ACLS:
        target_versions = acl.get('versions', [])
        if target_versions and pg_version not in target_versions:
            continue
        cursor.execute(_owner_format_query(cursor, acl['check'], owner))
        if not cursor.fetchall():
            grant_sql = _owner_format_grant(cursor, acl['grant'], owner)
            if not __opts__.get('test'):
                cursor.execute(grant_sql)
            changes[grant_sql] = 'execute'
    return changes


def _sync_database_acl(database, expected_users, cursor):
    acl = __salt__['mdb_postgresql.get_database_acl'](database, cursor)
    expected_acl = set()
    changes = {}
    name_filter = __salt__['pillar.get']('target-user', None)
    for user, options in expected_users.items():
        conns = options.get('connect_dbs', [])
        if options['login'] and (database in conns or '*' in conns):
            expected_acl.add(user)
    doomed = acl.difference(expected_acl)
    for user in doomed:
        if name_filter and user != name_filter:
            continue
        if not __opts__.get('test'):
            __salt__['mdb_postgresql.revoke_database_connect'](database, user, cursor)
        changes[user] = 'revoke connect'
    new_grants = expected_acl.difference(acl)
    for user in new_grants:
        if name_filter and user != name_filter:
            continue
        if not __opts__.get('test'):
            __salt__['mdb_postgresql.grant_database_connect'](database, user, cursor)
        changes[user] = 'grant connect'
    return changes


def _create_new_databases(ret, new_databases, expected, cursor):
    for database in new_databases:
        if not __opts__.get('test'):
            __salt__['mdb_postgresql.create_database'](
                database,
                expected[database]['user'],
                cursor,
                expected[database].get('lc_ctype', 'C'),
                expected[database].get('lc_collate', 'C'),
                expected[database].get('template', 'template0'),
            )
            _sync_database({'changes': {database: {}}}, database, expected, True, cursor)
        ret['changes'][database] = 'create'


def _sync_existing_databases(ret, databases, expected, allow_conn_map, cursor):
    for database in databases:
        _sync_database(ret, database, expected, allow_conn_map[database], cursor)


def _check_use1c():
    return '1c' == __salt__['pillar.get']('data:versions:postgres:edition')


def _sync_database(ret, database, expected, allow_conn, cursor):
    database_changes = {}
    if not allow_conn:
        cursor.execute(
            __salt__['mdb_postgresql.cmd_obj_escape'](
                cursor.mogrify('ALTER DATABASE %(database)s WITH ALLOW_CONNECTIONS true', {'database': database}),
                database,
            )
        )

    expected_users = _get_expected_users()
    with closing(__salt__['mdb_postgresql.get_connection'](database=database)) as conn:
        with conn as txn:
            db_cursor = txn.cursor()
            default_acl_changes = _sync_default_acl(expected[database]['user'], db_cursor)
            if default_acl_changes:
                database_changes.update(default_acl_changes)
    acl_changes = _sync_database_acl(database, expected_users, cursor)
    if acl_changes:
        database_changes.update(acl_changes)
    if database_changes:
        ret['changes'][database] = database_changes


def _check_db_exists(db_name, cursor):
    cursor.execute(
        cursor.mogrify(
            'SELECT datname FROM pg_catalog.pg_database WHERE lower(datname) = lower(%(db_name)s);',
            {'db_name': db_name},
        ),
    )
    return bool(cursor.fetchone())


def _rename_database(ret, old_name, new_name, cursor):
    cursor.execute(
        __salt__['mdb_postgresql.cmd_obj_escape'](
            __salt__['mdb_postgresql.cmd_obj_escape'](
                cursor.mogrify(
                    'ALTER DATABASE %(old_db_name)s RENAME TO %(new_db_name)s',
                    {
                        'old_db_name': old_name,
                        'new_db_name': new_name,
                    },
                ),
                old_name,
            ),
            new_name,
        )
    )
    ret['changes'][old_name] = {'name': new_name}


def sync_databases(name):
    """
    Sync postgresql databases with pillar data
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    name_filter = __salt__['pillar.get']('target-database', None)
    new_db_name = __salt__['pillar.get']('new-database-name', None)

    try:
        with closing(__salt__['mdb_postgresql.get_connection']()) as conn:
            conn.autocommit = True
            cursor = conn.cursor()
            if __salt__['mdb_postgresql.is_in_recovery'](cursor):
                ret['comment'] = 'Skipping on replica'
                return ret

            if name_filter and new_db_name:
                if not _check_db_exists(new_db_name, cursor):
                    _rename_database(ret, name_filter, new_db_name, cursor)
                    name_filter = new_db_name

            existing_data = [
                x
                for x in __salt__['mdb_postgresql.list_databases'](cursor)
                if x['datname'] not in ['postgres', 'template1']
            ]
            existing_set = set(x['datname'] for x in existing_data)
            allow_conn_map = {x['datname']: x['datallowconn'] for x in existing_data}
            expected = {}
            for database in __salt__['pillar.get']('data:unmanaged_dbs'):
                for key, value in database.items():
                    expected[key] = value
            doomed = existing_set.difference(set(expected.keys()))
            if name_filter:
                doomed = [x for x in doomed if x == name_filter]
            _drop_doomed_databases(ret, doomed, name_filter, allow_conn_map, cursor)
            new_databases = set(expected.keys()).difference(existing_set)
            if name_filter:
                new_databases = {x for x in new_databases if x == name_filter}
            _create_new_databases(ret, new_databases, expected, cursor)
            other = set(expected.keys()) - new_databases
            if name_filter:
                other = [x for x in other if x == name_filter]
            _sync_existing_databases(ret, other, expected, allow_conn_map, cursor)

    except Exception:
        ret['result'] = False
        ret['comment'] += format_exc()

    if ret['changes'] and __opts__.get('test') and ret['result']:
        ret['result'] = None
    return ret


def test_relation_exists(name, relnames):
    """
    Given a relation name, ensures there exists such an relation
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    try:
        with closing(__salt__['mdb_postgresql.get_connection']()) as conn:
            with conn as txn:
                cursor = txn.cursor()
                for relname in relnames:
                    query_ensure = "SELECT 1 FROM pg_stat_user_tables WHERE relname= %(relname)s"
                    cursor.execute(query_ensure, {'relname': relname})

                    res = cursor.fetchall()

                    if len(res) == 0:
                        ret['changes'][relname] = "{relname} relation needs to be created".format(relname=relname)
    except Exception:
        ret['result'] = False
        ret['comment'] += format_exc()

    if __opts__.get('test') and len(ret['changes']) != 0:
        ret['result'] = None
    return ret


def extension_present(name, version=None):
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    is_test = __opts__.get('test')
    try:
        with closing(__salt__['mdb_postgresql.get_connection']()) as conn:
            with conn as txn:
                cursor = txn.cursor()
                if __salt__['mdb_postgresql.is_in_recovery'](cursor):
                    ret['comment'] = 'Skipping on replica'
                    return ret
                cursor.execute(
                    cursor.mogrify(
                        "select extversion from pg_catalog.pg_extension where extname = %(extname)s", {'extname': name}
                    )
                )
                result = cursor.fetchone()
                if not result:
                    query = __salt__['mdb_postgresql.add_extension'](name, cursor, version, is_test)
                    ret['changes'][name] = query
                    return ret
                if not version:
                    return ret
                if str(result[0]) != str(version):
                    query = __salt__['mdb_postgresql.update_extension'](name, cursor, version, is_test)
                    ret['changes'][name] = query
    except Exception as e:
        ret['result'] = False
        ret['comment'] += str(e)

    if is_test and len(ret['changes']) != 0:
        ret['result'] = None
    return ret


def extension_present_dbs(name, version=None, recreate=False):
    ret = {'name': name, 'changes': defaultdict(dict), 'result': True, 'comment': ''}
    is_test = __opts__.get('test')

    try:
        with closing(__salt__['mdb_postgresql.get_connection']()) as conn:
            pg_cursor = conn.cursor()
            if __salt__['mdb_postgresql.is_in_recovery'](pg_cursor):
                ret['comment'] = 'Skipping on replica'
                return ret

            databases = [
                db
                for db in __salt__['mdb_postgresql.list_databases'](pg_cursor)
                if db['datname'] not in {'postgres', 'template1'}
            ]

            for db in databases:
                if not db['datallowconn']:
                    pg_cursor.execute(
                        __salt__['mdb_postgresql.cmd_obj_escape'](
                            pg_cursor.mogrify(
                                'ALTER DATABASE %(database)s WITH ALLOW_CONNECTIONS true', {'database': db['datname']}
                            ),
                            db['datname'],
                        )
                    )

                with closing(__salt__['mdb_postgresql.get_connection'](database=db['datname'])) as conn:
                    cursor = conn.cursor()
                    cursor.execute(
                        cursor.mogrify(
                            "SELECT extversion FROM pg_catalog.pg_extension WHERE extname = %(extname)s",
                            {'extname': name},
                        )
                    )
                    result = cursor.fetchone()
                    if not result:
                        continue

                    if not version or str(result[0]) == str(version):
                        continue

                    if recreate:
                        query1 = __salt__['mdb_postgresql.drop_extension'](name, cursor, cascade=True, test=is_test)
                        query2 = __salt__['mdb_postgresql.add_extension'](name, cursor, version, test=is_test)
                        ret['changes'][db['datname']][name] = '{q1}; {q2}'.format(q1=query1, q2=query2)
                    else:
                        query = __salt__['mdb_postgresql.update_extension'](name, cursor, version, is_test)
                        ret['changes'][db['datname']][name] = query
    except Exception as e:
        ret['result'] = False
        ret['comment'] += str(e)

    if is_test and ret['changes']:
        ret['result'] = None

    return ret


def _parse_unmanaged_dbs(unmanaged_dbs):
    """
    Parse normal pillar from smth like this:
    [
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
    ]
    """
    result = {}
    for item in unmanaged_dbs:
        for dbname, dbvalue in item.items():
            result[dbname] = dbvalue
    return result


def _parse_error_message(message):
    """
    Parse psql error message
    """
    if 'cannot drop extension' in message or re.search(r'required extension "[\w\d_]+" is not installed', message):
        message = message.strip()
        if not message.startswith('ERROR'):
            message = 'ERROR:  {}'.format(message)
        return message
    else:
        raise Exception(message)


def _build_extensions_sync_query(new_extensions, doomed_extensions):
    """
    Creates a transaction with creating and dropping extensions.
    Rollback if test, commits otherwise.
    """
    changes = {}
    result = ['BEGIN']
    for extname in doomed_extensions:
        result.append('DROP EXTENSION "{}"'.format(extname))
        changes[extname] = 'drop extension'
    for extname in new_extensions:
        result.append('CREATE EXTENSION IF NOT EXISTS "{}" CASCADE'.format(extname))
        changes[extname] = 'create extension cascade'
    if __opts__.get('test'):
        result.append('ROLLBACK')
    else:
        result.append('COMMIT;')
    return '; '.join(result), changes


def _sync_extensions_acls(cursor, target_extensions):
    """
    Sync extension-dependent ACLs (Access Control List)
    """
    pg_version = int(__salt__['pillar.get']('data:versions:postgres:major_version', 10))
    changes = {}
    for extension in target_extensions:
        if extension not in _EXTENSIONS_DEFAULT_ACLS:
            continue
        for acl in _EXTENSIONS_DEFAULT_ACLS[extension]:
            target_versions = acl.get('versions', [])
            if target_versions and pg_version not in target_versions:
                continue
            cursor.execute(acl['check'])
            if not cursor.fetchall():
                if not __opts__.get('test'):
                    cursor.execute(acl['grant'])
                changes[acl['grant']] = 'execute'
    return changes


def dfs(edges, used, current, result_order):
    if current in used:
        return
    for dependent in edges.get(current, []):
        dfs(edges, used, dependent, result_order)
    result_order.append(current)
    used.add(current)


def get_extensions_order_to_drop(deps, doomed_extensions):
    reverse_deps = dict()
    if deps:
        for dependent_extension in deps:
            for extension in deps[dependent_extension]:
                reverse_deps[extension] = reverse_deps.get(extension, [])
                reverse_deps[extension].append(dependent_extension)
    result_order = []
    used = set()
    for extension in doomed_extensions:
        dfs(reverse_deps, used, extension, result_order)
    return [e for e in result_order if e in doomed_extensions]


def sync_extensions(name):
    """
    Sync extension in target_database.
    If target_database is None, then sync unmanaged_dbs extensions.
    """
    with closing(__salt__['mdb_postgresql.get_connection']()) as conn:
        cursor = conn.cursor()
        if __salt__['mdb_postgresql.is_in_recovery'](cursor):
            return {'name': name, 'changes': {}, 'result': True, 'comment': 'Skipping on replica'}

    errors, changes = [], {}
    target_database = __salt__['pillar.get']('target-database')
    dbs_to_rename = {target_database: __salt__['pillar.get']('new-database-name')}
    unmanaged_dbs = _parse_unmanaged_dbs(__salt__['pillar.get']('data:unmanaged_dbs', []))
    target_databases = [target_database] if target_database else unmanaged_dbs.keys()
    for dbname in target_databases:
        if dbname in ['postgres', 'template0', 'template1']:
            continue
        db = unmanaged_dbs.get(dbname)
        if db is None:
            # We are trying to rename database right now, so the actual db_name isn't match with db_name in the pillar.
            # Try to get db pillar using new db_name.
            db = unmanaged_dbs.get(dbs_to_rename[dbname])
        target_extensions = db.get('extensions', [])
        if _check_use1c():
            target_extensions += _1C_EXTENSIONS
        with closing(__salt__['mdb_postgresql.get_connection'](database=dbname)) as conn:
            conn.autocommit = True
            cursor = conn.cursor()
            cursor.execute('SELECT extname FROM pg_extension')
            actual_extensions = [x[0] for x in cursor.fetchall()]
            new_extensions = set(target_extensions).difference(actual_extensions)
            doomed_extensions = (
                set(actual_extensions)
                .difference(target_extensions)
                .difference(_1C_EXTENSIONS)
                .difference(_HIDDEN_EXTENSIONS)
                .difference(
                    ['pg_repack'] if __salt__['dbaas.is_porto']() else []
                )  # because deleting during a repack can lead to unknown consequences
            )
            extensions_deps = __salt__['pillar.get']('extension-dependencies')
            if extensions_deps:
                # TODO remove following 4 strings after api release
                extensions_deps = extensions_deps[1:-1]
                extensions_deps = extensions_deps.replace('{', '[')
                extensions_deps = extensions_deps.replace('}', ']')
                extensions_deps = '{' + extensions_deps + '}'

                extensions_deps = literal_eval(extensions_deps)
            doomed_extensions = get_extensions_order_to_drop(extensions_deps, doomed_extensions)
            extensions_query, db_changes = _build_extensions_sync_query(new_extensions, doomed_extensions)
            if db_changes:
                try:
                    changes[dbname] = db_changes
                    cursor.execute(extensions_query)
                except Exception as e:
                    errors.append(_parse_error_message(str(e)))
                    continue
            acl_changes = _sync_extensions_acls(cursor, target_extensions)
            if acl_changes:
                db_changes.update(acl_changes)

            if db_changes:
                changes[dbname] = db_changes

    return {'name': name, 'result': True, 'changes': changes, 'comment': '\n'.join(errors)}
