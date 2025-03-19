# -*- coding: utf-8 -*-
"""
Schemas for ClickHouse clusters.
"""
from marshmallow import ValidationError, validates_schema
from marshmallow.fields import Boolean, Float, Int, List, Nested
from marshmallow.validate import Length, OneOf, Range, Regexp
from pytz import all_timezones_set

from .databases import ClickhouseDatabaseSpecSchemaV1
from .hosts import ClickhouseHostSpecSchemaV1
from .users import ClickhouseUserSpecSchemaV1
from ..constants import MY_CLUSTER_TYPE
from ..schema_defaults import get_mark_cache_size
from ..traits import ClickhouseClusterTraits
from ....apis.schemas.cluster import (
    ClusterConfigSchemaV1,
    ClusterConfigSpecSchemaV1,
    ConfigSchemaV1,
    CreateClusterRequestSchemaV1,
    ManagedClusterSchemaV1,
    RestoreClusterRequestSchemaV1,
    UpdateClusterRequestSchemaV1,
)
from ....apis.schemas.common import ListResponseSchemaV1
from ....apis.schemas.fields import (
    GrpcInt,
    GrpcUInt,
    MappedEnum,
    Str,
    UInt,
    UpperToLowerCaseEnum,
    XmlEscapedStr,
    MillisecondsMappedToSeconds,
)
from ....apis.schemas.objects import AccessSchemaV1, ResourcesSchemaV1, ResourcesUpdateSchemaV1
from ....utils.register import DbaasOperation, Resource, register_request_schema, register_response_schema
from ....utils.traits import ServiceAccountId
from ....utils.validation import Schema, TimeOfDay


class LogLevel(UpperToLowerCaseEnum):
    """
    Log level enumeration type.
    """

    def __init__(self, **kwargs) -> None:
        super().__init__(['TRACE', 'DEBUG', 'INFORMATION', 'WARNING', 'ERROR'], **kwargs)


class KafkaSettingsSchema(Schema):
    """
    Schema for Kafka settings.
    """

    securityProtocol = MappedEnum(
        {
            'SECURITY_PROTOCOL_UNSPECIFIED': None,
            'SECURITY_PROTOCOL_PLAINTEXT': 'PLAINTEXT',
            'SECURITY_PROTOCOL_SSL': 'SSL',
            'SECURITY_PROTOCOL_SASL_PLAINTEXT': 'SASL_PLAINTEXT',
            'SECURITY_PROTOCOL_SASL_SSL': 'SASL_SSL',
        },
        attribute='security_protocol',
    )
    saslMechanism = MappedEnum(
        {
            'SASL_MECHANISM_UNSPECIFIED': None,
            'SASL_MECHANISM_GSSAPI': 'GSSAPI',
            'SASL_MECHANISM_PLAIN': 'PLAIN',
            'SASL_MECHANISM_SCRAM_SHA_256': 'SCRAM-SHA-256',
            'SASL_MECHANISM_SCRAM_SHA_512': 'SCRAM-SHA-512',
        },
        attribute='sasl_mechanism',
    )
    saslUsername = XmlEscapedStr(attribute='sasl_username')
    saslPassword = XmlEscapedStr(attribute='sasl_password', load_only=True)


class ClickhouseDictionaryAttributeSchemaV1(Schema):
    """
    Schema for attributes of external dictionaries.
    """

    name = XmlEscapedStr(required=True)
    type = XmlEscapedStr(required=True)
    nullValue = XmlEscapedStr(attribute='null_value', missing='')
    expression = XmlEscapedStr()
    hierarchical = Boolean(missing=False)
    injective = Boolean(missing=False)


class ClickhouseDictionaryRangeAttributeSchemaV1(Schema):
    """
    Schema for rangeMin and rangeMax attributes of external dictionaries.
    """

    name = XmlEscapedStr(required=True)
    type = XmlEscapedStr()


