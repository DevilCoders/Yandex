"""
Redis operations description
"""
from ...core.types import ResponseType
from ...utils.operations import EMPTY_RESPONSE as _EMPTY
from ...utils.operations import (
    OperationHandle,
    annotation_maker,
    describe_operations,
    modify_hosts_event_details,
    modify_hosts_metadata,
    modify_shard_event_details,
    modify_shard_metadata,
    modify_reschedule_maintenance_metadata,
    move_event_details,
    move_metadata,
    restore_event_details,
    restore_metadata,
)
from .constants import MY_CLUSTER_TYPE
from .traits import RedisOperations

__a = annotation_maker('yandex.cloud.mdb.redis.v1')
__et = annotation_maker('yandex.cloud.events.mdb.redis')
_CLUSTER = (ResponseType.cluster, __a('Cluster'))
_SHARD = (ResponseType.shard, __a('Shard'))
_BACKUP = (ResponseType.backup, __a('Backup'))

describe_operations(
    cluster_type=MY_CLUSTER_TYPE,
    operation_types=RedisOperations,
    handles={
        RedisOperations.create: OperationHandle(
            'Create Redis cluster',
            response=_CLUSTER,
            metadata_annotation=__a('CreateClusterMetadata'),
            event_type=__et('CreateCluster'),
        ),
        RedisOperations.backup: OperationHandle(
            'Create a backup for Redis cluster',
            response=_BACKUP,
            metadata_annotation=__a('BackupClusterMetadata'),
            event_type=__et('BackupCluster'),
        ),
        RedisOperations.delete: OperationHandle(
            'Delete Redis cluster',
            response=_EMPTY,
            metadata_annotation=__a('DeleteClusterMetadata'),
            event_type=__et('DeleteCluster'),
        ),
        RedisOperations.restore: OperationHandle(
            'Create new Redis cluster from the backup',
            response=_CLUSTER,
            metadata=restore_metadata,
            event_details=restore_event_details,
            metadata_annotation=__a('RestoreClusterMetadata'),
            event_type=__et('RestoreCluster'),
        ),
        RedisOperations.start_failover: OperationHandle(
            'Start manual failover on Redis cluster',
            response=_CLUSTER,
            metadata_annotation=__a('StartClusterFailoverMetadata'),
            event_type=__et('StartClusterFailover'),
        ),
        RedisOperations.rebalance: OperationHandle(
            'Rebalance slot distribution in Redis cluster',
            response=_CLUSTER,
            metadata_annotation=__a('RebalanceClusterMetadata'),
            event_type=__et('RebalanceCluster'),
        ),
        RedisOperations.modify: OperationHandle(
            'Modify Redis cluster',
            response=_CLUSTER,
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
        ),
        RedisOperations.start: OperationHandle(
            'Start Redis cluster',
            response=_CLUSTER,
            metadata_annotation=__a('StartClusterMetadata'),
            event_type=__et('StartCluster'),
        ),
        RedisOperations.stop: OperationHandle(
            'Stop Redis cluster',
            response=_CLUSTER,
            metadata_annotation=__a('StopClusterMetadata'),
            event_type=__et('StopCluster'),
        ),
        RedisOperations.move: OperationHandle(
            'Move Redis cluster',
            response=_CLUSTER,
            metadata=move_metadata,
            event_details=move_event_details,
            metadata_annotation=__a('MoveClusterMetadata'),
            event_type=__et('MoveCluster'),
        ),
        RedisOperations.host_create: OperationHandle(
            'Add hosts to Redis cluster',
            response=_EMPTY,
            metadata_annotation=__a('AddClusterHostsMetadata'),
            event_type=__et('AddClusterHosts'),
            metadata=modify_hosts_metadata,
            event_details=modify_hosts_event_details,
        ),
        RedisOperations.host_modify: OperationHandle(
            'Modify hosts in Redis cluster',
            response=_EMPTY,
            metadata_annotation=__a('UpdateClusterHostsMetadata'),
            event_type=__et('UpdateClusterHosts'),
            metadata=modify_hosts_metadata,
            event_details=modify_hosts_event_details,
        ),
        RedisOperations.host_delete: OperationHandle(
            'Delete hosts from Redis cluster',
            response=_EMPTY,
            metadata_annotation=__a('DeleteClusterHostsMetadata'),
            event_type=__et('DeleteClusterHosts'),
            metadata=modify_hosts_metadata,
            event_details=modify_hosts_event_details,
        ),
        RedisOperations.shard_create: OperationHandle(
            'Add shard to Redis cluster',
            response=_SHARD,
            metadata_annotation=__a('AddClusterShardMetadata'),
            event_type=__et('AddClusterShard'),
            metadata=modify_shard_metadata,
            event_details=modify_shard_event_details,
        ),
        RedisOperations.shard_delete: OperationHandle(
            'Delete shard from Redis cluster',
            response=_EMPTY,
            metadata_annotation=__a('DeleteClusterShardMetadata'),
            event_type=__et('DeleteClusterShard'),
            metadata=modify_shard_metadata,
            event_details=modify_shard_event_details,
        ),
        RedisOperations.maintenance_reschedule: OperationHandle(
            'Reschedule maintenance in Redis cluster',
            response=_CLUSTER,
            metadata=modify_reschedule_maintenance_metadata,
            metadata_annotation=__a('RescheduleMaintenanceMetadata'),
            event_type=__et('RescheduleMaintenance'),
        ),
    },
)
