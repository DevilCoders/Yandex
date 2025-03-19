"""
MDB ClickHouse states.
"""

from __future__ import print_function, unicode_literals

from os.path import join

import traceback

__opts__ = {}
__salt__ = {}

from collections import OrderedDict


def __virtual__():
    if 'mdb_clickhouse.version' not in __salt__:
        return False, 'Unable to load mdb_clickhouse module.'
    return True


# Schema of system tables for non-HA clusters (clusters without ZooKeeper).

CREATE_TABLE_QUERIES = OrderedDict(
    (
        (
            "read_sli_part",
            "CREATE TABLE _system.read_sli_part (`shard` String) "
            "ENGINE = MergeTree PARTITION BY tuple() ORDER BY shard "
            "SETTINGS index_granularity = 8192, storage_policy = 'local'",
        ),
        (
            "read_sli",
            "CREATE TABLE _system.read_sli (`shard` String) "
            "ENGINE = Distributed('{cluster}', '_system', 'read_sli_part')",
        ),
        (
            "write_sli_part",
            "CREATE TABLE _system.write_sli_part (`hostname` String, `n` Int32, `insert_time` DateTime DEFAULT now()) "
            "ENGINE = MergeTree PARTITION BY toStartOfFiveMinute(insert_time) ORDER BY insert_time "
            "TTL insert_time + toIntervalMinute(10) "
            "SETTINGS index_granularity = 8192, storage_policy = 'local'",
        ),
        (
            "write_sli",
            "CREATE TABLE _system.write_sli (`hostname` String, `n` Int32, `insert_time` DateTime DEFAULT now()) "
            "ENGINE = Distributed('{cluster}', '_system', 'write_sli_part', n)",
        ),
        (
            "primary_election_part",
            "CREATE TABLE _system.primary_election_part (`hostname` String) "
            "ENGINE = MergeTree PARTITION BY tuple() ORDER BY hostname "
            "SETTINGS index_granularity = 8192, storage_policy = 'local'",
        ),
        (
            "primary_election",
            "CREATE TABLE _system.primary_election (`hostname` String) "
            "ENGINE = Distributed('{cluster}', '_system', 'primary_election_part')",
        ),
    )
)

INIT_TABLE_QUERIES = {
    "read_sli_part": "INSERT INTO _system.read_sli_part SELECT substitution FROM system.macros WHERE macro = 'shard'",
    "primary_election_part": "INSERT INTO _system.primary_election_part SELECT hostName()",
}

# Schema of system tables for HA clusters (clusters with ZooKeeper).

HA_TABLE_ZK_PATH = {
    'read_sli_part': '_system/read_sli_part/{shard}',
    'write_sli_part': '_system/write_sli_part/{shard}',
    'shard_primary_election': '_system/shard_primary_election/{shard}',
    'cluster_primary_election': '_system/cluster_primary_election',
}

