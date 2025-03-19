# -*- coding: utf-8 -*-
"""
Schemas for ClickHouse users.
"""

from marshmallow.fields import Int, Nested
from marshmallow.validate import Range

from ..constants import MY_CLUSTER_TYPE
from ..traits import ClickhouseClusterTraits
from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.fields import (
    BooleanMappedToInt,
    MappedEnum,
    MillisecondsMappedToSeconds,
    Str,
    UInt,
    XmlEscapedStr,
)
from ....apis.schemas.objects import UserSchemaV1
from ....utils.register import DbaasOperation, register_request_schema, register_response_schema, Resource
from ....utils.validation import Schema


class OverflowMode(MappedEnum):
    """
    Enumeration type for overflow mode fields.
    """

    def __init__(self, **kwargs) -> None:
        super().__init__(
            {
                'OVERFLOW_MODE_UNSPECIFIED': None,
                'OVERFLOW_MODE_THROW': 'throw',
                'OVERFLOW_MODE_BREAK': 'break',
            },
            **kwargs
        )


class GroupByOverflowMode(MappedEnum):
    """
    Enumeration type for GroupByOverflowMode field.
    """

    def __init__(self, **kwargs) -> None:
        super().__init__(
            {
                'GROUP_BY_OVERFLOW_MODE_UNSPECIFIED': None,
                'GROUP_BY_OVERFLOW_MODE_THROW': 'throw',
                'GROUP_BY_OVERFLOW_MODE_BREAK': 'break',
                'GROUP_BY_OVERFLOW_MODE_ANY': 'any',
            },
            **kwargs
        )


class DistributedProductMode(MappedEnum):
    """
    Enumeration type for DistributedProductMode field.
    """

    def __init__(self, **kwargs) -> None:
        super().__init__(
            {
                'DISTRIBUTED_PRODUCT_MODE_UNSPECIFIED': None,
                'DISTRIBUTED_PRODUCT_MODE_DENY': 'deny',
                'DISTRIBUTED_PRODUCT_MODE_LOCAL': 'local',
                'DISTRIBUTED_PRODUCT_MODE_GLOBAL': 'global',
                'DISTRIBUTED_PRODUCT_MODE_ALLOW': 'allow',
            },
            **kwargs
        )


class QuotaMode(MappedEnum):
    """
    Enumeration type for QuotaMode field.
    """

    def __init__(self, **kwargs) -> None:
        super().__init__(
            {
                'QUOTA_MODE_UNSPECIFIED': None,
                'QUOTA_MODE_DEFAULT': 'default',
                'QUOTA_MODE_KEYED': 'keyed',
                'QUOTA_MODE_KEYED_BY_IP': 'keyed_by_ip',
            },
            **kwargs
        )


class ClickhouseDataFilterSchemaV1(Schema):
    """
    Data filter schema.
    """

    tableName = XmlEscapedStr(attribute='table_name', required=True)
    filter = XmlEscapedStr(required=True)


class ClickhousePermissionSchemaV1(Schema):
    """
    Permission schema.
    """

    databaseName = Str(attribute='database_name', validate=ClickhouseClusterTraits.db_name.validate, required=True)
    dataFilters = Nested(ClickhouseDataFilterSchemaV1, attribute='data_filters', many=True)


