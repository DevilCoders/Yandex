@grpc_api
Feature: Check that grpc api won't mess cluster pillar

  Background:
    Given default headers

  Scenario: Create cluster with all pillar values set
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "description": "test cluster",
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        },
        "configSpec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                },
                "config": {
                    "logLevel": "TRACE",
                    "mergeTree": {
                        "replicatedDeduplicationWindow": 1,
                        "replicatedDeduplicationWindowSeconds": 1,
                        "partsToDelayInsert": 1,
                        "partsToThrowInsert": 1,
                        "maxReplicatedMergesInQueue": 1,
                        "numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge": 1,
                        "maxBytesToMergeAtMinSpaceInPool": 1,
                        "maxBytesToMergeAtMaxSpaceInPool": 2
                    },
                    "kafka": {
                        "securityProtocol": "SECURITY_PROTOCOL_SSL",
                        "saslMechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                        "saslUsername": "kafka_username",
                        "saslPassword": "kafka_pass"
                    },
                    "kafkaTopics": [
                        {
                            "name": "kafka_topic.names",
                            "settings": {
                                "securityProtocol": "SECURITY_PROTOCOL_SSL",
                                "saslMechanism": "SASL_MECHANISM_GSSAPI",
                                "saslUsername": "topic_username",
                                "saslPassword": "topic_pass"
                            }
                        }
                    ],
                    "rabbitmq": {
                        "username": "rmq_name",
                        "password": "rmq_password"
                    },
                    "compression": [
                        {
                            "minPartSize": 1024,
                            "minPartSizeRatio": 2048,
                            "method": "ZSTD"
                        }
                    ],
                    "graphiteRollup": [
                        {
                            "name": "graphite_name",
                            "patterns": [
                                {
                                    "regexp": "regexp",
                                    "function": "any",
                                    "retention": [
                                        {
                                            "age": 100,
                                            "precision": 100
                                        }
                                    ]
                                }
                            ]
                        }
                    ],
                    "dictionaries": [{
                         "name": "test_dict",
                         "mysqlSource": {
                             "db": "test_db",
                             "table": "test_table",
                             "replicas": [{
                                 "host": "test_host",
                                 "priority": 100
                             }]
                         },
                         "structure": {
                             "id": {
                                 "name": "id"
                             },
                             "attributes": [{
                                 "name": "text",
                                 "type": "String",
                                 "nullValue": ""
                             }]
                         },
                         "layout": {
                             "type": "FLAT"
                         },
                         "fixedLifetime": 300
                     }],
                    "maxConnections": 20,
                    "maxConcurrentQueries": 120,
                    "keepAliveTimeout": 1,
                    "uncompressedCacheSize": 5368709120,
                    "markCacheSize": 5368709121,
                    "maxTableSizeToDrop": 1,
                    "builtinDictionariesReloadInterval": 1,
                    "maxPartitionSizeToDrop": 1,
                    "timezone": "Africa/Conakry",
                    "geobaseUri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                    "queryLogRetentionSize": 1,
                    "queryLogRetentionTime": 1000,
                    "queryThreadLogEnabled": true,
                    "queryThreadLogRetentionSize": 1,
                    "queryThreadLogRetentionTime": 1000,
                    "partLogRetentionSize": 1,
                    "partLogRetentionTime": 1000,
                    "metricLogEnabled": true,
                    "metricLogRetentionSize": 1,
                    "metricLogRetentionTime": 1000,
                    "traceLogEnabled": true,
                    "traceLogRetentionSize": 1,
                    "traceLogRetentionTime": 1000,
                    "textLogEnabled": true,
                    "textLogRetentionSize": 1,
                    "textLogRetentionTime": 1000,
                    "textLogLevel": "TRACE",
                    "backgroundPoolSize": 1,
                    "backgroundSchedulePoolSize": 1
                }
            },
            "access": {
                "metrika": true,
                "serverless": true,
                "webSql": true,
                "dataLens": true,
                "dataTransfer": true,
                "yandexQuery": true
            },
            "cloudStorage": {
                "enabled": false,
                "data_cache_enabled": true,
                "data_cache_max_size": 1024,
                "move_factor": 0.1
            },
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
            "adminPassword": "admin_pass"
        },
        "databaseSpecs": [
            {
                "name": "testdb"
            }
        ],
        "userSpecs": [
            {
                "name": "test",
                "password": "test_password",
                "permissions": [
                    {
                        "databaseName": "testdb"
                    }
                ],
                "settings": {
                    "readonly": 2,
                    "allowDdl": false,

                    "connectTimeout": 20000,
                    "receiveTimeout": 400000,
                    "sendTimeout": 400000,

                    "insertQuorum": 2,
                    "insertQuorumTimeout": 30000,
                    "selectSequentialConsistency": true,
                    "replicationAlterPartitionsSync": 2,

                    "maxReplicaDelayForDistributedQueries": 300000,
                    "fallbackToStaleReplicasForDistributedQueries": true,
                    "distributedProductMode": "DISTRIBUTED_PRODUCT_MODE_ALLOW",
                    "distributedAggregationMemoryEfficient": true,
                    "distributedDdlTaskTimeout": 360000,
                    "skipUnavailableShards": true,

                    "compile": true,
                    "minCountToCompile": 2,
                    "compileExpressions": true,
                    "minCountToCompileExpression": 2,

                    "maxBlockSize": 32768,
                    "minInsertBlockSizeRows": 1048576,
                    "minInsertBlockSizeBytes": 268435456,
                    "maxInsertBlockSize": 524288,
                    "minBytesToUseDirectIo": 52428800,
                    "useUncompressedCache": true,
                    "mergeTreeMaxRowsToUseCache": 1048576,
                    "mergeTreeMaxBytesToUseCache": 2013265920,
                    "mergeTreeMinRowsForConcurrentRead": 163840,
                    "mergeTreeMinBytesForConcurrentRead": 251658240,
                    "maxBytesBeforeExternalGroupBy": 0,
                    "maxBytesBeforeExternalSort": 0,
                    "groupByTwoLevelThreshold": 100000,
                    "groupByTwoLevelThresholdBytes": 100000000,

                    "priority": 1,
                    "maxThreads": 10,
                    "maxMemoryUsage": 21474836480,
                    "maxMemoryUsageForUser": 53687091200,
                    "maxNetworkBandwidth": 1073741824,
                    "maxNetworkBandwidthForUser": 2147483648,

                    "forceIndexByDate": true,
                    "forcePrimaryKey": true,
                    "maxRowsToRead": 1000000,
                    "maxBytesToRead": 2000000,
                    "readOverflowMode": "OVERFLOW_MODE_THROW",
                    "maxRowsToGroupBy": 1000001,
                    "groupByOverflowMode": "GROUP_BY_OVERFLOW_MODE_ANY",
                    "maxRowsToSort": 1000002,
                    "maxBytesToSort": 2000002,
                    "sortOverflowMode": "OVERFLOW_MODE_THROW",
                    "maxResultRows": 1000003,
                    "maxResultBytes": 2000003,
                    "resultOverflowMode": "OVERFLOW_MODE_THROW",
                    "maxRowsInDistinct": 1000004,
                    "maxBytesInDistinct": 2000004,
                    "distinctOverflowMode": "OVERFLOW_MODE_THROW",
                    "maxRowsToTransfer": 1000005,
                    "maxBytesToTransfer": 2000005,
                    "transferOverflowMode": "OVERFLOW_MODE_THROW",
                    "maxExecutionTime": 600000,
                    "timeoutOverflowMode": "OVERFLOW_MODE_THROW",
                    "maxRowsInSet": 1000006,
                    "maxBytesInSet": 2000006,
                    "setOverflowMode": "OVERFLOW_MODE_THROW",
                    "maxRowsInJoin": 1000007,
                    "maxBytesInJoin": 2000007,
                    "joinOverflowMode": "OVERFLOW_MODE_THROW",
                    "maxColumnsToRead": 25,
                    "maxTemporaryColumns": 20,
                    "maxTemporaryNonConstColumns": 15,
                    "maxQuerySize": 524288,
                    "maxAstDepth": 2000,
                    "maxAstElements": 100000,
                    "maxExpandedAstElements": 1000000,
                    "minExecutionSpeed": 1000008,
                    "minExecutionSpeedBytes": 2000008,

                    "inputFormatValuesInterpretExpressions": true,
                    "inputFormatDefaultsForOmittedFields": true,
                    "outputFormatJsonQuote_64bitIntegers": true,
                    "outputFormatJsonQuoteDenormals": true,
                    "lowCardinalityAllowInNativeFormat": true,
                    "emptyResultForAggregationByEmptySet": true,

                    "httpConnectionTimeout": 3000,
                    "httpReceiveTimeout": 1800000,
                    "httpSendTimeout": 1800000,
                    "enableHttpCompression": true,
                    "sendProgressInHttpHeaders": true,
                    "httpHeadersProgressInterval": 1000,
                    "addHttpCorsHeader": true,

                    "quotaMode": "QUOTA_MODE_KEYED",

                    "countDistinctImplementation": "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12",
                    "joinedSubqueryRequiresAlias": true,
                    "joinUseNulls": true,
                    "transformNullIn": true
                },
                "quotas": [
                    {
                        "intervalDuration": 3600000,
                        "queries": 100,
                        "errors": 1000,
                        "resultRows": 1000000,
                        "readRows": 100000000,
                        "executionTime": 100000
                    },
                    {
                        "intervalDuration": 7200000,
                        "queries": 0,
                        "errors": 0,
                        "resultRows": 1000000,
                        "readRows": 100000000,
                        "executionTime": 0
                    }
                ]
            }
        ],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }],
        "networkId": "network1",
        "shardName": "firstShard",
        "serviceAccountId": "sa1",
        "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create ClickHouse cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "maintenance_window": {
            "weekly_maintenance_window": {
                "day": "MON",
                "hour": 1
            }
        },
        "description": "test cluster",
        "config_spec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                },
                "config": {
                    "log_level": "TRACE",
                    "merge_tree": {
                        "replicated_deduplication_window": 1,
                        "replicated_deduplication_window_seconds": 1,
                        "parts_to_delay_insert": 1,
                        "parts_to_throw_insert": 1,
                        "max_replicated_merges_in_queue": 1,
                        "number_of_free_entries_in_pool_to_lower_max_size_of_merge": 1,
                        "max_bytes_to_merge_at_min_space_in_pool": 1,
                        "max_bytes_to_merge_at_max_space_in_pool": 2
                    },
                    "kafka": {
                        "security_protocol": "SECURITY_PROTOCOL_SSL",
                        "sasl_mechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                        "sasl_username": "kafka_username",
                        "sasl_password": "kafka_pass"
                    },
                    "kafka_topics": [
                        {
                            "name": "kafka_topic.name",
                            "settings": {
                                "security_protocol": "SECURITY_PROTOCOL_SSL",
                                "sasl_mechanism": "SASL_MECHANISM_GSSAPI",
                                "sasl_username": "topic_username",
                                "sasl_password": "topic_pass"
                            }
                        }
                    ],
                    "rabbitmq": {
                        "username": "rmq_name",
                        "password": "rmq_password"
                    },
                    "compression": [
                        {
                            "min_part_size": 1024,
                            "min_part_size_ratio": 2048,
                            "method": "ZSTD"
                        }
                    ],
                    "graphite_rollup": [
                        {
                            "name": "graphite_name",
                            "patterns": [
                                {
                                    "regexp": "regexp",
                                    "function": "any",
                                    "retention": [
                                        {
                                            "age": 100,
                                            "precision": 100
                                        }
                                    ]
                                }
                            ]
                        }
                    ],
                    "dictionaries": [{
                        "name": "test_dict",
                        "mysql_source": {
                            "db": "test_db",
                            "table": "test_table",
                            "replicas": [{
                                "host": "test_host",
                                "priority": 100
                            }]
                        },
                        "structure": {
                            "id": {
                                "name": "id"
                            },
                            "attributes": [{
                                "name": "text",
                                "type": "String",
                                "null_value": ""
                            }]
                        },
                        "layout": {
                            "type": "FLAT"
                        },
                        "fixed_lifetime": 300
                    }],
                    "max_connections": 20,
                    "max_concurrent_queries": 120,
                    "keep_alive_timeout": 1,
                    "uncompressed_cache_size": 5368709120,
                    "mark_cache_size": 5368709121,
                    "max_table_size_to_drop": 1,
                    "builtin_dictionaries_reload_interval": 1,
                    "max_partition_size_to_drop": 1,
                    "timezone": "Africa/Conakry",
                    "geobase_uri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                    "query_log_retention_size": 1,
                    "query_log_retention_time": 1000,
                    "query_thread_log_enabled": true,
                    "query_thread_log_retention_size": 1,
                    "query_thread_log_retention_time": 1000,
                    "part_log_retention_size": 1,
                    "part_log_retention_time": 1000,
                    "metric_log_enabled": true,
                    "metric_log_retention_size": 1,
                    "metric_log_retention_time": 1000,
                    "trace_log_enabled": true,
                    "trace_log_retention_size": 1,
                    "trace_log_retention_time": 1000,
                    "text_log_enabled": true,
                    "text_log_retention_size": 1,
                    "text_log_retention_time": 1000,
                    "text_log_level": "TRACE",
                    "background_pool_size": 1,
                    "background_schedule_pool_size": 1
                }
            },
            "access": {
                "metrika": true,
                "serverless": true,
                "web_sql": true,
                "data_lens": true,
                "data_transfer": true,
                "yandex_query": true
            },
            "cloud_storage": {
                "enabled": true,
                "data_cache_enabled": true,
                "data_cache_max_size": 1024,
                "move_factor": 0.1
            },
            "embedded_keeper": false,
            "sql_user_management": false,
            "sql_database_management": false,
            "admin_password": "admin_pass"
        },
        "database_specs": [
            {
                "name": "testdb"
            }
        ],
        "user_specs": [
            {
                "name": "test",
                "password": "test_password",
                "permissions": [
                    {
                        "database_name": "testdb"
                    }
                ],
                "settings": {
                    "readonly": 2,
                    "allow_ddl": false,
                    "connect_timeout": 20000,
                    "receive_timeout": 400000,
                    "send_timeout": 400000,
                    "insert_quorum": 2,
                    "insert_quorum_timeout": 30000,
                    "select_sequential_consistency": true,
                    "replication_alter_partitions_sync": 2,
                    "max_replica_delay_for_distributed_queries": 300000,
                    "fallback_to_stale_replicas_for_distributed_queries": true,
                    "distributed_product_mode": "DISTRIBUTED_PRODUCT_MODE_ALLOW",
                    "distributed_aggregation_memory_efficient": true,
                    "distributed_ddl_task_timeout": 360000,
                    "skip_unavailable_shards": true,
                    "compile": true,
                    "min_count_to_compile": 2,
                    "compile_expressions": true,
                    "min_count_to_compile_expression": 2,
                    "max_block_size": 32768,
                    "min_insert_block_size_rows": 1048576,
                    "min_insert_block_size_bytes": 268435456,
                    "max_insert_block_size": 524288,
                    "min_bytes_to_use_direct_io": 52428800,
                    "use_uncompressed_cache": true,
                    "merge_tree_max_rows_to_use_cache": 1048576,
                    "merge_tree_max_bytes_to_use_cache": 2013265920,
                    "merge_tree_min_rows_for_concurrent_read": 163840,
                    "merge_tree_min_bytes_for_concurrent_read": 251658240,
                    "max_bytes_before_external_group_by": 0,
                    "max_bytes_before_external_sort": 0,
                    "group_by_two_level_threshold": 100000,
                    "group_by_two_level_threshold_bytes": 100000000,
                    "priority": 1,
                    "max_threads": 10,
                    "max_memory_usage": 21474836480,
                    "max_memory_usage_for_user": 53687091200,
                    "max_network_bandwidth": 1073741824,
                    "max_network_bandwidth_for_user": 2147483648,
                    "force_index_by_date": true,
                    "force_primary_key": true,
                    "max_rows_to_read": 1000000,
                    "max_bytes_to_read": 2000000,
                    "read_overflow_mode": "OVERFLOW_MODE_THROW",
                    "max_rows_to_group_by": 1000001,
                    "group_by_overflow_mode": "GROUP_BY_OVERFLOW_MODE_ANY",
                    "max_rows_to_sort": 1000002,
                    "max_bytes_to_sort": 2000002,
                    "sort_overflow_mode": "OVERFLOW_MODE_THROW",
                    "max_result_rows": 1000003,
                    "max_result_bytes": 2000003,
                    "result_overflow_mode": "OVERFLOW_MODE_THROW",
                    "max_rows_in_distinct": 1000004,
                    "max_bytes_in_distinct": 2000004,
                    "distinct_overflow_mode": "OVERFLOW_MODE_THROW",
                    "max_rows_to_transfer": 1000005,
                    "max_bytes_to_transfer": 2000005,
                    "transfer_overflow_mode": "OVERFLOW_MODE_THROW",
                    "max_execution_time": 600000,
                    "timeout_overflow_mode": "OVERFLOW_MODE_THROW",
                    "max_rows_in_set": 1000006,
                    "max_bytes_in_set": 2000006,
                    "set_overflow_mode": "OVERFLOW_MODE_THROW",
                    "max_rows_in_join": 1000007,
                    "max_bytes_in_join": 2000007,
                    "join_overflow_mode": "OVERFLOW_MODE_THROW",
                    "max_columns_to_read": 25,
                    "max_temporary_columns": 20,
                    "max_temporary_non_const_columns": 15,
                    "max_query_size": 524288,
                    "max_ast_depth": 2000,
                    "max_ast_elements": 100000,
                    "max_expanded_ast_elements": 1000000,
                    "min_execution_speed": 1000008,
                    "min_execution_speed_bytes": 2000008,
                    "input_format_values_interpret_expressions": true,
                    "input_format_defaults_for_omitted_fields": true,
                    "output_format_json_quote_64bit_integers": true,
                    "output_format_json_quote_denormals": true,
                    "low_cardinality_allow_in_native_format": true,
                    "empty_result_for_aggregation_by_empty_set": true,
                    "http_connection_timeout": 3000,
                    "http_receive_timeout": 1800000,
                    "http_send_timeout": 1800000,
                    "enable_http_compression": true,
                    "send_progress_in_http_headers": true,
                    "http_headers_progress_interval": 1000,
                    "add_http_cors_header": true,
                    "quota_mode": "QUOTA_MODE_KEYED",
                    "count_distinct_implementation": "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12",
                    "joined_subquery_requires_alias": true,
                    "join_use_nulls": true,
                    "transform_null_in": true
                },
                "quotas": [
                    {
                        "interval_duration": 3600000,
                        "queries": 100,
                        "errors": 1000,
                        "result_rows": 1000000,
                        "read_rows": 100000000,
                        "execution_time": 100000
                    },
                    {
                        "interval_duration": 7200000,
                        "queries": 0,
                        "errors": 0,
                        "result_rows": 1000000,
                        "read_rows": 100000000,
                        "execution_time": 0
                    }
                ]
            }
        ],
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            },
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            },
            {
                "type": "ZOOKEEPER",
                "zone_id": "myt"
            },
            {
                "type": "ZOOKEEPER",
                "zone_id": "myt"
            },
            {
                "type": "ZOOKEEPER",
                "zone_id": "myt"
            }
        ],
        "network_id": "network1",
        "shard_name": "firstShard",
        "service_account_id": "sa1",
        "security_group_ids": [
            "sg_id1",
            "sg_id2"
        ]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id1" acquired and finished by worker
    And we POST "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["firstShard"]
    }
    """
    And "worker_task_id4" acquired and finished by worker
    And we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "access": {
                "webSql": true,
                "dataLens": true,
                "metrika": true,
                "serverless": true,
                "dataTransfer": true,
                "yandexQuery": true
            },
            "backupWindowStart": {
                "hours":   22,
                "minutes": 15,
                "nanos":   100,
                "seconds": 30
            },
            "version": "22.3",
            "serviceAccountId": "sa1",
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
            "clickhouse": {
                "config": {
                    "defaultConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "effectiveConfig": {
                        "logLevel": "TRACE",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 1,
                            "replicatedDeduplicationWindowSeconds": 1,
                            "partsToDelayInsert": 1,
                            "partsToThrowInsert": 1,
                            "maxReplicatedMergesInQueue": 1,
                            "numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge": 1,
                            "maxBytesToMergeAtMinSpaceInPool": 1,
                            "maxBytesToMergeAtMaxSpaceInPool": 2
                        },
                        "kafka": {
                            "securityProtocol": "SECURITY_PROTOCOL_SSL",
                            "saslMechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                            "saslUsername": "kafka_username"
                        },
                        "kafkaTopics": [
                            {
                                "name": "kafka_topic.name",
                                "settings": {
                                    "securityProtocol": "SECURITY_PROTOCOL_SSL",
                                    "saslMechanism": "SASL_MECHANISM_GSSAPI",
                                    "saslUsername": "topic_username"
                                }
                            }
                        ],
                        "rabbitmq": {
                            "username": "rmq_name"
                        },
                        "compression": [
                            {
                                "minPartSize": 1024,
                                "minPartSizeRatio": 2048,
                                "method": "ZSTD"
                            }
                        ],
                        "dictionaries": [{
                            "name": "test_dict",
                            "mysqlSource": {
                                "db": "test_db",
                                "table": "test_table",
                                "replicas": [{
                                    "host": "test_host",
                                    "priority": 100
                                }]
                            },
                            "structure": {
                                "id": {
                                    "name": "id"
                                },
                                "attributes": [{
                                    "name": "text",
                                    "type": "String",
                                    "hierarchical": false,
                                    "injective": false,
                                    "nullValue": ""
                                }]
                            },
                            "layout": {
                                "type": "FLAT"
                            },
                            "fixedLifetime": 300
                        }],
                        "graphiteRollup": [
                            {
                                "name": "graphite_name",
                                "patterns": [
                                    {
                                        "regexp": "regexp",
                                        "function": "any",
                                        "retention": [
                                            {
                                                "age": 100,
                                                "precision": 100
                                            }
                                        ]
                                    }
                                ]
                            }
                        ],
                        "maxConnections": 20,
                        "maxConcurrentQueries": 120,
                        "keepAliveTimeout": 1,
                        "uncompressedCacheSize": 5368709120,
                        "markCacheSize": 5368709121,
                        "maxTableSizeToDrop": 1,
                        "builtinDictionariesReloadInterval": 1,
                        "maxPartitionSizeToDrop": 1,
                        "timezone": "Africa/Conakry",
                        "geobaseUri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                        "queryLogRetentionSize": 1,
                        "queryLogRetentionTime": 1000,
                        "queryThreadLogEnabled": true,
                        "queryThreadLogRetentionSize": 1,
                        "queryThreadLogRetentionTime": 1000,
                        "partLogRetentionSize": 1,
                        "partLogRetentionTime": 1000,
                        "metricLogEnabled": true,
                        "metricLogRetentionSize": 1,
                        "metricLogRetentionTime": 1000,
                        "traceLogEnabled": true,
                        "traceLogRetentionSize": 1,
                        "traceLogRetentionTime": 1000,
                        "textLogEnabled": true,
                        "textLogRetentionSize": 1,
                        "textLogRetentionTime": 1000,
                        "textLogLevel": "TRACE",
                        "backgroundPoolSize": 1,
                        "backgroundSchedulePoolSize": 1
                    },
                    "userConfig": {
                        "logLevel": "TRACE",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 1,
                            "replicatedDeduplicationWindowSeconds": 1,
                            "partsToDelayInsert": 1,
                            "partsToThrowInsert": 1,
                            "maxReplicatedMergesInQueue": 1,
                            "numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge": 1,
                            "maxBytesToMergeAtMinSpaceInPool": 1,
                            "maxBytesToMergeAtMaxSpaceInPool": 2
                        },
                        "kafka": {
                            "securityProtocol": "SECURITY_PROTOCOL_SSL",
                            "saslMechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                            "saslUsername": "kafka_username"
                        },
                        "kafkaTopics": [
                            {
                                "name": "kafka_topic.name",
                                "settings": {
                                    "securityProtocol": "SECURITY_PROTOCOL_SSL",
                                    "saslMechanism": "SASL_MECHANISM_GSSAPI",
                                    "saslUsername": "topic_username"
                                }
                            }
                        ],
                        "rabbitmq": {
                            "username": "rmq_name"
                        },
                        "compression": [
                            {
                                "minPartSize": 1024,
                                "minPartSizeRatio": 2048,
                                "method": "ZSTD"
                            }
                        ],
                        "graphiteRollup": [
                            {
                                "name": "graphite_name",
                                "patterns": [
                                    {
                                        "regexp": "regexp",
                                        "function": "any",
                                        "retention": [
                                            {
                                                "age": 100,
                                                "precision": 100
                                            }
                                        ]
                                    }
                                ]
                            }
                        ],
                        "dictionaries": [{
                            "name": "test_dict",
                            "mysqlSource": {
                                "db": "test_db",
                                "table": "test_table",
                                "replicas": [{
                                    "host": "test_host",
                                    "priority": 100
                                }]
                            },
                            "structure": {
                                "id": {
                                    "name": "id"
                                },
                                "attributes": [{
                                    "name": "text",
                                    "type": "String",
                                    "nullValue": ""
                                }]
                            },
                            "layout": {
                                "type": "FLAT"
                            },
                            "fixedLifetime": 300
                        }],
                        "maxConnections": 20,
                        "maxConcurrentQueries": 120,
                        "keepAliveTimeout": 1,
                        "uncompressedCacheSize": 5368709120,
                        "markCacheSize": 5368709121,
                        "maxTableSizeToDrop": 1,
                        "builtinDictionariesReloadInterval": 1,
                        "maxPartitionSizeToDrop": 1,
                        "timezone": "Africa/Conakry",
                        "geobaseUri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                        "queryLogRetentionSize": 1,
                        "queryLogRetentionTime": 1000,
                        "queryThreadLogEnabled": true,
                        "queryThreadLogRetentionSize": 1,
                        "queryThreadLogRetentionTime": 1000,
                        "partLogRetentionSize": 1,
                        "partLogRetentionTime": 1000,
                        "metricLogEnabled": true,
                        "metricLogRetentionSize": 1,
                        "metricLogRetentionTime": 1000,
                        "traceLogEnabled": true,
                        "traceLogRetentionSize": 1,
                        "traceLogRetentionTime": 1000,
                        "textLogEnabled": true,
                        "textLogRetentionSize": 1,
                        "textLogRetentionTime": 1000,
                        "textLogLevel": "TRACE",
                        "backgroundPoolSize": 1,
                        "backgroundSchedulePoolSize": 1
                    }
                },
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                }
            },
            "cloudStorage": {
                "enabled": true,
                "settings": {
                    "dataCacheEnabled": true,
                    "dataCacheMaxSize": 1024,
                    "moveFactor": 0.1
                }
            }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "UNKNOWN",
        "id": "cid1",
        "labels": {},
        "monitoring": [
            {
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm.yandex-team.ru/template/panel/dbaas_clickhouse_metrics/cid=cid1",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid1&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-clickhouse",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/clickhouse_cluster/cid1?section=monitoring",
                "name": "Console"
            }
        ],
        "name": "test",
        "networkId": "network1",
        "status": "RUNNING",
        "plannedOperation": null,
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "config": {
            "access": {
                "web_sql": true,
                "data_lens": true,
                "metrika": true,
                "serverless": true,
                "data_transfer": true,
                "yandex_query": true
            },
            "backup_window_start": {
                "hours":   22,
                "minutes": 15,
                "nanos":   100,
                "seconds": 30
            },
            "version": "22.3",
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "sql_user_management": false,
            "sql_database_management": false,
            "embedded_keeper": false,
            "clickhouse": {
                "config": {
                    "default_config": {
                        "background_pool_size": null,
                        "background_schedule_pool_size": null,
                        "builtin_dictionaries_reload_interval": "3600",
                        "compression": [],
                        "dictionaries": [],
                        "geobase_uri": "",
                        "graphite_rollup": [],
                        "kafka": {
                            "security_protocol": "SECURITY_PROTOCOL_UNSPECIFIED",
                            "sasl_mechanism": "SASL_MECHANISM_UNSPECIFIED",
                            "sasl_username": "",
                            "sasl_password": ""
                        },
                        "kafka_topics": [],
                        "rabbitmq": {
                            "username": "",
                            "password": ""
                        },
                        "keep_alive_timeout": "3",
                        "log_level": "INFORMATION",
                        "mark_cache_size": "5368709120",
                        "max_concurrent_queries": "500",
                        "max_connections": "4096",
                        "max_table_size_to_drop": "53687091200",
                        "max_partition_size_to_drop": "53687091200",
                        "timezone": "Europe/Moscow",
                        "merge_tree": {
                            "enable_mixed_granularity_parts": null,
                            "max_bytes_to_merge_at_max_space_in_pool": null,
                            "max_bytes_to_merge_at_min_space_in_pool": null,
                            "max_replicated_merges_in_queue": null,
                            "number_of_free_entries_in_pool_to_lower_max_size_of_merge": null,
                            "parts_to_delay_insert": null,
                            "parts_to_throw_insert": null,
                            "replicated_deduplication_window": "100",
                            "replicated_deduplication_window_seconds": "604800"
                        },
                        "metric_log_enabled": null,
                        "metric_log_retention_size": null,
                        "metric_log_retention_time": null,
                        "part_log_retention_size": null,
                        "part_log_retention_time": null,
                        "query_log_retention_size": null,
                        "query_log_retention_time": null,
                        "query_thread_log_enabled": null,
                        "query_thread_log_retention_size": null,
                        "query_thread_log_retention_time": null,
                        "text_log_enabled": null,
                        "text_log_level": "LOG_LEVEL_UNSPECIFIED",
                        "text_log_retention_size": null,
                        "text_log_retention_time": null,
                        "trace_log_enabled": null,
                        "trace_log_retention_size": null,
                        "trace_log_retention_time": null,
                        "uncompressed_cache_size": "8589934592"
                    },
                    "effective_config": {
                        "log_level": "TRACE",
                        "merge_tree": {
                            "enable_mixed_granularity_parts": true,
                            "replicated_deduplication_window": "1",
                            "replicated_deduplication_window_seconds": "1",
                            "parts_to_delay_insert": "1",
                            "parts_to_throw_insert": "1",
                            "max_replicated_merges_in_queue": "1",
                            "number_of_free_entries_in_pool_to_lower_max_size_of_merge": "1",
                            "max_bytes_to_merge_at_min_space_in_pool": "1",
                            "max_bytes_to_merge_at_max_space_in_pool": "2"
                        },
                        "kafka": {
                            "security_protocol": "SECURITY_PROTOCOL_SSL",
                            "sasl_mechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                            "sasl_username": "kafka_username",
                            "sasl_password": ""
                        },
                        "kafka_topics": [
                            {
                                "name": "kafka_topic.name",
                                "settings": {
                                    "security_protocol": "SECURITY_PROTOCOL_SSL",
                                    "sasl_mechanism": "SASL_MECHANISM_GSSAPI",
                                    "sasl_username": "topic_username",
                                    "sasl_password": ""
                                }
                            }
                        ],
                        "rabbitmq": {
                            "username": "rmq_name",
                            "password": ""
                        },
                        "compression": [
                            {
                                "min_part_size": "1024",
                                "min_part_size_ratio": 2048,
                                "method": "ZSTD"
                            }
                        ],
                        "dictionaries": [{
                            "fixed_lifetime": "300",
                            "layout": {
                                "size_in_cells": "0",
                                "type": "FLAT"
                            },
                            "mysql_source": {
                                "db": "test_db",
                                "table": "test_table",
                                "port": "0",
                                "user": "",
                                "password": "",
                                "replicas": [{
                                    "host": "test_host",
                                    "priority": "100",
                                    "port": "0",
                                    "user": "",
                                    "password": ""
                                }],
                                "where": "",
                                "invalidate_query": ""
                            },
                            "name": "test_dict",
                            "structure": {
                                "attributes": [{
                                    "name": "text",
                                    "type": "String",
                                    "expression": "",
                                    "hierarchical": false,
                                    "injective": false,
                                    "null_value": ""
                                }],
                                "id": {
                                    "name": "id"
                                },
                                "key": null,
                                "range_max": null,
                                "range_min": null
                            }
                        }],
                        "graphite_rollup": [
                            {
                                "name": "graphite_name",
                                "patterns": [
                                    {
                                        "regexp": "regexp",
                                        "function": "any",
                                        "retention": [
                                            {
                                                "age": "100",
                                                "precision": "100"
                                            }
                                        ]
                                    }
                                ]
                            }
                        ],
                        "max_connections": "20",
                        "max_concurrent_queries": "120",
                        "keep_alive_timeout": "1",
                        "uncompressed_cache_size": "5368709120",
                        "mark_cache_size": "5368709121",
                        "max_table_size_to_drop": "1",
                        "builtin_dictionaries_reload_interval": "1",
                        "max_partition_size_to_drop": "1",
                        "timezone": "Africa/Conakry",
                        "geobase_uri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                        "query_log_retention_size": "1",
                        "query_log_retention_time": "1000",
                        "query_thread_log_enabled": true,
                        "query_thread_log_retention_size": "1",
                        "query_thread_log_retention_time": "1000",
                        "part_log_retention_size": "1",
                        "part_log_retention_time": "1000",
                        "metric_log_enabled": true,
                        "metric_log_retention_size": "1",
                        "metric_log_retention_time": "1000",
                        "trace_log_enabled": true,
                        "trace_log_retention_size": "1",
                        "trace_log_retention_time": "1000",
                        "text_log_enabled": true,
                        "text_log_retention_size": "1",
                        "text_log_retention_time": "1000",
                        "text_log_level": "TRACE",
                        "background_pool_size": "1",
                        "background_schedule_pool_size": "1"
                    },
                    "user_config": {
                        "log_level": "TRACE",
                        "merge_tree": {
                            "replicated_deduplication_window": "1",
                            "replicated_deduplication_window_seconds": "1",
                            "parts_to_delay_insert": "1",
                            "parts_to_throw_insert": "1",
                            "max_replicated_merges_in_queue": "1",
                            "number_of_free_entries_in_pool_to_lower_max_size_of_merge": "1",
                            "max_bytes_to_merge_at_min_space_in_pool": "1",
                            "max_bytes_to_merge_at_max_space_in_pool": "2",
                            "enable_mixed_granularity_parts": true
                        },
                        "kafka": {
                            "security_protocol": "SECURITY_PROTOCOL_SSL",
                            "sasl_mechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                            "sasl_username": "kafka_username",
                            "sasl_password": ""
                        },
                        "kafka_topics": [
                            {
                                "name": "kafka_topic.name",
                                "settings": {
                                    "security_protocol": "SECURITY_PROTOCOL_SSL",
                                    "sasl_mechanism": "SASL_MECHANISM_GSSAPI",
                                    "sasl_username": "topic_username",
                                    "sasl_password": ""
                                }
                            }
                        ],
                        "rabbitmq": {
                            "username": "rmq_name",
                            "password": ""
                        },
                        "compression": [
                            {
                                "min_part_size": "1024",
                                "min_part_size_ratio": 2048,
                                "method": "ZSTD"
                            }
                        ],
                        "dictionaries": [{
                            "fixed_lifetime": "300",
                            "layout": {
                                "size_in_cells": "0",
                                "type": "FLAT"
                            },
                            "mysql_source": {
                                "db": "test_db",
                                "table": "test_table",
                                "port": "0",
                                "user": "",
                                "password": "",
                                "replicas": [{
                                    "host": "test_host",
                                    "priority": "100",
                                    "port": "0",
                                    "user": "",
                                    "password": ""
                                }],
                                "where": "",
                                "invalidate_query": ""
                            },
                            "name": "test_dict",
                            "structure": {
                                "attributes": [{
                                    "name": "text",
                                    "type": "String",
                                    "expression": "",
                                    "hierarchical": false,
                                    "injective": false,
                                    "null_value": ""
                                }],
                                "id": {
                                    "name": "id"
                                },
                                "key": null,
                                "range_max": null,
                                "range_min": null
                            }
                        }],
                        "graphite_rollup": [
                            {
                                "name": "graphite_name",
                                "patterns": [
                                    {
                                        "regexp": "regexp",
                                        "function": "any",
                                        "retention": [
                                            {
                                                "age": "100",
                                                "precision": "100"
                                            }
                                        ]
                                    }
                                ]
                            }
                        ],
                        "max_connections": "20",
                        "max_concurrent_queries": "120",
                        "keep_alive_timeout": "1",
                        "uncompressed_cache_size": "5368709120",
                        "mark_cache_size": "5368709121",
                        "max_table_size_to_drop": "1",
                        "builtin_dictionaries_reload_interval": "1",
                        "max_partition_size_to_drop": "1",
                        "timezone": "Africa/Conakry",
                        "geobase_uri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                        "query_log_retention_size": "1",
                        "query_log_retention_time": "1000",
                        "query_thread_log_enabled": true,
                        "query_thread_log_retention_size": "1",
                        "query_thread_log_retention_time": "1000",
                        "part_log_retention_size": "1",
                        "part_log_retention_time": "1000",
                        "metric_log_enabled": true,
                        "metric_log_retention_size": "1",
                        "metric_log_retention_time": "1000",
                        "trace_log_enabled": true,
                        "trace_log_retention_size": "1",
                        "trace_log_retention_time": "1000",
                        "text_log_enabled": true,
                        "text_log_retention_size": "1",
                        "text_log_retention_time": "1000",
                        "text_log_level": "TRACE",
                        "background_pool_size": "1",
                        "background_schedule_pool_size": "1"
                    }
                },
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": "10737418240"
                }
            },
            "zookeeper": {
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                }
            },
            "cloud_storage": {
                "enabled": true,
                "data_cache_enabled": true,
                "data_cache_max_size": "1024",
                "move_factor": 0.1
            }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folder_id": "folder1",
        "health": "HEALTH_UNKNOWN",
        "id": "cid1",
        "labels": {},
        "monitoring": [
            {
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm/cid=cid1",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon/cid=cid1&fExtID=folder1",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console/cid=cid1&fExtID=folder1",
                "name": "Console"
            }
        ],
        "name": "test",
        "network_id": "network1",
        "status": "RUNNING",
        "planned_operation": null,
        "service_account_id": "sa1",
        "maintenance_window": {
            "weekly_maintenance_window": {
                "day": "MON",
                "hour": "1"
            }
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/users/test"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test",
        "permissions": [
            {
                "databaseName": "testdb"
            }
        ],
        "settings": {
            "readonly": 2,
            "allowDdl": false,

            "connectTimeout": 20000,
            "receiveTimeout": 400000,
            "sendTimeout": 400000,

            "insertQuorum": 2,
            "insertQuorumTimeout": 30000,
            "selectSequentialConsistency": true,
            "replicationAlterPartitionsSync": 2,

            "maxReplicaDelayForDistributedQueries": 300000,
            "fallbackToStaleReplicasForDistributedQueries": true,
            "distributedProductMode": "DISTRIBUTED_PRODUCT_MODE_ALLOW",
            "distributedAggregationMemoryEfficient": true,
            "distributedDdlTaskTimeout": 360000,
            "skipUnavailableShards": true,

            "compile": true,
            "minCountToCompile": 2,
            "compileExpressions": true,
            "minCountToCompileExpression": 2,

            "maxBlockSize": 32768,
            "minInsertBlockSizeRows": 1048576,
            "minInsertBlockSizeBytes": 268435456,
            "maxInsertBlockSize": 524288,
            "minBytesToUseDirectIo": 52428800,
            "useUncompressedCache": true,
            "mergeTreeMaxRowsToUseCache": 1048576,
            "mergeTreeMaxBytesToUseCache": 2013265920,
            "mergeTreeMinRowsForConcurrentRead": 163840,
            "mergeTreeMinBytesForConcurrentRead": 251658240,
            "maxBytesBeforeExternalGroupBy": 0,
            "maxBytesBeforeExternalSort": 0,
            "groupByTwoLevelThreshold": 100000,
            "groupByTwoLevelThresholdBytes": 100000000,

            "priority": 1,
            "maxThreads": 10,
            "maxMemoryUsage": 21474836480,
            "maxMemoryUsageForUser": 53687091200,
            "maxNetworkBandwidth": 1073741824,
            "maxNetworkBandwidthForUser": 2147483648,

            "forceIndexByDate": true,
            "forcePrimaryKey": true,
            "maxRowsToRead": 1000000,
            "maxBytesToRead": 2000000,
            "readOverflowMode": "OVERFLOW_MODE_THROW",
            "maxRowsToGroupBy": 1000001,
            "groupByOverflowMode": "GROUP_BY_OVERFLOW_MODE_ANY",
            "maxRowsToSort": 1000002,
            "maxBytesToSort": 2000002,
            "sortOverflowMode": "OVERFLOW_MODE_THROW",
            "maxResultRows": 1000003,
            "maxResultBytes": 2000003,
            "resultOverflowMode": "OVERFLOW_MODE_THROW",
            "maxRowsInDistinct": 1000004,
            "maxBytesInDistinct": 2000004,
            "distinctOverflowMode": "OVERFLOW_MODE_THROW",
            "maxRowsToTransfer": 1000005,
            "maxBytesToTransfer": 2000005,
            "transferOverflowMode": "OVERFLOW_MODE_THROW",
            "maxExecutionTime": 600000,
            "timeoutOverflowMode": "OVERFLOW_MODE_THROW",
            "maxRowsInSet": 1000006,
            "maxBytesInSet": 2000006,
            "setOverflowMode": "OVERFLOW_MODE_THROW",
            "maxRowsInJoin": 1000007,
            "maxBytesInJoin": 2000007,
            "joinOverflowMode": "OVERFLOW_MODE_THROW",
            "maxColumnsToRead": 25,
            "maxTemporaryColumns": 20,
            "maxTemporaryNonConstColumns": 15,
            "maxQuerySize": 524288,
            "maxAstDepth": 2000,
            "maxAstElements": 100000,
            "maxExpandedAstElements": 1000000,
            "minExecutionSpeed": 1000008,
            "minExecutionSpeedBytes": 2000008,

            "inputFormatValuesInterpretExpressions": true,
            "inputFormatDefaultsForOmittedFields": true,
            "outputFormatJsonQuote_64bitIntegers": true,
            "outputFormatJsonQuoteDenormals": true,
            "lowCardinalityAllowInNativeFormat": true,
            "emptyResultForAggregationByEmptySet": true,

            "httpConnectionTimeout": 3000,
            "httpReceiveTimeout": 1800000,
            "httpSendTimeout": 1800000,
            "enableHttpCompression": true,
            "sendProgressInHttpHeaders": true,
            "httpHeadersProgressInterval": 1000,
            "addHttpCorsHeader": true,

            "quotaMode": "QUOTA_MODE_KEYED",

            "countDistinctImplementation": "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12",
            "joinedSubqueryRequiresAlias": true,
            "joinUseNulls": true,
            "transformNullIn": true
        },
        "quotas": [
            {
                "intervalDuration": 3600000,
                "queries": 100,
                "errors": 1000,
                "resultRows": 1000000,
                "readRows": 100000000,
                "executionTime": 100000
            },
            {
                "intervalDuration": 7200000,
                "queries": 0,
                "errors": 0,
                "resultRows": 1000000,
                "readRows": 100000000,
                "executionTime": 0
            }
        ]
    }
    """
    When we GET "/api/v1.0/config/myt-4.df.cloud.yandex.net"
    Then we get response with status 200
    And body at path "$.data" contains
    """
    {
        "backup": {
            "sleep": 7200,
            "start": {
                "hours": 22,
                "minutes": 15,
                "nanos": 100,
                "seconds": 30
            },
            "use_backup_service": false
        },
        "access": {
            "data_lens": true,
            "metrika": true,
            "serverless": true,
            "web_sql": true,
            "data_transfer": true,
            "yandex_query": true
        },
        "clickhouse": {
            "admin_password": {
                "hash": {
                    "data": "fc618395b55f38756c9a98ec058c267830f84d448ca4e0b893951784b163b617",
                    "encryption_version": 0
                },
                "password": {
                    "data": "admin_pass",
                    "encryption_version": 0
                }
            },
            "ch_version": "22.3.2.2",
            "config": {
                "background_pool_size": 1,
                "background_schedule_pool_size": 1,
                "builtin_dictionaries_reload_interval": 1,
                "compression": [
                    {
                        "method": "zstd",
                        "min_part_size": 1024,
                        "min_part_size_ratio": 2048
                    }
                ],
                "dictionaries": [{
                    "fixed_lifetime": 300,
                    "layout": {
                        "type": "flat"
                    },
                    "mysql_source": {
                        "db": "test_db",
                        "replicas": [{
                            "host": "test_host",
                            "priority": 100
                        }],
                        "table": "test_table"
                    },
                    "name": "test_dict",
                    "structure": {
                        "attributes": [{
                             "hierarchical": false,
                             "injective": false,
                             "name": "text",
                             "type": "String",
                             "null_value": ""
                        }],
                        "id": {
                            "name": "id"
                        }
                    }
                }],
                "geobase_uri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                "graphite_rollup": [
                    {
                        "name": "graphite_name",
                        "patterns": [
                            {
                                "regexp": "regexp",
                                "function": "any",
                                "retention": [
                                    {
                                        "age": 100,
                                        "precision": 100
                                    }
                                ]
                            }
                        ]
                    }
                ],
                "kafka": {
                    "sasl_mechanism": "SCRAM-SHA-512",
                    "sasl_password": {
                        "data": "kafka_pass",
                        "encryption_version": 0
                    },
                    "sasl_username": "kafka_username",
                    "security_protocol": "SSL"
                },
                "kafka_topics": [
                    {
                        "name": "kafka_topic.name",
                        "settings": {
                            "sasl_mechanism": "GSSAPI",
                            "sasl_password": {
                                "data": "topic_pass",
                                "encryption_version": 0
                            },
                            "sasl_username": "topic_username",
                            "security_protocol": "SSL"
                        }
                    }
                ],
                "keep_alive_timeout": 1,
                "log_level": "trace",
                "mark_cache_size": 5368709121,
                "max_concurrent_queries": 120,
                "max_connections": 20,
                "max_partition_size_to_drop": 1,
                "max_table_size_to_drop": 1,
                "merge_tree": {
                    "enable_mixed_granularity_parts": true,
                    "max_bytes_to_merge_at_min_space_in_pool": 1,
                    "max_bytes_to_merge_at_max_space_in_pool": 2,
                    "max_replicated_merges_in_queue": 1,
                    "number_of_free_entries_in_pool_to_lower_max_size_of_merge": 1,
                    "parts_to_delay_insert": 1,
                    "parts_to_throw_insert": 1,
                    "replicated_deduplication_window": 1,
                    "replicated_deduplication_window_seconds": 1
                },
                "metric_log_enabled": true,
                "metric_log_retention_size": 1,
                "metric_log_retention_time": 1,
                "part_log_retention_size": 1,
                "part_log_retention_time": 1,
                "query_log_retention_size": 1,
                "query_log_retention_time": 1,
                "query_thread_log_enabled": true,
                "query_thread_log_retention_size": 1,
                "query_thread_log_retention_time": 1,
                "rabbitmq": {
                    "password": {
                        "data": "rmq_password",
                        "encryption_version": 0
                    },
                    "username": "rmq_name"
                },
                "text_log_enabled": true,
                "text_log_level": "trace",
                "text_log_retention_size": 1,
                "text_log_retention_time": 1,
                "timezone": "Africa/Conakry",
                "trace_log_enabled": true,
                "trace_log_retention_size": 1,
                "trace_log_retention_time": 1,
                "uncompressed_cache_size": 5368709120
            },
            "databases": [
                "testdb"
            ],
            "format_schemas": {
                 "test_schema": {
                       "type": "protobuf",
                       "uri":  "https://bucket1.storage.yandexcloud.net/test_schema.proto"
                 }
            },
            "interserver_credentials": {
                "password": {
                    "data": "dummy",
                    "encryption_version": 0
                },
                "user": "interserver"
            },
            "models": {
                "test_model": {
                    "type": "catboost",
                    "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
                }
            },
            "embedded_keeper": false,
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "shard_groups": {
                "test_group": {
                     "description": "Test shard group",
                     "shard_names": ["firstShard"]
                }
            },
            "shards": {
                "shard_id1": {
                    "weight": 100
                }
            },
            "sql_database_management": false,
            "sql_user_management": false,
            "system_users": {
                "mdb_backup_admin": {
                    "hash": {
                        "data": "b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "dummy",
                        "encryption_version": 0
                    }
                }
            },
            "user_management_v2": true,
            "users": {
                "test": {
                    "databases": {
                        "testdb": {}
                    },
                    "hash": {
                        "data": "10a6e6cc8311a3e2bcc09bf6c199adecd5dd59408c343e926b129c4914f3cb01",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "test_password",
                        "encryption_version": 0
                    },
                    "quotas": [
                        {
                            "interval_duration": 3600,
                            "queries": 100,
                            "errors": 1000,
                            "result_rows": 1000000,
                            "read_rows": 100000000,
                            "execution_time": 100
                        },
                        {
                            "interval_duration": 7200,
                            "queries": 0,
                            "errors": 0,
                            "result_rows": 1000000,
                            "read_rows": 100000000,
                            "execution_time": 0
                        }
                    ],
                    "settings": {
                        "readonly": 2,
                        "allow_ddl": 0,

                        "connect_timeout": 20,
                        "receive_timeout": 400,
                        "send_timeout": 400,

                        "insert_quorum": 2,
                        "insert_quorum_timeout": 30,
                        "select_sequential_consistency": 1,
                        "replication_alter_partitions_sync": 2,

                        "max_replica_delay_for_distributed_queries": 300,
                        "fallback_to_stale_replicas_for_distributed_queries": 1,
                        "distributed_product_mode": "allow",
                        "distributed_aggregation_memory_efficient": 1,
                        "distributed_ddl_task_timeout": 360,
                        "skip_unavailable_shards": 1,

                        "compile": 1,
                        "min_count_to_compile": 2,
                        "compile_expressions": 1,
                        "min_count_to_compile_expression": 2,

                        "max_block_size": 32768,
                        "min_insert_block_size_rows": 1048576,
                        "min_insert_block_size_bytes": 268435456,
                        "max_insert_block_size": 524288,
                        "min_bytes_to_use_direct_io": 52428800,
                        "use_uncompressed_cache": 1,
                        "merge_tree_max_rows_to_use_cache": 1048576,
                        "merge_tree_max_bytes_to_use_cache": 2013265920,
                        "merge_tree_min_rows_for_concurrent_read": 163840,
                        "merge_tree_min_bytes_for_concurrent_read": 251658240,
                        "max_bytes_before_external_group_by": 0,
                        "max_bytes_before_external_sort": 0,
                        "group_by_two_level_threshold": 100000,
                        "group_by_two_level_threshold_bytes": 100000000,

                        "priority": 1,
                        "max_threads": 10,
                        "max_memory_usage": 21474836480,
                        "max_memory_usage_for_user": 53687091200,
                        "max_network_bandwidth": 1073741824,
                        "max_network_bandwidth_for_user": 2147483648,

                        "force_index_by_date": 1,
                        "force_primary_key": 1,
                        "max_rows_to_read": 1000000,
                        "max_bytes_to_read": 2000000,
                        "read_overflow_mode": "throw",
                        "max_rows_to_group_by": 1000001,
                        "group_by_overflow_mode": "any",
                        "max_rows_to_sort": 1000002,
                        "max_bytes_to_sort": 2000002,
                        "sort_overflow_mode": "throw",
                        "max_result_rows": 1000003,
                        "max_result_bytes": 2000003,
                        "result_overflow_mode": "throw",
                        "max_rows_in_distinct": 1000004,
                        "max_bytes_in_distinct": 2000004,
                        "distinct_overflow_mode": "throw",
                        "max_rows_to_transfer": 1000005,
                        "max_bytes_to_transfer": 2000005,
                        "transfer_overflow_mode": "throw",
                        "max_execution_time": 600,
                        "timeout_overflow_mode": "throw",
                        "max_rows_in_set": 1000006,
                        "max_bytes_in_set": 2000006,
                        "set_overflow_mode": "throw",
                        "max_rows_in_join": 1000007,
                        "max_bytes_in_join": 2000007,
                        "join_overflow_mode": "throw",
                        "max_columns_to_read": 25,
                        "max_temporary_columns": 20,
                        "max_temporary_non_const_columns": 15,
                        "max_query_size": 524288,
                        "max_ast_depth": 2000,
                        "max_ast_elements": 100000,
                        "max_expanded_ast_elements": 1000000,
                        "min_execution_speed": 1000008,
                        "min_execution_speed_bytes": 2000008,

                        "input_format_values_interpret_expressions": 1,
                        "input_format_defaults_for_omitted_fields": 1,
                        "output_format_json_quote_64bit_integers": 1,
                        "output_format_json_quote_denormals": 1,
                        "low_cardinality_allow_in_native_format": 1,
                        "empty_result_for_aggregation_by_empty_set": 1,

                        "http_connection_timeout": 3,
                        "http_receive_timeout": 1800,
                        "http_send_timeout": 1800,
                        "enable_http_compression": 1,
                        "send_progress_in_http_headers": 1,
                        "http_headers_progress_interval_ms": 1000,
                        "add_http_cors_header": 1,

                        "quota_mode": "keyed",

                        "count_distinct_implementation": "uniqHLL12",
                        "joined_subquery_requires_alias": 1,
                        "join_use_nulls": 1,
                        "transform_null_in": 1
                    }
                }
            },
            "zk_users": {
                "clickhouse": {
                    "password": {
                        "data": "dummy",
                        "encryption_version": 0
                    }
                }
            }
        },
        "clickhouse default pillar": true,
        "cloud_storage": {
            "enabled": true,
            "s3": {
                "bucket": "cloud-storage-cid1"
            },
            "settings": {
                "data_cache_enabled": true,
                "data_cache_max_size": 1024,
                "move_factor": 0.1
            }
        },
        "cluster_private_key": {
            "data": "1",
            "encryption_version": 0
        },
        "default pillar": true,
        "runlist": [
            "components.clickhouse_cluster"
        ],
        "s3_bucket": "yandexcloud-dbaas-cid1",
        "service_account_id": "sa1",
        "unmanaged": {
            "enable_zk_tls": true
        },
        "testing_repos": false,
        "versions": {}
    }
    """

  Scenario: Cluster with legacy pillar values
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "description": "test cluster",
        "configSpec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "cloudStorage": {
                "enabled": false
            },
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": true,
            "sqlDatabaseManagement": true,
            "adminPassword": "admin_pass"
        },
        "databaseSpecs": [],
        "userSpecs": [],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "networkId": "network1"
}
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create ClickHouse cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,clickhouse,keeper_hosts}', '{"zk1": 1,"zk2": 2,"zk3": 3}')
        WHERE subcid = 'subcid1'
    """
    And we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,clickhouse,cluster_name}', '"test_cluster"')
        WHERE subcid = 'subcid1'
    """
    And we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1"]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we GET "/api/v1.0/config/myt-1.df.cloud.yandex.net"
    Then we get response with status 200
    And body at path "$.data" contains
    """
    {
        "backup": {
            "sleep": 7200,
            "start": {
                "hours": 22,
                "minutes": 15,
                "nanos": 100,
                "seconds": 30
            },
            "use_backup_service": false
        },
        "clickhouse": {
            "admin_password": {
                "hash": {
                    "data": "fc618395b55f38756c9a98ec058c267830f84d448ca4e0b893951784b163b617",
                    "encryption_version": 0
                },
                "password": {
                    "data": "admin_pass",
                    "encryption_version": 0
                }
            },
            "ch_version": "22.3.2.2",
            "cluster_name": "test_cluster",
            "config": {
                "builtin_dictionaries_reload_interval": 3600,
                "compression": [],
                "keep_alive_timeout": 3,
                "log_level": "information",
                "mark_cache_size": 5368709120,
                "max_concurrent_queries": 500,
                "max_connections": 4096,
                "max_partition_size_to_drop": 53687091200,
                "max_table_size_to_drop":     53687091200,
                "merge_tree": {
                    "enable_mixed_granularity_parts": true,
                    "replicated_deduplication_window": 100,
                    "replicated_deduplication_window_seconds": 604800
                },
                "timezone": "Europe/Moscow",
                "uncompressed_cache_size": 8589934592
            },
            "databases": [],
            "format_schemas": {},
            "interserver_credentials": {
                "password": {
                    "data": "dummy",
                    "encryption_version": 0
                },
                "user": "interserver"
            },
            "models": {},
            "embedded_keeper": false,
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "shard_groups": {
                "test_group": {
                     "description": "Test shard group",
                     "shard_names": ["shard1"]
                }
            },
            "shards": {
                "shard_id1": {
                    "weight": 100
                }
            },
            "sql_database_management": true,
            "sql_user_management": true,
            "system_users": {
                "mdb_backup_admin": {
                    "hash": {
                        "data": "b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "dummy",
                        "encryption_version": 0
                    }
                }
            },
            "user_management_v2": true,
            "users": {},
            "zk_users": {
                "clickhouse": {
                    "password": {
                        "data": "dummy",
                        "encryption_version": 0
                    }
                }
            },
            "keeper_hosts": {"zk1": 1, "zk2": 2, "zk3": 3}
        },
        "clickhouse default pillar": true,
        "cloud_storage": {
            "enabled": false
        },
        "cluster_private_key": {
            "data": "1",
            "encryption_version": 0
        },
        "default pillar": true,
        "runlist": [
            "components.clickhouse_cluster"
        ],
        "s3_bucket": "yandexcloud-dbaas-cid1",
        "testing_repos": false,
        "unmanaged": {
            "enable_zk_tls": true
        },
        "versions": {}
    }
    """
