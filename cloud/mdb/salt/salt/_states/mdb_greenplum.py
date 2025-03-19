# -*- coding: utf-8 -*-
'''
Greenplum state for salt
'''
from __future__ import absolute_import, print_function, unicode_literals

from contextlib import closing
from copy import deepcopy
from traceback import format_exc

# Initialize salt built-ins here (will be overwritten on lazy-import)
__opts__ = {}
__salt__ = {}

DEFAULT_RESOURCE_GROUP = 'default_group'
DEFAULT_RESOURCE_QUEUE = 'pg_default'


def __virtual__():
    '''
    Only load if the mdb_greenplum module is present
    '''
    if 'mdb_greenplum.get_connection' not in __salt__:
        return (False, 'Unable to load mdb_greenplum module.')
    return True


def _set_system_users_settings(users):
    # We need to set some settings for "our" users (for example search_path)
    for user in ['gpadmin', 'monitor']:
        if user in users:
            if 'settings' not in users[user]:
                users[user]['settings'] = {}
            users[user]['settings']['search_path'] = 'public'
            users[user]['settings']['default_transaction_read_only'] = 'off'
            users[user]['settings']['pg_hint_plan.enable_hint'] = 'off'
            users[user]['settings']['standard_conforming_strings'] = 'on'
            users[user]['settings']['pg_hint_plan.enable_hint'] = 'off'

            if user == 'monitor':
                # do not log anything for monitor
                users[user]['settings']['log_min_messages'] = 'panic'
                users[user]['settings']['log_statement'] = 'none'
                users[user]['settings']['log_min_error_statement'] = 'panic'
                users[user]['settings']['log_min_duration_statement'] = '-1'


def _get_expected_users():
    users = deepcopy(
        {key: value for key, value in __salt__['pillar.get']('data:greenplum:users').items() if value.get('create')}
    )
    admin_user = __salt__['pillar.get']('data:greenplum:admin_user_name')
    if admin_user in users:
        options = users[admin_user]
        options['login'] = True
        options['createdb'] = True
        options['createrole'] = True
        options['createexttable(type=\'readable\')'] = True
        options['createexttable(type=\'writable\')'] = True
        options['createexttable(protocol=\'http\')'] = True
        options['grants'] = ['mdb_admin']
        options['resource_group'] = 'mdb_admin_group'
    if 'monitor' in users:
        options = users['monitor']
        options['login'] = True
        options['superuser'] = True
        options['resource_group'] = 'admin_group'
    _set_system_users_settings(users)
    return users


def _create_new_users(ret, new_users, expected, cursor):
    for user in new_users:
        if not __opts__.get('test'):
            __salt__['mdb_greenplum.create_user'](user, expected[user], cursor)
        ret['changes'][user] = 'create'


def _get_gp_resource_manager():
    gp_resource_manager = __salt__['pillar.get']('data:greenplum:config:gp_resource_manager', 'group')
    return gp_resource_manager


def _set_gp_resource_group(options):
    if 'resource_group' not in options or not options['resource_group']:
        resource_group_name = DEFAULT_RESOURCE_GROUP
    else:
        resource_group_name = options['resource_group']
    return resource_group_name


def _set_gp_resource_queue(options):
    if 'resource_queue' not in options or not options['resource_queue']:
        resource_queue_name = DEFAULT_RESOURCE_QUEUE
    else:
        resource_queue_name = options['resource_queue']
    return resource_queue_name


def check_segment_replica_status_call(name):
    ret = __salt__['mdb_greenplum.check_segment_replica_status']()
    ret['name'] = name
    return ret


def check_segment_replicas_up_status_call(name):
    ret = __salt__['mdb_greenplum.check_segment_replicas_up_status']()
    ret['name'] = name
    return ret


def check_segment_is_in_dead_status_call(name):
    ret = __salt__['mdb_greenplum.check_segment_is_in_dead_status']()
    ret['name'] = name
    return ret


def check_master_replica_is_alive_call(name):
    ret = __salt__['mdb_greenplum.check_master_replica_is_alive']()
    ret['name'] = name
    return ret


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
        in_db = __salt__['mdb_greenplum.get_user'](user, cursor)
        keys = [
            'superuser',
            'login',
            'createdb',
            'createrole',
            'createexttable(type=\'readable\')',
            'createexttable(type=\'writable\')',
            'createexttable(protocol=\'http\')',
            'resource_group',
        ]
        sync_passwords = __salt__['pillar.get']('sync-passwords')
        if sync_passwords or __opts__.get('test'):
            keys.append('encrypted_password')
        for key in keys:
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
                __salt__['mdb_greenplum.modify_user'](user, changes, cursor)
            elif not sync_passwords and 'encrypted_password' in changes:
                ret['comment'] += (
                    "Warning, user password won't be changed " "unless 'sync-passwords' is set to true in pillar"
                )
            ret['changes'][user] = changes


