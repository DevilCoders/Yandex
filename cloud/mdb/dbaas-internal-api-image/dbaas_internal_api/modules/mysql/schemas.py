"""
DBaaS Internal API MySQL cluster options schema

# TODO:
    support multiple versions
    validate using something like jsonschema
"""
from marshmallow.fields import Bool, Boolean, Float, Int, List, Nested
from marshmallow.validate import Range
from pytz import all_timezones_set

from ...apis.schemas.backups import BaseBackupSchemaV1
from ...apis.schemas.cluster import (
    ClusterConfigSchemaV1,
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
    GrpcStr,
    MappedEnum,
    Str,
    UnderscoreToSpaceCaseEnum,
    UnderscoreToHyphenCaseEnum,
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
)
from ...health.health import ServiceStatus
from ...utils import validation
from ...utils.types import BackupInitiator
from ...utils.register import (
    DbaasOperation,
    Resource,
    register_config_schema,
    register_request_schema,
    register_response_schema,
)
from ...utils.validation import OneOf, Schema, TimeOfDay, validate_size_in_kb
from . import schema_defaults as defaults
from .constants import (
    DEFAULT_AUTH_PLUGIN_5_7,
    DEFAULT_AUTH_PLUGIN_8_0,
    GLOBAL_VALID_ROLES_API_ENUM,
    LOG_SLOW_FILTER_ENUM,
    LOG_SLOW_RATE_TYPE_ENUM,
    MY_CLUSTER_TYPE,
    SQL_MODES_5_7,
    SQL_MODES_8_0,
    VALID_ROLES_API_ENUM,
    CHARSET_5_7,
    COLLATION_5_7,
    DEFAULT_CHARSET_5_7,
    DEFAULT_COLLATION_5_7,
    CHARSET_8_0,
    COLLATION_8_0,
    DEFAULT_CHARSET_8_0,
    DEFAULT_COLLATION_8_0,
    TRANSACTION_ISOLATION_ENUM,
    UPPER_AUTH_PLUGINS_5_7_ENUM,
    UPPER_AUTH_PLUGINS_8_0_ENUM,
    BINLOG_ROW_IMAGE_ENUM,
    SLAVE_PARALLEL_TYPE_ENUM,
    BINLOG_TRANSACTION_DEPENDENCY_TRACKING_ENUM,
    INNODB_PAGE_SIZES,
)

from .traits import HostReplicaType, HostRole, MySQLClusterTraits, ServiceType


class ReplicationSource(GrpcStr):
    """
    Type of replicationSource fields.
    """

    def __init__(self, **kwargs) -> None:
        super().__init__(attribute='replication_source', **kwargs)