class ClickhouseUserSettingsSchemaV1(Schema):
    """
    User settings schema.
    """

    # pylint: disable=invalid-name

    # Permissions
    readonly = Int(validate=Range(min=0, max=2), allow_none=True)
    allowDdl = BooleanMappedToInt(attribute='allow_ddl', allow_none=True)

    # Timeouts
    connectTimeout = MillisecondsMappedToSeconds(attribute='connect_timeout', validate=Range(min=1000), allow_none=True)
    receiveTimeout = MillisecondsMappedToSeconds(attribute='receive_timeout', validate=Range(min=1000), allow_none=True)
    sendTimeout = MillisecondsMappedToSeconds(attribute='send_timeout', validate=Range(min=1000), allow_none=True)

    # Replication settings
    insertQuorum = UInt(attribute='insert_quorum', allow_none=True)
    insertQuorumTimeout = MillisecondsMappedToSeconds(
        attribute='insert_quorum_timeout', validate=Range(min=1000), allow_none=True
    )
    insertQuorumParallel = BooleanMappedToInt(attribute='insert_quorum_parallel', allow_none=True)
    selectSequentialConsistency = BooleanMappedToInt(attribute='select_sequential_consistency', allow_none=True)
    replicationAlterPartitionsSync = Int(
        attribute='replication_alter_partitions_sync', validate=Range(min=0, max=2), allow_none=True
    )

    # Settings of distributed queries
    maxReplicaDelayForDistributedQueries = MillisecondsMappedToSeconds(
        attribute='max_replica_delay_for_distributed_queries', validate=Range(min=1000), allow_none=True
    )
    fallbackToStaleReplicasForDistributedQueries = BooleanMappedToInt(
        attribute='fallback_to_stale_replicas_for_distributed_queries', allow_none=True
    )
    distributedProductMode = DistributedProductMode(attribute='distributed_product_mode', allow_none=True)
    distributedAggregationMemoryEfficient = BooleanMappedToInt(
        attribute='distributed_aggregation_memory_efficient', allow_none=True
    )
    distributedDdlTaskTimeout = MillisecondsMappedToSeconds(attribute='distributed_ddl_task_timeout', allow_none=True)
    skipUnavailableShards = BooleanMappedToInt(attribute='skip_unavailable_shards', allow_none=True)

    # Query and expression compilations
    compile = BooleanMappedToInt(attribute='compile', allow_none=True)
    minCountToCompile = UInt(attribute='min_count_to_compile', allow_none=True)
    compileExpressions = BooleanMappedToInt(attribute='compile_expressions', allow_none=True)
    minCountToCompileExpression = UInt(attribute='min_count_to_compile_expression', allow_none=True)

    # I/O settings
    maxBlockSize = UInt(attribute='max_block_size', validate=Range(min=1), allow_none=True)
    minInsertBlockSizeRows = UInt(attribute='min_insert_block_size_rows', allow_none=True)
    minInsertBlockSizeBytes = UInt(attribute='min_insert_block_size_bytes', allow_none=True)
    maxInsertBlockSize = UInt(attribute='max_insert_block_size', validate=Range(min=1), allow_none=True)
    maxPartitionsPerInsertBlock = UInt(
        attribute='max_partitions_per_insert_block', validate=Range(min=1), allow_none=True
    )
    minBytesToUseDirectIo = UInt(attribute='min_bytes_to_use_direct_io', allow_none=True)
    useUncompressedCache = BooleanMappedToInt(attribute='use_uncompressed_cache', allow_none=True)
    mergeTreeMaxRowsToUseCache = UInt(
        attribute='merge_tree_max_rows_to_use_cache', validate=Range(min=1), allow_none=True
    )
    mergeTreeMaxBytesToUseCache = UInt(
        attribute='merge_tree_max_bytes_to_use_cache', validate=Range(min=1), allow_none=True
    )
    mergeTreeMinRowsForConcurrentRead = UInt(
        attribute='merge_tree_min_rows_for_concurrent_read', validate=Range(min=1), allow_none=True
    )
    mergeTreeMinBytesForConcurrentRead = UInt(
        attribute='merge_tree_min_bytes_for_concurrent_read', validate=Range(min=1), allow_none=True
    )
    maxBytesBeforeExternalGroupBy = UInt(attribute='max_bytes_before_external_group_by', allow_none=True)
    maxBytesBeforeExternalSort = UInt(attribute='max_bytes_before_external_sort', allow_none=True)
    groupByTwoLevelThreshold = UInt(attribute='group_by_two_level_threshold', allow_none=True)
    groupByTwoLevelThresholdBytes = UInt(attribute='group_by_two_level_threshold_bytes', allow_none=True)
    minBytesForWidePart = UInt(attribute='min_bytes_for_wide_part', allow_none=True, console_hidden=True)
    minRowsForWidePart = UInt(attribute='min_rows_for_wide_part', allow_none=True, console_hidden=True)
    deduplicateBlocksInDependentMaterializedViews = UInt(
        attribute='deduplicate_blocks_in_dependent_materialized_views', allow_none=True
    )

    # Resource usage limits and query priorities
    priority = UInt(attribute='priority', allow_none=True)
    maxThreads = UInt(attribute='max_threads', validate=Range(min=1), allow_none=True)
    maxMemoryUsage = UInt(attribute='max_memory_usage', allow_none=True)
    maxMemoryUsageForUser = UInt(attribute='max_memory_usage_for_user', allow_none=True)
    maxNetworkBandwidth = UInt(attribute='max_network_bandwidth', allow_none=True)
    maxNetworkBandwidthForUser = UInt(attribute='max_network_bandwidth_for_user', allow_none=True)
    maxConcurrentQueriesForUser = UInt(attribute='max_concurrent_queries_for_user', allow_none=True)

    # Query complexity limits
    forceIndexByDate = BooleanMappedToInt(attribute='force_index_by_date', allow_none=True)
    forcePrimaryKey = BooleanMappedToInt(attribute='force_primary_key', allow_none=True)
    maxRowsToRead = UInt(attribute='max_rows_to_read', allow_none=True)
    maxBytesToRead = UInt(attribute='max_bytes_to_read', allow_none=True)
    readOverflowMode = OverflowMode(attribute='read_overflow_mode', allow_none=True)
    maxRowsToGroupBy = UInt(attribute='max_rows_to_group_by', allow_none=True)
    groupByOverflowMode = GroupByOverflowMode(attribute='group_by_overflow_mode', allow_none=True)
    maxRowsToSort = UInt(attribute='max_rows_to_sort', allow_none=True)
    maxBytesToSort = UInt(attribute='max_bytes_to_sort', allow_none=True)
    sortOverflowMode = OverflowMode(attribute='sort_overflow_mode', allow_none=True)
    maxResultRows = UInt(attribute='max_result_rows', allow_none=True)
    maxResultBytes = UInt(attribute='max_result_bytes', allow_none=True)
    resultOverflowMode = OverflowMode(attribute='result_overflow_mode', allow_none=True)
    maxRowsInDistinct = UInt(attribute='max_rows_in_distinct', allow_none=True)
    maxBytesInDistinct = UInt(attribute='max_bytes_in_distinct', allow_none=True)
    distinctOverflowMode = OverflowMode(attribute='distinct_overflow_mode', allow_none=True)
    maxRowsToTransfer = UInt(attribute='max_rows_to_transfer', allow_none=True)
    maxBytesToTransfer = UInt(attribute='max_bytes_to_transfer', allow_none=True)
    transferOverflowMode = OverflowMode(attribute='transfer_overflow_mode', allow_none=True)
    maxExecutionTime = MillisecondsMappedToSeconds(
        attribute='max_execution_time', validate=Range(min=0), allow_none=True
    )
    timeoutOverflowMode = OverflowMode(attribute='timeout_overflow_mode', allow_none=True)
    maxRowsInSet = UInt(attribute='max_rows_in_set', allow_none=True)
    maxBytesInSet = UInt(attribute='max_bytes_in_set', allow_none=True)
    setOverflowMode = OverflowMode(attribute='set_overflow_mode', allow_none=True)
    maxRowsInJoin = UInt(attribute='max_rows_in_join', allow_none=True)
    maxBytesInJoin = UInt(attribute='max_bytes_in_join', allow_none=True)
    joinOverflowMode = OverflowMode(attribute='join_overflow_mode', allow_none=True)
    maxColumnsToRead = UInt(attribute='max_columns_to_read', allow_none=True)
    maxTemporaryColumns = UInt(attribute='max_temporary_columns', allow_none=True)
    maxTemporaryNonConstColumns = UInt(attribute='max_temporary_non_const_columns', allow_none=True)
    maxQuerySize = UInt(attribute='max_query_size', validate=Range(min=0), allow_none=True)
    maxAstDepth = UInt(attribute='max_ast_depth', validate=Range(min=0), allow_none=True)
    maxAstElements = UInt(attribute='max_ast_elements', validate=Range(min=0), allow_none=True)
    maxExpandedAstElements = UInt(attribute='max_expanded_ast_elements', validate=Range(min=0), allow_none=True)
    minExecutionSpeed = UInt(attribute='min_execution_speed', allow_none=True)
    minExecutionSpeedBytes = UInt(attribute='min_execution_speed_bytes', allow_none=True)

    # Settings of input and output formats
    inputFormatValuesInterpretExpressions = BooleanMappedToInt(
        attribute='input_format_values_interpret_expressions', allow_none=True
    )
    inputFormatDefaultsForOmittedFields = BooleanMappedToInt(
        attribute='input_format_defaults_for_omitted_fields', allow_none=True
    )
    inputFormatWithNamesUseHeader = BooleanMappedToInt(attribute='input_format_with_names_use_header', allow_none=True)
    inputFormatNullAsDefault = BooleanMappedToInt(attribute='input_format_null_as_default', allow_none=True)
    outputFormatJsonQuote_64bitIntegers = BooleanMappedToInt(
        attribute='output_format_json_quote_64bit_integers', allow_none=True
    )
    outputFormatJsonQuoteDenormals = BooleanMappedToInt(attribute='output_format_json_quote_denormals', allow_none=True)
    lowCardinalityAllowInNativeFormat = BooleanMappedToInt(
        attribute='low_cardinality_allow_in_native_format', allow_none=True
    )
    emptyResultForAggregationByEmptySet = BooleanMappedToInt(
        attribute='empty_result_for_aggregation_by_empty_set', allow_none=True
    )
    dateTimeInputFormat = MappedEnum(
        {
            'DATE_TIME_INPUT_FORMAT_UNSPECIFIED': None,
            'DATE_TIME_INPUT_FORMAT_BEST_EFFORT': 'best_effort',
            'DATE_TIME_INPUT_FORMAT_BASIC': 'basic',
        },
        attribute='date_time_input_format',
        allow_none=True,
    )
    dateTimeOutputFormat = MappedEnum(
        {
            'DATE_TIME_OUTPUT_FORMAT_UNSPECIFIED': None,
            'DATE_TIME_OUTPUT_FORMAT_SIMPLE': 'simple',
            'DATE_TIME_OUTPUT_FORMAT_ISO': 'iso',
            'DATE_TIME_OUTPUT_FORMAT_UNIX_TIMESTAMP': 'unix_timestamp',
        },
        attribute='date_time_output_format',
        allow_none=True,
    )
    formatRegexp = XmlEscapedStr(attribute='format_regexp', allow_none=True)
    formatRegexpEscapingRule = MappedEnum(
        {
            'FORMAT_REGEXP_ESCAPING_RULE_UNSPECIFIED': None,
            'FORMAT_REGEXP_ESCAPING_RULE_ESCAPED': 'Escaped',
            'FORMAT_REGEXP_ESCAPING_RULE_QUOTED': 'Quoted',
            'FORMAT_REGEXP_ESCAPING_RULE_CSV': 'CSV',
            'FORMAT_REGEXP_ESCAPING_RULE_JSON': 'JSON',
            'FORMAT_REGEXP_ESCAPING_RULE_RAW': 'Raw',
        },
        attribute='format_regexp_escaping_rule',
        allow_none=True,
    )
    formatRegexpSkipUnmatched = BooleanMappedToInt(attribute='format_regexp_skip_unmatched', allow_none=True)

    # HTTP-specific settings
    httpConnectionTimeout = MillisecondsMappedToSeconds(
        attribute='http_connection_timeout', validate=Range(min=1000), allow_none=True
    )
    httpReceiveTimeout = MillisecondsMappedToSeconds(
        attribute='http_receive_timeout', validate=Range(min=1000), allow_none=True
    )
    httpSendTimeout = MillisecondsMappedToSeconds(
        attribute='http_send_timeout', validate=Range(min=1000), allow_none=True
    )
    enableHttpCompression = BooleanMappedToInt(attribute='enable_http_compression', allow_none=True)
    sendProgressInHttpHeaders = BooleanMappedToInt(attribute='send_progress_in_http_headers', allow_none=True)
    httpHeadersProgressInterval = UInt(
        attribute='http_headers_progress_interval_ms', validate=Range(min=100), allow_none=True
    )
    addHttpCorsHeader = BooleanMappedToInt(attribute='add_http_cors_header', allow_none=True)

    # Quoting settings
    quotaMode = QuotaMode(attribute='quota_mode', allow_none=True)

    # Other settings
    countDistinctImplementation = MappedEnum(
        {
            'COUNT_DISTINCT_IMPLEMENTATION_UNSPECIFIED': None,
            'COUNT_DISTINCT_IMPLEMENTATION_UNIQ': 'uniq',
            'COUNT_DISTINCT_IMPLEMENTATION_UNIQ_COMBINED': 'uniqCombined',
            'COUNT_DISTINCT_IMPLEMENTATION_UNIQ_COMBINED_64': 'uniqCombined64',
            'COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12': 'uniqHLL12',
            'COUNT_DISTINCT_IMPLEMENTATION_UNIQ_EXACT': 'uniqExact',
        },
        attribute='count_distinct_implementation',
        allow_none=True,
    )
    joinAlgorithm = MappedEnum(
        {
            'JOIN_ALGORITHM_UNSPECIFIED': None,
            'JOIN_ALGORITHM_AUTO': 'auto',
            'JOIN_ALGORITHM_HASH': 'hash',
            'JOIN_ALGORITHM_PARTIAL_MERGE': 'partial_merge',
            'JOIN_ALGORITHM_PREFER_PARTIAL_MERGE': 'prefer_partial_merge',
        },
        attribute='join_algorithm',
        allow_none=True,
    )
    anyJoinDistinctRightTableKeys = BooleanMappedToInt(attribute='any_join_distinct_right_table_keys', allow_none=True)
    joinedSubqueryRequiresAlias = BooleanMappedToInt(attribute='joined_subquery_requires_alias', allow_none=True)
    joinUseNulls = BooleanMappedToInt(attribute='join_use_nulls', allow_none=True)
    transformNullIn = BooleanMappedToInt(attribute='transform_null_in', allow_none=True)


