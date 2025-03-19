# -*- coding: utf-8 -*-
"""
DBaaS Internal API MongoDB cluster options schema
# TODO:
    validate using something like jsonschema
"""

from marshmallow import validates_schema
from marshmallow.fields import Boolean, Float, Int, List, Nested
from marshmallow.validate import Length, Range

from . import schema_defaults as defaults
from ...apis.schemas.backups import BaseBackupSchemaV1
from ...apis.schemas.cluster import (
    ClusterConfigSchemaV1,
    CreateClusterRequestSchemaV1,
    ManagedClusterSchemaV1,
    RestoreClusterRequestSchemaV1,
    UpdateClusterRequestSchemaV1,
)
from ...apis.schemas.common import ListResponseSchemaV1
from ...apis.schemas.console import (
    ClustersConfigAvailableVersionSchemaV1,
    EstimateHostCreateRequestSchemaV1,
    RestoreHintWithTimeResponseSchemaV1,
    StringValueV1,
)
from ...apis.schemas.fields import MappedEnum, Str, UInt, UpperToLowerCaseEnum
from ...apis.schemas.objects import (
    AccessSchemaV1,
    DatabaseSchemaV1,
    HostSchemaV1,
    HostSpecSchemaV1,
    ResourcePresetSchemaV1,
    ResourcesSchemaV1,
    ResourcesUpdateSchemaV1,
    ShardSchemaV1,
    UserSchemaV1,
)
from ...health.health import ServiceStatus
from ...utils import validation
from ...utils.register import (
    DbaasOperation,
    Resource,
    register_config_schema,
    register_request_schema,
    register_response_schema,
)
from ...utils.types import BackupInitiator
from ...utils.validation import Schema, TimeOfDay
from .constants import (
    EDITION_ENTERPRISE,
    MONGOCFG_HOST_TYPE,
    MONGOD_HOST_TYPE,
    MONGOS_HOST_TYPE,
    MONGOINFRA_HOST_TYPE,
    MY_CLUSTER_TYPE,
    SUBCLUSTER_NAMES,
    VALID_ROLES,
)
from .traits import HostRole, MongoDBClusterTraits, MongoDBRoles, ServiceType
from .utils import validate_db_roles


class PerformanceDiagnostics(Schema):
    "MongoDB PerfDiag schema"

    profilingEnabled = Boolean(attribute='profiling_enabled')


class MongodbHostType(MappedEnum):
    """
    'type' field.
    """

    def __init__(self, **kwargs):
        super().__init__(
            {
                'MONGOD': MongoDBRoles.mongod,
                'MONGOS': MongoDBRoles.mongos,
                'MONGOCFG': MongoDBRoles.mongocfg,
                'MONGOINFRA': MongoDBRoles.mongoinfra,
            },
            **kwargs,
            skip_description=True,
        )


@register_response_schema(MY_CLUSTER_TYPE, Resource.RESOURCE_PRESET, DbaasOperation.INFO)
class MongodbResourcePresetSchemaV1(ResourcePresetSchemaV1):
    """
    MongoDB resource preset schema.
    """

    hostTypes = List(MongodbHostType(), attribute='roles')


@register_response_schema(MY_CLUSTER_TYPE, Resource.RESOURCE_PRESET, DbaasOperation.LIST)
class MongodbListResourcePresetsSchemaV1(ListResponseSchemaV1):
    """
    MongoDB resource preset list schema.
    """

    resourcePresets = Nested(MongodbResourcePresetSchemaV1, many=True, attribute='resource_presets', required=True)


class MongodbConsoleClustersConfigDiskSizeRangeSchemaV1(Schema):
    """
    Mongodb disk size range schema.
    """

    min = Int()
    max = Int()


class MongodbConsoleClustersConfigDiskSizesSchemaV1(Schema):
    """
    Mongodb disk sizes list schema.
    """

    sizes = List(Int())


class MongodbConsoleClustersConfigDiskTypesSchemaV1(Schema):
    """
    Mongodb available disk type schema.
    """

    diskTypeId = Str(attribute='disk_type_id')
    diskSizeRange = Nested(MongodbConsoleClustersConfigDiskSizeRangeSchemaV1(), attribute='disk_size_range')
    diskSizes = Nested(MongodbConsoleClustersConfigDiskSizesSchemaV1(), attribute='disk_sizes')
    minHosts = Int(attribute='min_hosts')
    maxHosts = Int(attribute='max_hosts')


class MongodbConsoleClustersConfigZoneSchemaV1(Schema):
    """
    Mongodb available zone schema.
    """

    zoneId = Str(attribute='zone_id')
    diskTypes = Nested(MongodbConsoleClustersConfigDiskTypesSchemaV1, many=True, attribute='disk_types')


class MongodbConsoleClustersConfigResourcePresetSchemaV1(Schema):
    """
    Mongodb available resource preset schema.
    """

    presetId = Str(attribute='preset_id')
    cpuLimit = Int(attribute='cpu_limit')
    cpuFraction = Int(attribute='cpu_fraction')
    memoryLimit = Int(attribute='memory_limit')
    type = Str(attribute='type')
    generation = Str(attribute='generation')
    generationName = Str(attribute='generation_name')
    decommissioning = Boolean(attribute='decommissioning')
    zones = Nested(MongodbConsoleClustersConfigZoneSchemaV1, many=True)


class MongodbConsoleClustersConfigHostCountPerDiskTypeSchemaV1(Schema):
    """
    Mongodb host count limits for disk type schema.
    """

    diskTypeId = Str(attribute='disk_type_id')
    minHostCount = Int(attribute='min_host_count')


class MongodbConsoleClustersConfigHostCountLimitsSchemaV1(Schema):
    """
    Mongodb host count limits schema.
    """

    minHostCount = Int(attribute='min_host_count')
    maxHostCount = Int(attribute='max_host_count')
    hostCountPerDiskType = Nested(
        MongodbConsoleClustersConfigHostCountPerDiskTypeSchemaV1, many=True, attribute='host_count_per_disk_type'
    )


