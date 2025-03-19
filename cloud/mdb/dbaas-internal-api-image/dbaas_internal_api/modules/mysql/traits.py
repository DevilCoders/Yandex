"""
DBaaS Internal API mysql cluster traits
"""

from enum import Enum, unique

from ...core.auth import DEFAULT_AUTH_ACTIONS
from ...utils.register import register_cluster_traits, VersionsColumn
from ...utils.traits import ClusterName, DatabaseName, Password, UserName
from ...utils.types import ComparableEnum
from .constants import INTERNAL_USERS, MY_CLUSTER_TYPE, SYSTEM_DATABASES


class MySQLTasks(ComparableEnum):
    """MySQL cluster tasks"""

    create = 'mysql_cluster_create'
    modify = 'mysql_cluster_modify'
    metadata = 'mysql_metadata_update'
    upgrade80 = 'mysql_cluster_upgrade_80'
    backup = 'mysql_cluster_create_backup'
    restore = 'mysql_cluster_restore'
    delete = 'mysql_cluster_delete'
    delete_metadata = 'mysql_cluster_delete_metadata'
    purge = 'mysql_cluster_purge'
    start = 'mysql_cluster_start'
    stop = 'mysql_cluster_stop'
    host_create = 'mysql_host_create'
    host_delete = 'mysql_host_delete'
    host_modify = 'mysql_host_modify'
    move = 'mysql_cluster_move'
    start_failover = 'mysql_cluster_start_failover'
    move_noop = 'noop'
    user_create = 'mysql_user_create'
    user_delete = 'mysql_user_delete'
    user_modify = 'mysql_user_modify'
    database_create = 'mysql_database_create'
    database_delete = 'mysql_database_delete'
    wait_backup_service = 'mysql_cluster_wait_backup_service'

    alert_group_create = 'mysql_alert_group_create'
    alert_group_delete = 'mysql_alert_group_delete'


class MySQLOperations(ComparableEnum):
    """MySQL cluster operations"""

    create = 'mysql_cluster_create'
    modify = 'mysql_cluster_modify'
    metadata = 'mysql_metadata_update'
    upgrade80 = 'mysql_cluster_upgrade_80'
    backup = 'mysql_cluster_create_backup'
    restore = 'mysql_cluster_restore'
    delete = 'mysql_cluster_delete'
    start = 'mysql_cluster_start'
    stop = 'mysql_cluster_stop'
    move = 'mysql_cluster_move'
    start_failover = 'mysql_cluster_start_failover'

    host_create = 'mysql_host_create'
    host_delete = 'mysql_host_delete'
    host_modify = 'mysql_host_modify'

    database_add = 'mysql_database_add'
    database_delete = 'mysql_database_delete'

    user_create = 'mysql_user_create'
    user_modify = 'mysql_user_modify'
    user_delete = 'mysql_user_delete'
    grant_permission = 'mysql_user_grant_permission'
    revoke_permission = 'mysql_user_revoke_permission'

    maintenance_reschedule = 'mysql_maintenance_reschedule'

    alert_group_create = 'mysql_alert_group_create'
    alert_group_delete = 'mysql_alert_group_delete'


class MySQLRoles(ComparableEnum):
    """Roles, aka components for this type of cluster"""

    mysql = 'mysql_cluster'


@register_cluster_traits(MY_CLUSTER_TYPE)
class MySQLClusterTraits:
    """
    Traits of MySQL clusters.
    """

    name = 'mysql'
    url_prefix = name
    service_slug = f'managed-{name}'
    tasks = MySQLTasks
    operations = MySQLOperations
    roles = MySQLRoles
    versions = [
        {
            'version': '5.7',
        },
        {
            'version': '8.0',
            'feature_flag': 'MDB_MYSQL_8_0',
        },
    ]
    cluster_name = ClusterName()
    db_name = DatabaseName(blacklist=SYSTEM_DATABASES)
    user_name = UserName(blacklist=INTERNAL_USERS, max=32)
    password = Password(regexp="[^\0\'\"\b\n\r\t\x1A\\%]*")
    versions_column = VersionsColumn.cluster
    versions_component = 'mysql'
    auth_actions = DEFAULT_AUTH_ACTIONS


@unique
class ServiceType(Enum):
    """
    Possible service types.
    """

    unspecified = 'ServiceTypeUnspecified'
    mysql = 'ServiceTypeMysql'


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
    quorum = 'HostReplicaTypeQuorum'