class ClickhouseUserQuotaSchemaV1(Schema):
    """
    User quota schema.
    """

    # pylint: disable=invalid-name

    intervalDuration = MillisecondsMappedToSeconds(
        attribute='interval_duration', validate=Range(min=1000), required=True
    )
    queries = UInt(attribute='queries', validate=Range(min=0))
    errors = UInt(attribute='errors', validate=Range(min=0))
    resultRows = UInt(attribute='result_rows', validate=Range(min=0))
    readRows = UInt(attribute='read_rows', validate=Range(min=0))
    executionTime = MillisecondsMappedToSeconds(attribute='execution_time', validate=Range(min=0))


@register_response_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.INFO)
class ClickhouseUserSchemaV1(UserSchemaV1):
    """
    ClickHouse user schema.
    """

    name = Str(validate=ClickhouseClusterTraits.user_name.validate, required=True)
    permissions = Nested(ClickhousePermissionSchemaV1, many=True, required=True)
    settings = Nested(ClickhouseUserSettingsSchemaV1)
    quotas = Nested(ClickhouseUserQuotaSchemaV1, many=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.LIST)
class ClickhouseListUsersResponseSchemaV1(ListResponseSchemaV1):
    """
    ClickHouse user list schema.
    """

    users = Nested(ClickhouseUserSchemaV1, many=True, required=True)