@register_response_schema(MY_CLUSTER_TYPE, Resource.RESOURCE_PRESET, DbaasOperation.INFO)
class MysqlResourcePresetSchemaV1(ResourcePresetSchemaV1):
    """
    MySQL resource preset schema.
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.RESOURCE_PRESET, DbaasOperation.LIST)
class MysqlListResourcePresetsSchemaV1(ListResponseSchemaV1):
    """
    MySQL resource preset list schema.
    """

    resourcePresets = Nested(MysqlResourcePresetSchemaV1, many=True, attribute='resource_presets', required=True)


class MysqlPermissionSchemaV1(Schema):
    """
    Permission schema.
    """

    databaseName = Str(attribute='database_name', validate=MySQLClusterTraits.db_name.validate, required=True)
    roles = List(UnderscoreToSpaceCaseEnum(VALID_ROLES_API_ENUM), required=True)


class MysqlConnectionLimitsSchemaV1(Schema):
    """
    User limits schema.
    """

    maxQuestionsPerHour = Int(validate=Range(min=0, max=2**32 - 1), attribute='MAX_QUERIES_PER_HOUR')
    maxUpdatesPerHour = Int(validate=Range(min=0, max=2**32 - 1), attribute='MAX_UPDATES_PER_HOUR')
    maxConnectionsPerHour = Int(validate=Range(min=0, max=2**32 - 1), attribute='MAX_CONNECTIONS_PER_HOUR')
    maxUserConnections = Int(validate=Range(min=0, max=2**32 - 1), attribute='MAX_USER_CONNECTIONS')


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.INFO)
@register_response_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.INFO)
class MysqlUserSchemaV1(UserSchemaV1):
    """
    MySQL user schema.
    """

    name = Str(validate=MySQLClusterTraits.user_name.validate, required=True)
    permissions = Nested(MysqlPermissionSchemaV1, many=True, required=True)
    globalPermissions = List(UnderscoreToSpaceCaseEnum(GLOBAL_VALID_ROLES_API_ENUM), attribute='global_permissions')
    connectionLimits = Nested(MysqlConnectionLimitsSchemaV1, attribute='connection_limits')
    authenticationPlugin = UpperToLowerCaseEnum(UPPER_AUTH_PLUGINS_8_0_ENUM, attribute='plugin')


class MysqlUserSpecSchemaV1(Schema):
    """
    MySQL user spec schema.
    """

    name = Str(validate=MySQLClusterTraits.user_name.validate, required=True)
    password = Str(validate=MySQLClusterTraits.password.validate, required=True)
    permissions = Nested(MysqlPermissionSchemaV1, many=True)
    globalPermissions = List(UnderscoreToSpaceCaseEnum(GLOBAL_VALID_ROLES_API_ENUM), attribute='global_permissions')
    connectionLimits = Nested(MysqlConnectionLimitsSchemaV1, attribute='connection_limits')
    authenticationPlugin = UpperToLowerCaseEnum(UPPER_AUTH_PLUGINS_8_0_ENUM, attribute='plugin')


@register_response_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.LIST)
class MysqlListUsersResponseSchemaV1(ListResponseSchemaV1):
    """
    MySQL user list schema.
    """

    users = Nested(MysqlUserSchemaV1, many=True, required=True)


class MysqlUserMetadata(UserSchemaV1):
    """
    MySQL user meta for Operation.metadata field
    """

    name = Str(validate=MySQLClusterTraits.user_name.validate, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.CREATE)
class MysqlCreateUserRequestSchemaV1(Schema):
    """
    Schema for create MySQL user request.
    """

    userSpec = Nested(MysqlUserSpecSchemaV1, attribute='user_spec', required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.MODIFY)
class MysqlUpdateUserRequestSchemaV1(Schema):
    """
    Schema for update MySQL user request.
    """

    password = Str(validate=MySQLClusterTraits.password.validate)
    permissions = Nested(MysqlPermissionSchemaV1, many=True)
    globalPermissions = List(UnderscoreToSpaceCaseEnum(GLOBAL_VALID_ROLES_API_ENUM), attribute='global_permissions')
    connectionLimits = Nested(MysqlConnectionLimitsSchemaV1, attribute='connection_limits')
    authenticationPlugin = UpperToLowerCaseEnum(UPPER_AUTH_PLUGINS_8_0_ENUM, attribute='plugin')


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.GRANT_PERMISSION)
class MysqlAddUserPermissionRequestSchemaV1(Schema):
    """
    Schema for add MySQL user permission request.
    """

    permission = Nested(MysqlPermissionSchemaV1, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.REVOKE_PERMISSION)
class MysqlRevokeUserPermissionRequestSchemaV1(Schema):
    """
    Schema for revoke MySQL user permission request.
    """

    permission = Nested(MysqlPermissionSchemaV1, required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.INFO)
class MysqlDatabaseSchemaV1(DatabaseSchemaV1):
    """
    MySQL database schema.
    """

    name = Str(validate=MySQLClusterTraits.db_name.validate, required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.LIST)
class MysqlListDatabasesResponseSchemaV1(ListResponseSchemaV1):
    """
    MySQL database list schema.
    """

    databases = Nested(MysqlDatabaseSchemaV1, many=True, required=True)


class MysqlDatabaseSpecSchemaV1(Schema):
    """
    MySQL database spec schema.
    """

    name = Str(validate=MySQLClusterTraits.db_name.validate, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.DATABASE, DbaasOperation.CREATE)
class MysqlCreateDatabaseRequestSchemaV1(Schema):
    """
    Schema for create MySQL database request.
    """

    databaseSpec = Nested(MysqlDatabaseSpecSchemaV1, attribute='database_spec', required=True)


class MysqlHostServiceSchemaV1(Schema):
    """
    Type of host services.
    """

    type = MappedEnum(
        {
            'MYSQL': ServiceType.mysql,
        }
    )
    health = MappedEnum(
        {
            'ALIVE': ServiceStatus.alive,
            'DEAD': ServiceStatus.dead,
            'UNKNOWN': ServiceStatus.unknown,
        }
    )


class MysqlHostSchemaV1(HostSchemaV1):
    """
    MySQL info host schema
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
            'QUORUM': HostReplicaType.quorum,
            'ASYNC': HostReplicaType.a_sync,
        },
        attribute='replica_type',
    )
    replicaUpstream = Str(attribute='replica_upstream')
    replicaLag = Int(attribute='replica_lag')
    services = List(Nested(MysqlHostServiceSchemaV1))
    replicationSource = ReplicationSource()
    backupPriority = Int(validate=Range(min=0, max=100), attribute='backup_priority')
    priority = Int(validate=Range(min=0, max=100), attribute='priority')


