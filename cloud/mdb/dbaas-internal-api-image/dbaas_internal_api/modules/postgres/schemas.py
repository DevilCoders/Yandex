"""
DBaaS Internal API PostgreSQL cluster options schema
"""
# pylint: disable=too-many-lines

from marshmallow.fields import Boolean, Float, Int, List, Nested
from marshmallow.validate import Range
from pytz import all_timezones_set

from . import schema_defaults as defaults
from ...apis.schemas.backups import BaseBackupSchemaV1
from ...apis.schemas.cluster import (
    ClusterConfigSchemaV1,
    ClusterConfigSpecSchemaV1,
    CreateClusterRequestSchemaV1,
    ManagedClusterSchemaV1,
    RestoreClusterRequestSchemaV1,
    StartClusterFailoverRequestSchemaV1,
    UpdateClusterRequestSchemaV1,
)
from ...apis.schemas.common import ListResponseSchemaV1
from ...apis.schemas.console import (
    ClustersConfigAvailableVersionSchemaV1,
    RestoreHintWithTimeResponseSchemaV1,
    StringValueV1,
)
from ...apis.schemas.fields import (
    IntBoolean,
    GrpcStr,
    MappedEnum,
    PrefixedUpperToLowerCaseAndSpacesEnum,
    PrefixedUpperToLowerCaseEnum,
    Str,
    UInt,
    UpperToLowerCaseEnum,
)
from ...apis.schemas.objects import (
    AccessSchemaV1,
    DatabaseSchemaV1,
    HostSchemaV1,
    HostSpecSchemaV1,
    ResourcePresetSchemaV1,
    ResourcesSchemaV1,
    ResourcesUpdateSchemaV1,
    UserSchemaV1,
)

from ...apis.schemas.alerts import (
    CreateAlertsGroupRequestSchemaV1,
    AlertGroupSchemaV1,
    AlertTemplateListSchemaV1,
    UpdateAlertGroupRequestSchemaV1,
)

from ...health.health import ServiceStatus
from ...utils import validation
from ...utils.types import BackupInitiator, KILOBYTE, TERABYTE, GIGABYTE, MEGABYTE, BackupMethod
from ...utils.register import (
    DbaasOperation,
    Resource,
    register_config_schema,
    register_request_schema,
    register_response_schema,
)
from ...utils.validation import OneOf, Schema, TimeOfDay
from .constants import MY_CLUSTER_TYPE, EDITION_1C
from .fields import PostgresqlSizeKB, PostgresqlSizeMB, PostgresqlTimespanMs
from .traits import HostReplicaType, HostRole, PostgresqlClusterTraits, ServiceType
from .validation import (
    ALLOWED_SHARED_PRELOAD_LIBS,
    COLLATE_VALIDATOR,
    validate_pg_qualstats_sample_rate,
    validate_search_path,
)

PREFIXED_SHARED_PRELOAD_LIBS = [
    'SHARED_PRELOAD_LIBRARIES_{}'.format(lib.upper()) for lib in ALLOWED_SHARED_PRELOAD_LIBS
]

LOG_LEVELS = [
    'DEBUG5',
    'DEBUG4',
    'DEBUG3',
    'DEBUG2',
    'DEBUG1',
    'LOG',
    'NOTICE',
    'WARNING',
    'ERROR',
    'FATAL',
    'PANIC',
]

PREFIXED_LOG_LEVELS = [
    'LOG_LEVEL_DEBUG5',
    'LOG_LEVEL_DEBUG4',
    'LOG_LEVEL_DEBUG3',
    'LOG_LEVEL_DEBUG2',
    'LOG_LEVEL_DEBUG1',
    'LOG_LEVEL_LOG',
    'LOG_LEVEL_NOTICE',
    'LOG_LEVEL_WARNING',
    'LOG_LEVEL_ERROR',
    'LOG_LEVEL_FATAL',
    'LOG_LEVEL_PANIC',
]

MAX_INT32 = 2147483647
BLOCK_SIZE = 8192
SEC_PER_DAY = 86400


class PostgresAccessSchemaV1(AccessSchemaV1):
    """
    PG specific Access schema.
    """

    serverless = Boolean(attribute='serverless', default=False)


class ReplicationSource(GrpcStr):
    """
    Type of replicationSource fields.
    """

    def __init__(self, **kwargs) -> None:
        super().__init__(attribute='replication_source', restart=True, **kwargs)