class ClickhouseUserSpecSchemaV1(Schema):
    """
    Clickhouse user spec schema.
    """

    name = Str(validate=ClickhouseClusterTraits.user_name.validate, required=True)
    password = Str(validate=ClickhouseClusterTraits.password.validate, required=True)
    permissions = Nested(ClickhousePermissionSchemaV1, many=True)
    settings = Nested(ClickhouseUserSettingsSchemaV1)
    quotas = Nested(ClickhouseUserQuotaSchemaV1, many=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.CREATE)
class ClickhouseCreateUserRequestSchemaV1(Schema):
    """
    Schema for create ClickHouse user request.
    """

    userSpec = Nested(ClickhouseUserSpecSchemaV1, attribute='user_spec', required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.MODIFY)
class ClickhouseUpdateUserRequestSchemaV1(Schema):
    """
    Schema for update ClickHouse user request.
    """

    password = Str(validate=ClickhouseClusterTraits.password.validate)
    permissions = Nested(ClickhousePermissionSchemaV1, many=True)
    settings = Nested(ClickhouseUserSettingsSchemaV1)
    quotas = Nested(ClickhouseUserQuotaSchemaV1, many=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.GRANT_PERMISSION)
class ClickhouseAddUserPermissionRequestSchemaV1(Schema):
    """
    Schema for add ClickHouse user permission request.
    """

    permission = Nested(ClickhousePermissionSchemaV1, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.USER, DbaasOperation.REVOKE_PERMISSION)
class ClickhouseRevokeUserPermissionRequestSchemaV1(Schema):
    """
    Schema for revoke ClickHouse user permission request.
    """

    databaseName = Str(attribute='database_name', validate=ClickhouseClusterTraits.db_name.validate, required=True)