CREATE_TABLE_QUERIES_HA = OrderedDict(
    (
        (
            "read_sli_part",
            (
                "CREATE TABLE _system.read_sli_part (`shard` String) "
                "ENGINE = ReplicatedMergeTree('/{zk_path}', '{{replica}}') "
                "PARTITION BY tuple() ORDER BY shard "
                "SETTINGS index_granularity = 8192, storage_policy = 'local'"
            ).format(zk_path=HA_TABLE_ZK_PATH['read_sli_part']),
        ),
        (
            "read_sli",
            "CREATE TABLE _system.read_sli (`shard` String) "
            "ENGINE = Distributed('{cluster}', '_system', 'read_sli_part')",
        ),
        (
            "write_sli_part",
            (
                "CREATE TABLE _system.write_sli_part (`hostname` String, `n` Int32, `insert_time` DateTime DEFAULT now()) "
                "ENGINE = ReplicatedMergeTree('/{zk_path}', '{{replica}}') "
                "PARTITION BY toDate(insert_time) ORDER BY insert_time "
                "TTL toDate(insert_time) + toIntervalDay(1) "
                "SETTINGS ttl_only_drop_parts = 1, index_granularity = 8192, storage_policy = 'local'"
            ).format(zk_path=HA_TABLE_ZK_PATH['write_sli_part']),
        ),
        (
            "write_sli",
            "CREATE TABLE _system.write_sli (`hostname` String, `n` Int32, `insert_time` DateTime DEFAULT now()) "
            "ENGINE = Distributed('{cluster}', '_system', 'write_sli_part', n)",
        ),
        (
            "shard_primary_election",
            (
                "CREATE TABLE _system.shard_primary_election (`n` Int32) "
                "ENGINE = ReplicatedMergeTree('/{zk_path}', '{{replica}}') "
                "PARTITION BY tuple() ORDER BY n "
                "SETTINGS index_granularity = 8192, storage_policy = 'local'"
            ).format(zk_path=HA_TABLE_ZK_PATH['shard_primary_election']),
        ),
        (
            "cluster_primary_election",
            (
                "CREATE TABLE _system.cluster_primary_election (`n` Int32) "
                "ENGINE = ReplicatedMergeTree('/{zk_path}', '{{replica}}') "
                "PARTITION BY tuple() ORDER BY n "
                "SETTINGS index_granularity = 8192, storage_policy = 'local'"
            ).format(zk_path=HA_TABLE_ZK_PATH['cluster_primary_election']),
        ),
    )
)

INIT_TABLE_QUERIES_HA = {
    "read_sli_part": "INSERT INTO _system.read_sli_part SELECT substitution FROM system.macros WHERE macro = 'shard'",
}


def wait_for_zookeeper(name, zk_hosts, wait_timeout=600):
    """
    Wait for zookeeper to start up.
    """
    ret = {'name': name, 'result': False, 'changes': {}, 'comment': ''}

    if __opts__['test']:
        ret['result'] = True
        return ret

    try:
        __salt__['mdb_clickhouse.create_zkflock_id'](zk_hosts=zk_hosts, wait_timeout=wait_timeout)
    except Exception:
        return _error(name, traceback.format_exc())

    ret['result'] = True
    return ret


def ensure_system_query_log(name):
    """
    Ensure that system.query_log has been initialized.
    """
    ret = {'name': name, 'result': False, 'changes': {}, 'comment': ''}
    test = __opts__['test']
    try:
        ret['changes'] = __salt__['mdb_clickhouse.ensure_system_query_log'](test)
    except Exception:
        return _error(name, traceback.format_exc())

    if ret['changes'] and test:
        ret['result'] = None
    else:
        ret['result'] = True
    return ret


def sync_databases(name):
    """
    Sync pillar databases info with ClickHouse.
    """
    ret = {'name': name, 'result': False, 'changes': {}, 'comment': ''}

    sql_database_management = __salt__['pillar.get']('data:clickhouse:sql_database_management', False)
    target_database = __salt__['pillar.get']('target-database', None)
    dbs_pillar = {'_system'}
    if not sql_database_management:
        dbs_pillar = dbs_pillar.union(__salt__['mdb_clickhouse.databases']() or [])

    dbs_ch = set(__salt__['mdb_clickhouse.get_existing_database_names']())

    create_db_list = list(sorted(dbs_pillar.difference(dbs_ch)))
    delete_db_set = dbs_ch.difference(dbs_pillar) if not sql_database_management else set()
    delete_db_list = list(sorted(delete_db_set))

    if not (create_db_list or delete_db_list):
        ret['result'] = True
        return ret

    if create_db_list:
        ret['changes']['created'] = create_db_list
    if delete_db_list:
        if delete_db_set != {target_database}:
            return _error(
                name,
                "Going to delete databases: [{databases}], but target is '{target}'".format(
                    databases=', '.join(delete_db_list), target=target_database
                ),
            )
        ret['changes']['deleted'] = delete_db_list

    if __opts__['test']:
        ret['result'] = None
        return ret

    try:
        for db in delete_db_list:
            __salt__['mdb_clickhouse.delete_database'](db)

        for db in create_db_list:
            __salt__['mdb_clickhouse.create_database'](db)
    except Exception:
        return _error(name, traceback.format_exc())

    ret['result'] = True
    return ret


