"""
DBaaS Internal API postgresql cluster traits
"""

from enum import Enum, unique

from ...core.auth import DEFAULT_AUTH_ACTIONS
from ...utils.register import register_cluster_traits, VersionsColumn
from ...utils.traits import ClusterName, DatabaseName, Password, UserName
from ...utils.types import ComparableEnum
from .constants import MY_CLUSTER_TYPE, EDITION_1C, ALLOW_DEPRECATED_VERSIONS_FEATURE_FLAG


class PostgresqlTasks(ComparableEnum):
    """Postgresql cluster tasks"""

    create = 'postgresql_cluster_create'
    modify = 'postgresql_cluster_modify'
    metadata = 'postgresql_metadata_update'

    upgrade11_1c = 'postgresql_cluster_upgrade_11_1c'
    upgrade11 = 'postgresql_cluster_upgrade_11'
    upgrade12 = 'postgresql_cluster_upgrade_12'
    upgrade13 = 'postgresql_cluster_upgrade_13'
    upgrade14 = 'postgresql_cluster_upgrade_14'

    delete = 'postgresql_cluster_delete'
    delete_metadata = 'postgresql_cluster_delete_metadata'
    purge = 'postgresql_cluster_purge'

    restore = 'postgresql_cluster_restore'
    backup = 'postgresql_cluster_create_backup'

    start = 'postgresql_cluster_start'
    stop = 'postgresql_cluster_stop'
    move = 'postgresql_cluster_move'

    start_failover = 'postgresql_cluster_start_failover'
    move_noop = 'noop'

    host_create = 'postgresql_host_create'
    host_delete = 'postgresql_host_delete'
    host_modify = 'postgresql_host_modify'

    user_create = 'postgresql_user_create'
    user_delete = 'postgresql_user_delete'
    user_modify = 'postgresql_user_modify'

    database_create = 'postgresql_database_create'
    database_delete = 'postgresql_database_delete'
    database_modify = 'postgresql_database_modify'

    alert_group_create = 'postgresql_alert_group_create'
    alert_group_delete = 'postgresql_alert_group_delete'
    alert_group_modify = 'postgresql_alert_group_modify'

    wait_backup_service = 'postgresql_cluster_wait_backup_service'


class PostgresqlOperations(ComparableEnum):
    """Postgresql cluster operations"""

    create = 'postgresql_cluster_create'
    modify = 'postgresql_cluster_modify'
    metadata = 'postgresql_metadata_update'
    upgrade11_1c = 'postgresql_cluster_upgrade_11_1c'
    upgrade11 = 'postgresql_cluster_upgrade_11'
    upgrade12 = 'postgresql_cluster_upgrade_12'
    upgrade13 = 'postgresql_cluster_upgrade_13'
    upgrade14 = 'postgresql_cluster_upgrade_14'
    delete = 'postgresql_cluster_delete'
    restore = 'postgresql_cluster_restore'
    backup = 'postgresql_cluster_create_backup'
    start = 'postgresql_cluster_start'
    stop = 'postgresql_cluster_stop'
    move = 'postgresql_cluster_move'
    start_failover = 'postgresql_cluster_start_failover'

    host_create = 'postgresql_host_create'
    host_delete = 'postgresql_host_delete'
    host_modify = 'postgresql_host_modify'

    database_add = 'postgresql_database_add'
    database_modify = 'postgresql_database_modify'
    database_delete = 'postgresql_database_delete'

    user_create = 'postgresql_user_create'
    user_modify = 'postgresql_user_modify'
    user_delete = 'postgresql_user_delete'
    grant_permission = 'postgresql_user_grant_permission'
    revoke_permission = 'postgresql_user_revoke_permission'

    maintenance_reschedule = 'postgresql_maintenance_reschedule'

    alert_group_create = 'postgresql_alert_group_create'
    alert_group_delete = 'postgresql_alert_group_delete'
    alert_group_modify = 'postgresql_alert_group_modify'


