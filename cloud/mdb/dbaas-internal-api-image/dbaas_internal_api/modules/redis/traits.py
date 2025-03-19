"""
DBaaS Internal API Redis cluster traits
"""

from enum import Enum, unique

from ...core.auth import DEFAULT_AUTH_ACTIONS
from ...utils.register import register_cluster_traits, VersionsColumn
from ...utils.traits import ClusterName, Password, ShardName, ValidString
from ...utils.types import ComparableEnum
from .constants import MY_CLUSTER_TYPE


class RedisTasks(ComparableEnum):
    """Redis cluster tasks"""

    create = 'redis_cluster_create'
    backup = 'redis_cluster_create_backup'
    delete = 'redis_cluster_delete'
    delete_metadata = 'redis_cluster_delete_metadata'
    purge = 'redis_cluster_purge'
    restore = 'redis_cluster_restore'
    modify = 'redis_cluster_modify'
    upgrade = 'redis_cluster_upgrade'
    start = 'redis_cluster_start'
    stop = 'redis_cluster_stop'
    move = 'redis_cluster_move'
    move_noop = 'noop'
    host_create = 'redis_host_create'
    host_modify = 'redis_host_modify'
    host_delete = 'redis_host_delete'
    start_failover = 'redis_cluster_start_failover'
    rebalance = 'redis_cluster_rebalance'
    shard_host_create = 'redis_shard_host_create'
    shard_host_delete = 'redis_shard_host_delete'
    shard_create = 'redis_shard_create'
    shard_delete = 'redis_shard_delete'


class RedisOperations(ComparableEnum):
    """Redis cluster operations"""

    create = 'redis_cluster_create'
    backup = 'redis_cluster_create_backup'
    delete = 'redis_cluster_delete'
    restore = 'redis_cluster_restore'
    modify = 'redis_cluster_modify'
    start = 'redis_cluster_start'
    stop = 'redis_cluster_stop'
    move = 'redis_cluster_move'
    start_failover = 'redis_cluster_start_failover'
    rebalance = 'redis_cluster_rebalance'
    host_create = 'redis_host_create'
    host_modify = 'redis_host_modify'
    host_delete = 'redis_host_delete'
    shard_create = 'redis_shard_create'
    shard_delete = 'redis_shard_delete'
    maintenance_reschedule = 'redis_maintenance_reschedule'


class RedisRoles(ComparableEnum):
    """Redis cluster roles"""

    redis = 'redis_cluster'


@register_cluster_traits(MY_CLUSTER_TYPE)
class RedisClusterTraits:
    """Redis clusters traits"""

    name = 'redis'
    url_prefix = name
    service_slug = f'managed-{name}'
    tasks = RedisTasks
    operations = RedisOperations
    roles = RedisRoles
    cluster_name = ClusterName()
    shard_name = ShardName()
    password = Password(regexp='[a-zA-Z0-9@=+?*.,!&#$^<>_-]*')
    versions_column = VersionsColumn.cluster
    versions_component = 'redis'
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
class ServiceType(Enum):
    """
    Possible service types.
    """

    unspecified = 'ServiceTypeUnspecified'
    redis = 'ServiceTypeRedis'
    sentinel = 'ServiceTypeSentinel'
    redis_cluster = 'ServiceTypeRedisCluster'


@unique
class PersistenceModes(Enum):
    """
    Possible persistence modes.
    """

    on = 'on'
    off = 'off'


class MemoryUnits(Enum):
    """
    Possible memory unit measures.
    """

    bytes = 'b'
    kilobytes = 'kb'
    megabytes = 'mb'
    gigabytes = 'gb'


class NotifyKeyspaceEvents(ValidString):
    """
    Describes notify-keyspace-events field.
    """

    # pylint: disable=redefined-builtin
    def __init__(self, allowed_symbols: str) -> None:
        regexp = f'[{allowed_symbols}]*'
        min_len = 0
        max_len = len(allowed_symbols)
        name = 'Notify keyspace events'
        error_messages = {'regexp_mismatch': '{name} \'{value}\' should be a subset of ' + allowed_symbols}
        super().__init__(regexp=regexp, min=min_len, max=max_len, name=name, error_messages=error_messages)