def reload_dictionaries(name):
    """
    Reload external dictionaries.
    """
    try:
        if __opts__['test']:
            return {
                'name': name,
                'result': None,
                'changes': {'dictionaries': 'reloaded'},
                'comment': 'Dictionaries would be reloaded',
            }

        __salt__['mdb_clickhouse.reload_dictionaries']()

        return {
            'name': name,
            'result': True,
            'changes': {'dictionaries': 'reloaded'},
            'comment': 'Dictionaries was reloaded',
        }
    except Exception as e:
        return {
            'name': name,
            'result': True,
            'changes': {'dictionaries': 'reloaded'},
            'comment': 'Dictionaries reload was initiated',
            'warnings': ['Dictionary reload failed: "{}"'.format(repr(e))],
        }


def reload_config(name):
    """
    Force reload of ClickHouse configs.
    """
    try:
        if __opts__['test']:
            return {
                'name': name,
                'result': None,
                'changes': {'config': 'reloaded'},
                'comment': 'Config would be reloaded',
            }

        __salt__['mdb_clickhouse.reload_config']()
        return {
            'name': name,
            'result': True,
            'changes': {'config': 'reloaded'},
            'comment': 'Config was reloaded',
        }
    except Exception as e:
        return {
            'name': name,
            'result': True,
            'changes': {'config': 'reloaded'},
            'comment': 'Config reload was initiated',
            'warnings': ['Config reload failed: "{}"'.format(repr(e))],
        }


def ensure_mdb_tables(name):
    """
    Ensure that MDB-specific system tables are created and initialized.
    """

    try:
        has_zookeeper = __salt__['mdb_clickhouse.has_zookeeper']()
        if has_zookeeper:
            create_queries = CREATE_TABLE_QUERIES_HA
            init_queries = INIT_TABLE_QUERIES_HA
        else:
            create_queries = CREATE_TABLE_QUERIES
            init_queries = INIT_TABLE_QUERIES

        # Calculating required changes.

        expected_tables = list(create_queries.keys())
        to_delete = []
        to_create = []
        to_init = []

        for existing_table in __salt__['mdb_clickhouse.get_existing_tables']('_system'):
            table_name = existing_table['name']

            create_query = create_queries.get(table_name)
            if not create_query or existing_table['create_table_query'] != create_query:
                to_delete.append(('_system', existing_table))
                continue

            init_query = init_queries.get(table_name)
            if init_query and not __salt__['mdb_clickhouse.has_data_parts']('_system', table_name):
                to_init.append(init_query)

            expected_tables.remove(table_name)

        for table_name in expected_tables:
            to_create.append(table_name)
            init_query = init_queries.get(table_name)
            if init_query:
                to_init.append(init_query)

        # Performing changes.

        changes = {}

        if to_delete:
            if not __opts__['test']:
                for database, table in to_delete:
                    __salt__['mdb_clickhouse.delete_table'](database, table['name'])

            changes['deleted_tables'] = [table['create_table_query'] for _db, table in to_delete]

        if to_create:
            if has_zookeeper:
                deleted_zk_nodes = []
                zk_hosts = __salt__['mdb_clickhouse.zookeeper_hosts']()
                zk_root = __salt__['mdb_clickhouse.zookeeper_root']()
                if zk_root is None:
                    zk_root = '/'
                shard_name = __salt__['mdb_clickhouse.shard_name']()
                replica_name = __salt__['mdb_clickhouse.hostname']()
                zk_user = __salt__['mdb_clickhouse.zookeeper_user']()
                embedded_keeper = __salt__['mdb_clickhouse.has_embedded_keeper']()
                if zk_user and not embedded_keeper:
                    zk_password = __salt__['mdb_clickhouse.zookeeper_password'](zk_user)
                else:
                    zk_password = None
                if zk_password:
                    zk_scheme = 'digest'
                else:
                    zk_user = None
                    zk_password = None
                    zk_scheme = None
                for table_name in to_create:
                    if table_name in HA_TABLE_ZK_PATH:
                        key = join(
                            zk_root, HA_TABLE_ZK_PATH[table_name].format(shard=shard_name), 'replicas', replica_name
                        )

                        if __salt__['zookeeper.exists'](
                            key, hosts=zk_hosts, scheme=zk_scheme, username=zk_user, password=zk_password
                        ):
                            if not __opts__['test']:
                                __salt__['zookeeper.delete'](
                                    key,
                                    recursive=True,
                                    hosts=zk_hosts,
                                    scheme=zk_scheme,
                                    username=zk_user,
                                    password=zk_password,
                                )
                            deleted_zk_nodes.append(key)
                if deleted_zk_nodes:
                    changes['delete_zk_nodes'] = deleted_zk_nodes

            if not __opts__['test']:
                for table_name in to_create:
                    __salt__['mdb_clickhouse.execute'](create_queries[table_name])

            changes['created_tables'] = [create_queries[table_name] for table_name in to_create]

        if to_init:
            if not __opts__['test']:
                for init_query in to_init:
                    __salt__['mdb_clickhouse.execute'](init_query)

            changes['inserted_data'] = to_init

        return {
            'name': name,
            'changes': changes,
            'result': None if (__opts__['test'] and changes) else True,
            'comment': '',
        }

    except Exception:
        return _error(name, traceback.format_exc())