class MysqlHostSpecSchemaV1(HostSpecSchemaV1):
    """
    MySQL create/info host schema
    """

    replicationSource = ReplicationSource()
    backupPriority = Int(validate=Range(min=0, max=100), attribute='backup_priority')
    priority = Int(validate=Range(min=0, max=100), attribute='priority')


@register_response_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.LIST)
class MysqlListHostsResponseSchemaV1(ListResponseSchemaV1):
    """
    MySQL host list schema.
    """

    hosts = Nested(MysqlHostSchemaV1, many=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_CREATE)
class MysqlAddHostsRequestSchemaV1(Schema):
    """
    Schema for add MySQL hosts request.
    """

    hostSpecs = Nested(MysqlHostSpecSchemaV1, many=True, attribute='host_specs', required=True)


class MysqlHostUpdateSpecSchemaV1(Schema):
    """
    Schema for update Mysql hosts.
    """

    hostName = Str(required=True, attribute='host_name')
    replicationSource = ReplicationSource()
    assignPublicIp = Boolean(attribute='assign_public_ip')
    backupPriority = Int(validate=Range(min=0, max=100), attribute='backup_priority')
    priority = Int(validate=Range(min=0, max=100), attribute='priority')


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_MODIFY)
class MysqlUpdateHostRequestSchemaV1(Schema):
    """
    Schema for update Mysql host request.
    """

    updateHostSpecs = Nested(MysqlHostUpdateSpecSchemaV1, many=True, attribute='update_host_specs', required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.HOST, DbaasOperation.BATCH_DELETE)
class MysqlDeleteHostsRequestsSchemaV1(Schema):
    """
    Schema for delete MySQL hosts request.
    """

    hostNames = List(Str(), attribute='host_names', required=True)


class MysqlResourcesUpdateSchemaV1(ResourcesUpdateSchemaV1):
    """Mysql-specific resources schema used in update"""

    diskSize = Int(attribute='disk_size')  # type: ignore


class MysqlResourcesSchemaV1(ResourcesSchemaV1):
    """Mysql-specific resources schema used in create"""

    diskSize = Int(attribute='disk_size', required=True)  # type: ignore