@register_response_schema(MY_CLUSTER_TYPE, Resource.RESOURCE_PRESET, DbaasOperation.INFO)
class PostgresqlResourcePresetSchemaV1(ResourcePresetSchemaV1):
    """
    PostgreSQL resource preset schema.
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.RESOURCE_PRESET, DbaasOperation.LIST)
class PostgresqlListResourcePresetsSchemaV1(ListResponseSchemaV1):
    """
    PostgreSQL resource preset list schema.
    """

    resourcePresets = Nested(PostgresqlResourcePresetSchemaV1, many=True, attribute='resource_presets', required=True)


class PostgresqlConsoleClustersConfigDiskSizeRangeSchemaV1(Schema):
    """
    PostgreSQL disk size range schema.
    """

    min = Int()
    max = Int()


class PostgresqlConsoleClustersConfigDiskSizesSchemaV1(Schema):
    """
    PostgreSQL disk sizes list schema.
    """

    sizes = List(Int())


class PostgresqlConsoleClustersConfigDiskTypesSchemaV1(Schema):
    """
    PostgreSQL available disk type schema.
    """

    diskTypeId = Str(attribute='disk_type_id')
    diskSizeRange = Nested(PostgresqlConsoleClustersConfigDiskSizeRangeSchemaV1(), attribute='disk_size_range')
    diskSizes = Nested(PostgresqlConsoleClustersConfigDiskSizesSchemaV1(), attribute='disk_sizes')
    minHosts = Int(attribute='min_hosts')
    maxHosts = Int(attribute='max_hosts')


class PostgresqlConsoleClustersConfigZoneSchemaV1(Schema):
    """
    PostgreSQL available zone schema.
    """

    zoneId = Str(attribute='zone_id')
    diskTypes = Nested(PostgresqlConsoleClustersConfigDiskTypesSchemaV1, many=True, attribute='disk_types')


class PostgresqlConsoleClustersConfigResourcePresetSchemaV1(Schema):
    """
    PostgreSQL available resource preset schema.
    """

    presetId = Str(attribute='preset_id')
    cpuLimit = Int(attribute='cpu_limit')
    cpuFraction = Int(attribute='cpu_fraction')
    memoryLimit = Int(attribute='memory_limit')
    type = Str(attribute='type')
    generation = Str(attribute='generation')
    generationName = Str(attribute='generation_name')
    decommissioning = Boolean(attribute='decommissioning')
    zones = Nested(PostgresqlConsoleClustersConfigZoneSchemaV1, many=True)


class PostgresqlConsoleClustersConfigHostCountPerDiskTypeSchemaV1(Schema):
    """
    Postgresql host count limits for disk type schema.
    """

    diskTypeId = Str(attribute='disk_type_id')
    minHostCount = Int(attribute='min_host_count')


class PostgresqlConsoleClustersConfigHostCountLimitsSchemaV1(Schema):
    """
    Postgresql host count limits schema.
    """

    minHostCount = Int(attribute='min_host_count')
    maxHostCount = Int(attribute='max_host_count')
    hostCountPerDiskType = Nested(
        PostgresqlConsoleClustersConfigHostCountPerDiskTypeSchemaV1, many=True, attribute='host_count_per_disk_type'
    )


class PostgresqlConsoleClustersConfigDefaultResourcesSchemaV1(Schema):
    """
    PostgreSQL default resources schema.
    """

    generation = Str(attribute='generation')
    generationName = Str(attribute='generation_name')
    resourcePresetId = Str(attribute='resource_preset_id')
    diskTypeId = Str(attribute='disk_type_id')
    diskSize = Int(attribute='disk_size')


@register_response_schema(MY_CLUSTER_TYPE, Resource.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
class PostgresqlConsoleClustersConfigSchemaV1(Schema):
    """
    PostgreSQL console clusters config schema.
    """

    clusterName = Nested(StringValueV1(), attribute='cluster_name')
    dbName = Nested(StringValueV1(), attribute='db_name')
    userName = Nested(StringValueV1(), attribute='user_name')
    password = Nested(StringValueV1())
    hostCountLimits = Nested(PostgresqlConsoleClustersConfigHostCountLimitsSchemaV1(), attribute='host_count_limits')
    resourcePresets = Nested(
        PostgresqlConsoleClustersConfigResourcePresetSchemaV1, many=True, attribute='resource_presets'
    )
    defaultResources = Nested(PostgresqlConsoleClustersConfigDefaultResourcesSchemaV1, attribute='default_resources')
    versions = List(Str())
    availableVersions = Nested(ClustersConfigAvailableVersionSchemaV1, many=True, attribute='available_versions')
    defaultVersion = Str(attribute='default_version')


class PostgresqlCommonConfigSchema(Schema):
    """
    PostgreSQL common (host, cluster) config schema
    """

    sharedBuffers = PostgresqlSizeKB(
        min=128 * KILOBYTE,
        max=MAX_INT32 * BLOCK_SIZE,
        attribute='shared_buffers',
        restart=True,
        variable_default=defaults.get_shared_buffers,
    )
    tempBuffers = PostgresqlSizeKB(min=800 * KILOBYTE, max=MAX_INT32 * BLOCK_SIZE, attribute='temp_buffers')
    workMem = PostgresqlSizeKB(min=64 * KILOBYTE, max=TERABYTE, attribute='work_mem')
    tempFileLimit = PostgresqlSizeKB(min=-1, max=MAX_INT32 * BLOCK_SIZE, attribute='temp_file_limit')
    backendFlushAfter = Int(validate=Range(min=0, max=128 * BLOCK_SIZE), attribute='backend_flush_after')
    oldSnapshotThreshold = PostgresqlTimespanMs(
        min=-1, max=SEC_PER_DAY * 1000 * 60, attribute='old_snapshot_threshold', restart=True
    )
    archiveTimeout = PostgresqlTimespanMs(min=10000, max=SEC_PER_DAY * 1000, attribute='archive_timeout')
    maxStandbyStreamingDelay = PostgresqlTimespanMs(min=-1, max=MAX_INT32, attribute='max_standby_streaming_delay')
    constraintExclusion = PrefixedUpperToLowerCaseEnum(
        [
            'CONSTRAINT_EXCLUSION_ON',
            'CONSTRAINT_EXCLUSION_OFF',
            'CONSTRAINT_EXCLUSION_PARTITION',
        ],
        prefix='CONSTRAINT_EXCLUSION_',
        attribute='constraint_exclusion',
    )
    cursorTupleFraction = Float(validate=Range(min=0, max=1), attribute='cursor_tuple_fraction')
    fromCollapseLimit = Int(validate=Range(min=1, max=MAX_INT32), attribute='from_collapse_limit')
    joinCollapseLimit = Int(validate=Range(min=1, max=MAX_INT32), attribute='join_collapse_limit')
    forceParallelMode = PrefixedUpperToLowerCaseEnum(
        [
            'FORCE_PARALLEL_MODE_ON',
            'FORCE_PARALLEL_MODE_OFF',
            'FORCE_PARALLEL_MODE_REGRESS',
        ],
        prefix='FORCE_PARALLEL_MODE_',
        attribute='force_parallel_mode',
    )
    clientMinMessages = PrefixedUpperToLowerCaseEnum(
        PREFIXED_LOG_LEVELS, prefix='LOG_LEVEL_', attribute='client_min_messages'
    )
    logMinMessages = PrefixedUpperToLowerCaseEnum(
        PREFIXED_LOG_LEVELS, prefix='LOG_LEVEL_', attribute='log_min_messages'
    )
    logMinErrorStatement = PrefixedUpperToLowerCaseEnum(
        PREFIXED_LOG_LEVELS, prefix='LOG_LEVEL_', attribute='log_min_error_statement'
    )
    logMinDurationStatement = PostgresqlTimespanMs(min=-1, max=MAX_INT32, attribute='log_min_duration_statement')
    logCheckpoints = Boolean(attribute='log_checkpoints')
    logConnections = Boolean(attribute='log_connections')
    logDisconnections = Boolean(attribute='log_disconnections')
    logDuration = Boolean(attribute='log_duration')
    logErrorVerbosity = PrefixedUpperToLowerCaseEnum(
        [
            'LOG_ERROR_VERBOSITY_TERSE',
            'LOG_ERROR_VERBOSITY_DEFAULT',
            'LOG_ERROR_VERBOSITY_VERBOSE',
        ],
        prefix='LOG_ERROR_VERBOSITY_',
        attribute='log_error_verbosity',
    )
    logLockWaits = Boolean(attribute='log_lock_waits')
    logStatement = PrefixedUpperToLowerCaseEnum(
        [
            'LOG_STATEMENT_NONE',
            'LOG_STATEMENT_DDL',
            'LOG_STATEMENT_MOD',
            'LOG_STATEMENT_ALL',
        ],
        prefix='LOG_STATEMENT_',
        attribute='log_statement',
    )
    logTempFiles = Int(validate=Range(min=-1, max=MAX_INT32), attribute='log_temp_files')
    searchPath = Str(validate=validate_search_path, attribute='search_path')
    rowSecurity = Boolean(attribute='row_security')
    defaultTransactionIsolation = PrefixedUpperToLowerCaseAndSpacesEnum(
        [
            'TRANSACTION_ISOLATION_READ_UNCOMMITTED',
            'TRANSACTION_ISOLATION_READ_COMMITTED',
            'TRANSACTION_ISOLATION_REPEATABLE_READ',
            'TRANSACTION_ISOLATION_SERIALIZABLE',
        ],
        prefix='TRANSACTION_ISOLATION_',
        attribute='default_transaction_isolation',
    )
    statementTimeout = PostgresqlTimespanMs(min=0, max=MAX_INT32, attribute='statement_timeout')
    lockTimeout = PostgresqlTimespanMs(min=0, max=MAX_INT32, attribute='lock_timeout')
    idleInTransactionSessionTimeout = PostgresqlTimespanMs(
        min=0, max=MAX_INT32, attribute='idle_in_transaction_session_timeout'
    )
    byteaOutput = MappedEnum({'BYTEA_OUTPUT_HEX': 'hex', 'BYTEA_OUTPUT_ESCAPED': 'escape'}, attribute='bytea_output')
    xmlbinary = PrefixedUpperToLowerCaseEnum(['XML_BINARY_BASE64', 'XML_BINARY_HEX'], prefix='XML_BINARY_')
    xmloption = PrefixedUpperToLowerCaseEnum(['XML_OPTION_DOCUMENT', 'XML_OPTION_CONTENT'], prefix='XML_OPTION_')
    ginPendingListLimit = PostgresqlSizeKB(min=64 * KILOBYTE, max=MAX_INT32, attribute='gin_pending_list_limit')
    deadlockTimeout = PostgresqlTimespanMs(min=1, max=MAX_INT32, attribute='deadlock_timeout')
    maxLocksPerTransaction = UInt(validate=Range(min=10, max=4096), attribute='max_locks_per_transaction', restart=True)
    maxPredLocksPerTransaction = UInt(
        validate=Range(min=10, max=4096), attribute='max_pred_locks_per_transaction', restart=True
    )
    arrayNulls = Boolean(attribute='array_nulls')
    backslashQuote = PrefixedUpperToLowerCaseEnum(
        [
            'BACKSLASH_QUOTE_BACKSLASH_QUOTE',
            'BACKSLASH_QUOTE_ON',
            'BACKSLASH_QUOTE_OFF',
            'BACKSLASH_QUOTE_SAFE_ENCODING',
        ],
        prefix='BACKSLASH_QUOTE_',
        attribute='backslash_quote',
    )
    defaultWithOids = Boolean(attribute='default_with_oids')
    escapeStringWarning = Boolean(attribute='escape_string_warning')
    loCompatPrivileges = Boolean(attribute='lo_compat_privileges')
    quoteAllIdentifiers = Boolean(attribute='quote_all_identifiers')
    standardConformingStrings = Boolean(attribute='standard_conforming_strings', restart=True)
    synchronizeSeqscans = Boolean(attribute='synchronize_seqscans')
    transformNullEquals = Boolean(attribute='transform_null_equals')
    exitOnError = Boolean(attribute='exit_on_error')
    seqPageCost = Float(validate=Range(min=0), attribute='seq_page_cost')
    randomPageCost = Float(validate=Range(min=0), attribute='random_page_cost')
    enableBitmapscan = Boolean(attribute='enable_bitmapscan')
    enableHashagg = Boolean(attribute='enable_hashagg')
    enableHashjoin = Boolean(attribute='enable_hashjoin')
    enableIndexscan = Boolean(attribute='enable_indexscan')
    enableIndexonlyscan = Boolean(attribute='enable_indexonlyscan')
    enableMaterial = Boolean(attribute='enable_material')
    enableMergejoin = Boolean(attribute='enable_mergejoin')
    enableNestloop = Boolean(attribute='enable_nestloop')
    enableSeqscan = Boolean(attribute='enable_seqscan')
    enableSort = Boolean(attribute='enable_sort')
    enableTidscan = Boolean(attribute='enable_tidscan')
    maxParallelWorkersPerGather = Int(validate=Range(min=0, max=1024), attribute='max_parallel_workers_per_gather')
    maxParallelWorkers = Int(validate=Range(min=0, max=1024), attribute='max_parallel_workers')
    timezone = Str(validate=OneOf(all_timezones_set))
    onlineAnalyzeEnable = Boolean(attribute='online_analyze_enable')
    plantunerFixEmptyTable = Boolean(attribute='plantuner_fix_empty_table')


class PostgresqlConfigSchema(PostgresqlCommonConfigSchema):
    """
    PostgreSQL cluster specify config schema
    """

    maxConnections = Int(
        validate=Range(min=10), attribute='max_connections', restart=True, variable_default=defaults.get_max_connections
    )
    vacuumCostDelay = Int(validate=Range(min=0, max=100), attribute='vacuum_cost_delay')
    vacuumCostPageHit = UInt(validate=Range(min=0, max=10000), attribute='vacuum_cost_page_hit')
    vacuumCostPageMiss = UInt(validate=Range(min=0, max=10000), attribute='vacuum_cost_page_miss')
    vacuumCostPageDirty = UInt(validate=Range(min=0, max=10000), attribute='vacuum_cost_page_dirty')
    vacuumCostLimit = UInt(validate=Range(min=1, max=10000), attribute='vacuum_cost_limit')
    autovacuumWorkMem = PostgresqlSizeKB(min=-1, max=MAX_INT32, attribute='autovacuum_work_mem')
    autovacuumMaxWorkers = Int(
        validate=Range(min=1, max=128),
        attribute='autovacuum_max_workers',
        variable_default=defaults.get_autovacuum_max_workers,
        restart=True,
    )
    autovacuumVacuumCostDelay = Int(
        validate=Range(min=-1, max=100),
        attribute='autovacuum_vacuum_cost_delay',
        variable_default=defaults.get_autovacuum_vacuum_cost_delay,
    )
    autovacuumVacuumCostLimit = Int(
        validate=Range(min=-1, max=10000),
        attribute='autovacuum_vacuum_cost_limit',
        variable_default=defaults.get_autovacuum_vacuum_cost_limit,
    )
    autovacuumNaptime = PostgresqlTimespanMs(min=1000, max=MAX_INT32, attribute='autovacuum_naptime')
    autovacuumVacuumScaleFactor = Float(validate=Range(min=0.0, max=100.0), attribute='autovacuum_vacuum_scale_factor')
    autovacuumAnalyzeScaleFactor = Float(
        validate=Range(min=0.0, max=100.0), attribute='autovacuum_analyze_scale_factor'
    )
    walLevel = PrefixedUpperToLowerCaseEnum(
        ['WAL_LEVEL_REPLICA', 'WAL_LEVEL_LOGICAL'], prefix='WAL_LEVEL_', attribute='wal_level', restart=True
    )
    synchronousCommit = PrefixedUpperToLowerCaseEnum(
        [
            'SYNCHRONOUS_COMMIT_ON',
            'SYNCHRONOUS_COMMIT_OFF',
            'SYNCHRONOUS_COMMIT_LOCAL',
            'SYNCHRONOUS_COMMIT_REMOTE_WRITE',
            'SYNCHRONOUS_COMMIT_REMOTE_APPLY',
        ],
        prefix='SYNCHRONOUS_COMMIT_',
        attribute='synchronous_commit',
    )
    maxWorkerProcesses = Int(validate=Range(min=0, max=1024), attribute='max_worker_processes', restart=True)
    maxPreparedTransactions = Int(validate=Range(min=0, max=1000), attribute='max_prepared_transactions', restart=True)
    maintenanceWorkMem = PostgresqlSizeKB(
        min=MEGABYTE,
        max=128 * GIGABYTE,
        attribute='maintenance_work_mem',
        variable_default=defaults.get_maintenance_work_mem,
    )
    bgwriterDelay = PostgresqlTimespanMs(min=10, max=10000, attribute='bgwriter_delay')
    bgwriterLruMaxpages = UInt(validate=Range(min=0, max=GIGABYTE - 1), attribute='bgwriter_lru_maxpages')
    bgwriterLruMultiplier = Float(validate=Range(min=1.0, max=10.0), attribute='bgwriter_lru_multiplier')
    bgwriterFlushAfter = Int(validate=Range(min=0, max=256), attribute='bgwriter_flush_after')
    checkpointTimeout = PostgresqlTimespanMs(min=30 * 1000, max=SEC_PER_DAY * 1000, attribute='checkpoint_timeout')
    checkpointCompletionTarget = Float(validate=Range(min=0.0, max=1.0), attribute='checkpoint_completion_target')
    checkpointFlushAfter = Int(validate=Range(min=0, max=256), attribute='checkpoint_flush_after')
    maxWalSize = PostgresqlSizeMB(
        min=32 * MEGABYTE,
        max=MAX_INT32 * MEGABYTE,
        attribute='max_wal_size',
        variable_default=defaults.get_max_wal_size,
    )
    minWalSize = PostgresqlSizeMB(
        min=32 * MEGABYTE,
        max=MAX_INT32 * MEGABYTE,
        attribute='min_wal_size',
        variable_default=defaults.get_min_wal_size,
    )
    defaultStatisticsTarget = Int(validate=Range(min=1, max=10000), attribute='default_statistics_target')
    trackActivityQuerySize = Int(
        validate=Range(min=100, max=102400), attribute='track_activity_query_size', restart=True
    )
    defaultTransactionReadOnly = Boolean(attribute='default_transaction_read_only')
    sharedPreloadLibraries = List(
        PrefixedUpperToLowerCaseEnum(PREFIXED_SHARED_PRELOAD_LIBS, prefix='SHARED_PRELOAD_LIBRARIES_'),
        restart=True,
        attribute='user_shared_preload_libraries',
    )
    effectiveIoConcurrency = Int(validate=Range(min=0, max=1000), attribute='effective_io_concurrency')
    effectiveCacheSize = PostgresqlSizeMB(min=0, max=512 * GIGABYTE, attribute='effective_cache_size')
    autoExplainLogMinDuration = PostgresqlTimespanMs(min=-1, max=MAX_INT32, attribute='auto_explain_log_min_duration')
    autoExplainLogAnalyze = Boolean(attribute='auto_explain_log_analyze')
    autoExplainLogBuffers = Boolean(attribute='auto_explain_log_buffers')
    autoExplainLogTiming = Boolean(attribute='auto_explain_log_timing')
    autoExplainLogTriggers = Boolean(attribute='auto_explain_log_triggers')
    autoExplainLogVerbose = Boolean(attribute='auto_explain_log_verbose')
    autoExplainLogNestedStatements = Boolean(attribute='auto_explain_log_nested_statements')
    autoExplainSampleRate = Float(validate=Range(min=0, max=1), attribute='auto_explain_sample_rate')
    pgHintPlanEnableHint = Boolean(attribute='pg_hint_plan_enable_hint')
    pgHintPlanEnableHintTable = Boolean(attribute='pg_hint_plan_enable_hint_table')
    pgHintPlanMessageLevel = PrefixedUpperToLowerCaseEnum(
        PREFIXED_LOG_LEVELS, prefix='LOG_LEVEL_', attribute='pg_hint_plan_message_level'
    )
    pgHintPlanDebugPrint = PrefixedUpperToLowerCaseEnum(
        [
            'PG_HINT_PLAN_DEBUG_PRINT_OFF',
            'PG_HINT_PLAN_DEBUG_PRINT_ON',
            'PG_HINT_PLAN_DEBUG_PRINT_DETAILED',
            'PG_HINT_PLAN_DEBUG_PRINT_VERBOSE',
        ],
        prefix='PG_HINT_PLAN_DEBUG_PRINT_',
        attribute='pg_hint_plan_debug_print',
    )
    pgQualstatsEnabled = Boolean(attribute='pg_qualstats_enabled')
    pgQualstatsTrackConstants = Boolean(attribute='pg_qualstats_track_constants')
    pgQualstatsMax = Int(validate=Range(100, MAX_INT32), attribute='pg_qualstats_max')
    pgQualstatsResolveOids = Boolean(attribute='pg_qualstats_resolve_oids', restart=True)
    pgQualstatsSampleRate = Float(validate=validate_pg_qualstats_sample_rate, attribute='pg_qualstats_sample_rate')
    maxReplicationSlots = Int(
        validate=Range(min=20, max=100),
        attribute='max_replication_slots',
        restart=True,
    )
    maxWalSenders = Int(validate=Range(min=20, max=100), attribute='max_wal_senders', restart=True)
    maxLogicalReplicationWorkers = Int(
        validate=Range(min=4, max=100), attribute='max_logical_replication_workers', restart=True
    )


class PostgresqlHostConfigSchema(PostgresqlCommonConfigSchema):
    """
    PostgreSQL cluster specify config schema
    """

    recoveryMinApplyDelay = PostgresqlTimespanMs(
        min=0, max=MAX_INT32, attribute='recovery_min_apply_delay', allow_none=True, restart=True
    )


class PostgresqlHostServiceSchemaV1(Schema):
    """
    Type of host services.
    """

    type = MappedEnum(
        {
            'POSTGRESQL': ServiceType.postgres,
            'POOLER': ServiceType.pooler,
        }
    )
    health = MappedEnum(
        {
            'ALIVE': ServiceStatus.alive,
            'DEAD': ServiceStatus.dead,
            'UNKNOWN': ServiceStatus.unknown,
        }
    )


class PostgresqlHost10ConfigSchema(PostgresqlHostConfigSchema):
    """
    PostgreSQL host config schema (version 10)
    """

    replacementSortTuples = UInt(attribute='replacement_sort_tuples')


class PostgresqlHost11ConfigSchema(PostgresqlHostConfigSchema):
    """
    PostgreSQL host config schema (version 11)
    """


class PostgresqlHost12ConfigSchema(PostgresqlHostConfigSchema):
    """
    PostgreSQL host config schema (version 12)
    """


class PostgresqlHost13ConfigSchema(PostgresqlHostConfigSchema):
    """
    PostgreSQL host config schema (version 13)
    """


class PostgresqlHost14ConfigSchema(PostgresqlHostConfigSchema):
    """
    PostgreSQL host config schema (version 14)
    """


class Postgresql10to13ConfigScheme:
    operatorPrecedenceWarning = Boolean(attribute='operator_precedence_warning')


@register_config_schema(MY_CLUSTER_TYPE, '10')
@register_config_schema(MY_CLUSTER_TYPE, f'10-{EDITION_1C}', feature_flag='MDB_POSTGRESQL_10_1C')
class Postgresql10ConfigSchema(PostgresqlConfigSchema, Postgresql10to13ConfigScheme):
    """
    PostgreSQL config schema (version 10)
    """

    replacementSortTuples = UInt(attribute='replacement_sort_tuples')


class Postgresql11ConfigSchemaCommon(PostgresqlConfigSchema):
    enableParallelAppend = Boolean(attribute="enable_parallel_append")
    enableParallelHash = Boolean(attribute="enable_parallel_hash")
    enablePartitionPruning = Boolean(attribute="enable_partition_pruning")
    enablePartitionwiseAggregate = Boolean(attribute="enable_partitionwise_aggregate")
    enablePartitionwiseJoin = Boolean(attribute="enable_partitionwise_join")
    jit = Boolean()
    maxParallelMaintenanceWorkers = Int(validate=Range(min=0), attribute="max_parallel_maintenance_workers")
    parallelLeaderParticipation = Boolean(attribute="parallel_leader_participation")


class Postgresql11to13ConfigScheme:
    vacuumCleanupIndexScaleFactor = Float(
        validate=Range(min=0, max=1e10), attribute="vacuum_cleanup_index_scale_factor"
    )


@register_config_schema(MY_CLUSTER_TYPE, '11')
@register_config_schema(MY_CLUSTER_TYPE, f'11-{EDITION_1C}', feature_flag='MDB_POSTGRESQL_11_1C')
class Postgresql11ConfigSchema(
    Postgresql11ConfigSchemaCommon, Postgresql10to13ConfigScheme, Postgresql11to13ConfigScheme
):
    """
    PostgreSQL config schema (version 11)
    """


class Postgresql12ConfigSchemaCommon(Postgresql11ConfigSchemaCommon):
    logTransactionSampleRate = Float(validate=Range(min=0, max=1), attribute="log_transaction_sample_rate")
    planCacheMode = PrefixedUpperToLowerCaseEnum(
        [
            'PLAN_CACHE_MODE_AUTO',
            'PLAN_CACHE_MODE_FORCE_CUSTOM_PLAN',
            'PLAN_CACHE_MODE_FORCE_GENERIC_PLAN',
        ],
        prefix='PLAN_CACHE_MODE_',
        attribute='plan_cache_mode',
    )


@register_config_schema(MY_CLUSTER_TYPE, '12')
@register_config_schema(MY_CLUSTER_TYPE, f'12-{EDITION_1C}', feature_flag='MDB_POSTGRESQL_12_1C')
class Postgresql12ConfigSchema(
    Postgresql12ConfigSchemaCommon, Postgresql10to13ConfigScheme, Postgresql11to13ConfigScheme
):
    """
    PostgreSQL config schema (version 12)
    """


class Postgresql13ConfigSchemaCommon(Postgresql12ConfigSchemaCommon):
    hashMemMultiplier = Float(validate=Range(min=1, max=1000), attribute="hash_mem_multiplier")
    logicalDecodingWorkMem = PostgresqlSizeKB(min=64 * KILOBYTE, max=TERABYTE, attribute='logical_decoding_work_mem')
    maintenanceIoConcurrency = Int(validate=Range(min=0, max=1000), attribute='maintenance_io_concurrency')
    maxSlotWalKeepSize = PostgresqlSizeMB(min=-1, max=MAX_INT32 * MEGABYTE, attribute='max_slot_wal_keep_size')
    walKeepSize = PostgresqlSizeMB(min=0, max=MAX_INT32 * MEGABYTE, attribute='wal_keep_size')
    enableIncrementalSort = Boolean(attribute='enable_incremental_sort')
    autovacuumVacuumInsertThreshold = Int(
        validate=Range(min=-1, max=MAX_INT32), attribute='autovacuum_vacuum_insert_threshold'
    )
    autovacuumVacuumInsertScaleFactor = Float(
        validate=Range(min=0, max=100), attribute='autovacuum_vacuum_insert_scale_factor'
    )
    logMinDurationSample = PostgresqlTimespanMs(min=-1, max=MAX_INT32, attribute='log_min_duration_sample')
    logStatementSampleRate = Float(validate=Range(min=0, max=1), attribute='log_statement_sample_rate')
    logParameterMaxLength = Int(validate=Range(min=-1, max=MAX_INT32), attribute='log_parameter_max_length')
    logParameterMaxLengthOnError = Int(
        validate=Range(min=-1, max=MAX_INT32), attribute='log_parameter_max_length_on_error'
    )


@register_config_schema(MY_CLUSTER_TYPE, '13')
@register_config_schema(MY_CLUSTER_TYPE, f'13-{EDITION_1C}', feature_flag='MDB_POSTGRESQL_13_1C')
class Postgresql13ConfigSchema(
    Postgresql13ConfigSchemaCommon, Postgresql10to13ConfigScheme, Postgresql11to13ConfigScheme
):
    """
    PostgreSQL config schema (version 13)
    """


class Postgresql14ConfigSchemaCommon(Postgresql13ConfigSchemaCommon):
    clientConnectionCheckInterval = PostgresqlTimespanMs(
        min=0, max=MAX_INT32, attribute='client_connection_check_interval'
    )
    enableAsyncAppend = Boolean(attribute='enable_async_append')
    enableGathermerge = Boolean(attribute='enable_gathermerge')
    enableMemoize = Boolean(attribute='enable_memoize')
    logRecoveryConflictWaits = Boolean(attribute='log_recovery_conflict_waits')
    vacuumFailsafeAge = Int(validate=Range(min=1, max=MAX_INT32), attribute='vacuum_failsafe_age')
    vacuumMultixactFailsafeAge = Int(validate=Range(min=1, max=MAX_INT32), attribute='vacuum_multixact_failsafe_age')


@register_config_schema(MY_CLUSTER_TYPE, '14')
@register_config_schema(MY_CLUSTER_TYPE, f'14-{EDITION_1C}', feature_flag='MDB_POSTGRESQL_14_1C')
class Postgresql14ConfigSchema(Postgresql14ConfigSchemaCommon):
    """
    PostgreSQL config schema (version 14)
    """


class Postgresql10ConfigSetSchema(Schema):
    """
    PostgreSQL Schema for effective, user,
    default config representation for 10
    """

    effectiveConfig = Nested(Postgresql10ConfigSchema, attribute='effective_config', required=True)
    userConfig = Nested(Postgresql10ConfigSchema, attribute='user_config', required=True)
    defaultConfig = Nested(Postgresql10ConfigSchema, attribute='default_config', required=True)


class Postgresql11ConfigSetSchema(Schema):
    """
    PostgreSQL Schema for effective, user,
    default config representation for 11
    """

    effectiveConfig = Nested(Postgresql11ConfigSchema, attribute='effective_config', required=True)
    userConfig = Nested(Postgresql11ConfigSchema, attribute='user_config', required=True)
    defaultConfig = Nested(Postgresql11ConfigSchema, attribute='default_config', required=True)


class Postgresql12ConfigSetSchema(Schema):
    """
    PostgreSQL Schema for effective, user,
    default config representation for 12
    """

    effectiveConfig = Nested(Postgresql12ConfigSchema, attribute='effective_config', required=True)
    userConfig = Nested(Postgresql12ConfigSchema, attribute='user_config', required=True)
    defaultConfig = Nested(Postgresql12ConfigSchema, attribute='default_config', required=True)


class Postgresql13ConfigSetSchema(Schema):
    """
    PostgreSQL Schema for effective, user,
    default config representation for 13
    """

    effectiveConfig = Nested(Postgresql13ConfigSchema, attribute='effective_config', required=True)
    userConfig = Nested(Postgresql13ConfigSchema, attribute='user_config', required=True)
    defaultConfig = Nested(Postgresql13ConfigSchema, attribute='default_config', required=True)


class Postgresql14ConfigSetSchema(Schema):
    """
    PostgreSQL Schema for effective, user,
    default config representation for 14
    """

    effectiveConfig = Nested(Postgresql14ConfigSchema, attribute='effective_config', required=True)
    userConfig = Nested(Postgresql14ConfigSchema, attribute='user_config', required=True)
    defaultConfig = Nested(Postgresql14ConfigSchema, attribute='default_config', required=True)


class ConnectionPoolerConfigSchema(Schema):
    """
    Schema for connection pooler config
    """

    poolingMode = UpperToLowerCaseEnum(['SESSION', 'TRANSACTION', 'STATEMENT'], attribute='pool_mode')
    poolDiscard = IntBoolean(attribute='server_reset_query_always', allow_none=True)


@register_config_schema('pg_perf_diag', 'any')
class PerformanceDiagnosticsConfigSchema(Schema):
    """
    Schema for PerformanceDiagnostics config
    """

    enabled = Boolean(attribute='enable', allow_none=True)
    sessionsSamplingInterval = Int(
        validate=Range(min=1, max=86400),
        attribute='pgsa_sample_period',
        variable_default=defaults.get_pgsa_sample_interval,
    )
    statementsSamplingInterval = Int(
        validate=Range(min=1, max=86400),
        attribute='pgss_sample_period',
        variable_default=defaults.get_pgss_sample_interval,
    )


class PostgresqlClusterConfigSchemaV1(ClusterConfigSchemaV1):
    """
    PostgreSQL cluster config schema.
    """

    postgresqlConfig_10 = Nested(Postgresql10ConfigSetSchema, attribute='postgresql_config_10')
    postgresqlConfig_10_1c = Nested(Postgresql10ConfigSetSchema, attribute=f'postgresql_config_10-{EDITION_1C}')
    postgresqlConfig_10_1C = postgresqlConfig_10_1c
    postgresqlConfig_11 = Nested(Postgresql11ConfigSetSchema, attribute='postgresql_config_11')
    postgresqlConfig_11_1c = Nested(Postgresql11ConfigSetSchema, attribute=f'postgresql_config_11-{EDITION_1C}')
    postgresqlConfig_11_1C = postgresqlConfig_11_1c
    postgresqlConfig_12 = Nested(Postgresql12ConfigSetSchema, attribute='postgresql_config_12')
    postgresqlConfig_12_1c = Nested(Postgresql12ConfigSetSchema, attribute=f'postgresql_config_12-{EDITION_1C}')
    postgresqlConfig_12_1C = postgresqlConfig_12_1c
    postgresqlConfig_13 = Nested(Postgresql13ConfigSetSchema, attribute='postgresql_config_13')
    postgresqlConfig_13_1c = Nested(Postgresql13ConfigSetSchema, attribute=f'postgresql_config_13-{EDITION_1C}')
    postgresqlConfig_13_1C = postgresqlConfig_13_1c
    postgresqlConfig_14 = Nested(Postgresql14ConfigSetSchema, attribute='postgresql_config_14')
    postgresqlConfig_14_1c = Nested(Postgresql14ConfigSetSchema, attribute=f'postgresql_config_14-{EDITION_1C}')
    postgresqlConfig_14_1C = postgresqlConfig_14_1c
    poolerConfig = Nested(ConnectionPoolerConfigSchema, attribute='pooler_config')
    resources = Nested(ResourcesSchemaV1, required=True)
    autofailover = Boolean(attribute='autofailover', allow_none=True)
    backupWindowStart = Nested(TimeOfDay, attribute='backup_window_start')
    backupRetainPeriodDays = Int(attribute='retain_period', validate=Range(7, 60))
    access = Nested(PostgresAccessSchemaV1, attribute='access')
    soxAudit = Boolean(attribute='sox_audit', allow_none=True)
    performanceDiagnostics = Nested(PerformanceDiagnosticsConfigSchema, attribute='perf_diag')
    monitoringCloudId = Str(attribute='monitoring_cloud_id', required=False)


class PostgresqlClusterConfigSpecSchemaV1(ClusterConfigSpecSchemaV1):
    """
    PostgreSQL cluster config spec schema.
    """

    postgresqlConfig_10 = Nested(Postgresql10ConfigSchema, attribute='postgresql_config_10')
    postgresqlConfig_10_1c = Nested(Postgresql10ConfigSchema, attribute=f'postgresql_config_10-{EDITION_1C}')
    # TODO remove duplicates in go-api (MDB-14761)
    postgresqlConfig_10_1C = postgresqlConfig_10_1c
    postgresqlConfig_11 = Nested(Postgresql11ConfigSchema, attribute='postgresql_config_11')
    postgresqlConfig_11_1c = Nested(Postgresql11ConfigSchema, attribute=f'postgresql_config_11-{EDITION_1C}')
    postgresqlConfig_11_1C = postgresqlConfig_11_1c
    postgresqlConfig_12 = Nested(Postgresql12ConfigSchema, attribute='postgresql_config_12')
    postgresqlConfig_12_1c = Nested(Postgresql12ConfigSchema, attribute=f'postgresql_config_12-{EDITION_1C}')
    postgresqlConfig_12_1C = postgresqlConfig_12_1c
    postgresqlConfig_13 = Nested(Postgresql13ConfigSchema, attribute='postgresql_config_13')
    postgresqlConfig_13_1c = Nested(Postgresql13ConfigSchema, attribute=f'postgresql_config_13-{EDITION_1C}')
    postgresqlConfig_13_1C = postgresqlConfig_13_1c
    postgresqlConfig_14 = Nested(Postgresql14ConfigSchema, attribute='postgresql_config_14')
    postgresqlConfig_14_1c = Nested(Postgresql14ConfigSchema, attribute=f'postgresql_config_14-{EDITION_1C}')
    postgresqlConfig_14_1C = postgresqlConfig_14_1c
    poolerConfig = Nested(ConnectionPoolerConfigSchema, attribute='pooler_config')
    resources = Nested(ResourcesSchemaV1, required=True)
    autofailover = Boolean(attribute='autofailover', allow_none=True)
    soxAudit = Boolean(attribute='sox_audit', allow_none=True)
    backupWindowStart = Nested(TimeOfDay, attribute='backup_window_start')
    backupRetainPeriodDays = Int(attribute='retain_period', validate=Range(7, 60))
    access = Nested(PostgresAccessSchemaV1, attribute='access')
    performanceDiagnostics = Nested(PerformanceDiagnosticsConfigSchema, attribute='perf_diag')
    monitoringCloudId = Str(attribute='monitoring_cloud_id', required=False)


class PostgresqlClusterConfigUpdateSpecSchemaV1(ClusterConfigSpecSchemaV1):
    """
    PostgreSQL cluster config schema.
    """

    postgresqlConfig_10 = Nested(Postgresql10ConfigSchema, attribute='postgresql_config_10')
    postgresqlConfig_10_1c = Nested(Postgresql10ConfigSchema, attribute=f'postgresql_config_10-{EDITION_1C}')
    postgresqlConfig_10_1C = postgresqlConfig_10_1c
    postgresqlConfig_11 = Nested(Postgresql11ConfigSchema, attribute='postgresql_config_11')
    postgresqlConfig_11_1c = Nested(Postgresql11ConfigSchema, attribute=f'postgresql_config_11-{EDITION_1C}')
    postgresqlConfig_11_1C = postgresqlConfig_11_1c
    postgresqlConfig_12 = Nested(Postgresql12ConfigSchema, attribute='postgresql_config_12')
    postgresqlConfig_12_1c = Nested(Postgresql12ConfigSchema, attribute=f'postgresql_config_12-{EDITION_1C}')
    postgresqlConfig_12_1C = postgresqlConfig_12_1c
    postgresqlConfig_13 = Nested(Postgresql13ConfigSchema, attribute='postgresql_config_13')
    postgresqlConfig_13_1c = Nested(Postgresql13ConfigSchema, attribute=f'postgresql_config_13-{EDITION_1C}')
    postgresqlConfig_13_1C = postgresqlConfig_13_1c
    postgresqlConfig_14 = Nested(Postgresql14ConfigSchema, attribute='postgresql_config_14')
    postgresqlConfig_14_1c = Nested(Postgresql14ConfigSchema, attribute=f'postgresql_config_14-{EDITION_1C}')
    postgresqlConfig_14_1C = postgresqlConfig_14_1c
    poolerConfig = Nested(ConnectionPoolerConfigSchema, attribute='pooler_config')
    resources = Nested(ResourcesUpdateSchemaV1)
    autofailover = Boolean(attribute='autofailover', allow_none=True)
    soxAudit = Boolean(attribute='sox_audit', allow_none=True)
    backupWindowStart = Nested(TimeOfDay, attribute='backup_window_start')
    backupRetainPeriodDays = Int(attribute='retain_period', validate=Range(7, 60))
    access = Nested(PostgresAccessSchemaV1, attribute='access')
    performanceDiagnostics = Nested(PerformanceDiagnosticsConfigSchema, attribute='perf_diag')
    monitoringCloudId = Str(attribute='monitoring_cloud_id', required=False)


class PostgresqlHostConfigSpecSchemaV1(Schema):
    """
    PostgreSQL host config spec schema.
    """

    postgresqlConfig_10 = Nested(PostgresqlHost10ConfigSchema, attribute='postgresql_config_10')
    postgresqlConfig_10_1c = Nested(PostgresqlHost10ConfigSchema, attribute=f'postgresql_config_10-{EDITION_1C}')
    postgresqlConfig_10_1C = postgresqlConfig_10_1c
    postgresqlConfig_11 = Nested(PostgresqlHost11ConfigSchema, attribute='postgresql_config_11')
    postgresqlConfig_11_1c = Nested(PostgresqlHost11ConfigSchema, attribute=f'postgresql_config_11-{EDITION_1C}')
    postgresqlConfig_11_1C = postgresqlConfig_11_1c
    postgresqlConfig_12 = Nested(PostgresqlHost12ConfigSchema, attribute='postgresql_config_12')
    postgresqlConfig_12_1c = Nested(PostgresqlHost12ConfigSchema, attribute=f'postgresql_config_12-{EDITION_1C}')
    postgresqlConfig_12_1C = postgresqlConfig_12_1c
    postgresqlConfig_13 = Nested(PostgresqlHost13ConfigSchema, attribute='postgresql_config_13')
    postgresqlConfig_13_1c = Nested(PostgresqlHost13ConfigSchema, attribute=f'postgresql_config_13-{EDITION_1C}')
    postgresqlConfig_13_1C = postgresqlConfig_13_1c
    postgresqlConfig_14 = Nested(PostgresqlHost14ConfigSchema, attribute='postgresql_config_14')
    postgresqlConfig_14_1c = Nested(PostgresqlHost14ConfigSchema, attribute=f'postgresql_config_14-{EDITION_1C}')
    postgresqlConfig_14_1C = postgresqlConfig_14_1c


class PostgresqlHostConfigUpdateSpecSchemaV1(Schema):
    """
    PostgreSQL host config schema.
    """

    postgresqlConfig_10 = Nested(PostgresqlHost10ConfigSchema, attribute='postgresql_config_10')
    postgresqlConfig_10_1c = Nested(PostgresqlHost10ConfigSchema, attribute=f'postgresql_config_10-{EDITION_1C}')
    postgresqlConfig_10_1C = postgresqlConfig_10_1c
    postgresqlConfig_11 = Nested(PostgresqlHost11ConfigSchema, attribute='postgresql_config_11')
    postgresqlConfig_11_1c = Nested(PostgresqlHost11ConfigSchema, attribute=f'postgresql_config_11-{EDITION_1C}')
    postgresqlConfig_11_1C = postgresqlConfig_11_1c
    postgresqlConfig_12 = Nested(PostgresqlHost12ConfigSchema, attribute='postgresql_config_12')
    postgresqlConfig_12_1c = Nested(PostgresqlHost12ConfigSchema, attribute=f'postgresql_config_12-{EDITION_1C}')
    postgresqlConfig_12_1C = postgresqlConfig_12_1c
    postgresqlConfig_13 = Nested(PostgresqlHost13ConfigSchema, attribute='postgresql_config_13')
    postgresqlConfig_13_1c = Nested(PostgresqlHost13ConfigSchema, attribute=f'postgresql_config_13-{EDITION_1C}')
    postgresqlConfig_13_1C = postgresqlConfig_13_1c
    postgresqlConfig_14 = Nested(PostgresqlHost14ConfigSchema, attribute='postgresql_config_14')
    postgresqlConfig_14_1c = Nested(PostgresqlHost14ConfigSchema, attribute=f'postgresql_config_14-{EDITION_1C}')
    postgresqlConfig_14_1C = postgresqlConfig_14_1c


@register_response_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.INFO)
class PostgresqlClusterSchemaV1(ManagedClusterSchemaV1):
    """
    PostgreSQL cluster schema.
    """

    config = Nested(PostgresqlClusterConfigSchemaV1, required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.LIST)
class PostgresqlListClustersResonseSchemaV1(ListResponseSchemaV1):
    """
    PostgreSQL cluster list schema.
    """

    clusters = Nested(PostgresqlClusterSchemaV1, many=True, required=True)


class PostgresqlExtensionSchemaV1(Schema):
    """
    PostgreSQL extension schema
    """

    name = Str(required=True)
    version = Str()


class PostgresqlDatabaseSpecSchemaV1(Schema):
    """
    PostgreSQL database spec schema.
    """

    name = Str(validate=PostgresqlClusterTraits.db_name.validate, required=True)
    owner = Str(required=True)
    lcCtype = Str(missing='C', validate=COLLATE_VALIDATOR, attribute='lc_ctype')
    lcCollate = Str(missing='C', validate=COLLATE_VALIDATOR, attribute='lc_collate')
    templateDb = Str(attribute='template')
    extensions = Nested(PostgresqlExtensionSchemaV1, many=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.INFO)
class PostgresqlDatabaseSchemaV1(DatabaseSchemaV1, PostgresqlDatabaseSpecSchemaV1):
    """
    PostgreSQL database schema.
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.LIST)
class PostgresqlListDatabasesResponseSchemaV1(ListResponseSchemaV1):
    """
    PostgreSQL database list schema.
    """

    databases = Nested(PostgresqlDatabaseSchemaV1, many=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.CREATE)
