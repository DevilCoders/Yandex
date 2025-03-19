# -*- coding: utf-8 -*-
"""
DBaaS Internal API PostgreSQL cluster utils
"""
import copy
import collections
import operator
from typing import Dict, List, Sequence

import humanfriendly
from flask import current_app
from flask_restful import abort

from .schema_defaults import get_min_wal_size, get_max_wal_size
from ...core.exceptions import DbaasClientError, ParseConfigError, PreconditionFailedError, UserInvalidError
from ...utils.types import MEGABYTE
from .constants import (
    DEFAULT_CONN_LIMIT,
    DEFAULT_SYS_CONN_LIMIT,
    MY_CLUSTER_TYPE,
    DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT,
    DEFAULT_MIN_WAL_SIZE_IN_MEGABYTE,
    EDITION_1C,
)
from ...utils.register import get_cluster_traits
from ...utils import metadb
from ...utils.version import Version
from .host_pillar import PostgresqlHostPillar
from .traits import PostgresqlClusterTraits
from .types import PostgresqlConfigSpec, UserWithPassword
from .pillar import PostgresqlClusterPillar


def get_user_pillar(user: UserWithPassword, connect_dbs=None) -> dict:
    """
    Generate user pillar from template and user object
    """
    if connect_dbs is None:
        connect_dbs = []
    pillar = copy.deepcopy(current_app.config['DEFAULT_USER_PILLAR_TEMPLATE'][MY_CLUSTER_TYPE])
    pillar.update(
        {
            'password': user.encrypted_password,
            'conn_limit': (user.conn_limit if user.conn_limit is not None else DEFAULT_CONN_LIMIT),
            'connect_dbs': user.connect_dbs + connect_dbs,
            'settings': user.settings,
            'login': user.login,
            'grants': user.grants,
        }
    )
    return pillar


def get_max_shared_buffers_bytes(flavor):
    """
    Returns maximum value for `shared_buffers` option
    for specified flavor in bytes.
    """
    return int(0.8 * flavor['memory_guarantee'])


def get_max_connections_limit(flavor):
    """
    Returns maximum value for `max_connections` option
    for specified flavor.
    """
    return max(100, int(200 * flavor['cpu_guarantee']))


def validate_shared_buffers(shared_buffers, flavor):
    """
    Calls abort if `shared_buffers` has value greater than
    maximum allowed value for specified flavor.
    Format of shared_buffers: '8MB'
    """
    max_shared_buffers = get_max_shared_buffers_bytes(flavor)
    if humanfriendly.parse_size(shared_buffers, binary=True) > max_shared_buffers:
        raise PreconditionFailedError(
            'shared_buffers ({shared_buffers} bytes) is too high for '
            'your instance type (max is {max_shared_buffers} bytes)'.format(
                shared_buffers=shared_buffers, max_shared_buffers=max_shared_buffers
            )
        )


def validate_max_connections(max_conn, flavor, sum_user_conns=0):
    """
    Calls abort if `max_connections` has value greater than
    maximum allowed value for specified flavor.
    """
    max_conn_limit = get_max_connections_limit(flavor)
    cur_max_conn = max_conn if max_conn else max_conn_limit

    if cur_max_conn > max_conn_limit:
        raise PreconditionFailedError(
            'max_connections ({cur_max_conn}) is too high for '
            'your instance type (max is {max_conn})'.format(cur_max_conn=cur_max_conn, max_conn=max_conn_limit)
        )

    if sum_user_conns > 0 and cur_max_conn < sum_user_conns:
        raise PreconditionFailedError(
            'max_connections ({max_conn}) is less than '
            'sum of users connection limit ({sum_user_conns})'.format(
                max_conn=cur_max_conn, sum_user_conns=sum_user_conns
            )
        )


def validate_min_max_wal_size(min_wal_size, max_wal_size, disk_size):
    """
    Calls abort if `min_wal_size` has value greater than `max_wal_size`
    Calls abort if `min_wal_size` has value less than `DEFAULT_MIN_WAL_SIZE_IN_MEGABYTE`
    Calls abort if `max_wal_size` has value greater than `DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT` of disk size
    """
    if min_wal_size > max_wal_size:
        raise PreconditionFailedError(f'max_wal_size ({max_wal_size}) is less than min_wal_size ({min_wal_size})')

    if max_wal_size > DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT * disk_size:
        raise PreconditionFailedError(
            f'max_wal_size ({max_wal_size}) is greater than'
            f' {DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT} of disk_size ({disk_size})'
        )
    if min_wal_size < DEFAULT_MIN_WAL_SIZE_IN_MEGABYTE * MEGABYTE:
        raise PreconditionFailedError(
            f'min_wal_size ({min_wal_size}) is less than minimum allowed'
            f' {DEFAULT_MIN_WAL_SIZE_IN_MEGABYTE} MEGABYTES'
        )