class ClickhouseDictionarySchemaV1(Schema):
    """
    Schema for external dictionaries.
    """

    class UIntRangeSchema(Schema):
        """
        Range schema.
        """

        min = UInt(required=True)
        max = UInt(required=True)

    class StructureSchema(Schema):
        """
        Structure schema.
        """

        class IdSchema(Schema):
            """
            Schema for the id field (numeric keys).
            """

            name = XmlEscapedStr(required=True)

        class KeySchema(Schema):
            """
            Schema for the key field (complex keys).
            """

            attributes = Nested(ClickhouseDictionaryAttributeSchemaV1, many=True, required=True, validate=Length(min=1))

        id = Nested(IdSchema)
        key = Nested(KeySchema)
        rangeMin = Nested(ClickhouseDictionaryRangeAttributeSchemaV1, attribute='range_min')
        rangeMax = Nested(ClickhouseDictionaryRangeAttributeSchemaV1, attribute='range_max')
        attributes = Nested(ClickhouseDictionaryAttributeSchemaV1, many=True, required=True, validate=Length(min=1))

        @validates_schema(skip_on_field_errors=True)
        def _validates_schema(self, data):
            if 'id' in data and 'key' in data:
                raise ValidationError("The parameters 'id' and 'key' are mutually exclusive.")
            if 'id' not in data and 'key' not in data:
                raise ValidationError("Either 'id' or 'key' must be specified.")

    class LayoutSchema(Schema):
        """
        Layout schema.
        """

        type = UpperToLowerCaseEnum(
            [
                'FLAT',
                'HASHED',
                'COMPLEX_KEY_HASHED',
                'RANGE_HASHED',
                'CACHE',
                'COMPLEX_KEY_CACHE',
            ],
            required=True,
        )
        sizeInCells = GrpcUInt(attribute='size_in_cells')

        @validates_schema(skip_on_field_errors=True)
        def _validates_schema(self, data):
            layout = data['type'].upper()
            if layout in ['CACHE', 'COMPLEX_KEY_CACHE']:
                if 'size_in_cells' not in data:
                    raise ValidationError(
                        "The parameter 'sizeInCells' is required for the layout '{0}'.".format(layout)
                    )
            else:
                if 'size_in_cells' in data:
                    raise ValidationError(
                        "The parameter 'sizeInCells' is not applicable for the layout '{0}'.".format(layout)
                    )

    class HttpSourceSchema(Schema):
        """
        HTTP source schema.
        """

        url = XmlEscapedStr(required=True)
        format = XmlEscapedStr(required=True)

    class MysqlSourceSchema(Schema):
        """
        MySQL source schema.
        """

        class ReplicaSchema(Schema):
            """
            Replica schema.
            """

            host = XmlEscapedStr(required=True)
            priority = GrpcInt(missing=100)
            port = GrpcUInt()
            user = XmlEscapedStr()
            password = XmlEscapedStr(load_only=True)

        db = XmlEscapedStr(required=True)
        table = XmlEscapedStr(required=True)
        port = GrpcUInt()
        user = XmlEscapedStr()
        password = XmlEscapedStr(load_only=True)
        replicas = Nested(ReplicaSchema, many=True, required=True)
        where = XmlEscapedStr()
        invalidateQuery = XmlEscapedStr(attribute='invalidate_query')

    class ClickhouseSourceSchema(Schema):
        """
        ClickHouse source schema.
        """

        db = XmlEscapedStr(required=True)
        table = XmlEscapedStr(required=True)
        host = XmlEscapedStr(required=True)
        port = GrpcUInt(missing=9000)
        user = XmlEscapedStr()
        password = XmlEscapedStr(load_only=True)
        where = XmlEscapedStr()

    class MongodbSourceSchema(Schema):
        """
        MongoDB source schema.
        """

        db = XmlEscapedStr(required=True)
        collection = XmlEscapedStr(required=True)
        host = XmlEscapedStr(required=True)
        port = GrpcUInt(missing=27017)
        user = XmlEscapedStr()
        password = XmlEscapedStr(load_only=True)

    class YtSourceSchema(Schema):
        """
        YT source schema.
        """

        clusters = List(XmlEscapedStr(), required=True)
        table = XmlEscapedStr(required=True)
        keys = List(XmlEscapedStr())
        fields = List(XmlEscapedStr())
        dateFields = List(XmlEscapedStr(), attribute='date_fields')
        datetimeFields = List(XmlEscapedStr(), attribute='datetime_fields')
        query = XmlEscapedStr()
        user = XmlEscapedStr(required=True)
        token = XmlEscapedStr(required=True, load_only=True)
        clusterSelection = MappedEnum(
            {
                'ORDERED': 'Ordered',
                'RANDOM': 'Random',
            },
            attribute='cluster_selection',
        )
        useQueryForCache = Boolean(attribute='use_query_for_cache')
        forceReadTable = Boolean(attribute='force_read_table')
        rangeExpansionLimit = UInt(attribute='range_expansion_limit')
        inputRowLimit = UInt(attribute='input_row_limit')
        ytSocketTimeout = UInt(attribute='yt_socket_timeout_msec')
        ytConnectionTimeout = UInt(attribute='yt_connection_timeout_msec')
        ytLookupTimeout = UInt(attribute='yt_lookup_timeout_msec')
        ytSelectTimeout = UInt(attribute='yt_select_timeout_msec')
        outputRowLimit = UInt(attribute='output_row_limit')
        ytRetryCount = UInt(attribute='yt_retry_count')

    class PostgresqlSourceSchema(Schema):
        """
        PostgreSQL source schema.
        """

        db = XmlEscapedStr(required=True)
        table = XmlEscapedStr(required=True)
        hosts = List(XmlEscapedStr(), required=True)
        port = GrpcUInt(missing=5432)
        user = XmlEscapedStr(required=True)
        password = XmlEscapedStr(load_only=True)
        invalidateQuery = XmlEscapedStr(attribute='invalidate_query')
        sslMode = MappedEnum(
            {
                'DISABLE': 'disable',
                'ALLOW': 'allow',
                'PREFER': 'prefer',
                'VERIFY_CA': 'verify-ca',
                'VERIFY_FULL': 'verify-full',
            },
            attribute='ssl_mode',
        )

    name = Str(required=True, validate=Regexp('^[a-zA-Z0-9_-]+$'))

    structure = Nested(StructureSchema, required=True)

    layout = Nested(LayoutSchema, required=True)

    fixedLifetime = Int(attribute='fixed_lifetime')
    lifetimeRange = Nested(UIntRangeSchema, attribute='lifetime_range')

    httpSource = Nested(HttpSourceSchema, attribute='http_source')
    mysqlSource = Nested(MysqlSourceSchema, attribute='mysql_source')
    clickhouseSource = Nested(ClickhouseSourceSchema, attribute='clickhouse_source')
    mongodbSource = Nested(MongodbSourceSchema, attribute='mongodb_source')
    ytSource = Nested(YtSourceSchema, attribute='yt_source', internal=True)
    postgresqlSource = Nested(PostgresqlSourceSchema, attribute='postgresql_source')

    @validates_schema(skip_on_field_errors=True)
    def _validates_schema(self, data):
        if {'fixed_lifetime', 'lifetime_range'}.isdisjoint(data):
            raise ValidationError("Dictionary lifetime is not specififed.")

        source_keys = {
            'http_source',
            'mysql_source',
            'clickhouse_source',
            'mongodb_source',
            'yt_source',
            'postgresql_source',
        }
        if source_keys.isdisjoint(data):
            raise ValidationError("Dictionary source is not specififed.")

        layout_type = data['layout']['type']
        is_layout_complex = layout_type in [
            'complex_key_hashed',
            'complex_key_cache',
        ]
        if 'id' in data['structure']:
            if is_layout_complex:
                raise ValidationError("The layout '{0}' cannot be used for numeric keys.".format(layout_type))
        else:
            if not is_layout_complex:
                raise ValidationError("The layout '{0}' cannot be used for complex keys.".format(layout_type))

        if layout_type == 'range_hashed':
            if 'range_min' not in data['structure']:
                raise ValidationError("Range min must be declared for dictionaries with range hashed layout.")
            if 'range_max' not in data['structure']:
                raise ValidationError("Range max must be declared for dictionaries with range hashed layout.")
        else:
            if 'range_min' in data['structure']:
                raise ValidationError(
                    "Range min must not be declared for dictionaries with layout other than range hashed."
                )
            if 'range_max' in data['structure']:
                raise ValidationError(
                    "Range max must not be declared for dictionaries with layout other than range hashed."
                )

        return data


