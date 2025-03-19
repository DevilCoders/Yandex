"""
DBaaS Internal API MySQL cluster utils
"""

import copy
from flask import current_app
from datetime import datetime

from .pillar import SYSTEM_IDM_USERS, MySQLPillar

from ...core.crypto import encrypt
from ...core.exceptions import DbaasClientError
from ...utils import metadb
from ...utils.version import Version
from ...utils.register import get_cluster_traits
from ...utils.types import GIGABYTE, MEGABYTE
from .constants import ALL_PRIVILEGES, DEFAULT_ROLES, MY_CLUSTER_TYPE

MIN_MEMORY_PER_CONNECTION = 8 * MEGABYTE


def gen_user_pillar(opts: dict, databases: set) -> dict:
    """
    Generate opts pillar from template
    """
    allowed_dbs_from_spec = {x['database_name']: x['roles'] for x in opts.get('permissions', [])}
    default_rw_on_all = {x: [ALL_PRIVILEGES] for x in databases}
    pillar = copy.deepcopy(current_app.config['DEFAULT_USER_PILLAR_TEMPLATE'][MY_CLUSTER_TYPE])
    allowed_dbs = allowed_dbs_from_spec or default_rw_on_all
    if opts.get("name") == 'admin':
        allowed_dbs = default_rw_on_all

    pillar.update(
        {
            'password': encrypt(opts['password']),
            'dbs': allowed_dbs,
        }
    )
    return pillar


def validate_options(old_config: dict, new_options, version, flavor, ha_host_count):
    # pylint: disable=unused-argument
    """
    Validate MySQL cluster options
    """

    validate_innodb_buffer_pool_size(new_options.get('innodb_buffer_pool_size'), flavor.get('memory_guarantee'))
    validate_query_cache_size(new_options.get('query_cache_size'), flavor.get('memory_guarantee'))
    validate_max_connections(new_options, flavor)
    validate_charset_and_collation(new_options.get('character_set_server'), new_options.get('collation_server'))
    validate_rpl_semi_sync_master_wait_for_slave_count(
        new_options.get('rpl_semi_sync_master_wait_for_slave_count'), ha_host_count
    )
    validate_mdb_offline_mode_lags(
        new_options.get('mdb_offline_mode_enable_lag', old_config.get('mdb_offline_mode_enable_lag')),
        new_options.get('mdb_offline_mode_disable_lag', old_config.get('mdb_offline_mode_disable_lag')),
    )
    validate_mdb_priority_choice_max_lag(new_options)


def validate_config_changes(old_config: dict, new_config: dict):
    """
    Validates changes
    """
    new_lower_case = new_config.get('lower_case_table_names')
    curr_lower_case = old_config.get('lower_case_table_names')
    if new_lower_case is not None and curr_lower_case != new_lower_case:
        raise DbaasClientError(
            'lower_case_table_names changing is not allowed! Actual value: {} new value: {}'.format(
                curr_lower_case, new_lower_case
            )
        )
    new_innodb_page_size = new_config.get('innodb_page_size')
    curr_innodb_page_size = old_config.get('innodb_page_size')
    if new_innodb_page_size is not None and curr_innodb_page_size != new_innodb_page_size:
        raise DbaasClientError(
            'innodb_page_size changing is not allowed! Actual value: {} new value: {}'.format(
                curr_innodb_page_size, new_innodb_page_size
            )
        )


def validate_need_restart(old_config: dict, new_config: dict):
    """
    Validate if restart needed
    """
    new_buffer_size = new_config.get('innodb_buffer_pool_size')
    curr_buffer_size = old_config.get('innodb_buffer_pool_size')

    restart = False
    if new_buffer_size is not None and new_buffer_size < curr_buffer_size:
        restart = True

    return restart


def validate_innodb_buffer_pool_size(innodb_buffer_pool_size, memory_guarantee):
    """
    Calls abort if `innodb_buffer_pool_size` has value greater than
    memory limit for specified flavor.
    """
    if not innodb_buffer_pool_size:
        return
    if memory_guarantee >= 8 * GIGABYTE:
        max_pool_size = int(0.8 * memory_guarantee)
    elif memory_guarantee >= 4 * GIGABYTE:
        max_pool_size = int(2.5 * GIGABYTE)
    else:
        max_pool_size = int(0.5 * GIGABYTE)
    if innodb_buffer_pool_size > max_pool_size:
        raise DbaasClientError(
            'innodb_buffer_pool_size: {} should be less than or equal to {}'.format(
                innodb_buffer_pool_size, max_pool_size
            )
        )


def validate_query_cache_size(query_cache_size, memory_guarantee):
    """
    Calls abort if `query_cache_size` has value greater than
    memory limit for specified flavor.
    """
    if not query_cache_size:
        return
    max_query_cache_size = int(0.1 * memory_guarantee)
    if query_cache_size > max_query_cache_size:
        raise DbaasClientError(
            'query_cache_size: {} should be less than or equal to {}'.format(query_cache_size, max_query_cache_size)
        )