def validate_max_slot_wal_keep_size(max_slot_wal_keep_size, disk_size):
    """
    Calls abort if `max_slot_wal_keep_size` has value greater than `DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT` of disk size
    Calls abort if `max_slot_wal_keep_size` has value less than `DEFAULT_MIN_WAL_SIZE_IN_MEGABYTE`
    """

    if max_slot_wal_keep_size != -1 and max_slot_wal_keep_size < DEFAULT_MIN_WAL_SIZE_IN_MEGABYTE * MEGABYTE:
        raise PreconditionFailedError(
            f'max_slot_wal_keep_size ({max_slot_wal_keep_size}) is less than minimum allowed'
            f' {DEFAULT_MIN_WAL_SIZE_IN_MEGABYTE} MEGABYTES'
        )

    if max_slot_wal_keep_size > DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT * disk_size:
        raise PreconditionFailedError(
            f'max_slot_wal_keep_size ({max_slot_wal_keep_size}) is greater than'
            f' {DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT} of disk_size ({disk_size})'
        )


def validate_wal_keep_size(wal_keep_size, disk_size):
    """
    Calls abort if `wal_keep_size` has value greater than `DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT` of disk size
    """

    if wal_keep_size > DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT * disk_size:
        raise PreconditionFailedError(
            f'wal_keep_size ({wal_keep_size}) is greater than'
            f' {DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT} of disk_size ({disk_size})'
        )


def validate_database_name(name: str):
    """
    Check that postgresql database name is valid
    """
    PostgresqlClusterTraits.db_name.validate(name, DbaasClientError)


def validate_user_name(name: str):
    """
    Calls abort if name not allowed
    """
    PostgresqlClusterTraits.user_name.validate(value=name, extype=DbaasClientError)


def validate_grants(users: List[UserWithPassword], sox_audit=False):
    """
    Raise UserInvalidError if grants are invalid
    """
    if are_grants_cyclic({u.name: u.grants for u in users}, sox_audit):
        raise UserInvalidError('User grants has a cycle')


def are_grants_cyclic(name_to_grants: Dict[str, list], sox_audit=False) -> bool:
    """
    Check if grants have a cycle
    """
    if sox_audit:
        name_to_grants.update({'reader': [], 'writer': []})

    def _is_cycle_exists(user, stack):
        if user in stack:
            return True
        stack.append(user)
        for role in name_to_grants[user]:
            has_loop = _is_cycle_exists(role, stack)
            if has_loop:
                return True
        stack.pop()
        return False

    for user in name_to_grants.keys():
        if _is_cycle_exists(user, []):
            return True
    return False


def get_all_available_roles(cluster_pillar: PostgresqlClusterPillar) -> Dict[str, list]:
    """
    Returns public users and default users with grants
    """
    roles = cluster_pillar.pgusers.get_public_users() + cluster_pillar.pgusers.get_default_users()
    return {u.name: u.grants for u in roles}


def get_assignable_grants(username: str, all_roles: Dict[str, list]):
    """
    Find all not assigned yet grants that will not create cyclic dependencies
    """
    inversed_users_graph = collections.defaultdict(list)  # username's grant -> username
    for role, grants in all_roles.items():
        for grant in grants:
            inversed_users_graph[grant].append(role)

    wrong_usernames = set()

    def _traverse(user: str):
        wrong_usernames.add(user)
        for u in inversed_users_graph[user]:
            _traverse(u)

    _traverse(username)
    return [role for role in all_roles if role not in wrong_usernames]


def validate_database_owner(database_specs, user_specs):
    """
    Validate that owner present for our database
    """
    for database in database_specs:
        if database['owner'] not in map(operator.itemgetter('name'), user_specs):
            abort(422, message='Database owner \'{name}\' not found'.format(name=database['owner']))


def validate_nodes_number(num_nodes):
    """
    Validate that we support @num_node
    """
    if num_nodes < 1:
        abort(422, message='\'{cluster}\' needs at least one node in cluster '.format(cluster=MY_CLUSTER_TYPE))


def get_system_conn_limit() -> int:
    """
    Get connections count reserved for system users
    """
    return sum(
        0 if user_name == 'postgres' else user.get('conn_limit', DEFAULT_SYS_CONN_LIMIT)
        for user_name, user in get_default_users().items()
    )


def get_sum_user_conns(users: Sequence[UserWithPassword]) -> int:
    """
    Sum users conn limits
    """
    sys_users = get_default_users()
    sum_conns = 0

    for user in users:
        if user.name in {'postgres'}:
            continue

        conn_default = DEFAULT_CONN_LIMIT
        if user.name in sys_users:
            conn_default = sys_users[user.name].get('conn_limit', DEFAULT_SYS_CONN_LIMIT)
        sum_conns += user.conn_limit if user.conn_limit is not None else conn_default

    return sum_conns


def validate_mem_options(options: dict, sum_user_conns: int, flavor: dict) -> None:
    """
    Perform max_connections and shared_buffers validations
    """

    # validate `shared_buffers` if specified
    shared_buffers = options.get('shared_buffers')
    if shared_buffers:
        validate_shared_buffers(shared_buffers, flavor)

    # validate `max_connections` if specified
    max_conn = options.get('max_connections')
    validate_max_connections(max_conn, flavor, sum_user_conns)