class ClickhouseConfigSchemaV1(ConfigSchemaV1):
    """
    ClickHouse DBMS config schema.
    """

    class MergeTreeSchema(Schema):
        """
        Schema for MergeTree engine configuration.
        """

        # pylint: disable=invalid-name
        replicatedDeduplicationWindow = UInt(attribute='replicated_deduplication_window')
        replicatedDeduplicationWindowSeconds = UInt(attribute='replicated_deduplication_window_seconds')
        partsToDelayInsert = UInt(attribute='parts_to_delay_insert')
        partsToThrowInsert = UInt(attribute='parts_to_throw_insert')
        maxReplicatedMergesInQueue = UInt(attribute='max_replicated_merges_in_queue')
        numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge = UInt(
            attribute='number_of_free_entries_in_pool_to_lower_max_size_of_merge'
        )
        maxBytesToMergeAtMinSpaceInPool = UInt(attribute='max_bytes_to_merge_at_min_space_in_pool')
        maxBytesToMergeAtMaxSpaceInPool = UInt(attribute='max_bytes_to_merge_at_max_space_in_pool')

    class KafkaTopicSchema(Schema):
        """
        Schema for per-topic Kafka settings.
        """

        name = Str(required=True, validate=Regexp('^[a-zA-Z0-9_.-]+$'))
        settings = Nested(KafkaSettingsSchema, required=True)

    class RabbitmqSchema(Schema):
        """
        Schema for RabbitMQ settings.
        """

        username = XmlEscapedStr()
        password = XmlEscapedStr(load_only=True)

    class CompressionSchema(Schema):
        """
        Schema for compression configuration.
        """

        minPartSize = Int(attribute='min_part_size', required=True, validate=Range(min=1))
        minPartSizeRatio = Float(attribute='min_part_size_ratio', required=True, validate=Range(min=0))
        method = UpperToLowerCaseEnum(['LZ4', 'ZSTD'], required=True)

    class GraphiteRollupSchema(Schema):
        """
        Schema for graphite rollup configuration.
        """

        class PatternSchema(Schema):
            """
            Pattern schema.
            """

            class RetentionSchema(Schema):
                """
                Retention schema.
                """

                age = UInt(required=True)
                precision = UInt(required=True)

            regexp = XmlEscapedStr()
            function = XmlEscapedStr(required=True)
            retention = Nested(RetentionSchema, many=True, required=True)

        name = Str(required=True, validate=Regexp('^[a-zA-Z0-9_-]+$'))
        patterns = Nested(PatternSchema, many=True, required=True)

    logLevel = LogLevel(attribute='log_level')
    mergeTree = Nested(MergeTreeSchema, restart=True, attribute='merge_tree')
    kafka = Nested(KafkaSettingsSchema, restart=True, affect_ddl=True)
    kafkaTopics = Nested(KafkaTopicSchema, many=True, attribute='kafka_topics', restart=True, affect_ddl=True)
    rabbitmq = Nested(RabbitmqSchema, restart=True, affect_ddl=True)
    compression = Nested(CompressionSchema, many=True, restart=True)
    dictionaries = Nested(ClickhouseDictionarySchemaV1, many=True, console_hidden=True, affect_ddl=True)
    graphiteRollup = Nested(GraphiteRollupSchema, many=True, attribute='graphite_rollup', restart=True, affect_ddl=True)
    maxConnections = Int(attribute='max_connections', validate=Range(min=10), restart=True)
    maxConcurrentQueries = Int(attribute='max_concurrent_queries', validate=Range(min=60), restart=True)
    keepAliveTimeout = UInt(attribute='keep_alive_timeout', restart=True)
    uncompressedCacheSize = UInt(attribute='uncompressed_cache_size', restart=True)
    markCacheSize = Int(
        attribute='mark_cache_size', validate=Range(min=1), restart=True, variable_default=get_mark_cache_size
    )
    maxTableSizeToDrop = UInt(attribute='max_table_size_to_drop', restart=False)
    builtinDictionariesReloadInterval = UInt(attribute='builtin_dictionaries_reload_interval', console_hidden=True)
    maxPartitionSizeToDrop = UInt(attribute='max_partition_size_to_drop', restart=False)
    timezone = Str(validate=OneOf(all_timezones_set), restart=True)
    geobaseUri = Str(
        attribute='geobase_uri', validate=ClickhouseClusterTraits.geobase_uri.validate, restart=True, affect_ddl=True
    )
    queryLogRetentionSize = UInt(attribute='query_log_retention_size')
    queryLogRetentionTime = MillisecondsMappedToSeconds(attribute='query_log_retention_time')
    queryThreadLogEnabled = Boolean(attribute='query_thread_log_enabled', restart=True)
    queryThreadLogRetentionSize = UInt(attribute='query_thread_log_retention_size')
    queryThreadLogRetentionTime = MillisecondsMappedToSeconds(attribute='query_thread_log_retention_time')
    partLogRetentionSize = UInt(attribute='part_log_retention_size')
    partLogRetentionTime = MillisecondsMappedToSeconds(attribute='part_log_retention_time')
    metricLogEnabled = Boolean(attribute='metric_log_enabled', restart=True)
    metricLogRetentionSize = UInt(attribute='metric_log_retention_size')
    metricLogRetentionTime = MillisecondsMappedToSeconds(attribute='metric_log_retention_time')
    traceLogEnabled = Boolean(attribute='trace_log_enabled', restart=True)
    traceLogRetentionSize = UInt(attribute='trace_log_retention_size')
    traceLogRetentionTime = MillisecondsMappedToSeconds(attribute='trace_log_retention_time')
    textLogEnabled = Boolean(attribute='text_log_enabled', restart=True)
    textLogRetentionSize = UInt(attribute='text_log_retention_size')
    textLogRetentionTime = MillisecondsMappedToSeconds(attribute='text_log_retention_time')
    textLogLevel = LogLevel(attribute='text_log_level', restart=True)
    backgroundPoolSize = UInt(attribute='background_pool_size', validate=Range(min=1), restart=True)
    backgroundSchedulePoolSize = UInt(attribute='background_schedule_pool_size', validate=Range(min=1), restart=True)