class MongodbConsoleClustersConfigDefaultResourcesSchemaV1(Schema):
    """
    MongoDB default resources schema.
    """

    generation = Str(attribute='generation')
    generationName = Str(attribute='generation_name')
    resourcePresetId = Str(attribute='resource_preset_id')
    diskTypeId = Str(attribute='disk_type_id')
    diskSize = Int(attribute='disk_size')


class MongodbConsoleClustersConfigHostTypeSchemaV1(Schema):
    """
    Mongodb available host type schema.
    """

    type = MongodbHostType()
    resourcePresets = Nested(
        MongodbConsoleClustersConfigResourcePresetSchemaV1, many=True, attribute='resource_presets'
    )
    defaultResources = Nested(MongodbConsoleClustersConfigDefaultResourcesSchemaV1, attribute='default_resources')


class FCVSchemaV1(Schema):
    """
    Database available feature compatibility versions schema
    """

    id = Str()
    fcvs = List(Str, default=[])


@register_response_schema(MY_CLUSTER_TYPE, Resource.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
class MongodbConsoleClustersConfigSchemaV1(Schema):
    """
    Mongodb console clusters config schema.
    """

    clusterName = Nested(StringValueV1(), attribute='cluster_name')
    dbName = Nested(StringValueV1(), attribute='db_name')
    userName = Nested(StringValueV1(), attribute='user_name')
    password = Nested(StringValueV1())
    hostCountLimits = Nested(MongodbConsoleClustersConfigHostCountLimitsSchemaV1(), attribute='host_count_limits')
    hostTypes = Nested(MongodbConsoleClustersConfigHostTypeSchemaV1, many=True, attribute='host_types')
    versions = List(Str())
    availableVersions = Nested(ClustersConfigAvailableVersionSchemaV1, many=True, attribute='available_versions')
    availableFCVs = Nested(FCVSchemaV1, many=True, attribute='available_fcvs')
    defaultVersion = Str(attribute='default_version')


class MongodbPermissionSchemaV1(Schema):
    """
    Permission schema.
    """

    databaseName = Str(
        attribute='database_name', validate=MongoDBClusterTraits.role_assign_db_name.validate, required=True
    )
    roles = List(Str(validate=validation.OneOf(VALID_ROLES)), required=True)

    @validates_schema(skip_on_field_errors=True)
    def _validate_db_permissions(self, data):
        return validate_db_roles(data)


@register_response_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.INFO)
class MongodbUserSchemaV1(UserSchemaV1):
    """
    MongoDB user schema.
    """

    name = Str(validate=MongoDBClusterTraits.user_name.validate, required=True)
    permissions = Nested(MongodbPermissionSchemaV1, many=True, required=True)


class MongodbUserSpecSchemaV1(Schema):
    """
    MongoDB user spec schema.
    """

    name = Str(validate=MongoDBClusterTraits.user_name.validate, required=True)
    password = Str(validate=MongoDBClusterTraits.password.validate, required=True)
    permissions = Nested(MongodbPermissionSchemaV1, many=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.LIST)
class MongodbListUsersResponseSchemaV1(ListResponseSchemaV1):
    """
    MongoDB user list schema.
    """

    users = Nested(MongodbUserSchemaV1, many=True, required=True)


class MongodbUserMetadata(UserSchemaV1):
    """
    MongoDB user meta for Operation.metadata field
    """

    name = Str(validate=MongoDBClusterTraits.user_name.validate, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.CREATE)
class MongodbCreateUserRequestSchemaV1(Schema):
    """
    Schema for create MongoDB user request.
    """

    userSpec = Nested(MongodbUserSpecSchemaV1, attribute='user_spec', required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.MODIFY)
class MongodbUpdateUserRequestSchemaV1(Schema):
    """
    Schema for update MongoDB user request.
    """

    password = Str(validate=MongoDBClusterTraits.password.validate)
    permissions = Nested(MongodbPermissionSchemaV1, many=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.GRANT_PERMISSION)
class MongodbAddUserPermissionRequestSchemaV1(Schema):
    """
    Schema for add MongoDB user permission request.
    """

    permission = Nested(MongodbPermissionSchemaV1, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.REVOKE_PERMISSION)
class MongodbRevokeUserPermissionRequestSchemaV1(Schema):
    """
    Schema for revoke MongoDB user permission request.
    """

    databaseName = Str(
        attribute='database_name', validate=MongoDBClusterTraits.role_assign_db_name.validate, required=True
    )


@register_response_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.INFO)
class MongodbDatabaseSchemaV1(DatabaseSchemaV1):
    """
    MongoDB database schema.
    """

    name = Str(validate=MongoDBClusterTraits.db_name.validate, required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.LIST)
class MongodbListDatabasesResponseSchemaV1(ListResponseSchemaV1):
    """
    MongoDB database list schema.
    """

    databases = Nested(MongodbDatabaseSchemaV1, many=True, required=True)


class MongodbDatabaseSpecSchemaV1(Schema):
    """
    MongoDB database spec schema.
    """

    name = Str(validate=MongoDBClusterTraits.db_name.validate, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.CREATE)
class MongodbCreateDatabaseRequestSchemaV1(Schema):
    """
    Schema for create MongoDB database request.
    """

    databaseSpec = Nested(MongodbDatabaseSpecSchemaV1, attribute='database_spec', required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.INFO)
