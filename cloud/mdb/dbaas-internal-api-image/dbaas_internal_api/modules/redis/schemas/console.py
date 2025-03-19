"""
Schemas for Redis console.
"""
from marshmallow.fields import Boolean, Int, List, Nested

from ....apis.schemas.console import ClustersConfigAvailableVersionSchemaV1, RestoreHintResponseSchemaV1, StringValueV1
from ....apis.schemas.fields import Str
from ....utils.register import DbaasOperation, Resource, register_response_schema
from ....utils.validation import Schema
from ..constants import MY_CLUSTER_TYPE


class RedisConsoleClustersConfigDiskSizeRangeSchemaV1(Schema):
    """
    Redis disk size range schema.
    """

    min = Int()
    max = Int()


class RedisConsoleClustersConfigDiskSizesSchemaV1(Schema):
    """
    Redis disk sizes list schema.
    """

    sizes = List(Int())


class RedisConsoleClustersConfigDiskTypesSchemaV1(Schema):
    """
    Redis available disk type schema.
    """

    diskTypeId = Str(attribute='disk_type_id')
    diskSizeRange = Nested(RedisConsoleClustersConfigDiskSizeRangeSchemaV1(), attribute='disk_size_range')
    diskSizes = Nested(RedisConsoleClustersConfigDiskSizesSchemaV1(), attribute='disk_sizes')
    minHosts = Int(attribute='min_hosts')
    maxHosts = Int(attribute='max_hosts')


class RedisConsoleClustersConfigZoneSchemaV1(Schema):
    """
    Redis available zone schema.
    """

    zoneId = Str(attribute='zone_id')
    diskTypes = Nested(RedisConsoleClustersConfigDiskTypesSchemaV1, many=True, attribute='disk_types')


class RedisConsoleClustersConfigResourcePresetSchemaV1(Schema):
    """
    Redis available resource preset schema.
    """

    presetId = Str(attribute='preset_id')
    cpuLimit = Int(attribute='cpu_limit')
    cpuFraction = Int(attribute='cpu_fraction')
    memoryLimit = Int(attribute='memory_limit')
    type = Str(attribute='type')
    generation = Str(attribute='generation')
    generationName = Str(attribute='generation_name')
    decommissioning = Boolean(attribute='decommissioning')
    zones = Nested(RedisConsoleClustersConfigZoneSchemaV1, many=True)


class RedisConsoleClustersConfigHostCountLimitsSchemaV1(Schema):
    """
    Redis host count limits schema.
    """

    minHostCount = Int(attribute='min_host_count')
    maxHostCount = Int(attribute='max_host_count')


class RedisConsoleClustersConfigDefaultResourcesSchemaV1(Schema):
    """
    Redis default resources schema.
    """

    generation = Str(attribute='generation')
    generationName = Str(attribute='generation_name')
    resourcePresetId = Str(attribute='resource_preset_id')
    diskTypeId = Str(attribute='disk_type_id')
    diskSize = Int(attribute='disk_size')


@register_response_schema(MY_CLUSTER_TYPE, Resource.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
class RedisConsoleClustersConfigSchemaV1(Schema):
    """
    Redis console clusters config schema.
    """

    clusterName = Nested(StringValueV1(), attribute='cluster_name')
    password = Nested(StringValueV1())
    hostCountLimits = Nested(RedisConsoleClustersConfigHostCountLimitsSchemaV1(), attribute='host_count_limits')
    resourcePresets = Nested(RedisConsoleClustersConfigResourcePresetSchemaV1, many=True, attribute='resource_presets')
    defaultResources = Nested(RedisConsoleClustersConfigDefaultResourcesSchemaV1, attribute='default_resources')
    versions = List(Str())
    availableVersions = Nested(ClustersConfigAvailableVersionSchemaV1, many=True, attribute='available_versions')
    defaultVersion = Str(attribute='default_version')
    tlsSupportedVersions = List(Str(), attribute='tls_supported_versions')
    persistenceModes = List(Str(), attribute='persistence_modes')


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.RESTORE_HINTS)
class RedisRestoreHintsResponseSchemaV1(RestoreHintResponseSchemaV1):
    """
    Schema for Redis restore hint
    """