class MysqlConfigSchema(Schema):
    """
    Common mysql config schema
    """

    innodbBufferPoolSize = Int(
        validate=Range(min=134217728),
        attribute='innodb_buffer_pool_size',
        variable_default=defaults.get_innodb_buffer_pool_size,
    )
    innodbFlushLogAtTrxCommit = Int(
        validate=OneOf([1, 2]),
        attribute='innodb_flush_log_at_trx_commit',
    )
    innodbLockWaitTimeout = Int(
        validate=Range(min=1, max=28800),
        attribute='innodb_lock_wait_timeout',
    )
    innodbPrintAllDeadlocks = Bool(attribute='innodb_print_all_deadlocks')
    innodbAdaptiveHashIndex = Bool(attribute='innodb_adaptive_hash_index')
    innodbNumaInterleave = Bool(attribute='innodb_numa_interleave', restart=True)

    innodbLogBufferSize = Int(validate=Range(min=1048576, max=268435456), attribute='innodb_log_buffer_size')
    innodbLogFileSize = Int(
        validate=Range(min=268435456, max=4294967296), attribute='innodb_log_file_size', restart=True
    )
    innodbIoCapacity = Int(validate=Range(min=100, max=100000), attribute='innodb_io_capacity')
    innodbIoCapacityMax = Int(validate=Range(min=100, max=100000), attribute='innodb_io_capacity_max')
    innodbReadIoThreads = Int(validate=Range(min=1, max=16), attribute='innodb_read_io_threads', restart=True)
    innodbWriteIoThreads = Int(validate=Range(min=1, max=16), attribute='innodb_write_io_threads', restart=True)
    innodbPurgeThreads = Int(validate=Range(min=1, max=16), attribute='innodb_purge_threads', restart=True)
    innodbThreadConcurrency = Int(validate=Range(min=0, max=1000), attribute='innodb_thread_concurrency')
    innodbTempDataFileMaxSize = Int(
        validate=Range(min=1073741824, max=107374182400), attribute='innodb_temp_data_file_max_size', restart=True
    )

    maxConnections = Int(
        validate=Range(min=10, max=100000),
        attribute='max_connections',
        variable_default=defaults.get_max_connections,
    )
    threadCacheSize = Int(
        validate=Range(min=10, max=10000),
        attribute='thread_cache_size',
        variable_default=defaults.get_thread_cache_size,
    )
    threadStack = Int(
        validate=[Range(min=131072, max=16777216), validate_size_in_kb], attribute='thread_stack', restart=True
    )
    maxAllowedPacket = Int(
        validate=[Range(min=1048576, max=1073741824), validate_size_in_kb],
        attribute='max_allowed_packet',
    )
    netReadTimeout = Int(
        validate=Range(min=1, max=1200),
        attribute='net_read_timeout',
    )
    netWriteTimeout = Int(
        validate=Range(min=1, max=1200),
        attribute='net_write_timeout',
    )

    transactionIsolation = UnderscoreToHyphenCaseEnum(
        TRANSACTION_ISOLATION_ENUM,
        attribute='transaction_isolation',
    )

    logErrorVerbosity = Int(validate=Range(min=2, max=3), attribute='log_error_verbosity')

    slowQueryLog = Bool(attribute='slow_query_log')

    logSlowSpStatements = Bool(attribute='log_slow_sp_statements')

    slowQueryLogAlwaysWriteTime = Float(
        validate=Range(min=0.0, max=3600.0),
        attribute='slow_query_log_always_write_time',
    )

    longQueryTime = Float(
        validate=Range(min=0.0, max=3600.0),
        attribute='long_query_time',
        variable_default=lambda *_, **__: 10,
    )

    logSlowRateLimit = Int(
        validate=Range(min=1, max=1000),
        attribute='log_slow_rate_limit',
    )

    logSlowRateType = UpperToLowerCaseEnum(LOG_SLOW_RATE_TYPE_ENUM, attribute='log_slow_rate_type')

    logSlowFilter = List(
        UpperToLowerCaseEnum(LOG_SLOW_FILTER_ENUM),
        attribute='log_slow_filter',
    )

    generalLog = Bool(attribute='general_log')
    auditLog = Bool(restart=True, attribute='audit_log')
    defaultTimeZone = Str(
        validate=OneOf(all_timezones_set),
        attribute='default_time_zone',
    )
    explicitDefaultsForTimestamp = Bool(attribute='explicit_defaults_for_timestamp')
    groupConcatMaxLen = Int(
        validate=Range(min=4, max=33554432),
        attribute='group_concat_max_len',
    )
    tmpTableSize = Int(
        validate=[Range(min=1024, max=512 * 1024**2), validate_size_in_kb],
        attribute='tmp_table_size',
    )
    maxHeapTableSize = Int(
        validate=[Range(min=16384, max=512 * 1024**2), validate_size_in_kb],
        attribute='max_heap_table_size',
    )
    joinBufferSize = Int(
        validate=[Range(min=1024, max=16777216), validate_size_in_kb],
        attribute='join_buffer_size',
    )
    sortBufferSize = Int(
        validate=[Range(min=1024, max=16777216), validate_size_in_kb],
        attribute='sort_buffer_size',
    )
    tableDefinitionCache = Int(
        validate=Range(min=400, max=524288),
        attribute='table_definition_cache',
    )
    tableOpenCache = Int(
        validate=Range(min=400, max=524288),
        attribute='table_open_cache',
    )
    tableOpenCacheInstances = Int(validate=Range(min=1, max=32), attribute='table_open_cache_instances', restart=True)

    autoIncrementIncrement = Int(
        validate=Range(min=1, max=65535),
        attribute='auto_increment_increment',
    )
    autoIncrementOffset = Int(
        validate=Range(min=1, max=65535),
        attribute='auto_increment_offset',
    )

    syncBinlog = Int(
        validate=Range(min=0, max=4096),
        attribute='sync_binlog',
    )
    binlogCacheSize = Int(
        validate=Range(min=4096, max=67108864),
        attribute='binlog_cache_size',
    )
    binlogGroupCommitSyncDelay = Int(
        validate=Range(min=0, max=50000),
        attribute='binlog_group_commit_sync_delay',
    )
    binlogRowImage = Str(
        validate=OneOf(BINLOG_ROW_IMAGE_ENUM),
        attribute='binlog_row_image',
    )
    binlogRowsQueryLogEvents = Bool(attribute='binlog_rows_query_log_events')
    rplSemiSyncMasterWaitForSlaveCount = Int(
        validate=Range(min=1, max=2), attribute='rpl_semi_sync_master_wait_for_slave_count'
    )
    mdbPreserveBinlogBytes = Int(
        validate=Range(min=1024**3, max=1 * 1024**4),
        attribute='mdb_preserve_binlog_bytes',
    )

    binlogTransactionDependencyTracking = Str(
        validate=OneOf(BINLOG_TRANSACTION_DEPENDENCY_TRACKING_ENUM),
        attribute='binlog_transaction_dependency_tracking',
    )
    slaveParallelType = Str(
        validate=OneOf(SLAVE_PARALLEL_TYPE_ENUM),
        attribute='slave_parallel_type',
    )
    slaveParallelWorkers = Int(
        validate=Range(min=0, max=64),
        attribute='slave_parallel_workers',
    )
    interactiveTimeout = Int(
        validate=Range(min=600, max=86400),
        attribute='interactive_timeout',
    )
    waitTimeout = Int(
        validate=Range(min=600, max=86400),
        attribute='wait_timeout',
    )
    mdbOfflineModeEnableLag = Int(
        validate=Range(min=10 * 60, max=5 * 24 * 60 * 60),
        attribute='mdb_offline_mode_enable_lag',
    )
    mdbOfflineModeDisableLag = Int(
        validate=Range(min=60, max=24 * 60 * 60),
        attribute='mdb_offline_mode_disable_lag',
    )
    rangeOptimizerMaxMemSize = Int(
        validate=Range(min=1024**2, max=256 * 1024**2),
        attribute='range_optimizer_max_mem_size',
    )
    innodbOnlineAlterLogMaxSize = Int(
        validate=[Range(min=65536, max=100 * 1024**3), validate_size_in_kb],
        attribute='innodb_online_alter_log_max_size',
    )
    innodbFtMinTokenSize = Int(
        validate=Range(min=0, max=16),
        attribute='innodb_ft_min_token_size',
        restart=True,
    )
    innodbFtMaxTokenSize = Int(
        validate=Range(min=10, max=84),
        attribute='innodb_ft_max_token_size',
        restart=True,
    )
    innodbStrictMode = Bool(
        attribute='innodb_strict_mode',
    )
    lowerCaseTableNames = Int(
        validate=Range(min=0, max=1),
        attribute='lower_case_table_names',
        restart=True,
    )
    innodbPageSize = Int(
        validate=OneOf(INNODB_PAGE_SIZES),
        attribute='innodb_page_size',
    )

    innodbStatusOutput = Bool(attribute='innodb_status_output')

    mdbPriorityChoiceMaxLag = Int(
        validate=Range(min=0, max=24 * 60 * 60),
        attribute='mdb_priority_choice_max_lag',
    )
    maxDigestLength = Int(
        validate=[Range(min=1024, max=8 * 1024), validate_size_in_kb],
        attribute='max_digest_length',
        restart=True,
    )
    maxSpRecursionDepth = Int(
        validate=Range(min=0, max=255),
        attribute='max_sp_recursion_depth',
    )
    innodbCompressionLevel = Int(
        validate=Range(min=0, max=9),
        attribute='innodb_compression_level',
    )


