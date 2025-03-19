"""
MongoDB operations description
"""

from ...core.types import ResponseType
from ...utils.operations import EMPTY_RESPONSE as _EMPTY
from ...utils.operations import (
    OperationHandle,
    annotation_maker,
    delete_backup_metadata,
    describe_operations,
    modify_database_event_details,
    modify_database_metadata,
    modify_hosts_event_details,
    modify_hosts_metadata,
    modify_shard_event_details,
    modify_shard_metadata,
    modify_reschedule_maintenance_metadata,
    modify_user_event_details,
    modify_user_metadata,
    mongodb_hosts_resetup_metadata,
    mongodb_hosts_restart_metadata,
    mongodb_hosts_stepdown_metadata,
    move_event_details,
    move_metadata,
    restore_event_details,
    restore_metadata,
)
from .constants import MY_CLUSTER_TYPE
from .traits import MongoDBOperations

__a = annotation_maker('yandex.cloud.mdb.mongodb.v1')
__et = annotation_maker('yandex.cloud.events.mdb.mongodb')
_CLUSTER = (ResponseType.cluster, __a('Cluster'))
_SHARD = (ResponseType.shard, __a('Shard'))
_BACKUP = (ResponseType.backup, __a('Backup'))
_DATABASE = (ResponseType.database, __a('Database'))
_USER = (ResponseType.user, __a('User'))