def cleanup_log_tables(name):
    try:
        deleted_tables = []
        existing_table_names = [
            existing_table['name'] for existing_table in __salt__['mdb_clickhouse.get_existing_tables']('system')
        ]

        force_drop_enabled = False
        for table, settings in __salt__['mdb_clickhouse.system_tables']().items():
            if not settings['enabled'] and table in existing_table_names:
                if not __opts__['test']:

                    if not force_drop_enabled:
                        __salt__['mdb_clickhouse.enable_force_drop_table']()
                        force_drop_enabled = True

                    __salt__['mdb_clickhouse.delete_table']('system', table)
                deleted_tables.append(table)

        if force_drop_enabled:
            __salt__['mdb_clickhouse.disable_force_drop_table']()
        return {
            'name': name,
            'changes': {'deleted_tables': deleted_tables} if deleted_tables else {},
            'result': None if (__opts__['test'] and deleted_tables) else True,
            'comment': '',
        }
    except Exception:
        return _error(name, traceback.format_exc())


def resetup_required(name):
    """
    Detect if clickhouse resetup tool should be executed
    """
    ret = {'name': name, 'comment': ''}
    required = __salt__['mdb_clickhouse.resetup_required']()
    ret['result'] = None if __opts__['test'] and required else True
    ret['changes'] = {'run': 'Resetup tool will be executed'} if required else {}
    return ret


def ensure_admin_user(name):
    """
    Creates admin user with all needed grants.
    """
    password_hash = __salt__['mdb_clickhouse.admin_password_hash']()
    grants = __salt__['mdb_clickhouse.admin_grants']()
    grantees = None
    if __salt__['mdb_clickhouse.version_cmp']('21.4') >= 0:
        grantees = [['ANY'], ['admin']]
    return _ensure_sql_user('admin', password_hash, grants, grantees)


def ensure_sql_user(name):
    """
    Manage ClickHouse user created via SQL.
    """
    password_hash = __salt__['mdb_clickhouse.user_password_hash'](name)
    grants = __salt__['mdb_clickhouse.user_grants'](name)
    result = _ensure_sql_user(name, password_hash, grants)
    if result['result'] is False:
        return result

    try:
        result['changes'].update(_ensure_user_quotas(name))
        result['changes'].update(_ensure_user_settings(name))
        return result
    except Exception:
        return _error(name, traceback.format_exc())