class PostgresqlCreateDatabaseRequestSchemaV1(Schema):
    """
    Schema for create PostgreSQL database request.
    """

    databaseSpec = Nested(PostgresqlDatabaseSpecSchemaV1, attribute='database_spec', required=True)


class PostgresqlPermissionSchemaV1(Schema):
    """
    Permission schema for PostgreSQL user.
    """

    databaseName = Str(attribute='database_name', validate=PostgresqlClusterTraits.db_name.validate, required=True)


class PostgresqlUserSettingsSchemaV1(Schema):
    """
    Settings schema for PostgreSQL user.
    """

    defaultTransactionIsolation = PrefixedUpperToLowerCaseAndSpacesEnum(
        [
            'TRANSACTION_ISOLATION_READ_UNCOMMITTED',
            'TRANSACTION_ISOLATION_READ_COMMITTED',
            'TRANSACTION_ISOLATION_REPEATABLE_READ',
            'TRANSACTION_ISOLATION_SERIALIZABLE',
        ],
        prefix='TRANSACTION_ISOLATION_',
        attribute='default_transaction_isolation',
    )
    lockTimeout = PostgresqlTimespanMs(min=0, max=MAX_INT32, attribute='lock_timeout')
    logMinDurationStatement = PostgresqlTimespanMs(min=-1, max=MAX_INT32, attribute='log_min_duration_statement')
    logStatement = PrefixedUpperToLowerCaseEnum(
        [
            'LOG_STATEMENT_NONE',
            'LOG_STATEMENT_DDL',
            'LOG_STATEMENT_MOD',
            'LOG_STATEMENT_ALL',
        ],
        prefix='LOG_STATEMENT_',
        attribute='log_statement',
    )
    synchronousCommit = PrefixedUpperToLowerCaseEnum(
        [
            'SYNCHRONOUS_COMMIT_ON',
            'SYNCHRONOUS_COMMIT_OFF',
            'SYNCHRONOUS_COMMIT_LOCAL',
            'SYNCHRONOUS_COMMIT_REMOTE_WRITE',
            'SYNCHRONOUS_COMMIT_REMOTE_APPLY',
        ],
        prefix='SYNCHRONOUS_COMMIT_',
        attribute='synchronous_commit',
    )
    tempFileLimit = PostgresqlSizeKB(min=-1, max=MAX_INT32 * BLOCK_SIZE, attribute='temp_file_limit')
    poolingMode = UpperToLowerCaseEnum(['SESSION', 'TRANSACTION', 'STATEMENT'], attribute='pool_mode')
    preparedStatementsPooling = Boolean(attribute='usr_pool_reserve_prepaped_statements')
    catchupTimeout = Int(attribute='catchup_timeout', validate=Range(min=0, max=SEC_PER_DAY))


