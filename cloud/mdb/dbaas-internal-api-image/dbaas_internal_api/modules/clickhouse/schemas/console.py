# -*- coding: utf-8 -*-
"""
Schemas for ClickHouse console.
"""

from marshmallow.fields import Boolean, Int, List, Nested

from ....apis.schemas.console import ClustersConfigAvailableVersionSchemaV1, StringValueV1
from ....apis.schemas.fields import Environment, Str
from ....utils.register import DbaasOperation, Resource, register_response_schema
from ....utils.validation import Schema
from ..constants import MY_CLUSTER_TYPE
from .hosts import ClickhouseHostType


class ClickhouseConsoleClustersConfigDiskSizeRangeSchemaV1(Schema):
    """
    ClickHouse disk size range schema.
    """

    min = Int()
    max = Int()


class ClickhouseConsoleClustersConfigDiskSizesSchemaV1(Schema):
    """
    ClickHouse disk sizes list schema.
    """

    sizes = List(Int())


class ClickhouseConsoleClustersConfigDiskTypesSchemaV1(Schema):
    """
    ClickHouse available disk type schema.
    """

    diskTypeId = Str(attribute='disk_type_id')
    diskSizeRange = Nested(ClickhouseConsoleClustersConfigDiskSizeRangeSchemaV1(), attribute='disk_size_range')
    diskSizes = Nested(ClickhouseConsoleClustersConfigDiskSizesSchemaV1(), attribute='disk_sizes')
    minHosts = Int(attribute='min_hosts')
    maxHosts = Int(attribute='max_hosts')


class ClickhouseConsoleClustersConfigZoneSchemaV1(Schema):
    """
    ClickHouse available zone schema.
    """

    zoneId = Str(attribute='zone_id')
    diskTypes = Nested(ClickhouseConsoleClustersConfigDiskTypesSchemaV1, many=True, attribute='disk_types')


class ClickhouseConsoleClustersConfigResourcePresetSchemaV1(Schema):
    """
    ClickHouse available resource preset schema.
    """

    presetId = Str(attribute='preset_id')
    cpuLimit = Int(attribute='cpu_limit')
    cpuFraction = Int(attribute='cpu_fraction')
    memoryLimit = Int(attribute='memory_limit')
    type = Str(attribute='type')
    generation = Str(attribute='generation')
    generationName = Str(attribute='generation_name')
    decommissioning = Boolean(attribute='decommissioning')
    zones = Nested(ClickhouseConsoleClustersConfigZoneSchemaV1, many=True)


class ClickhouseConsoleClustersConfigDefaultResourcesSchemaV1(Schema):
    """
    ClickHouse default resources schema.
    """

    generation = Str(attribute='generation')
    generationName = Str(attribute='generation_name')
    resourcePresetId = Str(attribute='resource_preset_id')
    diskTypeId = Str(attribute='disk_type_id')
    diskSize = Int(attribute='disk_size')


class ClickhouseConsoleClustersConfigHostTypeSchemaV1(Schema):
    """
    ClickHouse available host type schema.
    """

    type = ClickhouseHostType()
    resourcePresets = Nested(
        ClickhouseConsoleClustersConfigResourcePresetSchemaV1, many=True, attribute='resource_presets'
    )
    defaultResources = Nested(ClickhouseConsoleClustersConfigDefaultResourcesSchemaV1, attribute='default_resources')


class ClickhouseConsoleClustersConfigHostCountPerDiskTypeSchemaV1(Schema):
    """
    ClickHouse host count limits for disk type schema.
    """

    diskTypeId = Str(attribute='disk_type_id')
    minHostCount = Int(attribute='min_host_count')


class ClickhouseConsoleClustersConfigHostCountLimitsSchemaV1(Schema):
    """
    ClickHouse host count limits schema.
    """

    minHostCount = Int(attribute='min_host_count')
    maxHostCount = Int(attribute='max_host_count')
    hostCountPerDiskType = Nested(
        ClickhouseConsoleClustersConfigHostCountPerDiskTypeSchemaV1, many=True, attribute='host_count_per_disk_type'
    )


@register_response_schema(MY_CLUSTER_TYPE, Resource.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
class ClickhouseConsoleClustersConfigSchemaV1(Schema):
    """
    ClickHouse console clusters config schema.
    """

    clusterName = Nested(StringValueV1(), attribute='cluster_name')
    dbName = Nested(StringValueV1(), attribute='db_name')
    userName = Nested(StringValueV1(), attribute='user_name')
    mlModelName = Nested(StringValueV1(), attribute='ml_model_name')
    mlModelUri = Nested(StringValueV1(), attribute='ml_model_uri')
    formatSchemaName = Nested(StringValueV1(), attribute='format_schema_name')
    formatSchemaUri = Nested(StringValueV1(), attribute='format_schema_uri')
    password = Nested(StringValueV1())
    hostCountLimits = Nested(ClickhouseConsoleClustersConfigHostCountLimitsSchemaV1(), attribute='host_count_limits')
    hostTypes = Nested(ClickhouseConsoleClustersConfigHostTypeSchemaV1, many=True, attribute='host_types')
    versions = List(Str())
    availableVersions = Nested(ClustersConfigAvailableVersionSchemaV1, many=True, attribute='available_versions')
    defaultVersion = Str(attribute='default_version')


class ClickHouseRestoreHintResource(Schema):
    """
    Resource in restore hint
    """

    diskSize = Int(attribute='disk_size')
    resourcePresetId = Str(attribute='resource_preset_id')
    minHostsCount = Int(attribute='min_hosts_count')


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.RESTORE_HINTS)
class ClickHouseRestoreHintsResponseSchemaV1(Schema):
    """
    Schema for restore hint

    Differences from RestoreHintsResponseSchemaV1:
    - It don't have version
    - It has custom resources hint
    """

    resources = Nested(ClickHouseRestoreHintResource, attribute='resources')
    networkId = Str(attribute='network_id')
    environment = Environment(attribute='env')
