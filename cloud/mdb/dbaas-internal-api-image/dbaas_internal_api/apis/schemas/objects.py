# -*- coding: utf-8 -*-
"""
DBaaS Internal API object schemas
"""

from marshmallow import Schema
from marshmallow.fields import Boolean, Int, List, Nested, Float

from ...apis.schemas.fields import MappedEnum
from ...utils.types import HostStatus
from ...utils import validation
from .fields import ClusterId, GrpcInt, Str, ZoneId


class DatabaseSchemaV1(Schema):
    """
    Database schema.
    """

    clusterId = ClusterId(required=True)


class UserSchemaV1(Schema):
    """
    User schema.
    """

    clusterId = ClusterId(required=True)


class ResourcesSchemaV1(Schema):
    """
    Resources schema.
    """

    resourcePresetId = Str(attribute='resource_preset_id', required=True)
    diskSize = GrpcInt(attribute='disk_size', required=True)
    diskTypeId = Str(attribute='disk_type_id', required=True)


class ResourcesUpdateSchemaV1(Schema):
    """
    Resources schema used in ClusterUpdate operation.
    """

    resourcePresetId = Str(attribute='resource_preset_id')
    diskSize = GrpcInt(attribute='disk_size')
    diskTypeId = Str(attribute='disk_type_id')


class ResourcePresetSchemaV1(Schema):
    """
    Resource preset schema.
    """

    id = Str(required=True)
    zoneIds = List(Str(), attribute='zone_ids', required=True)
    cores = Int(required=True)
    coreFraction = Int(attribute='core_fraction', required=True)
    memory = Int(required=True)
    type = Str(required=True)
    generation = Int(required=True)


class ShardSchemaV1(Schema):
    """
    Shard schema.
    """

    name = Str(required=True)
    clusterId = ClusterId(required=True)


class MetricSchemaV1(Schema):
    """
    Host system metric schema
    """

    timestamp = Int(required=True)


class CPUMetricSchemaV1(MetricSchemaV1):
    """
    CPU metric schema
    """

    used = Float(required=True)


class MemoryMetricSchemaV1(MetricSchemaV1):
    """
    Memory metric schema
    """

    used = Int(required=True)
    total = Int(required=True)


class DiskMetricSchemaV1(MetricSchemaV1):
    """
    Disk metric schema
    """

    used = Int(required=True)
    total = Int(required=True)


class SystemMetricsSchemaV1(Schema):
    """
    Host system metrics schema
    """

    cpu = Nested(CPUMetricSchemaV1)
    memory = Nested(MemoryMetricSchemaV1)
    disk = Nested(DiskMetricSchemaV1)


class HostWithoutPublicIpSchemaV1(Schema):
    """
    Host schema.
    """

    name = Str(validate=validation.hostname_validator, required=True)
    clusterId = ClusterId(required=True)
    zoneId = ZoneId(required=True)
    resources = Nested(ResourcesSchemaV1, required=True)
    subnetId = Str(attribute='subnet_id')
    health = MappedEnum(
        {
            'UNKNOWN': HostStatus.unknown,
            'ALIVE': HostStatus.alive,
            'DEAD': HostStatus.dead,
            'DEGRADED': HostStatus.degraded,
        }
    )
    system = Nested(SystemMetricsSchemaV1)


class HostSchemaV1(HostWithoutPublicIpSchemaV1):
    assignPublicIp = Boolean(attribute='assign_public_ip', missing=False)


class HostSpecSchemaV1(Schema):
    """
    Host spec schema.
    """

    zoneId = ZoneId(required=True)
    subnetId = Str(attribute='subnet_id')
    assignPublicIp = Boolean(attribute='assign_public_ip', missing=False)


class AccessSchemaV1(Schema):
    """
    Access schema.
    """

    webSql = Boolean(attribute='web_sql', default=False)
    dataLens = Boolean(attribute='data_lens', default=False)
    dataTransfer = Boolean(attribute='data_transfer', default=False)
    serverless = Boolean(attribute='serverless', default=False)