@register_response_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.INFO)
class PostgresqlUserSchemaV1(UserSchemaV1):
    """
    PostgreSQL user schema.
    """

    name = Str(validate=PostgresqlClusterTraits.user_name.validate, required=True)
    permissions = Nested(PostgresqlPermissionSchemaV1, many=True)
    connLimit = Int(attribute='conn_limit', validate=Range(min=0))
    settings = Nested(PostgresqlUserSettingsSchemaV1)
    login = Boolean()
    grants = List(Str(validate=PostgresqlClusterTraits.role_name.validate), many=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.LIST)
class PostgresqlListUsersResponseSchemaV1(ListResponseSchemaV1):
    """
    PostgreSQL user list schema.
    """

    users = Nested(PostgresqlUserSchemaV1, many=True)


class PostgresqlUserSpecSchemaV1(Schema):
    """
    PostgreSQL create user schema.
    """

    name = Str(validate=PostgresqlClusterTraits.user_name.validate, required=True)
    password = Str(validate=PostgresqlClusterTraits.password.validate, required=True)
    permissions = Nested(PostgresqlPermissionSchemaV1, many=True)
    connLimit = Int(attribute='conn_limit', validate=Range(min=0))
    settings = Nested(PostgresqlUserSettingsSchemaV1)
    login = Boolean()
    grants = List(Str(validate=PostgresqlClusterTraits.role_name.validate), many=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.CREATE)