def sync_users(name):
    """
    Creates role
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    try:
        with closing(__salt__['mdb_greenplum.get_connection']()) as conn:
            with conn as txn:
                cursor = txn.cursor()
                existing = __salt__['mdb_greenplum.list_users'](cursor)
                expected = _get_expected_users()
                gp_resource_manager = _get_gp_resource_manager()
                for user, options in expected.items():
                    if options.get('login') and options.get('password'):
                        options['encrypted_password'] = __salt__['mdb_greenplum.get_md5_encrypted_password'](
                            user, options['password']
                        )
                    if gp_resource_manager == 'group':
                        options['resource_group'] = _set_gp_resource_group(options)
                    else:
                        options['resource_queue'] = _set_gp_resource_queue(options)
                new_users = set(expected.keys()).difference(set(existing))
                _create_new_users(ret, new_users, expected, cursor)
                other = set(expected.keys()) - new_users
                _sync_existing_users(ret, other, expected, cursor)
    except Exception:
        ret['result'] = False
        ret['comment'] += format_exc()

    if ret['changes'] and __opts__.get('test') and ret['result']:
        ret['result'] = None
    return ret


def sync_user_grants(name):
    """
    Sync greenplum grants for all users with pillar data
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    name_filter = __salt__['pillar.get']('target-user', None)

    try:
        with closing(__salt__['mdb_greenplum.get_connection']()) as conn:
            with conn as txn:
                cursor = txn.cursor()
                users = _get_expected_users()
                admin_user = __salt__['pillar.get']('data:greenplum:admin_user_name')
                for user, options in users.items():
                    admin_option = True if user == admin_user else False
                    if name_filter and user != name_filter:
                        continue
                    in_db = __salt__['mdb_greenplum.get_user_role_membership'](user, cursor)
                    new_roles = set(options.get('grants', [])).difference(set(in_db))
                    for role in new_roles:
                        if not __opts__.get('test'):
                            __salt__['mdb_greenplum.assign_role_to_user'](user, role, admin_option, cursor)
                        if user not in ret['changes']:
                            ret['changes'][user] = {}
                        ret['changes'][user][role] = 'assign'
    except Exception:
        ret['result'] = False
        ret['comment'] += format_exc()

    if ret['changes'] and __opts__.get('test') and ret['result']:
        ret['result'] = None
    return ret


def extension_present(name, version=None, database='postgres'):
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    is_test = __opts__.get('test')
    try:
        with closing(__salt__['mdb_greenplum.get_connection'](database=database)) as conn:
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
                    query = __salt__['mdb_greenplum.add_extension'](name, cursor, version, is_test)
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


def extension_update_in_all_dbs(name, version=None):
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    is_test = __opts__.get('test')
    if not version:
        return ret
    try:
        with closing(__salt__['mdb_greenplum.get_master_conn']()) as conn:
            conn.autocommit = True
            cursor = conn.cursor()
            for x in __salt__['mdb_greenplum.list_databases'](cursor):
                try:
                    with closing(__salt__['mdb_greenplum.get_connection'](database=x['datname'])) as conn:
                        with conn as txn:
                            cursor = txn.cursor()
                            cursor.execute(
                                cursor.mogrify(
                                    "select extversion from pg_catalog.pg_extension where extname = %(extname)s",
                                    {'extname': name},
                                )
                            )
                            result = cursor.fetchone()
                            if not result:
                                continue
                            else:
                                if str(result[0]) != str(version):
                                    query = __salt__['mdb_greenplum.update_extension'](name, cursor, version, is_test)
                                    ret['changes'][name] = query
                except Exception as e:
                    ret['result'] = False
                    ret['comment'] += str(e)

    except Exception as e:
        ret['result'] = False
        ret['comment'] += str(e)

    if is_test and len(ret['changes']) != 0:
        ret['result'] = None
    return ret