describe_operations(
    cluster_type=MY_CLUSTER_TYPE,
    operation_types=MongoDBOperations,
    handles={
        MongoDBOperations.create: OperationHandle(
            'Create MongoDB cluster',
            metadata_annotation=__a('CreateClusterMetadata'),
            event_type=__et('CreateCluster'),
            response=_CLUSTER,
        ),
        MongoDBOperations.modify: OperationHandle(
            'Modify MongoDB cluster',
            response=_CLUSTER,
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
        ),
        MongoDBOperations.metadata: OperationHandle(
            'Update MongoDB cluster metadata',
            response=_CLUSTER,
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
        ),
        MongoDBOperations.start: OperationHandle(
            'Start MongoDB cluster',
            response=_CLUSTER,
            metadata_annotation=__a('StartClusterMetadata'),
            event_type=__et('StartCluster'),
        ),
        MongoDBOperations.stop: OperationHandle(
            'Stop MongoDB cluster',
            response=_CLUSTER,
            metadata_annotation=__a('StopClusterMetadata'),
            event_type=__et('StopCluster'),
        ),
        MongoDBOperations.move: OperationHandle(
            'Move MongoDB cluster',
            response=_CLUSTER,
            metadata=move_metadata,
            event_details=move_event_details,
            metadata_annotation=__a('MoveClusterMetadata'),
            event_type=__et('MoveCluster'),
        ),
        MongoDBOperations.delete: OperationHandle(
            'Delete MongoDB cluster',
            metadata_annotation=__a('DeleteClusterMetadata'),
            event_type=__et('DeleteCluster'),
            response=_EMPTY,
        ),
        MongoDBOperations.restore: OperationHandle(
            'Create new MongoDB cluster from the backup',
            response=_CLUSTER,
            metadata=restore_metadata,
            event_details=restore_event_details,
            metadata_annotation=__a('RestoreClusterMetadata'),
            event_type=__et('RestoreCluster'),
        ),
        MongoDBOperations.backup: OperationHandle(
            'Create a backup for MongoDB cluster',
            response=_BACKUP,
            metadata_annotation=__a('BackupClusterMetadata'),
            event_type=__et('BackupCluster'),
        ),
        MongoDBOperations.host_create: OperationHandle(
            'Add hosts to MongoDB cluster',
            response=_EMPTY,
            metadata_annotation=__a('AddClusterHostsMetadata'),
            event_type=__et('AddClusterHosts'),
            metadata=modify_hosts_metadata,
            event_details=modify_hosts_event_details,
        ),
        MongoDBOperations.host_delete: OperationHandle(
            'Delete hosts from MongoDB cluster',
            response=_EMPTY,
            metadata_annotation=__a('DeleteClusterHostsMetadata'),
            event_type=__et('DeleteClusterHosts'),
            metadata=modify_hosts_metadata,
            event_details=modify_hosts_event_details,
        ),
        MongoDBOperations.database_add: OperationHandle(
            'Add database to MongoDB cluster',
            response=_DATABASE,
            metadata=modify_database_metadata,
            event_details=modify_database_event_details,
            metadata_annotation=__a('CreateDatabaseMetadata'),
            event_type=__et('CreateDatabase'),
        ),
        MongoDBOperations.database_delete: OperationHandle(
            'Delete database from MongoDB cluster',
            response=_EMPTY,
            metadata=modify_database_metadata,
            event_details=modify_database_event_details,
            metadata_annotation=__a('DeleteDatabaseMetadata'),
            event_type=__et('DeleteDatabase'),
        ),
        MongoDBOperations.user_create: OperationHandle(
            'Create user in MongoDB cluster',
            response=_USER,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('CreateUserMetadata'),
            event_type=__et('CreateUser'),
        ),
        MongoDBOperations.user_modify: OperationHandle(
            'Modify user in MongoDB cluster',
            response=_USER,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('UpdateUserMetadata'),
            event_type=__et('UpdateUser'),
        ),
        MongoDBOperations.user_delete: OperationHandle(
            'Delete user from MongoDB cluster',
            response=_EMPTY,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('DeleteUserMetadata'),
            event_type=__et('DeleteUser'),
        ),
        MongoDBOperations.grant_permission: OperationHandle(
            'Grant permission to user in MongoDB cluster',
            response=_USER,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('GrantUserPermissionMetadata'),
            event_type=__et('GrantUserPermission'),
        ),
        MongoDBOperations.revoke_permission: OperationHandle(
            'Revoke permission from user in MongoDB cluster',
            response=_USER,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('RevokeUserPermissionMetadata'),
            event_type=__et('RevokeUserPermission'),
        ),
        MongoDBOperations.enable_sharding: OperationHandle(
            'Enable sharding for MongoDB cluster',
            response=_EMPTY,
            metadata_annotation=__a('EnableClusterShardingMetadata'),
            event_type=__et('EnableClusterSharding'),
        ),
        MongoDBOperations.shard_create: OperationHandle(
            'Add shard to MongoDB cluster',
            response=_SHARD,
            metadata_annotation=__a('AddClusterShardMetadata'),
            event_type=__et('AddClusterShard'),
            metadata=modify_shard_metadata,
            event_details=modify_shard_event_details,
        ),
        MongoDBOperations.shard_delete: OperationHandle(
            'Delete shard from MongoDB cluster',
            response=_EMPTY,
            metadata_annotation=__a('DeleteClusterShardMetadata'),
            event_type=__et('DeleteClusterShard'),
            metadata=modify_shard_metadata,
            event_details=modify_shard_event_details,
        ),
        MongoDBOperations.maintenance_reschedule: OperationHandle(
            'Reschedule maintenance in MongoDB cluster',
            response=_CLUSTER,
            metadata=modify_reschedule_maintenance_metadata,
            metadata_annotation=__a('RescheduleMaintenanceMetadata'),
            event_type=__et('RescheduleMaintenance'),
        ),
        MongoDBOperations.hosts_resetup: OperationHandle(
            'Resetup given hosts',
            response=_CLUSTER,
            metadata=mongodb_hosts_resetup_metadata,
            metadata_annotation=__a('HostsResetupMetadata'),
            event_type=__et('HostsResetup'),
        ),
        MongoDBOperations.hosts_restart: OperationHandle(
            'Restart given hosts',
            response=_CLUSTER,
            metadata=mongodb_hosts_restart_metadata,
            metadata_annotation=__a('HostsRestartMetadata'),
            event_type=__et('HostsRestart'),
        ),
        MongoDBOperations.hosts_stepdown: OperationHandle(
            'Stepdown given hosts',
            response=_CLUSTER,
            metadata=mongodb_hosts_stepdown_metadata,
            metadata_annotation=__a('HostsStepdownMetadata'),
            event_type=__et('HostsStepdown'),
        ),
        MongoDBOperations.backup_delete: OperationHandle(
            'Delete given backup',
            response=_CLUSTER,
            metadata=delete_backup_metadata,
            metadata_annotation=__a('DeleteBackupMetadata'),
            event_type=__et('DeleteBackupMaintenance'),
        ),
    },
)