class PostgresqlCreateUserRequestSchemaV1(Schema):
    """
    Schema for create PostgreSQL user request.
    """

    userSpec = Nested(PostgresqlUserSpecSchemaV1, attribute='user_spec', required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.MODIFY)
class PostgresqlUpdateUserRequestSchemaV1(Schema):
    """
    Schema for update Postgresql user request.
    """

    password = Str(validate=PostgresqlClusterTraits.password.validate)
    permissions = Nested(PostgresqlPermissionSchemaV1, many=True)
    connLimit = Int(attribute='conn_limit', validate=Range(min=0))
    settings = Nested(PostgresqlUserSettingsSchemaV1)
    login = Boolean()
    grants = List(Str(validate=PostgresqlClusterTraits.role_name.validate), many=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.GRANT_PERMISSION)
class PostgresqlAddUserPermissionRequestSchemaV1(Schema):
    """
    Schema for add Postgresql user permission request.
    """

    permission = Nested(PostgresqlPermissionSchemaV1, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.REVOKE_PERMISSION)
class PostgresqlRevokeUserPermissionRequestSchemaV1(Schema):
    """
    Schema for revoke Postgresql user permission request.
    """

    databaseName = Str(attribute='database_name', validate=PostgresqlClusterTraits.db_name.validate, required=True)


