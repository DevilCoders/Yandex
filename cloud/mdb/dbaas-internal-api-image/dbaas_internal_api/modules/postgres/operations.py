"""
Postgresql operations description
"""
from google.protobuf.descriptor import Descriptor
from google.protobuf.message import Message
from yandex.cloud.events.mdb.postgresql import cluster_pb2, database_pb2, user_pb2
from ...core.types import ResponseType
from ...utils.operations import EMPTY_RESPONSE as _EMPTY
from ...utils.operations import (
    OperationHandle,
    annotation_maker,
    describe_operations,
    modify_database_event_details,
    modify_database_metadata,
    modify_hosts_event_details,
    modify_hosts_metadata,
    modify_reschedule_maintenance_metadata,
    modify_user_event_details,
    modify_user_metadata,
    move_event_details,
    move_metadata,
    modify_alert_group_event_details,
    operation_request_to_dict_factory,
    restore_event_details,
    restore_metadata,
)
from .constants import MY_CLUSTER_TYPE
from .traits import PostgresqlOperations

__a = annotation_maker('yandex.cloud.mdb.postgresql.v1')
__et = annotation_maker('yandex.cloud.events.mdb.postgresql')
_CLUSTER = (ResponseType.cluster, __a('Cluster'))
_BACKUP = (ResponseType.backup, __a('Backup'))
_ALERT_GROUP = (ResponseType.alert_group, __a('AlertGroup'))
_DATABASE = (ResponseType.database, __a('Database'))
_USER = (ResponseType.user, __a('User'))
_rp = operation_request_to_dict_factory


def _rpa(proto: Message) -> str:
    """
    protobuf type -> type
    """
    descriptor: Descriptor = proto.DESCRIPTOR
    return descriptor.full_name