def validate_max_connections(new_options, flavor):
    """
    Validates max_connections option
    """
    max_connections = new_options.get('max_connections')
    if not max_connections:
        return

    max_max_connections = int(flavor['memory_guarantee'] / MIN_MEMORY_PER_CONNECTION)
    if max_connections > max_max_connections:
        raise DbaasClientError(
            'max_connections: {} should be less than or equal to {}'.format(max_connections, max_max_connections)
        )


def validate_charset_and_collation(charset, collation):
    """
    Validates charset and collation
    """
    if not charset and not collation:
        return
    if not charset or not collation:
        raise DbaasClientError('character_set_server and collation_server should be specified together')
    if '_' in collation:
        collation_charset = collation[: collation.find('_')]
    else:
        # special case for 'binary' collation
        collation_charset = collation
    if charset != collation_charset:
        raise DbaasClientError('collation_server should correspond to character_set_server')


def validate_rpl_semi_sync_master_wait_for_slave_count(wait_slave, ha_host_count):
    """
    Validate rpl_semi_sync_master_wait_for_slave_count
    """
    if wait_slave is None or ha_host_count is None:
        return
    if wait_slave > ha_host_count - 1:
        raise DbaasClientError(
            'rpl_semi_sync_master_wait_for_slave_count should be less or equal than number of HA replicas'
        )


def validate_mdb_offline_mode_lags(enable_lag, disable_lag):
    """
    Validate that mdb_offline_mode_disable_lag < mdb_offline_mode_enable_lag
    """
    if disable_lag is None or enable_lag is None:
        return
    if disable_lag >= enable_lag:
        raise DbaasClientError('mdb_offline_mode_enable_lag must be greater than mdb_offline_mode_disable_lag')


def validate_mdb_priority_choice_max_lag(new_options):
    """
    Validate that mdb_priority_choice_max_lag <= mdb_offline_mode_disable_lag
    """
    mdb_priority_choice_max_lag = new_options.get('mdb_priority_choice_max_lag')
    mdb_offline_mode_disable_lag = new_options.get('mdb_offline_mode_disable_lag')
    if mdb_priority_choice_max_lag is None or mdb_offline_mode_disable_lag is None:
        return
    if mdb_priority_choice_max_lag > mdb_offline_mode_disable_lag:
        raise DbaasClientError('mdb_priority_choice_max_lag must be less than mdb_offline_mode_disable_lag')


def get_cluster_version(cid: str, pillar: MySQLPillar) -> Version:
    """
    Lookup dbaas.versions entry

    :param cid: cluster id of desired cluster
    :param pillar: FIXME: remove it
    :return: actual version of cluster
    """

    # (maj, min, edition) or None
    traits = get_cluster_traits(MY_CLUSTER_TYPE)
    cluster = metadb.get_clusters_versions(traits.versions_component, [cid]).get(cid)
    if cluster:
        version = Version.load(cluster['major_version'])
    else:
        # [MDB-13171] Todo: for backward compatibility - remove after migration of all old clusters to dbaas.versions
        version = pillar.version
    return version


def get_cluster_version_at_time(cid: str, timestamp: datetime, pillar: MySQLPillar) -> Version:
    """
    Lookup dbaas.versions_revs entry

    :param cid: cluster id of desired cluster
    :param timestamp: point-in-time for which cluster version is selected
    :param pillar: FIXME: remove it
    :return: cluster version that was actual at `timestamp` point in time
    """

    # (maj, min, edition) or None
    traits = get_cluster_traits(MY_CLUSTER_TYPE)
    rev = metadb.get_cluster_rev_by_timestamp(cid=cid, timestamp=timestamp)
    cluster = metadb.get_cluster_version_at_rev(traits.versions_component, cid, rev)[0]
    if cluster:
        version = Version.load(cluster['major_version'])
    else:
        # [MDB-13171] Todo: for backward compatibility - remove after migration of all old clusters to dbaas.versions
        version = pillar.version
    return version


def populate_idm_system_users_if_need(pillar: MySQLPillar, sox_audit: bool, version: Version):
    """
    Create system IDM users if need
    """
    if not sox_audit:
        return

    for name, db_roles in SYSTEM_IDM_USERS.items():
        if not pillar.user_exists(name):
            permissions = get_idm_system_user_permissions(db_roles, pillar.database_names)
            pillar.add_user({"name": name, "permissions": permissions}, version)


def get_idm_system_user_permissions(db_roles, db_names):
    """
    Get system IDM user permissions
    """
    permissions = [
        {
            'database_name': db_name,
            'roles': copy.deepcopy(db_roles if db_roles else DEFAULT_ROLES),
        }
        for db_name in db_names
    ]
    return permissions