class PostgresqlHostSchemaV1(HostSchemaV1):
    """
    PostgreSQL host schema.
    """

    role = MappedEnum(
        {
            'UNKNOWN': HostRole.unknown,
            'MASTER': HostRole.master,
            'REPLICA': HostRole.replica,
        }
    )
    replicaType = MappedEnum(
        {
            'UNKNOWN': HostReplicaType.unknown,
            'ASYNC': HostReplicaType.a_sync,
            'SYNC': HostReplicaType.sync,
        },
        attribute='replica_type',
    )
    services = List(Nested(PostgresqlHostServiceSchemaV1))
    replicationSource = ReplicationSource()
    priority = Int()
    configSpec = Nested(PostgresqlHostConfigSpecSchemaV1, attribute='config_spec')


class PostgresqlHostSpecSchemaV1(HostSpecSchemaV1):
    """
    PostgreSQL host spec schema.
    """

    replicationSource = ReplicationSource()
    priority = Int(validate=Range(0, 100))
    configSpec = Nested(PostgresqlHostConfigUpdateSpecSchemaV1, attribute='config_spec')


@register_response_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.LIST)
class PostgresqlListHostsResponseSchemaV1(ListResponseSchemaV1):
    """
    PostgreSQL host list schema.
    """

    hosts = Nested(PostgresqlHostSchemaV1, many=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_CREATE)