class MongodbShardSchemaV1(ShardSchemaV1):
    """
    MongoDB shard schema.
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.LIST)
class MongodbListShardsResponseSchemaV1(ListResponseSchemaV1):
    """
    MongoDB shard list schema.
    """

    shards = Nested(MongodbShardSchemaV1, many=True, required=True)


class MongodbHostServiceSchemaV1(Schema):
    """
    Type of host services.
    """

    type = MappedEnum(
        {
            'MONGOD': ServiceType.mongod,
            'MONGOS': ServiceType.mongos,
            'MONGOCFG': ServiceType.mongocfg,
        }
    )
    health = MappedEnum(
        {
            'ALIVE': ServiceStatus.alive,
            'DEAD': ServiceStatus.dead,
            'UNKNOWN': ServiceStatus.unknown,
        }
    )


class MongodbHostSchemaV1(HostSchemaV1):
    """
    MongoDB info host schema
    """

    type = MongodbHostType(required=True)
    shardName = Str(attribute='shard_name', validate=MongoDBClusterTraits.shard_name.validate, required=True)
    role = MappedEnum(
        {
            'UNKNOWN': HostRole.unknown,
            'PRIMARY': HostRole.primary,
            'SECONDARY': HostRole.secondary,
        }
    )
    services = List(Nested(MongodbHostServiceSchemaV1))


class MongodbHostSpecSchemaV1(HostSpecSchemaV1):
    """
    MongoDB create/info host schema
    """

    type = MongodbHostType()
    shardName = Str(attribute='shard_name', validate=MongoDBClusterTraits.shard_name.validate)


@register_response_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.LIST)
class MongodbListHostsResponseSchemaV1(ListResponseSchemaV1):
    """
    MongoDB host list schema.
    """

    hosts = Nested(MongodbHostSchemaV1, many=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_CREATE)
class MongodbAddHostsRequestSchemaV1(Schema):
    """
    Schema for add MongoDB hosts request.
    """

    hostSpecs = Nested(MongodbHostSpecSchemaV1, many=True, attribute='host_specs', required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_DELETE)
class MongodbDeleteHostsRequestsSchemaV1(Schema):
    """
    Schema for delete MongoDB hosts request.
    """

    hostNames = List(Str(), attribute='host_names', required=True)


class MongodbResourcesUpdateSchemaV1(ResourcesUpdateSchemaV1):
    """Mongodb-specific resources schema used in update"""

    diskSize = Int(attribute='disk_size')  # type: ignore


class MongodbResourcesSchemaV1(ResourcesSchemaV1):
    """Mongodb-specific resources schema used in create"""

    diskSize = Int(attribute='disk_size', required=True)  # type: ignore


class MongodbHostSpecCostEstSchemaV1(Schema):
    """
    Used for billing estimation.
    Additional fields provide Resource data usually inferred from cluster`s info.
    """

    host = Nested(MongodbHostSpecSchemaV1, required=True)
    resources = Nested(MongodbResourcesSchemaV1, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BILLING_CREATE_HOSTS)
class MongodbHostSpecCostEstRequestSchemaV1(EstimateHostCreateRequestSchemaV1):
    """
    Provides an estimate for host creation in console.
    """

    billingHostSpecs = Nested(MongodbHostSpecCostEstSchemaV1, many=True, attribute='billing_host_specs', required=True)


class OperationProfilingSchema(Schema):
    # pylint: disable=missing-docstring
    slowOpThreshold = Int(validate=Range(min=0, max=36000000), attribute='slowOpThresholdMs', restart=True)
    mode = MappedEnum(
        {
            'OFF': 'off',
            'SLOW_OP': 'slowOp',
            'ALL': 'all',
        },
        restart=True,
    )


class MongoDBEnterpriseSecuritySchema(Schema):
    # pylint: disable=missing-docstring
    class KMIPSchema(Schema):
        # pylint: disable=missing-docstring
        serverName = Str(attribute='serverName')
        port = Int(attribute='port', validate=Range(min=0, max=66535))
        serverCa = Str(attribute='serverCa')
        clientCertificate = Str(attribute='clientCertificate', load_only=True)
        keyIdentifier = Str(attribute='keyIdentifier')

    enableEncryption = Boolean(attribute='enableEncryption')
    kmip = Nested(KMIPSchema)


@register_config_schema(SUBCLUSTER_NAMES[MONGOD_HOST_TYPE], '3.6')
class Mongodb36MongodConfigSchemaV1(Schema):
    """
    MongoDB 3.6 mongod config schema
    """

    class StorageSchema(Schema):
        # pylint: disable=missing-docstring
        class WiredTigerSchema(Schema):
            # pylint: disable=missing-docstring
            class CollectionConfig(Schema):
                # pylint: disable=missing-docstring
                blockCompressor = UpperToLowerCaseEnum(('NONE', 'SNAPPY', 'ZLIB'), restart=True)

            class EngineConfigSchema(Schema):
                # pylint: disable=missing-docstring
                cacheSizeGB = Float(
                    validate=Range(min=0.25, max=10240), restart=True, variable_default=defaults.get_cache_size_gb
                )

            engineConfig = Nested(EngineConfigSchema)
            collectionConfig = Nested(CollectionConfig)

        class StorageJournalSchema(Schema):
            # pylint: disable=missing-docstring
            enabled = Boolean()
            commitInterval = Int(validate=Range(min=1, max=500), restart=True)

        wiredTiger = Nested(WiredTigerSchema)
        journal = Nested(StorageJournalSchema)

    class NetSchema(Schema):
        # pylint: disable=missing-docstring
        maxIncomingConnections = Int(validate=Range(min=10, max=16384), restart=True)

    storage = Nested(StorageSchema)
    operationProfiling = Nested(OperationProfilingSchema)
    net = Nested(NetSchema)


@register_config_schema(SUBCLUSTER_NAMES[MONGOD_HOST_TYPE], '4.0')
class Mongodb40MongodConfigSchemaV1(Schema):
    """
    MongoDB 4.0 mongod config schema
    """

    class StorageSchema(Schema):
        # pylint: disable=missing-docstring
        class WiredTigerSchema(Schema):
            # pylint: disable=missing-docstring
            class CollectionConfig(Schema):
                # pylint: disable=missing-docstring
                blockCompressor = UpperToLowerCaseEnum(('NONE', 'SNAPPY', 'ZLIB'), restart=True)

            class EngineConfigSchema(Schema):
                # pylint: disable=missing-docstring
                cacheSizeGB = Float(
                    validate=Range(min=0.25, max=10240), restart=True, variable_default=defaults.get_cache_size_gb
                )

            engineConfig = Nested(EngineConfigSchema)
            collectionConfig = Nested(CollectionConfig)

        class StorageJournalSchema(Schema):
            # pylint: disable=missing-docstring
            commitInterval = Int(validate=Range(min=1, max=500), restart=True)

        wiredTiger = Nested(WiredTigerSchema)
        journal = Nested(StorageJournalSchema)

    class NetSchema(Schema):
        # pylint: disable=missing-docstring
        maxIncomingConnections = Int(validate=Range(min=10, max=16384), restart=True)

    storage = Nested(StorageSchema)
    operationProfiling = Nested(OperationProfilingSchema)
    net = Nested(NetSchema)


@register_config_schema(SUBCLUSTER_NAMES[MONGOD_HOST_TYPE], '4.2')
class Mongodb42MongodConfigSchemaV1(Schema):
    """
    MongoDB 4.2 mongod config schema
    """

    class StorageSchema(Schema):
        # pylint: disable=missing-docstring
        class WiredTigerSchema(Schema):
            # pylint: disable=missing-docstring
            class CollectionConfig(Schema):
                # pylint: disable=missing-docstring
                blockCompressor = UpperToLowerCaseEnum(('NONE', 'SNAPPY', 'ZLIB'), restart=True)

            class EngineConfigSchema(Schema):
                # pylint: disable=missing-docstring
                cacheSizeGB = Float(
                    validate=Range(min=0.25, max=10240), restart=True, variable_default=defaults.get_cache_size_gb
                )

            engineConfig = Nested(EngineConfigSchema)
            collectionConfig = Nested(CollectionConfig)

        class StorageJournalSchema(Schema):
            # pylint: disable=missing-docstring
            commitInterval = Int(validate=Range(min=1, max=500), restart=True)

        wiredTiger = Nested(WiredTigerSchema)
        journal = Nested(StorageJournalSchema)

    class NetSchema(Schema):
        # pylint: disable=missing-docstring
        maxIncomingConnections = Int(validate=Range(min=10, max=16384), restart=True)

    storage = Nested(StorageSchema)
    operationProfiling = Nested(OperationProfilingSchema)
    net = Nested(NetSchema)


@register_config_schema(SUBCLUSTER_NAMES[MONGOD_HOST_TYPE], '4.4')
class Mongodb44MongodConfigSchemaV1(Schema):
    """
    MongoDB 4.4 mongod config schema
    """

    class StorageSchema(Schema):
        # pylint: disable=missing-docstring
        class WiredTigerSchema(Schema):
            # pylint: disable=missing-docstring
            class CollectionConfig(Schema):
                # pylint: disable=missing-docstring
                blockCompressor = UpperToLowerCaseEnum(('NONE', 'SNAPPY', 'ZLIB'), restart=True)

            class EngineConfigSchema(Schema):
                # pylint: disable=missing-docstring
                cacheSizeGB = Float(
                    validate=Range(min=0.25, max=10240), restart=True, variable_default=defaults.get_cache_size_gb
                )

            engineConfig = Nested(EngineConfigSchema)
            collectionConfig = Nested(CollectionConfig)

        class StorageJournalSchema(Schema):
            # pylint: disable=missing-docstring
            commitInterval = Int(validate=Range(min=1, max=500), restart=True)

        wiredTiger = Nested(WiredTigerSchema)
        journal = Nested(StorageJournalSchema)

    class NetSchema(Schema):
        # pylint: disable=missing-docstring
        maxIncomingConnections = Int(validate=Range(min=10, max=16384), restart=True)

    storage = Nested(StorageSchema)
    operationProfiling = Nested(OperationProfilingSchema)
    net = Nested(NetSchema)


@register_config_schema(SUBCLUSTER_NAMES[MONGOD_HOST_TYPE], f'4.4-{EDITION_ENTERPRISE}')
class Mongodb44EnterpriseMongodConfigSchemaV1(Schema):
    """
    MongoDB 4.4-enterprise mongod config schema
    """

    class StorageSchema(Schema):
        # pylint: disable=missing-docstring
        class WiredTigerSchema(Schema):
            # pylint: disable=missing-docstring
            class CollectionConfig(Schema):
                # pylint: disable=missing-docstring
                blockCompressor = UpperToLowerCaseEnum(('NONE', 'SNAPPY', 'ZLIB'), restart=True)

            class EngineConfigSchema(Schema):
                # pylint: disable=missing-docstring
                cacheSizeGB = Float(
                    validate=Range(min=0.25, max=10240), restart=True, variable_default=defaults.get_cache_size_gb
                )

            engineConfig = Nested(EngineConfigSchema)
            collectionConfig = Nested(CollectionConfig)

        class StorageJournalSchema(Schema):
            # pylint: disable=missing-docstring
            commitInterval = Int(validate=Range(min=1, max=500), restart=True)

        wiredTiger = Nested(WiredTigerSchema)
        journal = Nested(StorageJournalSchema)

    class NetSchema(Schema):
        # pylint: disable=missing-docstring
        maxIncomingConnections = Int(validate=Range(min=10, max=16384), restart=True)

    class AuditLogSchema(Schema):
        # pylint: disable=missing-docstring
        filter = Str()

    class SetParameterSchema(Schema):
        # pylint: disable=missing-docstring
        auditAuthorizationSuccess = Boolean()

    storage = Nested(StorageSchema)
    operationProfiling = Nested(OperationProfilingSchema)
    net = Nested(NetSchema)
    security = Nested(MongoDBEnterpriseSecuritySchema)
    auditLog = Nested(AuditLogSchema)
    setParameter = Nested(SetParameterSchema)


@register_config_schema(SUBCLUSTER_NAMES[MONGOD_HOST_TYPE], '5.0')
class Mongodb50MongodConfigSchemaV1(Schema):
    """
    MongoDB 5.0 mongod config schema
    """

    class StorageSchema(Schema):
        # pylint: disable=missing-docstring
        class WiredTigerSchema(Schema):
            # pylint: disable=missing-docstring
            class CollectionConfig(Schema):
                # pylint: disable=missing-docstring
                blockCompressor = UpperToLowerCaseEnum(('NONE', 'SNAPPY', 'ZLIB'), restart=True)

            class EngineConfigSchema(Schema):
                # pylint: disable=missing-docstring
                cacheSizeGB = Float(
                    validate=Range(min=0.25, max=10240), restart=True, variable_default=defaults.get_cache_size_gb
                )

            engineConfig = Nested(EngineConfigSchema)
            collectionConfig = Nested(CollectionConfig)

        class StorageJournalSchema(Schema):
            # pylint: disable=missing-docstring
            commitInterval = Int(validate=Range(min=1, max=500), restart=True)

        wiredTiger = Nested(WiredTigerSchema)
        journal = Nested(StorageJournalSchema)

    class NetSchema(Schema):
        # pylint: disable=missing-docstring
        maxIncomingConnections = Int(validate=Range(min=10, max=16384), restart=True)

    storage = Nested(StorageSchema)
    operationProfiling = Nested(OperationProfilingSchema)
    net = Nested(NetSchema)


@register_config_schema(SUBCLUSTER_NAMES[MONGOD_HOST_TYPE], f'5.0-{EDITION_ENTERPRISE}')
class Mongodb50EnterpriseMongodConfigSchemaV1(Schema):
    """
    MongoDB 5.0-enterprise mongod config schema
    """

    class StorageSchema(Schema):
        # pylint: disable=missing-docstring
        class WiredTigerSchema(Schema):
            # pylint: disable=missing-docstring
            class CollectionConfig(Schema):
                # pylint: disable=missing-docstring
                blockCompressor = UpperToLowerCaseEnum(('NONE', 'SNAPPY', 'ZLIB'), restart=True)

            class EngineConfigSchema(Schema):
                # pylint: disable=missing-docstring
                cacheSizeGB = Float(
                    validate=Range(min=0.25, max=10240), restart=True, variable_default=defaults.get_cache_size_gb
                )

            engineConfig = Nested(EngineConfigSchema)
            collectionConfig = Nested(CollectionConfig)

        class StorageJournalSchema(Schema):
            # pylint: disable=missing-docstring
            commitInterval = Int(validate=Range(min=1, max=500), restart=True)

        wiredTiger = Nested(WiredTigerSchema)
        journal = Nested(StorageJournalSchema)

    class NetSchema(Schema):
        # pylint: disable=missing-docstring
        maxIncomingConnections = Int(validate=Range(min=10, max=16384), restart=True)

    class AuditLogSchema(Schema):
        # pylint: disable=missing-docstring
        filter = Str()
        runtimeConfiguration = Boolean()

    class SetParameterSchema(Schema):
        # pylint: disable=missing-docstring
        auditAuthorizationSuccess = Boolean()

    storage = Nested(StorageSchema)
    operationProfiling = Nested(OperationProfilingSchema)
    net = Nested(NetSchema)
    security = Nested(MongoDBEnterpriseSecuritySchema)
    auditLog = Nested(AuditLogSchema)
    setParameter = Nested(SetParameterSchema)


@register_config_schema(SUBCLUSTER_NAMES[MONGOS_HOST_TYPE], '3.6')
@register_config_schema(SUBCLUSTER_NAMES[MONGOS_HOST_TYPE], '4.0')
@register_config_schema(SUBCLUSTER_NAMES[MONGOS_HOST_TYPE], '4.2')
@register_config_schema(SUBCLUSTER_NAMES[MONGOS_HOST_TYPE], '4.4')
@register_config_schema(SUBCLUSTER_NAMES[MONGOS_HOST_TYPE], f'4.4-{EDITION_ENTERPRISE}')
@register_config_schema(SUBCLUSTER_NAMES[MONGOS_HOST_TYPE], '5.0')
@register_config_schema(SUBCLUSTER_NAMES[MONGOS_HOST_TYPE], f'5.0-{EDITION_ENTERPRISE}')
class MongodbBaseMongosConfigSchemaV1(Schema):
    """
    MongoDB base mongos config schema
    """

    class NetSchema(Schema):
        # pylint: disable=missing-docstring
        maxIncomingConnections = Int(validate=Range(min=10, max=16384), restart=True)

    net = Nested(NetSchema)


@register_config_schema(SUBCLUSTER_NAMES[MONGOCFG_HOST_TYPE], '3.6')
@register_config_schema(SUBCLUSTER_NAMES[MONGOCFG_HOST_TYPE], '4.0')
@register_config_schema(SUBCLUSTER_NAMES[MONGOCFG_HOST_TYPE], '4.2')
@register_config_schema(SUBCLUSTER_NAMES[MONGOCFG_HOST_TYPE], '4.4')
@register_config_schema(SUBCLUSTER_NAMES[MONGOCFG_HOST_TYPE], f'4.4-{EDITION_ENTERPRISE}')
@register_config_schema(SUBCLUSTER_NAMES[MONGOCFG_HOST_TYPE], '5.0')
@register_config_schema(SUBCLUSTER_NAMES[MONGOCFG_HOST_TYPE], f'5.0-{EDITION_ENTERPRISE}')
class MongodbBaseMongocfgConfigSchemaV1(Schema):
    """
    MongoDB base mongocfg config schema
    """

    class StorageSchema(Schema):
        # pylint: disable=missing-docstring
        class WiredTigerSchema(Schema):
            # pylint: disable=missing-docstring
            class EngineConfigSchema(Schema):
                # pylint: disable=missing-docstring
                cacheSizeGB = Float(
                    validate=Range(min=0.25, max=10240), restart=True, variable_default=defaults.get_cache_size_gb
                )

            engineConfig = Nested(EngineConfigSchema)

        wiredTiger = Nested(WiredTigerSchema)

    class NetSchema(Schema):
        # pylint: disable=missing-docstring
        maxIncomingConnections = Int(validate=Range(min=10, max=16384), restart=True)

    net = Nested(NetSchema)
    storage = Nested(StorageSchema)
    operationProfiling = Nested(OperationProfilingSchema)


@register_config_schema(SUBCLUSTER_NAMES[MONGOINFRA_HOST_TYPE], '3.6')
@register_config_schema(SUBCLUSTER_NAMES[MONGOINFRA_HOST_TYPE], '4.0')
@register_config_schema(SUBCLUSTER_NAMES[MONGOINFRA_HOST_TYPE], '4.2')
@register_config_schema(SUBCLUSTER_NAMES[MONGOINFRA_HOST_TYPE], '4.4')
@register_config_schema(SUBCLUSTER_NAMES[MONGOINFRA_HOST_TYPE], f'4.4-{EDITION_ENTERPRISE}')
@register_config_schema(SUBCLUSTER_NAMES[MONGOINFRA_HOST_TYPE], '5.0')
@register_config_schema(SUBCLUSTER_NAMES[MONGOINFRA_HOST_TYPE], f'5.0-{EDITION_ENTERPRISE}')
class MongodbBaseMongoinfraConfigSchemaV1(Schema):
    """
    MongoDB base mongoinfra config schema
    """

    pass


class Mongodb36MongodConfigSetSchemaV1(Schema):
    """
    MongoDB 3.6 mongod DBMS config set schema.
    """

    effectiveConfig = Nested(Mongodb36MongodConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Mongodb36MongodConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Mongodb36MongodConfigSchemaV1, attribute='default_config')


class Mongodb40MongodConfigSetSchemaV1(Schema):
    """
    MongoDB 4.0 mongod DBMS config set schema.
    """

    effectiveConfig = Nested(Mongodb40MongodConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Mongodb40MongodConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Mongodb40MongodConfigSchemaV1, attribute='default_config')


class Mongodb42MongodConfigSetSchemaV1(Schema):
    """
    MongoDB 4.2 mongod DBMS config set schema.
    """

    effectiveConfig = Nested(Mongodb42MongodConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Mongodb42MongodConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Mongodb42MongodConfigSchemaV1, attribute='default_config')


class Mongodb44MongodConfigSetSchemaV1(Schema):
    """
    MongoDB 4.4 mongod DBMS config set schema.
    """

    effectiveConfig = Nested(Mongodb44MongodConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Mongodb44MongodConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Mongodb44MongodConfigSchemaV1, attribute='default_config')


class Mongodb44EnterpriseMongodConfigSetSchemaV1(Schema):
    """
    MongoDB 4.4-enterprise mongod DBMS config set schema.
    """

    effectiveConfig = Nested(Mongodb44EnterpriseMongodConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Mongodb44EnterpriseMongodConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Mongodb44EnterpriseMongodConfigSchemaV1, attribute='default_config')


class Mongodb50MongodConfigSetSchemaV1(Schema):
    """
    MongoDB 5.0 mongod DBMS config set schema.
    """

    effectiveConfig = Nested(Mongodb50MongodConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Mongodb50MongodConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Mongodb50MongodConfigSchemaV1, attribute='default_config')


class Mongodb50EnterpriseMongodConfigSetSchemaV1(Schema):
    """
    MongoDB 5.0 mongod DBMS config set schema.
    """

    effectiveConfig = Nested(Mongodb50EnterpriseMongodConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Mongodb50EnterpriseMongodConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Mongodb50EnterpriseMongodConfigSchemaV1, attribute='default_config')


class MongodbBaseMongosConfigSetSchemaV1(Schema):
    """
    MongoDB mongod DBMS config set schema.
    """

    effectiveConfig = Nested(MongodbBaseMongosConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(MongodbBaseMongosConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(MongodbBaseMongosConfigSchemaV1, attribute='default_config')


class MongodbBaseMongocfgConfigSetSchemaV1(Schema):
    """
    MongoDB mongod DBMS config set schema.
    """

    effectiveConfig = Nested(MongodbBaseMongocfgConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(MongodbBaseMongocfgConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(MongodbBaseMongocfgConfigSchemaV1, attribute='default_config')


class MongodbConfigSpecCreateSchemaV1(ClusterConfigSchemaV1):
    """
    MongoDB cluster config schema used in create.
    """

    class Mongodb36SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            config = Nested(Mongodb36MongodConfigSchemaV1)
            resources = Nested(ResourcesSchemaV1, required=True)

        mongod = Nested(Mongod, required=True)

    class Mongodb40SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            config = Nested(Mongodb40MongodConfigSchemaV1)
            resources = Nested(ResourcesSchemaV1, required=True)

        mongod = Nested(Mongod, required=True)

    class Mongodb42SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            config = Nested(Mongodb42MongodConfigSchemaV1)
            resources = Nested(ResourcesSchemaV1, required=True)

        mongod = Nested(Mongod, required=True)

    class Mongodb44SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            config = Nested(Mongodb44MongodConfigSchemaV1)
            resources = Nested(ResourcesSchemaV1, required=True)

        mongod = Nested(Mongod, required=True)

    class Mongodb44EnterpriseSchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            config = Nested(Mongodb44EnterpriseMongodConfigSchemaV1)
            resources = Nested(ResourcesSchemaV1, required=True)

        mongod = Nested(Mongod, required=True)

    class Mongodb50SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            config = Nested(Mongodb50MongodConfigSchemaV1)
            resources = Nested(ResourcesSchemaV1, required=True)

        mongod = Nested(Mongod, required=True)

    class Mongodb50EnterpriseSchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            config = Nested(Mongodb50EnterpriseMongodConfigSchemaV1)
            resources = Nested(ResourcesSchemaV1, required=True)

        mongod = Nested(Mongod, required=True)

    mongodbSpec_3_6 = Nested(Mongodb36SchemaV1, attribute='mongodb_spec_3_6')
    mongodbSpec_4_0 = Nested(Mongodb40SchemaV1, attribute='mongodb_spec_4_0')
    mongodbSpec_4_2 = Nested(Mongodb42SchemaV1, attribute='mongodb_spec_4_2')
    mongodbSpec_4_4 = Nested(Mongodb44SchemaV1, attribute='mongodb_spec_4_4')
    mongodbSpec_4_4_enterprise = Nested(Mongodb44EnterpriseSchemaV1, attribute=f'mongodb_spec_4_4-{EDITION_ENTERPRISE}')
    mongodbSpec_4_4Enterprise = mongodbSpec_4_4_enterprise
    mongodbSpec_5_0 = Nested(Mongodb50SchemaV1, attribute='mongodb_spec_5_0')
    mongodbSpec_5_0_enterprise = Nested(Mongodb50EnterpriseSchemaV1, attribute=f'mongodb_spec_5_0-{EDITION_ENTERPRISE}')
    mongodbSpec_5_0Enterprise = mongodbSpec_5_0_enterprise
    backupWindowStart = Nested(TimeOfDay, attribute='backup_window_start')
    backupRetainPeriodDays = UInt(validate=Range(min=7, max=35), attribute='backup_retain_period_days')
    performanceDiagnostics = Nested(PerformanceDiagnostics, attribute='performance_diagnostics')
    access = Nested(AccessSchemaV1, attribute='access')


# This is RestartSchema, not plain Schema!
class MongodbConfigSpecUpdateSchemaV1(ClusterConfigSchemaV1):
    """
    MongoDB cluster config schema used in update.
    The only difference compared to Create is that resources are not required.
    TODO: too much copy-paste, mb a smarter approach later.
    """

    class Mongodb36SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(Mongodb36MongodConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongosConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongocfgConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    class Mongodb40SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(Mongodb40MongodConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongosConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongocfgConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    class Mongodb42SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(Mongodb42MongodConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongosConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongocfgConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    class Mongodb44SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(Mongodb44MongodConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongosConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongocfgConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    class Mongodb44EnterpriseSchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(Mongodb44EnterpriseMongodConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongosConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongocfgConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    class Mongodb50SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(Mongodb50MongodConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongosConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongocfgConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    class Mongodb50EnterpriseSchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(Mongodb50EnterpriseMongodConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongosConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: This is an input schema, so only one config.
            config = Nested(MongodbBaseMongocfgConfigSchemaV1)
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesUpdateSchemaV1)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    mongodbSpec_3_6 = Nested(Mongodb36SchemaV1, attribute='mongodb_spec_3_6')
    mongodbSpec_4_0 = Nested(Mongodb40SchemaV1, attribute='mongodb_spec_4_0')
    mongodbSpec_4_2 = Nested(Mongodb42SchemaV1, attribute='mongodb_spec_4_2')
    mongodbSpec_4_4 = Nested(Mongodb44SchemaV1, attribute='mongodb_spec_4_4')
    mongodbSpec_4_4_enterprise = Nested(Mongodb44EnterpriseSchemaV1, attribute=f'mongodb_spec_4_4-{EDITION_ENTERPRISE}')
    mongodbSpec_4_4Enterprise = mongodbSpec_4_4_enterprise
    mongodbSpec_5_0 = Nested(Mongodb50SchemaV1, attribute='mongodb_spec_5_0')
    mongodbSpec_5_0_enterprise = Nested(Mongodb50EnterpriseSchemaV1, attribute=f'mongodb_spec_5_0-{EDITION_ENTERPRISE}')
    mongodbSpec_5_0Enterprise = mongodbSpec_5_0_enterprise
    featureCompatibilityVersion = Str(attribute='feature_compatibility_version')
    backupWindowStart = Nested(TimeOfDay, attribute='backup_window_start')
    backupRetainPeriodDays = UInt(validate=Range(min=7, max=35), attribute='backup_retain_period_days')
    performanceDiagnostics = Nested(PerformanceDiagnostics, attribute='performance_diagnostics')
    access = Nested(AccessSchemaV1, attribute='access')


class MongodbClusterConfigSchemaV1(ClusterConfigSchemaV1):
    """
    MongoDB cluster config schema used to display config.
    """

    class Mongodb36SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(Mongodb36MongodConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongosConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongocfgConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSetSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSetSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    class Mongodb40SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(Mongodb40MongodConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongosConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongocfgConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSetSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSetSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    class Mongodb42SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(Mongodb42MongodConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongosConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongocfgConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSetSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSetSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    class Mongodb44SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(Mongodb44MongodConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongosConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongocfgConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSetSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSetSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    class Mongodb44EnterpriseSchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(Mongodb44EnterpriseMongodConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongosConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongocfgConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSetSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSetSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    class Mongodb50SchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(Mongodb50MongodConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongosConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongocfgConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSetSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSetSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    class Mongodb50EnterpriseSchemaV1(Schema):
        """
        MongoDB base schema
        """

        class Mongod(Schema):
            """
            Mongod schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(Mongodb50EnterpriseMongodConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongos(Schema):
            """
            Mongos schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongosConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongocfg(Schema):
            """
            Mongocfg schema
            """

            # NOTE: Output schema, return config set.
            config = Nested(MongodbBaseMongocfgConfigSetSchemaV1)
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        class Mongoinfra(Schema):
            """
            Mongoinfra schema
            """

            # NOTE: This is an input schema, so only one config.
            config_mongos = Nested(MongodbBaseMongosConfigSetSchemaV1, attribute='config_mongos')
            config_mongocfg = Nested(MongodbBaseMongocfgConfigSetSchemaV1, attribute='config_mongocfg')
            resources = Nested(MongodbResourcesSchemaV1, required=True)

        mongod = Nested(Mongod)
        mongos = Nested(Mongos)
        mongocfg = Nested(Mongocfg)
        mongoinfra = Nested(Mongoinfra)

    mongodb_3_6 = Nested(Mongodb36SchemaV1, attribute='mongodb_3_6')
    mongodb_4_0 = Nested(Mongodb40SchemaV1, attribute='mongodb_4_0')
    mongodb_4_2 = Nested(Mongodb42SchemaV1, attribute='mongodb_4_2')
    mongodb_4_4 = Nested(Mongodb44SchemaV1, attribute='mongodb_4_4')
    mongodb_4_4_enterprise = Nested(Mongodb44EnterpriseSchemaV1, attribute=f'mongodb_4_4-{EDITION_ENTERPRISE}')
    mongodb_4_4Enterprise = mongodb_4_4_enterprise
    mongodb_5_0 = Nested(Mongodb50SchemaV1, attribute='mongodb_5_0')
    mongodb_5_0_enterprise = Nested(Mongodb50EnterpriseSchemaV1, attribute=f'mongodb_5_0-{EDITION_ENTERPRISE}')
    mongodb_5_0Enterprise = mongodb_5_0_enterprise
    featureCompatibilityVersion = Str(attribute='feature_compatibility_version')
    backupWindowStart = Nested(TimeOfDay, attribute='backup_window_start')
    backupRetainPeriodDays = UInt(validate=Range(min=7, max=35), attribute='backup_retain_period_days')
    performanceDiagnostics = Nested(PerformanceDiagnostics, attribute='performance_diagnostics')
    access = Nested(AccessSchemaV1, attribute='access')


@register_response_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.INFO)
class MongodbClusterSchemaV1(ManagedClusterSchemaV1):
    """
    MongoDB cluster schema.
    """

    config = Nested(MongodbClusterConfigSchemaV1, required=True)
    sharded = Boolean()


@register_response_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.LIST)
class MongodbListClustersResponseSchemaV1(ListResponseSchemaV1):
    """
    MongoDB cluster list schema.
    """

    clusters = Nested(MongodbClusterSchemaV1, many=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CREATE)
@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.BILLING_CREATE)
class MongodbCreateClusterRequestSchemaV1(CreateClusterRequestSchemaV1):
    """
    Schema for create Mongodb cluster request.
    """

    name = Str(validate=MongoDBClusterTraits.cluster_name.validate, required=True)
    configSpec = Nested(MongodbConfigSpecCreateSchemaV1, attribute='config_spec', required=True)
    databaseSpecs = List(Nested(MongodbDatabaseSpecSchemaV1), attribute='database_specs', required=True)
    userSpecs = List(Nested(MongodbUserSpecSchemaV1), attribute='user_specs', required=True)
    hostSpecs = List(Nested(MongodbHostSpecSchemaV1), attribute='host_specs', required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MODIFY)
class MongodbUpdateClusterRequestSchemaV1(UpdateClusterRequestSchemaV1):
    """
    Schema for update MongoDB cluster request.
    """

    configSpec = Nested(MongodbConfigSpecUpdateSchemaV1, attribute='config_spec')
    name = Str(validate=MongoDBClusterTraits.cluster_name.validate)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.RESTORE)
class MongodbRestoreClusterRequestSchemaV1(RestoreClusterRequestSchemaV1):
    """
    Schema for restore MongoDB cluster request.
    """

    class RecoveryTargetSpec(Schema):
        # pylint: disable=missing-docstring
        timestamp = UInt(required=True)
        inc = UInt(validate=Range(min=1))

    recoveryTargetSpec = Nested(RecoveryTargetSpec, attribute='recovery_target_spec')
    configSpec = Nested(MongodbConfigSpecCreateSchemaV1, attribute='config_spec', required=True)
    hostSpecs = Nested(
        MongodbHostSpecSchemaV1, validate=Length(min=1), attribute='host_specs', required=True, many=True
    )


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.RESTORE_HINTS)
class MongodbRestoreHintsResponseSchemaV1(RestoreHintWithTimeResponseSchemaV1):
    """
    Schema for MongoDB restore hint
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.INFO)
class MongodbBackupSchemaV1(BaseBackupSchemaV1):
    """
    Schema for MongoDB backup
    """

    sourceShardNames = List(Str(validate=MongoDBClusterTraits.shard_name.validate), attribute='shard_names')
    size = Int()
    type = MappedEnum(
        {
            'BACKUP_TYPE_UNSPECIFIED': BackupInitiator.unspecified,
            'AUTOMATED': BackupInitiator.automated,
            'MANUAL': BackupInitiator.manual,
        }
    )


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.LIST)
class MongodbListClusterBackupsResponseSchemaV1(ListResponseSchemaV1):
    """
    Schema for MongoDB backups listing
    """

    backups = Nested(MongodbBackupSchemaV1, many=True, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.ENABLE_SHARDING)
class MongodbCreateShardingRequestSchema(Schema):
    """
    Schema for MongoDB enable sharding request
    """

    class MongosSchema(Schema):
        """
        Mongos subcluster schema.
        """

        resources = Nested(ResourcesSchemaV1, required=True)

    class MongocfgSchema(Schema):
        """
        Mongocfg subcluster schema.
        """

        resources = Nested(ResourcesSchemaV1, required=True)

    class MongoinfraSchema(Schema):
        """
        Mongocfg subcluster schema.
        """

        resources = Nested(ResourcesSchemaV1, required=True)

    mongos = Nested(MongosSchema)
    mongocfg = Nested(MongocfgSchema)

    hostSpecs = List(Nested(MongodbHostSpecSchemaV1), attribute='host_specs', validate=Length(min=1), required=True)
    mongoinfra = Nested(MongoinfraSchema)


@register_request_schema(MY_CLUSTER_TYPE, Resource.SHARD, DbaasOperation.CREATE)
class MongodbAddShardRequestSchemaV1(Schema):
    """
    MongoDB add shard schema.
    """

    shardName = Str(attribute='shard_name', validate=MongoDBClusterTraits.shard_name.validate, required=True)
    hostSpecs = Nested(
        MongodbHostSpecSchemaV1, attribute='host_specs', many=True, validate=Length(min=1), required=True
    )


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.STOP)
class MongodbStopClusterRequestSchemaV1(Schema):
    """
    Schema for stop cluster request.
    """

    pass


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE)
class MongodbDeleteClusterRequestSchemaV1(Schema):
    """
    Schema for stop cluster request.
    """

    pass
