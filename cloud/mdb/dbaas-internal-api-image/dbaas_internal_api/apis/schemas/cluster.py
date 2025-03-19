# -*- coding: utf-8 -*-
"""
DBaaS Internal API create cluster schema
"""

from marshmallow.fields import DateTime, Nested, Int, List, Bool

from ...apis.schemas.fields import MappedEnum
from ...apis.schemas.alerts import AlertGroupSchemaHeaderV1
from ...utils.types import ClusterStatus, ClusterHealth
from ...utils.validation import Schema
from .common import ListRequestSchemaV1, ListResponseSchemaV1
from .fields import Description, Environment, Filter, FolderId, Labels, Str


class MonitoringSchemaV1(Schema):
    """
    Monitoring schema.
    """

    link = Str(required=True, description='Link to monitoring interface')
    description = Str(required=False, description='Object description')
    name = Str(required=True, description='Monitoring subsystem name')


class AnytimeMaintenanceWindowSchemaV1(Schema):
    pass


class WeeklyMaintenanceWindowSchemaV1(Schema):
    day = Str()
    hour = Int()


class MaintenanceWindowSchemaV1(Schema):
    anytime = Nested(AnytimeMaintenanceWindowSchemaV1, many=False)
    weeklyMaintenanceWindow = Nested(WeeklyMaintenanceWindowSchemaV1, many=False, attribute="weekly_maintenance_window")


class MaintenanceOperationSchemaV1(Schema):
    info = Str(description='Operation description', required=True)
    delayedUntil = DateTime(attribute='delayed_until', required=True)
    nextMaintenanceWindowTime = DateTime(attribute='next_maintenance_window_time')
    latestMaintenanceTime = DateTime(attribute='latest_maintenance_time', required=True)


class ClusterSchemaV1(Schema):
    """
    Base cluster schema.
    """

    id = Str(required=True)
    folderId = FolderId(required=True)
    createdAt = DateTime(attribute='created_at', required=True)
    name = Str(required=True)
    description = Description()
    labels = Labels()
    environment = Environment(required=True)
    networkId = Str(attribute='network_id', required=True)
    health = MappedEnum(
        {
            'UNKNOWN': ClusterHealth.unknown,
            'ALIVE': ClusterHealth.alive,
            'DEAD': ClusterHealth.dead,
            'DEGRADED': ClusterHealth.degraded,
        }
    )
    status = MappedEnum(
        {
            'UNKNOWN': ClusterStatus.unknown,
            'CREATING': ClusterStatus.creating,
            'RUNNING': [
                ClusterStatus.running,
            ],
            'UPDATING': [
                ClusterStatus.modifying,
                ClusterStatus.restoring_online,
                # show cluster as UPDATING for a user instead of STOPPED (like a restore-offline did it),
                # cause maintenance task is visible for him
                ClusterStatus.maintaining_offline,
            ],
            'ERROR': [
                ClusterStatus.create_error,
                ClusterStatus.modify_error,
                ClusterStatus.start_error,
                ClusterStatus.stop_error,
                ClusterStatus.restoring_offline_error,
                ClusterStatus.restoring_online_error,
                ClusterStatus.maintain_offline_error,
            ],
            'STOPPING': ClusterStatus.stopping,
            'STOPPED': [
                ClusterStatus.stopped,
                ClusterStatus.restoring_offline,
            ],
            'STARTING': ClusterStatus.starting,
        },
        required=True,
    )
    zoneId = Str(attribute='zone_id')
    hostGroupIds = List(Str(), attribute='host_group_ids')
    securityGroupIds = List(Str(), attribute='user_sgroup_ids')


class TypedClusterSchemaV1(ClusterSchemaV1):
    """
    Base typed cluster schema.
    """

    monitoring = Nested(MonitoringSchemaV1, many=True)


class ManagedClusterSchemaV1(TypedClusterSchemaV1):
    """
    Base managed cluster schema.
    """

    maintenanceWindow = Nested(MaintenanceWindowSchemaV1, many=False, attribute='maintenance_window')
    plannedOperation = Nested(MaintenanceOperationSchemaV1, many=False, attribute='planned_operation')
    deletionProtection = Bool(attribute='deletion_protection', default=False)


class GenericClusterSchemaV1(ClusterSchemaV1):
    """
    Generic cluster schema.
    """

    type = Str(required=True)


class ClusterConfigSchemaV1(Schema):
    """
    Base cluster config schema.
    """

    version = Str()


class ClusterConfigSpecSchemaV1(ClusterConfigSchemaV1):
    """
    Base cluster config spec schema.
    """


class ConfigSchemaV1(Schema):
    """
    Base schema for DBMS configs.
    """


class CreateClusterRequestSchemaV1(Schema):
    """
    Schema for create cluster request.
    """

    folderId = FolderId(required=True)
    description = Description()
    labels = Labels()
    environment = Environment(required=True)
    networkId = Str(attribute='network_id', missing='')
    maintenanceWindow = Nested(MaintenanceWindowSchemaV1, many=False, attribute='maintenance_window')
    securityGroupIds = List(Str, attribute='security_group_ids')
    deletionProtection = Bool(attribute='deletion_protection', required=False)
    monitoringCloudId = Str(attribute='monitoring_cloud_id', required=False)
    monitoringStubRequestId = Str(attribute='monitoring_stub_request_id', required=False)
    defaultAlertGroup = Nested(AlertGroupSchemaHeaderV1, attribute='alert_group_spec', required=False, many=False)


class UpdateClusterRequestSchemaV1(Schema):
    """
    Schema for update cluster request.
    """

    description = Description()
    labels = Labels()
    maintenanceWindow = Nested(MaintenanceWindowSchemaV1, many=False, attribute='maintenance_window')
    securityGroupIds = List(Str, attribute='security_group_ids')
    deletionProtection = Bool(attribute='deletion_protection', required=False)
    monitoringCloudId = Str(attribute='monitoring_cloud_id', required=False)


class StartClusterFailoverRequestSchemaV1(Schema):
    """
    Schema for cluster failover request.
    """


class RestoreClusterRequestSchemaV1(Schema):
    """
    Schema for restore cluster request.
    """

    backupId = Str(attribute='backup_id', required=True)
    name = Str(required=True)
    description = Description()
    labels = Labels()
    environment = Environment(required=True)
    networkId = Str(attribute='network_id', missing='')
    folderId = FolderId()
    securityGroupIds = List(Str, attribute='security_group_ids')
    deletionProtection = Bool(attribute='deletion_protection', required=False)


class MoveClusterRequestSchemaV1(Schema):
    """
    Schema for move cluster request.
    """

    destinationFolderId = Str(attribute='destination_folder_id', required=True)


class ListClustersRequestSchemaV1(ListRequestSchemaV1):
    """
    Schema for list clusters request.
    """

    folderId = FolderId(required=True)
    filter = Filter()


class ListClusterHostsRequestSchemaV1(Schema):
    """
    Schema for list cluster hosts request.
    """

    filter = Filter()


class ListGenericClustersResponseSchemaV1(ListResponseSchemaV1):
    """
    ClickHouse cluster list schema.
    """

    clusters = Nested(GenericClusterSchemaV1, many=True, required=True)