class PostgresqlAddHostsRequestSchemaV1(Schema):
    """
    Schema for add PostgreSQL hosts request.
    """

    hostSpecs = Nested(PostgresqlHostSpecSchemaV1, many=True, attribute='host_specs', required=True)


class PostgresqlHostUpdateSpecSchemaV1(Schema):
    """
    Schema for update Postgresql hosts.
    """

    hostName = Str(required=True, attribute='host_name')
    replicationSource = ReplicationSource()
    priority = Int(validate=Range(0, 100))
    configSpec = Nested(PostgresqlHostConfigUpdateSpecSchemaV1, attribute='config_spec')
    assignPublicIp = Boolean(attribute='assign_public_ip')


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_MODIFY)
class PostgresqlUpdateHostRequestSchemaV1(Schema):
    """
    Schema for update Postgresql host request.
    """

    updateHostSpecs = Nested(PostgresqlHostUpdateSpecSchemaV1, many=True, attribute='update_host_specs', required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_DELETE)
class PostgresqlDeleteHostsRequestSchemaV1(Schema):
    """
    Schema for delete PostgreSQL hosts request.
    """

    hostNames = List(Str(), attribute='host_names', required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CREATE)
@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.BILLING_CREATE)
class PostgresqlCreateClusterRequestSchemaV1(CreateClusterRequestSchemaV1):
    """
    Schema for create PostgreSQL cluster request.
    """

    name = Str(validate=PostgresqlClusterTraits.cluster_name.validate, required=True)
    configSpec = Nested(PostgresqlClusterConfigSpecSchemaV1, attribute='config_spec', required=True)
    databaseSpecs = List(Nested(PostgresqlDatabaseSpecSchemaV1), attribute='database_specs', required=True)
    userSpecs = List(Nested(PostgresqlUserSpecSchemaV1), attribute='user_specs', required=True)
    hostSpecs = List(Nested(PostgresqlHostSpecSchemaV1), attribute='host_specs', required=True)
    hostGroupIds = List(Str(), attribute='host_group_ids')


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MODIFY)
class PostgresqlUpdateClusterRequestSchemaV1(UpdateClusterRequestSchemaV1):
    """
    Schema for update Postgresql cluster request.
    """

    name = Str(validate=PostgresqlClusterTraits.cluster_name.validate)
    configSpec = Nested(PostgresqlClusterConfigUpdateSpecSchemaV1, attribute='config_spec')


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.RESTORE)
class PostgresqlRestoreClusterRequestSchemaV1(RestoreClusterRequestSchemaV1):
    """
    Schema for restore Postgresql cluster request.
    """

    time = validation.GrpcTimestamp(required=True)
    timeInclusive = Boolean(missing=False, attribute='time_inclusive')
    configSpec = Nested(PostgresqlClusterConfigSpecSchemaV1, attribute='config_spec', required=True)
    # don't use here PostgresqlHostSpecSchemaV1,
    # cause don't want to deal with replicationSource
    # in restore logic
    hostSpecs = Nested(HostSpecSchemaV1, attribute='host_specs', many=True, required=True)
    hostGroupIds = List(Str(), attribute='host_group_ids')


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.RESTORE_HINTS)
class PostgresqlRestoreHintsResponseSchemaV1(RestoreHintWithTimeResponseSchemaV1):
    """
    Schema for Postgresql restore hint
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.INFO)
class PostgresqlBackupSchemaV1(BaseBackupSchemaV1):
    """
    Schema for Postgresql backup
    """

    size = Int(attribute='size', required=False)
    type = MappedEnum(
        {
            'BACKUP_TYPE_UNSPECIFIED': BackupInitiator.unspecified,
            'AUTOMATED': BackupInitiator.automated,
            'MANUAL': BackupInitiator.manual,
        },
        required=False,
        attribute='btype',
    )
    method = MappedEnum(
        {
            'BACKUP_TYPE_UNSPECIFIED': BackupMethod.unspecified,
            'BASE': BackupMethod.base,
            'INCREMENTAL': BackupMethod.increment,
        },
        required=False,
        attribute='method',
    )


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.LIST)
class PostgresqlListClusterBackupsReponseSchemaV1(ListResponseSchemaV1):
    """
    Schema for Postgresql backups listing
    """

    backups = Nested(PostgresqlBackupSchemaV1, many=True, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.MODIFY)
class UpdateDatabaseRequestSchemaV1(Schema):
    """
    Describes database modification options
    """

    extensions = Nested(PostgresqlExtensionSchemaV1, many=True, required=False)
    newDatabaseName = Str(
        attribute='new_database_name', validate=PostgresqlClusterTraits.db_name.validate, required=False
    )


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.START_FAILOVER)
class PostgresqlStartClusterFailoverRequestSchemaV1(StartClusterFailoverRequestSchemaV1):
    """
    Schema for failover PostgreSQL cluster request.
    """

    hostName = Str(attribute='host_name', validate=validation.hostname_validator, required=False)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.STOP)
class PostgresqlStopClusterRequestSchemaV1(Schema):
    """
    Schema for stop cluster request.
    """

    pass


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE)
class PostgresqlDeleteClusterRequestSchemaV1(Schema):
    """
    Schema for stop cluster request.
    """


@register_request_schema(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.CREATE)
class PostgresqlCreateAlertGroupRequestSchemaV1(CreateAlertsGroupRequestSchemaV1):
    """
    Schema for create alert group request.
    """

    pass


@register_response_schema(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.ALERTS_TEMPLATE)
class PostgresqlAlertTemplateSchemaV1(AlertTemplateListSchemaV1):
    """
    Schema for Postgresql alerts template
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.INFO)
class PostgresqlAlertGroupSchemaV1(AlertGroupSchemaV1):
    """
    Schema for Postgresql alert group
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.LIST)
class PostgresqlListAlertGroupResponseSchemaV1(ListResponseSchemaV1):
    """
    Schema for List alert group request.
    """

    alertGroups = Nested(PostgresqlAlertGroupSchemaV1, many=True, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.MODIFY)
class PostgresqlAlertGroupSchemaUpdateV1(UpdateAlertGroupRequestSchemaV1):
    """
    Schema for Postgresql alert group update
    """


@register_request_schema(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.DELETE)
class PostgresqlAlertGroupSchemaDeleteV1(Schema):
    """
    Schema for Postgresql alert group delete
    """