@register_config_schema(MY_CLUSTER_TYPE, '5.7')
class Mysql57ConfigSchemaV1(MysqlConfigSchema):
    """
    MySQL mysql config schema (version 5.7)
    """

    sqlMode = List(
        Str(validate=OneOf(SQL_MODES_5_7)),
        attribute='sql_mode',
    )
    characterSetServer = Str(
        validate=OneOf(CHARSET_5_7),
        attribute='character_set_server',
        variable_default=lambda *_, **__: DEFAULT_CHARSET_5_7,
    )
    collationServer = Str(
        validate=OneOf(COLLATION_5_7),
        attribute='collation_server',
        variable_default=lambda *_, **__: DEFAULT_COLLATION_5_7,
    )
    defaultAuthenticationPlugin = UpperToLowerCaseEnum(
        UPPER_AUTH_PLUGINS_5_7_ENUM,
        attribute='default_authentication_plugin',
        variable_default=lambda *_, **__: DEFAULT_AUTH_PLUGIN_5_7.upper(),
        restart=True,
    )
    showCompatibility56 = Bool(attribute='show_compatibility_56', variable_default=lambda *_, **__: False)
    queryCacheType = Int(
        validate=Range(min=0, max=2),
        attribute='query_cache_type',
        restart=True,
    )
    queryCacheSize = Int(
        attribute='query_cache_size',
    )
    queryCacheLimit = Int(
        validate=Range(min=0, max=4294967295),
        attribute='query_cache_limit',
    )


