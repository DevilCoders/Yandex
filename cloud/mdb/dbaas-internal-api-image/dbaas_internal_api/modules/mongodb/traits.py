"""
DBaaS Internal API mongodb cluster traits
"""

from enum import Enum, unique

from ...core.auth import DEFAULT_AUTH_ACTIONS
from ...utils.register import register_cluster_traits, VersionsColumn
from ...utils.traits import ClusterName, DatabaseName, Password, ShardName, UserName
from ...utils.types import ComparableEnum
from .constants import (
    INTERNAL_USERS,
    MY_CLUSTER_TYPE,
    SYSTEM_DATABASES,
    ALLOW_DEPRECATED_VERSIONS_FEATURE_FLAG,
    ENTERPRISE_MONGO_FEATURE_FLAG,
    EDITION_ENTERPRISE,
)


class MongoDBTasks(ComparableEnum):
    """MongoDB cluster tasks"""

    create = 'mongodb_cluster_create'
    backup = 'mongodb_cluster_create_backup'
    wait_backup_service = 'mongodb_cluster_wait_backup_service'
    start = 'mongodb_cluster_start'
    stop = 'mongodb_cluster_stop'
    modify = 'mongodb_cluster_modify'
    metadata = 'mongodb_metadata_update'
    upgrade = 'mongodb_cluster_upgrade'
    delete = 'mongodb_cluster_delete'
    delete_metadata = 'mongodb_cluster_delete_metadata'
    purge = 'mongodb_cluster_purge'
    restore = 'mongodb_cluster_restore'
    enable_sharding = 'mongodb_cluster_enable_sharding'
    shard_create = 'mongodb_shard_create'
    shard_delete = 'mongodb_shard_delete'
    host_create = 'mongodb_host_create'
    host_delete = 'mongodb_host_delete'
    move = 'mongodb_cluster_move'
    move_noop = 'noop'
    user_create = 'mongodb_user_create'
    user_delete = 'mongodb_user_delete'
    user_modify = 'mongodb_user_modify'
    database_create = 'mongodb_database_create'
    database_delete = 'mongodb_database_delete'


class MongoDBOperations(ComparableEnum):
    """MongoDB cluster operations"""

    create = 'mongodb_cluster_create'
    backup = 'mongodb_cluster_create_backup'
    modify = 'mongodb_cluster_modify'
    metadata = 'mongodb_metadata_update'
    delete = 'mongodb_cluster_delete'
    restore = 'mongodb_cluster_restore'
    enable_sharding = 'mongodb_cluster_enable_sharding'
    start = 'mongodb_cluster_start'
    stop = 'mongodb_cluster_stop'
    move = 'mongodb_cluster_move'

    shard_create = 'mongodb_shard_create'
    shard_delete = 'mongodb_shard_delete'

    host_create = 'mongodb_host_create'
    host_delete = 'mongodb_host_delete'

    database_add = 'mongodb_database_add'
    database_delete = 'mongodb_database_delete'

    user_create = 'mongodb_user_create'
    user_modify = 'mongodb_user_modify'
    user_delete = 'mongodb_user_delete'
    grant_permission = 'mongodb_user_grant_permission'
    revoke_permission = 'mongodb_user_revoke_permission'

    maintenance_reschedule = 'mongodb_maintenance_reschedule'

    hosts_resetup = 'mongodb_cluster_resetup_hosts'
    hosts_restart = 'mongodb_cluster_restart_hosts'
    hosts_stepdown = 'mongodb_cluster_stepdown_hosts'
    backup_delete = 'mongodb_backup_delete'


class MongoDBRoles(ComparableEnum):
    """Roles, aka components for this type of cluster"""

    mongod = 'mongodb_cluster.mongod'
    mongos = 'mongodb_cluster.mongos'
    mongocfg = 'mongodb_cluster.mongocfg'
    mongoinfra = 'mongodb_cluster.mongoinfra'


MongoDBServices = {e.value: e.name for e in MongoDBRoles}


@register_cluster_traits(MY_CLUSTER_TYPE)
class MongoDBClusterTraits:
    """
    Traits of MongoDB clusters.
    """

    name = 'mongodb'
    url_prefix = name
    service_slug = f'managed-{name}'
    tasks = MongoDBTasks
    operations = MongoDBOperations
    roles = MongoDBRoles
    versions = [
        {
            'version': '3.6',
            'deprecated': True,
            'allow_deprecated_feature_flag': ALLOW_DEPRECATED_VERSIONS_FEATURE_FLAG,
        },
        {
            'version': '4.0',
            'deprecated': True,
            'allow_deprecated_feature_flag': ALLOW_DEPRECATED_VERSIONS_FEATURE_FLAG,
        },
        {
            'version': '4.2',
        },
        {
            'version': '4.4',
        },
        {
            'version': f'4.4-{EDITION_ENTERPRISE}',
            'feature_flag': ENTERPRISE_MONGO_FEATURE_FLAG,
        },
        {
            'version': '5.0',
            'default': True,
        },
        {
            'version': f'5.0-{EDITION_ENTERPRISE}',
            'feature_flag': ENTERPRISE_MONGO_FEATURE_FLAG,
        },
    ]
    feature_compatibility_versions = {
        '3.6': ['3.4', '3.6'],
        '4.0': ['3.6', '4.0'],
        '4.2': ['4.0', '4.2'],
        '4.4': ['4.2', '4.4'],
        f'4.4-{EDITION_ENTERPRISE}': ['4.2', '4.4'],
        '5.0': ['4.4', '5.0'],
        f'5.0-{EDITION_ENTERPRISE}': ['4.4', '5.0'],
    }
    cluster_name = ClusterName()
    shard_name = ShardName()
    db_name = DatabaseName(blacklist=SYSTEM_DATABASES)
    role_assign_db_name = DatabaseName()
    user_name = UserName(blacklist=INTERNAL_USERS)
    password = Password()
    versions_column = VersionsColumn.subcluster
    versions_component = 'mongodb'
    auth_actions = DEFAULT_AUTH_ACTIONS


@unique
class HostRole(Enum):
    """
    Possible host roles.
    """

    unknown = 'HostRoleUnknown'
    primary = 'HostRolePrimary'
    secondary = 'HostRoleSecondary'


@unique
class ServiceType(Enum):
    """
    Possible service types.
    """

    unspecified = 'ServiceTypeUnspecified'
    mongod = 'ServiceTypeMongod'
    mongos = 'ServiceTypeMongos'
    mongocfg = 'ServiceTypeMongocfg'