describe_operations(
    cluster_type=MY_CLUSTER_TYPE,
    operation_types=PostgresqlOperations,
    handles={
        PostgresqlOperations.create: OperationHandle(
            'Create PostgreSQL cluster',
            metadata_annotation=__a('CreateClusterMetadata'),
            event_type=__et('CreateCluster'),
            response=_CLUSTER,
            request_parameters_annotation=_rpa(cluster_pb2.CreateCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.CreateCluster().RequestParameters),
        ),
        PostgresqlOperations.modify: OperationHandle(
            'Modify PostgreSQL cluster',
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
            response=_CLUSTER,
            request_parameters_annotation=_rpa(cluster_pb2.UpdateCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.UpdateCluster().RequestParameters),
        ),
        PostgresqlOperations.metadata: OperationHandle(
            'Update PostgreSQL cluster metadata',
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
            response=_CLUSTER,
            request_parameters_annotation=_rpa(cluster_pb2.UpdateCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.UpdateCluster().RequestParameters),
        ),
        PostgresqlOperations.start_failover: OperationHandle(
            'Start manual failover on PostgreSQL cluster',
            response=_CLUSTER,
            metadata_annotation=__a('StartClusterFailoverMetadata'),
            event_type=__et('StartClusterFailover'),
            request_parameters_annotation=_rpa(cluster_pb2.StartClusterFailoverRequest().RequestParameters),
            request_parameters=_rp(cluster_pb2.StartClusterFailoverRequest().RequestParameters),
        ),
        PostgresqlOperations.start: OperationHandle(
            'Start PostgreSQL cluster',
            metadata_annotation=__a('StartClusterMetadata'),
            event_type=__et('StartCluster'),
            response=_CLUSTER,
            request_parameters_annotation=_rpa(cluster_pb2.StartCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.StartCluster().RequestParameters),
        ),
        PostgresqlOperations.stop: OperationHandle(
            'Stop PostgreSQL cluster',
            metadata_annotation=__a('StopClusterMetadata'),
            event_type=__et('StopCluster'),
            response=_CLUSTER,
            request_parameters_annotation=_rpa(cluster_pb2.StopCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.StopCluster().RequestParameters),
        ),
        PostgresqlOperations.move: OperationHandle(
            'Move PostgreSQL cluster',
            metadata=move_metadata,
            event_details=move_event_details,
            metadata_annotation=__a('MoveClusterMetadata'),
            event_type=__et('MoveCluster'),
            response=_CLUSTER,
            request_parameters_annotation=_rpa(cluster_pb2.MoveCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.MoveCluster().RequestParameters),
        ),
        PostgresqlOperations.upgrade11: OperationHandle(
            'Upgrade PostgreSQL cluster to 11 version',
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
            response=_CLUSTER,
            request_parameters_annotation=_rpa(cluster_pb2.UpdateCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.UpdateCluster().RequestParameters),
        ),
        PostgresqlOperations.upgrade11_1c: OperationHandle(
            'Upgrade PostgreSQL cluster to 11-1c version',
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
            response=_CLUSTER,
            request_parameters_annotation=_rpa(cluster_pb2.UpdateCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.UpdateCluster().RequestParameters),
        ),
        PostgresqlOperations.upgrade12: OperationHandle(
            'Upgrade PostgreSQL cluster to 12 version',
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
            response=_CLUSTER,
            request_parameters_annotation=_rpa(cluster_pb2.UpdateCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.UpdateCluster().RequestParameters),
        ),
        PostgresqlOperations.upgrade13: OperationHandle(
            'Upgrade PostgreSQL cluster to 13 version',
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
            response=_CLUSTER,
            request_parameters_annotation=_rpa(cluster_pb2.UpdateCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.UpdateCluster().RequestParameters),
        ),
        PostgresqlOperations.upgrade14: OperationHandle(
            'Upgrade PostgreSQL cluster to 14 version',
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
            response=_CLUSTER,
            request_parameters_annotation=_rpa(cluster_pb2.UpdateCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.UpdateCluster().RequestParameters),
        ),
        PostgresqlOperations.delete: OperationHandle(
            'Delete PostgreSQL cluster',
            metadata_annotation=__a('DeleteClusterMetadata'),
            event_type=__et('DeleteCluster'),
            response=_EMPTY,
            request_parameters_annotation=_rpa(cluster_pb2.DeleteCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.DeleteCluster().RequestParameters),
        ),
        PostgresqlOperations.restore: OperationHandle(
            'Create new PostgreSQL cluster from the backup',
            response=_CLUSTER,
            metadata=restore_metadata,
            event_details=restore_event_details,
            metadata_annotation=__a('RestoreClusterMetadata'),
            event_type=__et('RestoreCluster'),
            request_parameters_annotation=_rpa(cluster_pb2.RestoreCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.RestoreCluster().RequestParameters),
        ),
        PostgresqlOperations.alert_group_create: OperationHandle(
            'Create a Alert Group for PostgreSQL cluster',
            response=_ALERT_GROUP,
            metadata_annotation=__a('CreateAlertsGroupClusterMetadata'),
            event_details=modify_alert_group_event_details,
            event_type=__et('UpdateCluster'),
        ),
        PostgresqlOperations.alert_group_modify: OperationHandle(
            'Modify Alert Group for PostgreSQL cluster',
            response=_ALERT_GROUP,
            event_details=modify_alert_group_event_details,
            metadata_annotation=__a('ModifyAlertsGroupClusterMetadata'),
            event_type=__et('UpdateCluster'),
        ),
        PostgresqlOperations.alert_group_delete: OperationHandle(
            'Delete Alert Group for PostgreSQL cluster',
            response=_ALERT_GROUP,
            event_details=modify_alert_group_event_details,
            metadata_annotation=__a('DeleteAlertsGroupClusterMetadata'),
            event_type=__et('UpdateCluster'),
        ),
        PostgresqlOperations.backup: OperationHandle(
            'Create a backup for PostgreSQL cluster',
            response=_BACKUP,
            metadata_annotation=__a('BackupClusterMetadata'),
            event_type=__et('BackupCluster'),
            request_parameters_annotation=_rpa(cluster_pb2.BackupCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.BackupCluster().RequestParameters),
        ),
        PostgresqlOperations.host_create: OperationHandle(
            'Add hosts to PostgreSQL cluster',
            response=_EMPTY,
            metadata_annotation=__a('AddClusterHostsMetadata'),
            event_type=__et('AddClusterHosts'),
            metadata=modify_hosts_metadata,
            event_details=modify_hosts_event_details,
            request_parameters_annotation=_rpa(cluster_pb2.AddClusterHosts().RequestParameters),
            request_parameters=_rp(cluster_pb2.AddClusterHosts().RequestParameters),
        ),
        PostgresqlOperations.host_delete: OperationHandle(
            'Delete hosts from PostgreSQL cluster',
            response=_EMPTY,
            metadata_annotation=__a('DeleteClusterHostsMetadata'),
            event_type=__et('DeleteClusterHosts'),
            metadata=modify_hosts_metadata,
            event_details=modify_hosts_event_details,
            request_parameters_annotation=_rpa(cluster_pb2.DeleteClusterHosts().RequestParameters),
            request_parameters=_rp(cluster_pb2.DeleteClusterHosts().RequestParameters),
        ),
        # XXX: there are no this task in API!
        PostgresqlOperations.host_modify: OperationHandle(
            'Modify hosts in PostgreSQL cluster',
            response=_EMPTY,
            metadata_annotation=__a('UpdateClusterHostsMetadata'),
            event_type=__et('UpdateClusterHosts'),
            metadata=modify_hosts_metadata,
            event_details=modify_hosts_event_details,
            request_parameters_annotation=_rpa(cluster_pb2.UpdateClusterHosts().RequestParameters),
            request_parameters=_rp(cluster_pb2.UpdateClusterHosts().RequestParameters),
        ),
        PostgresqlOperations.database_add: OperationHandle(
            'Add database to PostgreSQL cluster',
            response=_DATABASE,
            metadata=modify_database_metadata,
            event_details=modify_database_event_details,
            metadata_annotation=__a('CreateDatabaseMetadata'),
            event_type=__et('CreateDatabase'),
            request_parameters_annotation=_rpa(database_pb2.CreateDatabase().RequestParameters),
            request_parameters=_rp(database_pb2.CreateDatabase().RequestParameters),
        ),
        PostgresqlOperations.database_modify: OperationHandle(
            'Modify database in PostgreSQL cluster',
            response=_DATABASE,
            metadata=modify_database_metadata,
            event_details=modify_database_event_details,
            metadata_annotation=__a('UpdateDatabaseMetadata'),
            event_type=__et('UpdateDatabase'),
            request_parameters_annotation=_rpa(database_pb2.UpdateDatabase().RequestParameters),
            request_parameters=_rp(database_pb2.UpdateDatabase().RequestParameters),
        ),
        PostgresqlOperations.database_delete: OperationHandle(
            'Delete database from PostgreSQL cluster',
            response=_EMPTY,
            metadata=modify_database_metadata,
            event_details=modify_database_event_details,
            metadata_annotation=__a('DeleteDatabaseMetadata'),
            event_type=__et('DeleteDatabase'),
            request_parameters_annotation=_rpa(database_pb2.DeleteDatabase().RequestParameters),
            request_parameters=_rp(database_pb2.DeleteDatabase().RequestParameters),
        ),
        PostgresqlOperations.user_create: OperationHandle(
            'Create user in PostgreSQL cluster',
            response=_USER,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('CreateUserMetadata'),
            event_type=__et('CreateUser'),
            request_parameters_annotation=_rpa(user_pb2.CreateUser().RequestParameters),
            request_parameters=_rp(user_pb2.CreateUser().RequestParameters),
        ),
        PostgresqlOperations.user_modify: OperationHandle(
            'Modify user in PostgreSQL cluster',
            response=_USER,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('UpdateUserMetadata'),
            event_type=__et('UpdateUser'),
            request_parameters_annotation=_rpa(user_pb2.UpdateUser().RequestParameters),
            request_parameters=_rp(user_pb2.UpdateUser().RequestParameters),
        ),
        PostgresqlOperations.user_delete: OperationHandle(
            'Delete user from PostgreSQL cluster',
            response=_EMPTY,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('DeleteUserMetadata'),
            event_type=__et('DeleteUser'),
            request_parameters_annotation=_rpa(user_pb2.DeleteUser().RequestParameters),
            request_parameters=_rp(user_pb2.DeleteUser().RequestParameters),
        ),
        PostgresqlOperations.grant_permission: OperationHandle(
            'Grant permission to user in PostgreSQL cluster',
            response=_USER,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('GrantUserPermissionMetadata'),
            event_type=__et('GrantUserPermission'),
            request_parameters_annotation=_rpa(user_pb2.GrantUserPermission().RequestParameters),
            request_parameters=_rp(user_pb2.GrantUserPermission().RequestParameters),
        ),
        PostgresqlOperations.revoke_permission: OperationHandle(
            'Revoke permission from user in PostgreSQL cluster',
            response=_USER,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('RevokeUserPermissionMetadata'),
            event_type=__et('RevokeUserPermission'),
            request_parameters_annotation=_rpa(user_pb2.RevokeUserPermission().RequestParameters),
            request_parameters=_rp(user_pb2.RevokeUserPermission().RequestParameters),
        ),
        PostgresqlOperations.maintenance_reschedule: OperationHandle(
            'Reschedule maintenance in PostgreSQL cluster',
            response=_CLUSTER,
            metadata=modify_reschedule_maintenance_metadata,
            metadata_annotation=__a('RescheduleMaintenanceMetadata'),
            event_type=__et('RescheduleMaintenance'),
            request_parameters_annotation=_rpa(cluster_pb2.RescheduleMaintenance().RequestParameters),
            request_parameters=_rp(cluster_pb2.RescheduleMaintenance().RequestParameters),
        ),
    },
)