@register_config_schema(MY_CLUSTER_TYPE, '8.0', feature_flag='MDB_MYSQL_8_0')
class Mysql80ConfigSchemaV1(MysqlConfigSchema):
    """
    MySQL mysql config schema (version 8.0)
    """

    sqlMode = List(
        Str(validate=OneOf(SQL_MODES_8_0)),
        attribute='sql_mode',
    )
    characterSetServer = Str(
        validate=OneOf(CHARSET_8_0),
        attribute='character_set_server',
        variable_default=lambda *_, **__: DEFAULT_CHARSET_8_0,
    )
    collationServer = Str(
        validate=OneOf(COLLATION_8_0),
        attribute='collation_server',
        variable_default=lambda *_, **__: DEFAULT_COLLATION_8_0,
    )
    defaultAuthenticationPlugin = UpperToLowerCaseEnum(
        UPPER_AUTH_PLUGINS_8_0_ENUM,
        attribute='default_authentication_plugin',
        variable_default=lambda *_, **__: DEFAULT_AUTH_PLUGIN_8_0.upper(),
        restart=True,
    )
    regexpTimeLimit = Int(
        validate=Range(min=0, max=1048576),
        attribute='regexp_time_limit',
    )


class Mysql57ConfigSetSchemaV1(Schema):
    """
    MySQL Schema for effective, user,
    default config representation for 5.7
    """

    effectiveConfig = Nested(Mysql57ConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Mysql57ConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Mysql57ConfigSchemaV1, attribute='default_config')


class Mysql80ConfigSetSchemaV1(Schema):
    """
    MySQL Schema for effective, user,
    default config representation for 8.0
    """

    effectiveConfig = Nested(Mysql80ConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(Mysql80ConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(Mysql80ConfigSchemaV1, attribute='default_config')


@register_config_schema('my_perf_diag', 'any')
class PerformanceDiagnosticsV1(Schema):
    """
    Schema for PerformanceDiagnostics config
    """

    enabled = Bool(attribute='enabled', default=False)
    sessionsSamplingInterval = Int(
        validate=Range(min=1, max=86400),
        attribute='my_sessions_sample_period',
        variable_default=defaults.get_my_sessions_sample_interval,
    )
    statementsSamplingInterval = Int(
        validate=Range(min=1, max=86400),
        attribute='my_statements_sample_period',
        variable_default=defaults.get_my_statements_sample_interval,
    )


class MysqlConfigSpecCreateSchemaV1(ClusterConfigSchemaV1):
    """
    MySQL cluster config schema used in create.
    """

    mysqlConfig_5_7 = Nested(Mysql57ConfigSchemaV1, attribute='mysql_config_5_7')
    mysqlConfig_8_0 = Nested(Mysql80ConfigSchemaV1, attribute='mysql_config_8_0')
    resources = Nested(ResourcesSchemaV1, required=True)
    backupWindowStart = Nested(TimeOfDay, attribute='backup_window_start')
    backupRetainPeriodDays = Int(attribute='retain_period', validate=Range(7, 60))
    access = Nested(AccessSchemaV1, attribute='access')
    performanceDiagnostics = Nested(PerformanceDiagnosticsV1, attribute='perf_diag')
    soxAudit = Boolean(attribute='sox_audit', allow_none=True)


# This is RestartSchema, not plain Schema!
class MysqlConfigSpecUpdateSchemaV1(ClusterConfigSchemaV1):
    """
    MySQL cluster config schema used in update.
    The only difference compared to Create is that resources are not required.
    TODO: too much copy-paste, mb a smarter approach later.
    """

    mysqlConfig_5_7 = Nested(Mysql57ConfigSchemaV1, attribute='mysql_config_5_7')
    mysqlConfig_8_0 = Nested(Mysql80ConfigSchemaV1, attribute='mysql_config_8_0')
    resources = Nested(MysqlResourcesUpdateSchemaV1)
    backupWindowStart = Nested(TimeOfDay, attribute='backup_window_start')
    backupRetainPeriodDays = Int(attribute='retain_period', validate=Range(7, 60))
    access = Nested(AccessSchemaV1, attribute='access')
    performanceDiagnostics = Nested(PerformanceDiagnosticsV1, attribute='perf_diag')
    soxAudit = Boolean(attribute='sox_audit', allow_none=True)


class MysqlClusterConfigSchemaV1(ClusterConfigSchemaV1):
    """
    MySQL cluster config schema used to display config.
    """

    mysqlConfig_5_7 = Nested(Mysql57ConfigSetSchemaV1, attribute='mysql_config_5_7')
    mysqlConfig_8_0 = Nested(Mysql80ConfigSetSchemaV1, attribute='mysql_config_8_0')
    resources = Nested(MysqlResourcesSchemaV1, required=True)
    backupWindowStart = Nested(TimeOfDay, attribute='backup_window_start')
    backupRetainPeriodDays = Int(attribute='retain_period', validate=Range(7, 60))
    access = Nested(AccessSchemaV1, attribute='access')
    performanceDiagnostics = Nested(PerformanceDiagnosticsV1, attribute='perf_diag')
    soxAudit = Boolean(attribute='sox_audit', allow_none=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.INFO)
class MysqlClusterSchemaV1(ManagedClusterSchemaV1):
    """
    MySQL cluster schema.
    """

    config = Nested(MysqlClusterConfigSchemaV1, required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.LIST)
class MysqlListClustersResponseSchemaV1(ListResponseSchemaV1):
    """
    MySQL cluster list schema.
    """

    clusters = Nested(MysqlClusterSchemaV1, many=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CREATE)
@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.BILLING_CREATE)
class MysqlCreateClusterRequestSchemaV1(CreateClusterRequestSchemaV1):
    """
    Schema for create Mysql cluster request.
    """

    name = Str(validate=MySQLClusterTraits.cluster_name.validate, required=True)
    configSpec = Nested(MysqlConfigSpecCreateSchemaV1, attribute='config_spec', required=True)
    databaseSpecs = List(Nested(MysqlDatabaseSpecSchemaV1), attribute='database_specs', required=True)
    userSpecs = List(Nested(MysqlUserSpecSchemaV1), attribute='user_specs', required=True)
    hostSpecs = List(Nested(MysqlHostSpecSchemaV1), attribute='host_specs', required=True)
    hostGroupIds = List(Str(), attribute='host_group_ids')


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MODIFY)
class MysqlUpdateClusterRequestSchemaV1(UpdateClusterRequestSchemaV1):
    """
    Schema for update MySQL cluster request.
    """

    configSpec = Nested(MysqlConfigSpecUpdateSchemaV1, attribute='config_spec')
    name = Str(validate=MySQLClusterTraits.cluster_name.validate)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.RESTORE)