class PostgresqlRoles(ComparableEnum):
    """Roles, aka components for this type of cluster"""

    postgresql = 'postgresql_cluster'


@register_cluster_traits(MY_CLUSTER_TYPE)
class PostgresqlClusterTraits:
    """
    Traits of Postgresql clusters.
    """

    name = 'postgresql'
    url_prefix = name
    service_slug = f'managed-{name}'
    tasks = PostgresqlTasks
    operations = PostgresqlOperations
    roles = PostgresqlRoles
    versions = [
        {
            'version': '10',
            'versioned_config_key': 'postgresqlConfig_10',
            'allow_deprecated_feature_flag': ALLOW_DEPRECATED_VERSIONS_FEATURE_FLAG,
        },
        {
            'version': f'10-{EDITION_1C}',
            'feature_flag': 'MDB_POSTGRESQL_10_1C',
            'versioned_config_key': 'postgresqlConfig_10_1c',
            'allow_deprecated_feature_flag': ALLOW_DEPRECATED_VERSIONS_FEATURE_FLAG,
        },
        {
            'version': '11',
            'versioned_config_key': 'postgresqlConfig_11',
        },
        {
            'version': f'11-{EDITION_1C}',
            'feature_flag': 'MDB_POSTGRESQL_11_1C',
            'versioned_config_key': 'postgresqlConfig_11_1c',
        },
        {
            'version': '12',
            'versioned_config_key': 'postgresqlConfig_12',
        },
        {
            'version': f'12-{EDITION_1C}',
            'feature_flag': 'MDB_POSTGRESQL_12_1C',
            'versioned_config_key': 'postgresqlConfig_12_1c',
        },
        {
            'version': '13',
            'versioned_config_key': 'postgresqlConfig_13',
        },
        {
            'version': f'13-{EDITION_1C}',
            'feature_flag': 'MDB_POSTGRESQL_13_1C',
            'versioned_config_key': 'postgresqlConfig_13_1c',
        },
        {
            'version': '14',
            'versioned_config_key': 'postgresqlConfig_14',
        },
        {
            'version': f'14-{EDITION_1C}',
            'feature_flag': 'MDB_POSTGRESQL_14_1C',
            'versioned_config_key': 'postgresqlConfig_14_1c',
        },
    ]
    cluster_name = ClusterName()
    db_name = DatabaseName(blacklist=['template0', 'template1', 'postgres'])
    user_name = UserName(
        regexp='(?!pg_)[a-zA-Z0-9_][a-zA-Z0-9_-]*',
        blacklist=[
            'admin',
            'repl',
            'monitor',
            'postgres',
            'public',
            'none',
            'mdb_admin',
            'mdb_replication',
            'mdb_monitor',
        ],
    )
    role_name = UserName(
        regexp='(?!pg_)[a-zA-Z0-9_][a-zA-Z0-9_-]*',
        blacklist=[
            'admin',
            'repl',
            'monitor',
            'postgres',
            'public',
            'none',
        ],
    )
    password = Password()
    versions_column = VersionsColumn.cluster
    versions_component = 'postgres'
    auth_actions = DEFAULT_AUTH_ACTIONS


@unique
class HostRole(Enum):
    """
    Possible host roles.
    """

    unknown = 'HostRoleUnknown'
    master = 'HostRoleMaster'
    replica = 'HostRoleReplica'


@unique
class HostReplicaType(Enum):
    """
    Possible host replica types.
    """

    unknown = 'HostReplicaTypeUnknown'
    a_sync = 'HostReplicaTypeAsync'
    sync = 'HostReplicaTypeSync'


@unique
class ServiceType(Enum):
    """
    Possible service types.
    """

    unspecified = 'ServiceTypeUnspecified'
    postgres = 'ServiceTypePostgres'
    pooler = 'ServiceTypePooler'
