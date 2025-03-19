"""
MySQL operations description
"""

from google.protobuf.descriptor import Descriptor
from google.protobuf.message import Message
from yandex.cloud.events.mdb.mysql import cluster_pb2, database_pb2, user_pb2

from ...core.types import ResponseType
from ...utils.operations import EMPTY_RESPONSE as _EMPTY, modify_alert_group_event_details
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
    operation_request_to_dict_factory,
    restore_event_details,
    restore_metadata,
)
from .constants import MY_CLUSTER_TYPE
from .traits import MySQLOperations


__a = annotation_maker('yandex.cloud.mdb.mysql.v1')
__et = annotation_maker('yandex.cloud.events.mdb.mysql')
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
    operation_types=MySQLOperations,
    handles={
        MySQLOperations.create: OperationHandle(
            'Create MySQL cluster',
            metadata_annotation=__a('CreateClusterMetadata'),
            event_type=__et('CreateCluster'),
            response=_CLUSTER,
            # for some reason mypy blocks direct access to Request Parameters ('cluster_pb2.CreateCluster.RequestParameters')
            # so, use workaround:
            request_parameters_annotation=_rpa(cluster_pb2.CreateCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.CreateCluster().RequestParameters),
        ),
        MySQLOperations.modify: OperationHandle(
            'Modify MySQL cluster',
            response=_CLUSTER,
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
            request_parameters_annotation=_rpa(cluster_pb2.UpdateCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.UpdateCluster().RequestParameters),
        ),
        MySQLOperations.metadata: OperationHandle(
            'Update MySQL cluster metadata',
            response=_CLUSTER,
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
            request_parameters_annotation=_rpa(cluster_pb2.UpdateCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.UpdateCluster().RequestParameters),
        ),
        MySQLOperations.start_failover: OperationHandle(
            'Start manual failover on MySQL cluster',
            response=_CLUSTER,
            metadata_annotation=__a('StartClusterFailoverMetadata'),
            event_type=__et('StartClusterFailover'),
            request_parameters_annotation=_rpa(cluster_pb2.StartClusterFailover().RequestParameters),
            request_parameters=_rp(cluster_pb2.StartClusterFailover().RequestParameters),
        ),
        MySQLOperations.start: OperationHandle(
            'Start MySQL cluster',
            response=_CLUSTER,
            metadata_annotation=__a('StartClusterMetadata'),
            event_type=__et('StartCluster'),
            request_parameters_annotation=_rpa(cluster_pb2.StartCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.StartCluster().RequestParameters),
        ),
        MySQLOperations.stop: OperationHandle(
            'Stop MySQL cluster',
            response=_CLUSTER,
            metadata_annotation=__a('StopClusterMetadata'),
            event_type=__et('StopCluster'),
            request_parameters_annotation=_rpa(cluster_pb2.StopCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.StopCluster().RequestParameters),
        ),
        MySQLOperations.move: OperationHandle(
            'Move MySQL cluster',
            response=_CLUSTER,
            metadata=move_metadata,
            event_details=move_event_details,
            metadata_annotation=__a('MoveClusterMetadata'),
            event_type=__et('MoveCluster'),
            request_parameters_annotation=_rpa(cluster_pb2.MoveCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.MoveCluster().RequestParameters),
        ),
        MySQLOperations.delete: OperationHandle(
            'Delete MySQL cluster',
            metadata_annotation=__a('DeleteClusterMetadata'),
            event_type=__et('DeleteCluster'),
            response=_EMPTY,
            request_parameters_annotation=_rpa(cluster_pb2.DeleteCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.DeleteCluster().RequestParameters),
        ),
        MySQLOperations.restore: OperationHandle(
            'Create new MySQL cluster from the backup',
            response=_CLUSTER,
            metadata=restore_metadata,
            event_details=restore_event_details,
            metadata_annotation=__a('RestoreClusterMetadata'),
            event_type=__et('RestoreCluster'),
            request_parameters_annotation=_rpa(cluster_pb2.RestoreCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.RestoreCluster().RequestParameters),
        ),
        MySQLOperations.backup: OperationHandle(
            'Create a backup for MySQL cluster',
            response=_BACKUP,
            metadata_annotation=__a('BackupClusterMetadata'),
            event_type=__et('BackupCluster'),
            request_parameters_annotation=_rpa(cluster_pb2.BackupCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.BackupCluster().RequestParameters),
        ),
        MySQLOperations.host_create: OperationHandle(
            'Add hosts to MySQL cluster',
            response=_EMPTY,
            metadata_annotation=__a('AddClusterHostsMetadata'),
            event_type=__et('AddClusterHosts'),
            metadata=modify_hosts_metadata,
            event_details=modify_hosts_event_details,
            request_parameters_annotation=_rpa(cluster_pb2.AddClusterHosts().RequestParameters),
            request_parameters=_rp(cluster_pb2.AddClusterHosts().RequestParameters),
        ),
        MySQLOperations.host_delete: OperationHandle(
            'Delete hosts from MySQL cluster',
            response=_EMPTY,
            metadata_annotation=__a('DeleteClusterHostsMetadata'),
            event_type=__et('DeleteClusterHosts'),
            metadata=modify_hosts_metadata,
            event_details=modify_hosts_event_details,
            request_parameters_annotation=_rpa(cluster_pb2.DeleteClusterHosts().RequestParameters),
            request_parameters=_rp(cluster_pb2.DeleteClusterHosts().RequestParameters),
        ),
        MySQLOperations.host_modify: OperationHandle(
            'Modify hosts in MySQL cluster',
            response=_EMPTY,
            metadata_annotation=__a('UpdateClusterHostsMetadata'),
            event_type=__et('UpdateClusterHosts'),
            metadata=modify_hosts_metadata,
            event_details=modify_hosts_event_details,  # save update_host_specs !
            request_parameters_annotation=_rpa(cluster_pb2.UpdateClusterHosts().RequestParameters),
            request_parameters=_rp(cluster_pb2.UpdateClusterHosts().RequestParameters),
        ),
        MySQLOperations.database_add: OperationHandle(
            'Add database to MySQL cluster',
            response=_DATABASE,
            metadata=modify_database_metadata,
            event_details=modify_database_event_details,
            metadata_annotation=__a('CreateDatabaseMetadata'),
            event_type=__et('CreateDatabase'),
            request_parameters_annotation=_rpa(database_pb2.CreateDatabase().RequestParameters),
            request_parameters=_rp(database_pb2.CreateDatabase().RequestParameters),
        ),
        MySQLOperations.database_delete: OperationHandle(
            'Delete database from MySQL cluster',
            response=_EMPTY,
            metadata=modify_database_metadata,
            event_details=modify_database_event_details,
            metadata_annotation=__a('DeleteDatabaseMetadata'),
            event_type=__et('DeleteDatabase'),
            request_parameters_annotation=_rpa(database_pb2.DeleteDatabase().RequestParameters),
            request_parameters=_rp(database_pb2.DeleteDatabase().RequestParameters),
        ),
        MySQLOperations.user_create: OperationHandle(
            'Create user in MySQL cluster',
            response=_USER,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('CreateUserMetadata'),
            event_type=__et('CreateUser'),
            request_parameters_annotation=_rpa(user_pb2.CreateUser().RequestParameters),
            request_parameters=_rp(user_pb2.CreateUser().RequestParameters),
        ),
        MySQLOperations.user_modify: OperationHandle(
            'Modify user in MySQL cluster',
            response=_USER,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('UpdateUserMetadata'),
            event_type=__et('UpdateUser'),
            request_parameters_annotation=_rpa(user_pb2.UpdateUser().RequestParameters),
            request_parameters=_rp(user_pb2.UpdateUser().RequestParameters),
        ),
        MySQLOperations.user_delete: OperationHandle(
            'Delete user from MySQL cluster',
            response=_EMPTY,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('DeleteUserMetadata'),
            event_type=__et('DeleteUser'),
            request_parameters_annotation=_rpa(user_pb2.DeleteUser().RequestParameters),
            request_parameters=_rp(user_pb2.DeleteUser().RequestParameters),
        ),
        MySQLOperations.grant_permission: OperationHandle(
            'Grant permission to user in MySQL cluster',
            response=_USER,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('GrantUserPermissionMetadata'),
            event_type=__et('GrantUserPermission'),
            request_parameters_annotation=_rpa(user_pb2.GrantUserPermission().RequestParameters),
            request_parameters=_rp(user_pb2.GrantUserPermission().RequestParameters),
        ),
        MySQLOperations.revoke_permission: OperationHandle(
            'Revoke permission from user in MySQL cluster',
            response=_USER,
            metadata=modify_user_metadata,
            event_details=modify_user_event_details,
            metadata_annotation=__a('RevokeUserPermissionMetadata'),
            event_type=__et('RevokeUserPermission'),
            request_parameters_annotation=_rpa(user_pb2.RevokeUserPermission().RequestParameters),
            request_parameters=_rp(user_pb2.RevokeUserPermission().RequestParameters),
        ),
        MySQLOperations.upgrade80: OperationHandle(
            'Upgrade MySQL cluster to 8.0 version',
            metadata_annotation=__a('UpdateClusterMetadata'),
            event_type=__et('UpdateCluster'),
            response=_CLUSTER,
            request_parameters_annotation=_rpa(cluster_pb2.UpdateCluster().RequestParameters),
            request_parameters=_rp(cluster_pb2.UpdateCluster().RequestParameters),
        ),
        MySQLOperations.maintenance_reschedule: OperationHandle(
            'Reschedule maintenance in MySQL cluster',
            response=_CLUSTER,
            metadata=modify_reschedule_maintenance_metadata,
            metadata_annotation=__a('RescheduleMaintenanceMetadata'),
            event_type=__et('RescheduleMaintenance'),
            request_parameters_annotation=_rpa(cluster_pb2.RescheduleMaintenance().RequestParameters),
            request_parameters=_rp(cluster_pb2.RescheduleMaintenance().RequestParameters),
        ),
        MySQLOperations.alert_group_create: OperationHandle(
            'Create a Alert Group for MySQL cluster',
            response=_ALERT_GROUP,
            metadata_annotation=__a('CreateAlertsGroupClusterMetadata'),
            event_details=modify_alert_group_event_details,
            event_type=__et('UpdateCluster'),
        ),
        MySQLOperations.alert_group_delete: OperationHandle(
            'Delete Alert Group for MySQL cluster',
            response=_ALERT_GROUP,
            event_details=modify_alert_group_event_details,
            metadata_annotation=__a('DeleteAlertsGroupClusterMetadata'),
            event_type=__et('UpdateCluster'),
        ),
    },
)