def protocol_present(name, read_func=None, write_func=None, trusted=False, database='postgres'):
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    is_test = __opts__.get('test')
    if not (write_func or read_func):
        ret['result'] = False
        ret['comment'] += 'The command must specify either a read call handler or a write call handler'
    else:
        try:
            with closing(__salt__['mdb_greenplum.get_connection'](database=database)) as conn:
                with conn as txn:
                    cursor = txn.cursor()
                    if __salt__['mdb_postgresql.is_in_recovery'](cursor):
                        ret['comment'] = 'Skipping on replica'
                        return ret
                    cursor.execute(
                        cursor.mogrify(
                            "select 1 from pg_catalog.pg_extprotocol where ptcname = %(ptcname)s", {'ptcname': name}
                        )
                    )
                    result = cursor.fetchone()
                    if not result:
                        query = __salt__['mdb_greenplum.add_protocol'](
                            name, cursor, write_func, read_func, trusted, is_test
                        )
                        ret['changes'][name] = query
                        return ret
        except Exception as e:
            ret['result'] = False
            ret['comment'] += str(e)

        if is_test and len(ret['changes']) != 0:
            ret['result'] = None
    return ret


def _create_new_resource_group(ret, new_groups, cursor):
    for group in new_groups:
        if not __opts__.get('test'):
            __salt__['mdb_greenplum.create_resource_group'](group, cursor)
        ret['changes'][group] = 'create'


def sync_resource_groups(name, rg_names=None):
    """
    Here we just to check whether resource group exists, if it's not, then create
    We don't set any parameters to resource group because of client can change anything by yourself
    """
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    is_test = __opts__.get('test')
    if rg_names is None:
        ret['result'] = False
        ret['comment'] += 'Specify list of resource groups please'
    try:
        with closing(__salt__['mdb_greenplum.get_master_conn']()) as conn:
            with conn as txn:
                conn.autocommit = True
                cursor = txn.cursor()
                existing = __salt__['mdb_greenplum.list_resource_groups'](cursor)
                new_groups = set(rg_names).difference(set(existing))
                _create_new_resource_group(ret, new_groups, cursor)
    except Exception as e:
        ret['result'] = False
        ret['comment'] += str(e)
    if is_test and len(ret['changes']) != 0:
        ret['result'] = None
    return ret


def sync_database(name, database, owner=None, options=None):
    if not owner:
        owner = 'gpadmin'
    ret = {'name': name, 'changes': {}, 'result': True, 'comment': ''}
    changes = []
    try:
        with closing(__salt__['mdb_greenplum.get_master_conn']()) as conn:
            conn.autocommit = True
            cursor = conn.cursor()
            existing_data = [
                x
                for x in __salt__['mdb_greenplum.list_databases'](cursor)
                if x['datname'] not in ['postgres', 'template1']
            ]
            existing_set = set(x['datname'] for x in existing_data)
            if database not in existing_set:
                if not __opts__.get('test'):
                    __salt__['mdb_greenplum.create_database'](database=database, owner=owner, cursor=cursor)
                changes.append('create')
            if options:
                database_options = __salt__['mdb_greenplum.get_database_options'](database=database, cursor=cursor)
                for key, value in options.items():
                    if database_options.get(key) != value:
                        if not __opts__.get('test'):
                            database_options = __salt__['mdb_greenplum.database_set_option'](
                                database=database, cursor=cursor, key=key, value=value
                            )
                        changes.append('set {key}={value}'.format(key=key, value=value))
            if changes:
                ret['changes'][database] = ';'.join(changes)
    except Exception:
        ret['result'] = False
        ret['comment'] += format_exc()
    if ret['changes'] and __opts__.get('test') and ret['result']:
        ret['result'] = None
    return ret


def reload_segment(name, port, content_id):
    is_test = __opts__.get('test')
    ret = __salt__['mdb_greenplum.reload_segment'](port=port, content_id=content_id, test=is_test)
    ret['name'] = name
    return ret


def gp_toolkit_grant_permissions(name, runas):
    is_test = __opts__.get('test')
    dbs = __salt__['mdb_greenplum.list_all_databases']()
    exclude = ["template0", "postgres", "diskquota"]
    dbs_to_run = [x for x in dbs if x not in exclude]
    ret = {"name": name, "result": True, "changes": {}, "comment": ""}
    if is_test:
        ret["result"] = None
        ret["comment"] = "PostgreSQL file {} will be executed in the db: {}".format(name, dbs_to_run)
    else:
        for db in dbs_to_run:
            res = __salt__["postgres.psql_file"](name, runas=runas, maintenance_db=db)
            ret_code = res[0]
            if ret_code != 0:
                ret["comment"] = "PostgreSQL file {} has been failed in the db: {}".format(name, db)
                ret["result"] = False
                break
            else:
                ret["comment"] = "PostgreSQL file {} has been executed in the db: {} successfully".format(name, db)
                changes = {
                    "stdout": res[1],
                    "stderr": res[2],
                }
                ret["changes"].update({db: changes["stdout"]})
    return ret