class ClickhouseConfigSetSchemaV1(Schema):
    """
    ClickHouse DBMS config set schema.
    """

    effectiveConfig = Nested(ClickhouseConfigSchemaV1, attribute='effective_config', required=True)
    userConfig = Nested(ClickhouseConfigSchemaV1, attribute='user_config')
    defaultConfig = Nested(ClickhouseConfigSchemaV1, attribute='default_config')


class ClickhouseAccessSchemaV1(AccessSchemaV1):
    """
    ClickHouse specific accesses
    """

    metrika = Boolean(attribute='metrika', default=False)


class ClickhouseCloudStorageSchemaV1(Schema):
    """
    Cloud storage schema.
    """

    enabled = Boolean(attribute='enabled', default=False)
    dataCacheEnabled = Boolean(attribute='data_cache_enabled', restart=True)
    dataCacheMaxSize = UInt(attribute='data_cache_max_size', restart=True)
    moveFactor = Float(attribute='move_factor', restart=True)


class ClickhouseClusterConfigSchemaV1(ClusterConfigSchemaV1):
    """
    ClickHouse cluster config schema.
    """

    class ClickhouseSchema(Schema):
        """
        ClickHouse subcluster schema.
        """

        config = Nested(ClickhouseConfigSetSchemaV1, required=True)
        resources = Nested(ResourcesSchemaV1, required=True)

    class ZookeeperSchema(Schema):
        """
        Zookeeper subcluster schema.
        """

        resources = Nested(ResourcesSchemaV1, required=True)

    clickhouse = Nested(ClickhouseSchema, required=True)
    zookeeper = Nested(ZookeeperSchema, required=True)
    backupWindowStart = Nested(TimeOfDay, attribute='backup_window_start')
    access = Nested(ClickhouseAccessSchemaV1, attribute='access')
    serviceAccountId = Str(attribute='service_account_id')
    cloudStorage = Nested(ClickhouseCloudStorageSchemaV1, attribute='cloud_storage')
    mysqlProtocol = Boolean(attribute='mysql_protocol')
    postgresqlProtocol = Boolean(attribute='postgresql_protocol')
    sqlUserManagement = Boolean(attribute='sql_user_management')
    sqlDatabaseManagement = Boolean(attribute='sql_database_management')
    embeddedKeeper = Boolean(attribute='embedded_keeper')


