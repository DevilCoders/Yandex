"""
DBaaS Internal API clickhouse cluster traits
"""

from enum import Enum, unique

from ...core.auth import DEFAULT_AUTH_ACTIONS
from ...utils.register import register_cluster_traits, VersionsColumn
from ...utils.traits import ClusterName, DatabaseName, ExternalResourceURI, Password, ShardName, UserName, ValidString
from ...utils.types import ComparableEnum
from .constants import MY_CLUSTER_TYPE


class ClickhouseTasks(ComparableEnum):
    """ClickHouse cluster tasks"""

    create = 'clickhouse_cluster_create'
    backup = 'clickhouse_cluster_create_backup'
    start = 'clickhouse_cluster_start'
    stop = 'clickhouse_cluster_stop'
    modify = 'clickhouse_cluster_modify'
    metadata = 'clickhouse_metadata_update'
    upgrade = 'clickhouse_cluster_upgrade'
    delete = 'clickhouse_cluster_delete'
    delete_metadata = 'clickhouse_cluster_delete_metadata'
    purge = 'clickhouse_cluster_purge'
    restore = 'clickhouse_cluster_restore'
    host_create = 'clickhouse_host_create'
    host_delete = 'clickhouse_host_delete'
    zookeeper_host_create = 'clickhouse_zookeeper_host_create'
    zookeeper_host_delete = 'clickhouse_zookeeper_host_delete'
    shard_create = 'clickhouse_shard_create'
    shard_modify = 'clickhouse_shard_modify'
    shard_delete = 'clickhouse_shard_delete'
    add_zookeeper = 'clickhouse_add_zookeeper'
    move = 'clickhouse_cluster_move'
    move_noop = 'noop'
    user_create = 'clickhouse_user_create'
    user_delete = 'clickhouse_user_delete'
    user_modify = 'clickhouse_user_modify'
    database_create = 'clickhouse_database_create'
    database_delete = 'clickhouse_database_delete'
    dictionary_create = 'clickhouse_dictionary_create'
    dictionary_delete = 'clickhouse_dictionary_delete'
    model_create = 'clickhouse_model_create'
    model_delete = 'clickhouse_model_delete'
    model_modify = 'clickhouse_model_modify'
    format_schema_create = 'clickhouse_format_schema_create'
    format_schema_delete = 'clickhouse_format_schema_delete'
    format_schema_modify = 'clickhouse_format_schema_modify'


class ClickhouseOperations(ComparableEnum):
    """ClickHouse cluster operations"""

    create = 'clickhouse_cluster_create'
    backup = 'clickhouse_cluster_create_backup'
    modify = 'clickhouse_cluster_modify'
    metadata = 'clickhouse_metadata_update'
    delete = 'clickhouse_cluster_delete'
    restore = 'clickhouse_cluster_restore'
    start = 'clickhouse_cluster_start'
    stop = 'clickhouse_cluster_stop'
    move = 'clickhouse_cluster_move'

    add_zookeeper = 'clickhouse_add_zookeeper'
    add_dictionary = 'clickhouse_add_dictionary'
    update_dictionary = 'clickhouse_update_dictionary'
    delete_dictionary = 'clickhouse_delete_dictionary'

    host_create = 'clickhouse_host_create'
    host_delete = 'clickhouse_host_delete'
    host_modify = 'clickhouse_host_modify'

    shard_create = 'clickhouse_shard_create'
    shard_modify = 'clickhouse_shard_modify'
    shard_delete = 'clickhouse_shard_delete'

    database_add = 'clickhouse_database_add'
    database_delete = 'clickhouse_database_delete'

    user_create = 'clickhouse_user_create'
    user_modify = 'clickhouse_user_modify'
    user_delete = 'clickhouse_user_delete'
    grant_permission = 'clickhouse_user_grant_permission'
    revoke_permission = 'clickhouse_user_revoke_permission'

    model_create = 'clickhouse_model_create'
    model_delete = 'clickhouse_model_delete'
    model_modify = 'clickhouse_model_modify'

    format_schema_create = 'clickhouse_format_schema_create'
    format_schema_delete = 'clickhouse_format_schema_delete'
    format_schema_modify = 'clickhouse_format_schema_modify'

    shard_group_create = 'clickhouse_shard_group_create'
    shard_group_delete = 'clickhouse_shard_group_delete'
    shard_group_modify = 'clickhouse_shard_group_update'

    maintenance_reschedule = 'clickhouse_maintenance_reschedule'


class ClickhouseRoles(ComparableEnum):
    """Roles, aka components for this type of cluster"""

    clickhouse = 'clickhouse_cluster'
    zookeeper = 'zk'


@register_cluster_traits(MY_CLUSTER_TYPE)
class ClickhouseClusterTraits:
    """
    Traits of ClickHouse clusters.
    """

    name = 'clickhouse'
    url_prefix = name
    service_slug = f'managed-{name}'
    tasks = ClickhouseTasks
    operations = ClickhouseOperations
    roles = ClickhouseRoles
    cluster_name = ClusterName()
    shard_name = ShardName(regexp='[a-zA-Z0-9][a-zA-Z0-9-]*')
    db_name = DatabaseName()
    user_name = UserName(regexp='[a-zA-Z_][a-zA-Z0-9_-]*')
    ml_model_name = ValidString(regexp='[a-zA-Z_-][a-zA-Z0-9_-]*', min=1, max=63, name='ML model name')
    ml_model_uri = ExternalResourceURI()
    format_schema_name = ValidString(regexp='[a-zA-Z_-][a-zA-Z0-9_-]*', min=1, max=63, name='format schema name')
    format_schema_uri = ExternalResourceURI()
    geobase_uri = ExternalResourceURI(suffix_regexp=r'\.(?:tar|tar\.gz|tgz|zip)(?:\?\S+)?')
    password = Password()
    versions_column = VersionsColumn.subcluster
    versions_component = 'clickhouse'
    auth_actions = DEFAULT_AUTH_ACTIONS


@unique
class ServiceType(Enum):
    """
    Possible service types.
    """

    unspecified = 'ServiceTypeUnspecified'
    clickhouse = 'ServiceTypeClickHouse'
    zookeeper = 'ServiceTypeZooKeeper'