def _ensure_sql_user(name, password_hash, grants, grantees=None):
    try:
        changes = {}
        changes.update(_ensure_user_exist(name, password_hash, grantees))
        if not changes:
            changes.update(_ensure_user_password(name, password_hash))
            changes.update(_ensure_user_grantees(name, grantees))
        changes.update(_ensure_user_grants(name, grants))
        return {
            'name': name,
            'changes': changes,
            'result': None if changes and __opts__['test'] else True,
            'comment': '',
        }
    except Exception:
        return _error(name, traceback.format_exc())


def _ensure_user_exist(name, password_hash, grantees):
    changes = {}
    if name not in __salt__['mdb_clickhouse.get_existing_user_names']():
        changes['user_created'] = name
        if not __opts__['test']:
            __salt__['mdb_clickhouse.create_user'](name, password_hash, grantees)
    return changes


def _ensure_user_password(name, target_password_hash):
    changes = {}
    existing_password_hash = __salt__['mdb_clickhouse.get_existing_user_password_hash'](name)
    if target_password_hash.lower() != existing_password_hash.lower():
        changes['user_password_changed'] = name
        if not __opts__['test']:
            __salt__['mdb_clickhouse.change_user_password'](name, target_password_hash)
    return changes


def _ensure_user_grantees(name, grantees):
    """
    Manage ClickHouse user grantees (for ClickHouse 21.4+)
    """

    if not grantees:
        grantees = [['ANY'], []]

    changes = {}
    if not _check_user_grantees(name, grantees):
        statement = "ALTER USER {} GRANTEES {}".format(name, ", ".join(grantees[0]) if grantees[0] else "ANY")
        if grantees[1]:
            statement += " EXCEPT {}".format(", ".join(grantees[1]))
        changes['grantees_created'] = statement
        if not __opts__['test']:
            __salt__['mdb_clickhouse.execute'](statement)
    return changes


def _check_user_grantees(user_name, target_grantees):
    """
    Compare existing and target user grantees. It returns True if the grantees equal, or False otherwise.
    """
    actual_grantees = __salt__['mdb_clickhouse.get_existing_user_grantees'](user_name)
    if not target_grantees and actual_grantees == [['ANY'], []]:
        return True
    if sorted(actual_grantees[0]) != sorted(target_grantees[0]):
        return False
    if sorted(actual_grantees[1]) != sorted(target_grantees[1]):
        return False
    return True


def _ensure_user_grants(user_name, target_grants):
    queries = []
    changes = {}
    existing_grants = __salt__['mdb_clickhouse.get_existing_user_grants'](user_name)
    target_grant_types = {grant.access_type for grant in target_grants}
    for grant in existing_grants.keys():
        if grant not in target_grant_types:
            query = __salt__['mdb_clickhouse.grant'](grant).revoke_statement(user_name)
            if not __opts__['test']:
                __salt__['mdb_clickhouse.execute'](query)
            changes[grant] = {'reason': 'removing, not in config', 'actions': [query]}

    existing_grants = __salt__['mdb_clickhouse.get_existing_user_grants'](user_name)

    for grant in target_grants:
        check_result = grant.compare_with_existing(existing_grants.get(grant.access_type, []))
        if check_result:
            queries.extend(grant.grant_statements(user_name))
            changes[grant.access_type] = {
                'reason': check_result,
                'actions': grant.grant_statements(user_name),
            }

    if not __opts__['test']:
        for query in queries:
            __salt__['mdb_clickhouse.execute'](query)
    return {'grants_changed': changes} if changes else {}