class ClickhouseSchema(Schema):
    """
    ClickHouse subcluster schema.
    """

    config = Nested(ClickhouseConfigSchemaV1)
    resources = Nested(ResourcesUpdateSchemaV1)


class ZookeeperSchema(Schema):
    """
    Zookeeper subcluster schema.
    """

    resources = Nested(ResourcesUpdateSchemaV1)


class ClickhouseClusterConfigSpecSchemaV1(ClusterConfigSpecSchemaV1):
    """
    ClickHouse cluster config spec schema.
    """

    version = Str(restart=True)
    clickhouse = Nested(ClickhouseSchema)
    zookeeper = Nested(ZookeeperSchema)
    backupWindowStart = Nested(TimeOfDay, attribute='backup_window_start')
    access = Nested(ClickhouseAccessSchemaV1, attribute='access')
    cloudStorage = Nested(ClickhouseCloudStorageSchemaV1, attribute='cloud_storage')
    mysqlProtocol = Boolean(attribute='mysql_protocol')
    postgresqlProtocol = Boolean(attribute='postgresql_protocol')
    sqlUserManagement = Boolean(attribute='sql_user_management')
    sqlDatabaseManagement = Boolean(attribute='sql_database_management')
    adminPassword = Str(attribute='admin_password', validate=ClickhouseClusterTraits.password.validate)
    embeddedKeeper = Boolean(attribute='embedded_keeper')