def validate_disk_options(options: dict, disk_size: int):
    """
    Perform min_wal_size and max_wal_size validations
    """
    min_wal_size = options.get('min_wal_size')
    if min_wal_size:
        min_wal_size = humanfriendly.parse_size(min_wal_size, binary=True)
    else:
        min_wal_size = get_min_wal_size(disk_size=disk_size)

    max_wal_size = options.get('max_wal_size')
    if max_wal_size:
        max_wal_size = humanfriendly.parse_size(max_wal_size, binary=True)
    else:
        max_wal_size = get_max_wal_size(disk_size=disk_size)

    validate_min_max_wal_size(
        min_wal_size=min_wal_size,
        max_wal_size=max_wal_size,
        disk_size=disk_size,
    )

    wal_keep_size = options.get('wal_keep_size')
    if wal_keep_size:
        wal_keep_size = humanfriendly.parse_size(wal_keep_size, binary=True)
    else:
        wal_keep_size = 0
    validate_wal_keep_size(
        wal_keep_size=wal_keep_size,
        disk_size=disk_size,
    )

    max_slot_wal_keep_size = options.get('max_slot_wal_keep_size')
    if max_slot_wal_keep_size:
        max_slot_wal_keep_size = humanfriendly.parse_size(max_slot_wal_keep_size, binary=True)
        validate_max_slot_wal_keep_size(
            max_slot_wal_keep_size=max_slot_wal_keep_size,
            disk_size=disk_size,
        )


def validate_postgres_options(options, users, flavor, disk_size):
    """
    Perform postgresql-specific validations for options
    """
    validate_mem_options(options=options, sum_user_conns=get_sum_user_conns(users), flavor=flavor)
    validate_disk_options(options=options, disk_size=disk_size)


def permissions_to_databases(permissions):
    """
    Converts permissions format to flat databases list
    """
    return list(map(operator.itemgetter('database_name'), permissions))


def get_default_users() -> dict:
    """
    Get default users from app config
    """
    return current_app.config['DEFAULT_PILLAR_TEMPLATE'][MY_CLUSTER_TYPE]['data']['config']['pgusers']


def is_public_user(user_name: str, opts: dict) -> bool:
    """
    Return true if user is public
    """

    conditions = [
        user_name not in get_default_users(),
    ]
    return all(conditions)


def format_extensions(extensions):
    """
    Properly format extensions list
    """
    return [{'name': x} for x in extensions]


def parse_extensions(extensions):
    """
    Parse list of extension specs to internal representation
    """
    return sorted(set(x['name'] for x in extensions))


def get_postgresql_config(host_spec):
    """
    Get host spec pg config
    """
    try:
        # Load and validate version
        conf_obj = PostgresqlConfigSpec(host_spec['config_spec'], version_required=False)
    except ValueError as err:
        raise ParseConfigError(err)
    return conf_obj


def host_spec_make_pillar(host_spec):
    """
    Return host spec pillar
    """
    pillar = PostgresqlHostPillar(dict())
    if 'replication_source' in host_spec:
        pillar.set_repl_source(host_spec['replication_source'])
    if 'priority' in host_spec:
        pillar.set_priority(host_spec['priority'])
    if 'config_spec' in host_spec:
        pillar.set_config(get_postgresql_config(host_spec).get_config())
    return pillar


def get_cluster_version(cid):
    """
    Lookup dbaas.versions entry
    :param cid: cluster id of desired cluster
    :return: actual version of postgresql cluster
    """

    # (maj, min, edition) or None
    traits = get_cluster_traits(MY_CLUSTER_TYPE)
    cluster = metadb.get_clusters_versions(traits.versions_component, [cid])[cid]
    version = Version.load(cluster['major_version'])
    if cluster['edition'] == EDITION_1C:
        version.edition = EDITION_1C
    return version


def get_cluster_version_at_time(cid, timestamp):
    """
    Lookup dbaas.versions_revs entry
    :param cid: cluster id of desired cluster
    :param timestamp: timestamp to restore
    :return: version of postgresql cluster at timestamp
    """

    # (maj, min, edition) or None
    traits = get_cluster_traits(MY_CLUSTER_TYPE)
    rev = metadb.get_cluster_rev_by_timestamp(cid=cid, timestamp=timestamp)
    cluster = metadb.get_cluster_version_at_rev(traits.versions_component, cid, rev)[0]
    version = Version.load(cluster['major_version'])
    if cluster['edition'] == EDITION_1C:
        version.edition = EDITION_1C
    return version


def get_cluster_major_version(cid):
    traits = get_cluster_traits(MY_CLUSTER_TYPE)
    cluster = metadb.get_clusters_versions(traits.versions_component, [cid])[cid]
    return cluster['major_version']