def _ensure_user_quotas(name):  # update quota keyed
    """
    Manage ClickHouse user quotas created via SQL.
    """
    quotas = __salt__['mdb_clickhouse.user_quotas'](name)
    quota_mode = __salt__['mdb_clickhouse.user_quota_mode'](name)

    quota_map = {
        '{user}_quota_{interval}'.format(user=name, interval=quota_interval['interval_duration']): quota_interval
        for quota_interval in quotas
    }

    keyed_by = {
        'keyed': ' KEYED BY user_name',
        'keyed_by_ip': ' KEYED BY ip_address',
    }.get(quota_mode, '')

    queries = []
    to_add = _check_user_quotas(name, quotas, quota_mode)
    for quota in to_add:
        quota_params = quota_map[quota]
        params = ', '.join(
            map(
                lambda param: '{param} = {value}'.format(param=param, value=quota_params.get(param) or 0),
                ['queries', 'errors', 'result_rows', 'read_rows', 'execution_time'],
            )
        )
        statement = (
            'CREATE QUOTA OR REPLACE `{quota_name}`{keyed_by} FOR INTERVAL {interval_duration} second '
            'MAX {params} TO `{user}`'.format(
                quota_name=quota,
                keyed_by=keyed_by,
                interval_duration=quota_params['interval_duration'],
                params=params,
                user=name,
            )
        )
        queries.append(statement)

    changes = {}
    if queries:
        changes['quota_created'] = queries

    if not __opts__['test']:
        for query in queries:
            __salt__['mdb_clickhouse.execute'](query)
    return changes


def _ensure_user_settings(name):
    """
    Manage ClickHouse user settings created via SQL.
    """

    def format_setting(item):
        setting, value = item
        if not isinstance(value, int):
            value = "'{}'".format(value)
        return '{} = {}'.format(setting, value)

    changes = {}
    user_settings_name = '{user}_profile'.format(user=name)
    target_settings = __salt__['mdb_clickhouse.user_settings'](name)
    if not _check_user_settings(name, target_settings):
        if len(target_settings) > 0:
            statement = 'CREATE SETTINGS PROFILE OR REPLACE `{name}` SETTINGS {settings} TO `{user}`'.format(
                name=user_settings_name,
                user=name,
                settings=', '.join(map(format_setting, sorted(target_settings.items()))),
            )
            changes['profile_created'] = statement
            if not __opts__['test']:
                __salt__['mdb_clickhouse.execute'](statement)
    return changes


def _check_user_quotas(user_name, target_quotas, target_quota_mode):
    """
    Compare existing and target quotas. It returns list of quotas to add.
    """
    existing_quotas = __salt__['mdb_clickhouse.get_existing_user_quotas'](user_name)
    target_keys = {
        'keyed': ['user_name'],
        'keyed_by_ip': ['ip_address'],
    }.get(target_quota_mode, [])
    to_add = []
    for quota in target_quotas:
        quota_name = '{user}_quota_{interval}'.format(user=user_name, interval=quota['interval_duration'])
        actual = existing_quotas.get(quota_name)
        if (
            quota_name not in existing_quotas
            or actual['keys'] != target_keys
            or any(
                str(quota.get(opt) or 0) != str(actual[opt])
                for opt in ['queries', 'errors', 'result_rows', 'read_rows', 'execution_time']
            )
            or user_name not in actual['apply_to_list']
        ):
            to_add.append(quota_name)

    return to_add


def _check_user_settings(user_name, target_settings):
    """
    Compare existing and target user settings. It returns True if the settings equal, or False otherwise.
    """
    actual_settings = __salt__['mdb_clickhouse.get_existing_user_settings'](user_name)

    if set(actual_settings.keys()) != set(target_settings.keys()):
        return False

    for opt_name, opt_value in target_settings.items():
        if actual_settings[opt_name] != str(opt_value):
            return False

    return True


def cleanup_users(name):
    """
    Removes ClickHouse users created via SQL, that shouldn't exist
    """
    result = {
        'name': name,
        'changes': {},
        'result': True,
        'comment': '',
    }

    try:
        target_users = set(__salt__['mdb_clickhouse.user_names']())
        actual_users = set(__salt__['mdb_clickhouse.get_existing_user_names']())

        to_delete = list(actual_users.difference(target_users))
        if len(to_delete) == 0:
            return result

        result['changes']['user_deleted'] = to_delete
        if not __opts__['test']:
            for user in to_delete:
                __salt__['mdb_clickhouse.delete_user'](user)
        else:
            result['result'] = None
    except Exception:
        return _error(name, traceback.format_exc())

    return result