class ClickhouseClusterCreateConfigSpecSchemaV1(ClickhouseClusterConfigSpecSchemaV1):
    """
    ClickHouse cluster create config spec schema.
    """

    clickhouse = Nested(ClickhouseSchema, required=True)


@register_response_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.INFO)
class ClickhouseClusterSchemaV1(ManagedClusterSchemaV1):
    """
    ClickHouse cluster schema.
    """

    config = Nested(ClickhouseClusterConfigSchemaV1, required=True)
    serviceAccountId = Str(attribute='service_account_id')


@register_response_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.LIST)
class ClickhouseListClustersResponseSchemaV1(ListResponseSchemaV1):
    """
    ClickHouse cluster list schema.
    """

    clusters = Nested(ClickhouseClusterSchemaV1, many=True, required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CREATE)
@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.BILLING_CREATE)
class ClickhouseCreateClusterRequestSchemaV1(CreateClusterRequestSchemaV1):
    """
    Schema for create ClickHouse cluster request.
    """

    name = Str(validate=ClickhouseClusterTraits.cluster_name.validate, required=True)
    configSpec = Nested(ClickhouseClusterCreateConfigSpecSchemaV1, attribute='config_spec', required=True)
    databaseSpecs = List(Nested(ClickhouseDatabaseSpecSchemaV1), attribute='database_specs', required=True)
    userSpecs = List(Nested(ClickhouseUserSpecSchemaV1), attribute='user_specs', required=True)
    hostSpecs = List(Nested(ClickhouseHostSpecSchemaV1), validate=Length(min=1), attribute='host_specs', required=True)
    shardName = Str(attribute='shard_name', allow_none=True, validate=ClickhouseClusterTraits.shard_name.validate)
    serviceAccountId = Str(attribute='service_account_id', allow_none=True, validate=ServiceAccountId().validate)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.MODIFY)
class ClickhouseUpdateClusterRequestSchemaV1(UpdateClusterRequestSchemaV1):
    """
    Schema for update ClickHouse cluster request.
    """

    name = Str(validate=ClickhouseClusterTraits.cluster_name.validate)
    configSpec = Nested(ClickhouseClusterConfigSpecSchemaV1, attribute='config_spec')
    serviceAccountId = Str(attribute='service_account_id', allow_none=True, validate=ServiceAccountId().validate)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.RESTORE)
class ClickhouseRestoreClusterRequestSchemaV1(RestoreClusterRequestSchemaV1):
    """
    Schema for restore ClickHouse cluster request.
    """

    configSpec = Nested(ClickhouseClusterCreateConfigSpecSchemaV1, attribute='config_spec', required=True)
    hostSpecs = Nested(
        ClickhouseHostSpecSchemaV1, validate=Length(min=1), attribute='host_specs', required=True, many=True
    )
    serviceAccountId = Str(attribute='service_account_id', allow_none=True, validate=ServiceAccountId().validate)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.ADD_ZOOKEEPER)
class ClickhouseAddClusterZookeeperRequestSchemaV1(Schema):
    """
    Schema for add ZooKeeper request.
    """

    resources = Nested(ResourcesUpdateSchemaV1)
    hostSpecs = List(Nested(ClickhouseHostSpecSchemaV1), attribute='host_specs')


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.CREATE_DICTIONARY)
class ClickhouseCreateExternalDictionarySchemaV1(Schema):
    """
    Schema for create external dictionary request.
    """

    externalDictionary = Nested(ClickhouseDictionarySchemaV1, attribute='external_dictionary', required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE_DICTIONARY)
class ClickhouseDeleteExternalDictionarySchemaV1(Schema):
    """
    Schema for delete external dictionary request.
    """

    externalDictionaryName = Str(attribute='external_dictionary_name', required=True)


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.STOP)
class ClickhouseStopClusterRequestSchemaV1(Schema):
    """
    Schema for stop cluster request.
    """

    pass


@register_request_schema(MY_CLUSTER_TYPE, Resource.CLUSTER, DbaasOperation.DELETE)
class ClickhouseDeleteClusterRequestSchemaV1(Schema):
    """
    Schema for stop cluster request.
    """

    pass