class MysqlRestoreClusterRequestSchemaV1(RestoreClusterRequestSchemaV1):
    """
    Schema for restore Mysql cluster request.
    """

    time = validation.GrpcTimestamp(required=True)
    configSpec = Nested(MysqlConfigSpecCreateSchemaV1, attribute='config_spec', required=True)
    # don't use here MysqlHostSpecSchemaV1,
    # cause don't want to deal with replicationSource
    # in restore logic
    hostSpecs = Nested(HostSpecSchemaV1, attribute='host_specs', many=True, required=True)
    hostGroupIds = List(Str(), attribute='host_group_ids')


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.RESTORE_HINTS)
class MysqlRestoreHintsResponseSchemaV1(RestoreHintWithTimeResponseSchemaV1):
    """
    Schema for MySQL restore hint
    """


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.START_FAILOVER)
class MysqlStartClusterFailoverRequestSchemaV1(StartClusterFailoverRequestSchemaV1):
    """
    Schema for failover MySQL cluster request.
    """

    hostName = Str(attribute='host_name', validate=validation.hostname_validator, required=False)


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.INFO)
class MysqlBackupSchemaV1(BaseBackupSchemaV1):
    """
    Schema for Mysql backup
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


@register_response_schema(MY_CLUSTER_TYPE, Resource.BACKUP, DbaasOperation.LIST)
class MysqlListClusterBackupsReponseSchemaV1(ListResponseSchemaV1):
    """
    Schema for Mysql backups listing
    """

    backups = Nested(MysqlBackupSchemaV1, many=True, required=True)


class MysqlConsoleClustersConfigDiskSizeRangeSchemaV1(Schema):
    """
    MySQL disk size range schema.
    """

    min = Int()
    max = Int()


class MysqlConsoleClustersConfigDiskSizesSchemaV1(Schema):
    """
    MySQL disk sizes list schema.
    """

    sizes = List(Int())


class MysqlConsoleClustersConfigDiskTypesSchemaV1(Schema):
    """
    MySQL available disk type schema.
    """

    diskTypeId = Str(attribute='disk_type_id')
    diskSizeRange = Nested(MysqlConsoleClustersConfigDiskSizeRangeSchemaV1(), attribute='disk_size_range')
    diskSizes = Nested(MysqlConsoleClustersConfigDiskSizesSchemaV1(), attribute='disk_sizes')
    minHosts = Int(attribute='min_hosts')
    maxHosts = Int(attribute='max_hosts')


class MysqlConsoleClustersConfigZoneSchemaV1(Schema):
    """
    MySQL available zone schema.
    """

    zoneId = Str(attribute='zone_id')
    diskTypes = Nested(MysqlConsoleClustersConfigDiskTypesSchemaV1, many=True, attribute='disk_types')


class MysqlConsoleClustersConfigResourcePresetSchemaV1(Schema):
    """
    MySQL available resource preset schema.
    """

    presetId = Str(attribute='preset_id')
    cpuLimit = Int(attribute='cpu_limit')
    cpuFraction = Int(attribute='cpu_fraction')
    memoryLimit = Int(attribute='memory_limit')
    type = Str(attribute='type')
    generation = Str(attribute='generation')
    generationName = Str(attribute='generation_name')
    decommissioning = Bool(attribute='decommissioning')
    zones = Nested(MysqlConsoleClustersConfigZoneSchemaV1, many=True)


class MysqlConsoleClustersConfigHostCountPerDiskTypeSchemaV1(Schema):
    """
    Mysql host count limits for disk type schema.
    """

    diskTypeId = Str(attribute='disk_type_id')
    minHostCount = Int(attribute='min_host_count')


class MysqlConsoleClustersConfigHostCountLimitsSchemaV1(Schema):
    """
    Mysql host count limits schema.
    """

    minHostCount = Int(attribute='min_host_count')
    maxHostCount = Int(attribute='max_host_count')
    hostCountPerDiskType = Nested(
        MysqlConsoleClustersConfigHostCountPerDiskTypeSchemaV1, many=True, attribute='host_count_per_disk_type'
    )


class MysqlConsoleClustersConfigDefaultResourcesSchemaV1(Schema):
    """
    MySQL default resources schema.
    """

    generation = Str(attribute='generation')
    generationName = Str(attribute='generation_name')
    resourcePresetId = Str(attribute='resource_preset_id')
    diskTypeId = Str(attribute='disk_type_id')
    diskSize = Int(attribute='disk_size')


@register_response_schema(MY_CLUSTER_TYPE, Resource.CONSOLE_CLUSTERS_CONFIG, DbaasOperation.INFO)
class MysqlConsoleClustersConfigSchemaV1(Schema):
    """
    MySQL console clusters config schema.
    """

    clusterName = Nested(StringValueV1(), attribute='cluster_name')
    dbName = Nested(StringValueV1(), attribute='db_name')
    userName = Nested(StringValueV1(), attribute='user_name')
    password = Nested(StringValueV1())
    hostCountLimits = Nested(MysqlConsoleClustersConfigHostCountLimitsSchemaV1(), attribute='host_count_limits')
    resourcePresets = Nested(MysqlConsoleClustersConfigResourcePresetSchemaV1, many=True, attribute='resource_presets')
    defaultResources = Nested(MysqlConsoleClustersConfigDefaultResourcesSchemaV1, attribute='default_resources')
    versions = List(Str())
    availableVersions = Nested(ClustersConfigAvailableVersionSchemaV1, many=True, attribute='available_versions')
    defaultVersion = Str(attribute='default_version')


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.STOP)
class MysqlStopClusterRequestSchemaV1(Schema):
    """
    Schema for stop cluster request.
    """


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE)
class MysqlDeleteClusterRequestSchemaV1(Schema):
    """
    Schema for stop cluster request.
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.ALERTS_TEMPLATE)
class PostgresqlAlertTemplateSchemaV1(AlertTemplateListSchemaV1):
    """
    Schema for Postgresql alerts template
    """


@register_request_schema(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.CREATE)
class MysqlCreateAlertGroupRequestSchemaV1(CreateAlertsGroupRequestSchemaV1):
    """
    Schema for create alert group request.
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.INFO)
class MysqlAlertGroupSchemaV1(AlertGroupSchemaV1):
    """
    Schema for Mysql alert group
    """


@register_response_schema(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.LIST)
class MysqlListAlertGroupResponseSchemaV1(ListResponseSchemaV1):
    """
    Schema for List alert group request.
    """

    alertGroups = Nested(MysqlAlertGroupSchemaV1, many=True, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.ALERT_GROUP, DbaasOperation.DELETE)
class MysqlAlertGroupSchemaDeleteV1(Schema):
    """
    Schema for Mysql alert group delete
    """