def cleanup_user_quotas(name):
    """
    Removes ClickHouse user quotas created via SQL, that shouldn't exist
    """
    result = {
        'name': name,
        'changes': {},
        'result': True,
        'comment': '',
    }

    try:
        target_quotas = []
        for user_name in __salt__['mdb_clickhouse.user_names']():
            for quota in __salt__['mdb_clickhouse.user_quotas'](user_name):
                target_quotas.append('{0}_quota_{1}'.format(user_name, quota['interval_duration']))

        actual_quotas = set(__salt__['mdb_clickhouse.get_existing_quota_names']())
        to_delete = list(sorted(actual_quotas.difference(target_quotas)))
        if len(to_delete) == 0:
            return result

        result['changes']['quota_deleted'] = to_delete
        if not __opts__['test']:
            for quota in to_delete:
                __salt__['mdb_clickhouse.execute']('DROP QUOTA `{name}`;'.format(name=quota))
        else:
            result['result'] = None
    except Exception:
        return _error(name, traceback.format_exc())
    return result


def cleanup_user_settings_profiles(name):
    """
    Removes ClickHouse user settings profiles created via SQL, that shouldn't exist
    """
    result = {
        'name': name,
        'changes': {},
        'result': True,
        'comment': '',
    }

    try:
        target_profiles = []
        for user_name in __salt__['mdb_clickhouse.user_names']():
            if len(__salt__['mdb_clickhouse.user_settings'](user_name)) > 0:
                target_profiles.append('{0}_profile'.format(user_name))

        actual_profiles = set(__salt__['mdb_clickhouse.get_existing_profile_names']())
        to_delete = list(sorted(actual_profiles.difference(target_profiles)))
        if len(to_delete) == 0:
            return result

        result['changes']['profile_deleted'] = to_delete
        if not __opts__['test']:
            for quota in to_delete:
                __salt__['mdb_clickhouse.execute']('DROP SETTINGS PROFILE `{name}`;'.format(name=quota))
        else:
            result['result'] = None
    except Exception:
        return _error(name, traceback.format_exc())
    return result


def cache_user_object(name, object_name, object_type):
    changes = {}
    try:
        if not __opts__['test']:
            changes['uri_cached'] = __salt__['mdb_clickhouse.cache_user_object'](object_type, object_name)
        else:
            changes = {'cached_object': (object_type, name)}
        return {
            'name': name,
            'changes': changes,
            'result': None if changes and __opts__['test'] else True,
            'comment': '',
        }
    except Exception:
        return _error(name, traceback.format_exc())


def check_restart_required(name):
    result = fail_if_clickhouse_upgrade(name)
    if result['result']:
        result = fail_if_clickhouse_keeper_moved(name)
    return result


def fail_if_clickhouse_upgrade(name):
    """
    Compare target and existing clickhouse packages version. Fail if differs.
    """
    result = {
        'name': name,
        'changes': {},
        'result': True,
        'comment': '',
    }

    current_version = __salt__['pkg.version']('clickhouse-server')
    if not current_version:
        return result

    target_version = __salt__['mdb_clickhouse.version']()
    if current_version == target_version:
        return {
            'name': name,
            'changes': {},
            'result': True,
            'comment': '',
        }

    return _error(
        name,
        'Failed to update ClickHouse from "{}" to "{}" without "service-restart" flag.'.format(
            current_version, target_version
        ),
    )


def fail_if_clickhouse_keeper_moved(name):
    """
    Check if clickhouse keeper change common process to separate and vice versa
    """
    result = {
        'name': name,
        'changes': {},
        'result': True,
        'comment': '',
    }

    if not __salt__['mdb_clickhouse.has_embedded_keeper']():
        return result

    has_keeper_now = __salt__['mdb_clickhouse.check_keeper_service']()

    if __salt__['mdb_clickhouse.has_separated_keeper']() == has_keeper_now:
        return result

    return _error(name, 'Failed to change ClickHouse keeper process without "service-restart" flag.')


def _error(name, comment):
    return {'name': name, 'changes': {}, 'result': False, 'comment': comment}
