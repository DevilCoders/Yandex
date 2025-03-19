Feature: Create/Modify Porto MySQL Cluster 8.0

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_MYSQL_8_0"]
    """
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_8_0": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt",
            "backupPriority": 5
        }, {
            "zoneId": "iva",
            "priority": 7
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API",
        "deletionProtection": false
    }
    """
    And "default cluster config" data
    """
    {
        "config": {
            "version": "8.0",
            "access": {
                "webSql": false,
                "dataLens": false,
                "dataTransfer": false,
                "serverless": false
            },
            "backupRetainPeriodDays": 7,
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "mysqlConfig_8_0": {
                "defaultConfig": {
                    "innodbBufferPoolSize": 1610612736,
                    "characterSetServer": "utf8mb4",
                    "collationServer": "utf8mb4_0900_ai_ci",
                    "defaultTimeZone": "Europe/Moscow",
                    "groupConcatMaxLen": 1024,
                    "netReadTimeout": 30,
                    "netWriteTimeout": 60,
                    "tmpTableSize": 16777216,
                    "maxHeapTableSize": 16777216,
                    "mdbPriorityChoiceMaxLag": 60,
                    "innodbFlushLogAtTrxCommit": 1,
                    "innodbLockWaitTimeout": 50,
                    "transactionIsolation": "REPEATABLE_READ",
                    "logErrorVerbosity": 3,
                    "longQueryTime": 0.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
                    "auditLog": false,
                    "generalLog": false,
                    "maxAllowedPacket": 16777216,
                    "maxConnections": 128,
                    "sqlMode": [
                        "ONLY_FULL_GROUP_BY",
                        "STRICT_TRANS_TABLES",
                        "NO_ZERO_IN_DATE",
                        "NO_ZERO_DATE",
                        "ERROR_FOR_DIVISION_BY_ZERO",
                        "NO_ENGINE_SUBSTITUTION",
                        "NO_DIR_IN_CREATE"
                    ],
                    "innodbAdaptiveHashIndex" : true,
                    "innodbNumaInterleave" : false,
                    "innodbLogBufferSize" : 16777216,
                    "innodbLogFileSize" : 268435456,
                    "innodbIoCapacity" : 200,
                    "innodbIoCapacityMax" : 2000,
                    "innodbReadIoThreads" : 4,
                    "innodbWriteIoThreads" : 4,
                    "innodbPurgeThreads" : 4,
                    "innodbThreadConcurrency" : 0,
                    "threadCacheSize" : 10,
                    "threadStack" : 196608,
                    "joinBufferSize" : 262144,
                    "sortBufferSize" : 262144,
                    "tableDefinitionCache" : 2000,
                    "tableOpenCache" : 4000,
                    "tableOpenCacheInstances" : 16,
                    "explicitDefaultsForTimestamp" : true,
                    "autoIncrementIncrement" : 1,
                    "autoIncrementOffset" : 1,
                    "syncBinlog" : 1,
                    "binlogCacheSize" : 32768,
                    "binlogGroupCommitSyncDelay" : 0,
                    "binlogRowImage" : "FULL",
                    "binlogRowsQueryLogEvents" : false,
                    "mdbPreserveBinlogBytes" : 1073741824,
                    "rplSemiSyncMasterWaitForSlaveCount" : 1,
                    "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                    "slaveParallelType" : "DATABASE",
                    "slaveParallelWorkers" : 0,
                    "defaultAuthenticationPlugin": "CACHING_SHA2_PASSWORD",
                    "interactiveTimeout": 28800,
                    "waitTimeout": 28800,
                    "mdbOfflineModeEnableLag": 86400,
                    "mdbOfflineModeDisableLag": 300,
                    "rangeOptimizerMaxMemSize": 8388608,
                    "innodbOnlineAlterLogMaxSize": 134217728,
                    "innodbFtMinTokenSize": 3,
                    "innodbFtMaxTokenSize": 84,
                    "innodbStrictMode": true,
                    "maxDigestLength": 1024,
                    "lowerCaseTableNames": 0,
                    "innodbPageSize": 16384,
                    "maxSpRecursionDepth": 0,
                    "innodbCompressionLevel": 6
                },
                "effectiveConfig": {
                    "innodbBufferPoolSize": 1610612736,
                    "characterSetServer": "utf8mb4",
                    "collationServer": "utf8mb4_0900_ai_ci",
                    "defaultTimeZone": "Europe/Moscow",
                    "groupConcatMaxLen": 1024,
                    "netReadTimeout": 30,
                    "netWriteTimeout": 60,
                    "tmpTableSize": 16777216,
                    "maxHeapTableSize": 16777216,
                    "mdbPriorityChoiceMaxLag": 60,
                    "innodbFlushLogAtTrxCommit": 1,
                    "innodbLockWaitTimeout": 50,
                    "transactionIsolation": "REPEATABLE_READ",
                    "logErrorVerbosity": 3,
                    "longQueryTime": 0.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
                    "auditLog": false,
                    "generalLog": false,
                    "maxAllowedPacket": 16777216,
                    "maxConnections": 128,
                    "sqlMode": [
                        "ONLY_FULL_GROUP_BY",
                        "STRICT_TRANS_TABLES",
                        "NO_ZERO_IN_DATE",
                        "NO_ZERO_DATE",
                        "ERROR_FOR_DIVISION_BY_ZERO",
                        "NO_ENGINE_SUBSTITUTION",
                        "NO_DIR_IN_CREATE"
                    ],
                    "innodbAdaptiveHashIndex" : true,
                    "innodbNumaInterleave" : false,
                    "innodbLogBufferSize" : 16777216,
                    "innodbLogFileSize" : 268435456,
                    "innodbIoCapacity" : 200,
                    "innodbIoCapacityMax" : 2000,
                    "innodbReadIoThreads" : 4,
                    "innodbWriteIoThreads" : 4,
                    "innodbPurgeThreads" : 4,
                    "innodbThreadConcurrency" : 0,
                    "threadCacheSize" : 10,
                    "threadStack" : 196608,
                    "joinBufferSize" : 262144,
                    "sortBufferSize" : 262144,
                    "tableDefinitionCache" : 2000,
                    "tableOpenCache" : 4000,
                    "tableOpenCacheInstances" : 16,
                    "explicitDefaultsForTimestamp" : true,
                    "autoIncrementIncrement" : 1,
                    "autoIncrementOffset" : 1,
                    "syncBinlog" : 1,
                    "binlogCacheSize" : 32768,
                    "binlogGroupCommitSyncDelay" : 0,
                    "binlogRowImage" : "FULL",
                    "binlogRowsQueryLogEvents" : false,
                    "mdbPreserveBinlogBytes" : 1073741824,
                    "rplSemiSyncMasterWaitForSlaveCount" : 1,
                    "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                    "slaveParallelType" : "DATABASE",
                    "slaveParallelWorkers" : 0,
                    "defaultAuthenticationPlugin": "CACHING_SHA2_PASSWORD",
                    "interactiveTimeout": 28800,
                    "waitTimeout": 28800,
                    "mdbOfflineModeEnableLag": 86400,
                    "mdbOfflineModeDisableLag": 300,
                    "rangeOptimizerMaxMemSize": 8388608,
                    "innodbOnlineAlterLogMaxSize": 134217728,
                    "innodbFtMinTokenSize": 3,
                    "innodbFtMaxTokenSize": 84,
                    "innodbStrictMode": true,
                    "maxDigestLength": 1024,
                    "lowerCaseTableNames": 0,
                    "innodbPageSize": 16384,
                    "maxSpRecursionDepth": 0,
                    "innodbCompressionLevel": 6
                },
                "userConfig": {}
            },
            "soxAudit": false,
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
        "id": "cid1",
        "labels": {},
        "monitoring": [
            {
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm.yandex-team.ru/template/panel/dbaas_mysql_metrics/cid=cid1",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid1&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mysql",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/mysql_cluster/cid1?section=monitoring",
                "name": "Console"
            }
        ],
        "name": "test",
        "networkId": "",
        "status": "RUNNING"
    }
    """
    And "default sas host config" data
    """
    {
        "data": {
            "backup": {
                "sleep": 7200,
                "retain_period": 7,
                "use_backup_service": true,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                }
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "dbaas": {
                "assign_public_ip": false,
                "on_dedicated_host": false,
                "cloud": {
                    "cloud_ext_id": "cloud1"
                },
                "cluster": {
                    "subclusters": {
                        "subcid1": {
                            "hosts": {
                                "iva-1.db.yandex.net": {"geo": "iva"},
                                "myt-1.db.yandex.net": {"geo": "myt"},
                                "sas-1.db.yandex.net": {"geo": "sas"}
                            },
                            "name": "test",
                            "roles": [
                                "mysql_cluster"
                            ],
                            "shards": {}
                        }
                    }
                },
                "cluster_hosts": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "cluster_id": "cid1",
                "cluster_name": "test",
                "cluster_type": "mysql_cluster",
                "disk_type_id": "local-ssd",
                "flavor": {
                    "cpu_guarantee": 1.0,
                    "cpu_limit": 1.0,
                    "cpu_fraction": 100,
                    "gpu_limit": 0,
                    "description": "s1.porto.1",
                    "id": "00000000-0000-0000-0000-000000000000",
                    "io_limit": 20971520,
                    "io_cores_limit": 0,
                    "memory_guarantee": 4294967296,
                    "memory_limit": 4294967296,
                    "name": "s1.porto.1",
                    "network_guarantee": 16777216,
                    "network_limit": 16777216,
                    "type": "standard",
                    "generation": 1,
                    "platform_id": "mdb-v1",
                    "vtype": "porto"
                },
                "folder": {
                    "folder_ext_id": "folder1"
                },
                "fqdn": "sas-1.db.yandex.net",
                "geo": "sas",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "shard_id": null,
                "shard_name": null,
                "space_limit": 10737418240,
                "subcluster_id": "subcid1",
                "subcluster_name": "test",
                "vtype": "porto",
                "vtype_id": null
            },
            "cluster_nodes": {
                "ha": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ]
            },
            "default pillar": true,
            "mysql": {
                "_max_server_id": 3,
                "backup_priority": 0,
                "config": {
                    "long_query_time": 0,
                    "log_slow_rate_limit": 1,
                    "log_slow_rate_type": "session",
                    "log_slow_sp_statements": false,
                    "slow_query_log": false,
                    "slow_query_log_always_write_time": 10,
                    "default_time_zone": "Europe/Moscow",
                    "group_concat_max_len": 1024,
                    "net_read_timeout": 30,
                    "net_write_timeout": 60,
                    "query_cache_limit": 1048576,
                    "query_cache_size": 0,
                    "query_cache_type": 0,
                    "tmp_table_size": 16777216,
                    "max_heap_table_size": 16777216,
                    "innodb_flush_log_at_trx_commit": 1,
                    "innodb_lock_wait_timeout": 50,
                    "transaction_isolation": "REPEATABLE-READ",
                    "log_error_verbosity": 3,
                    "audit_log": false,
                    "general_log": false,
                    "max_allowed_packet": 16777216,
                    "server-id": 3,
                    "sql_mode": [
                        "ONLY_FULL_GROUP_BY",
                        "STRICT_TRANS_TABLES",
                        "NO_ZERO_IN_DATE",
                        "NO_ZERO_DATE",
                        "ERROR_FOR_DIVISION_BY_ZERO",
                        "NO_ENGINE_SUBSTITUTION",
                        "NO_DIR_IN_CREATE"
                    ],
                    "innodb_adaptive_hash_index" : true,
                    "innodb_numa_interleave" : false,
                    "innodb_log_buffer_size" : 16777216,
                    "innodb_log_file_size" : 268435456,
                    "innodb_io_capacity" : 200,
                    "innodb_io_capacity_max" : 2000,
                    "innodb_read_io_threads" : 4,
                    "innodb_write_io_threads" : 4,
                    "innodb_purge_threads" : 4,
                    "innodb_thread_concurrency" : 0,
                    "thread_cache_size" : 10,
                    "thread_stack" : 196608,
                    "join_buffer_size" : 262144,
                    "sort_buffer_size" : 262144,
                    "table_definition_cache" : 2000,
                    "table_open_cache" : 4000,
                    "table_open_cache_instances" : 16,
                    "explicit_defaults_for_timestamp" : true,
                    "auto_increment_increment" : 1,
                    "auto_increment_offset" : 1,
                    "sync_binlog" : 1,
                    "binlog_cache_size" : 32768,
                    "binlog_group_commit_sync_delay" : 0,
                    "binlog_row_image" : "FULL",
                    "binlog_rows_query_log_events" : false,
                    "mdb_preserve_binlog_bytes": 1073741824,
                    "mdb_priority_choice_max_lag": 60,
                    "rpl_semi_sync_master_wait_for_slave_count" : 1,
                    "binlog_transaction_dependency_tracking" : "COMMIT_ORDER",
                    "slave_parallel_type" : "DATABASE",
                    "slave_parallel_workers" : 0,
                    "interactive_timeout": 28800,
                    "wait_timeout": 28800,
                    "mdb_offline_mode_enable_lag": 86400,
                    "mdb_offline_mode_disable_lag": 300,
                    "range_optimizer_max_mem_size": 8388608,
                    "innodb_ft_max_token_size": 84,
                    "innodb_ft_min_token_size": 3,
                    "innodb_online_alter_log_max_size": 134217728,
                    "innodb_strict_mode": true,
                    "max_digest_length": 1024,
                    "lower_case_table_names": 0,
                    "innodb_page_size": 16384,
                    "max_sp_recursion_depth": 0,
                    "innodb_compression_level": 6
                },
                "databases": [
                    "testdb"
                ],
                "priority": 0,
                "users": {
                    "admin": {
                        "dbs": {
                            "*": [
                                "ALL PRIVILEGES"
                            ]
                        },
                        "hosts": "__cluster__",
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "connection_limits": {
                            "MAX_USER_CONNECTIONS": 30
                        }
                    },
                    "monitor": {
                        "dbs": {
                            "*": [
                                "REPLICATION CLIENT"
                            ],
                            "mysql": [
                                "SELECT"
                            ]
                        },
                        "hosts": "__cluster__",
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "connection_limits": {
                            "MAX_USER_CONNECTIONS": 10
                        }
                    },
                    "repl": {
                        "dbs": {
                            "*": [
                                "REPLICATION SLAVE"
                            ]
                        },
                        "hosts": "__cluster__",
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "connection_limits": {
                            "MAX_USER_CONNECTIONS": 10
                        }
                    },
                    "test": {
                        "dbs": {
                            "testdb": [
                                "ALL PRIVILEGES"
                            ]
                        },
                        "password": {
                            "data": "test_password",
                            "encryption_version": 0
                        },
                        "services": [
                            "mysql"
                        ]
                    }
                },
                "zk_hosts": [
                    "localhost"
                ],
                "version": {
                    "major_human": "8.0",
                    "major_num": "800"
                }
            },
            "mysql default pillar": true,
            "runlist": [
                "components.mysql_cluster"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
            "sox_audit": false,
            "versions": {
                "mysql": {
                    "edition": "some hidden value",
                    "major_version": "8.0",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                },
                "xtrabackup": {
                    "edition": "some hidden value",
                    "major_version": "some hidden value",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                }

            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """
    And "default iva host config" data
    """
    {
        "data": {
            "backup": {
                "sleep": 7200,
                "retain_period": 7,
                "use_backup_service": true,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                }
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "dbaas": {
                "assign_public_ip": false,
                "on_dedicated_host": false,
                "cloud": {
                    "cloud_ext_id": "cloud1"
                },
                "cluster": {
                    "subclusters": {
                        "subcid1": {
                            "hosts": {
                                "iva-1.db.yandex.net": {"geo": "iva"},
                                "myt-1.db.yandex.net": {"geo": "myt"},
                                "sas-1.db.yandex.net": {"geo": "sas"}
                            },
                            "name": "test",
                            "roles": [
                                "mysql_cluster"
                            ],
                            "shards": {}
                        }
                    }
                },
                "cluster_hosts": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "cluster_id": "cid1",
                "cluster_name": "test",
                "cluster_type": "mysql_cluster",
                "disk_type_id": "local-ssd",
                "flavor": {
                    "cpu_guarantee": 1.0,
                    "cpu_limit": 1.0,
                    "cpu_fraction": 100,
                    "gpu_limit": 0,
                    "description": "s1.porto.1",
                    "id": "00000000-0000-0000-0000-000000000000",
                    "io_limit": 20971520,
                    "io_cores_limit": 0,
                    "memory_guarantee": 4294967296,
                    "memory_limit": 4294967296,
                    "name": "s1.porto.1",
                    "network_guarantee": 16777216,
                    "network_limit": 16777216,
                    "type": "standard",
                    "generation": 1,
                    "platform_id": "mdb-v1",
                    "vtype": "porto"
                },
                "folder": {
                    "folder_ext_id": "folder1"
                },
                "fqdn": "iva-1.db.yandex.net",
                "geo": "iva",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "shard_id": null,
                "shard_name": null,
                "space_limit": 10737418240,
                "subcluster_id": "subcid1",
                "subcluster_name": "test",
                "vtype": "porto",
                "vtype_id": null
            },
            "cluster_nodes": {
                "ha": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ]
            },
            "default pillar": true,
            "mysql": {
                "_max_server_id": 3,
                "backup_priority": 0,
                "config": {
                    "long_query_time": 0,
                    "log_slow_rate_limit": 1,
                    "log_slow_rate_type": "session",
                    "log_slow_sp_statements": false,
                    "slow_query_log": false,
                    "slow_query_log_always_write_time": 10,
                    "default_time_zone": "Europe/Moscow",
                    "group_concat_max_len": 1024,
                    "net_read_timeout": 30,
                    "net_write_timeout": 60,
                    "query_cache_limit": 1048576,
                    "query_cache_size": 0,
                    "query_cache_type": 0,
                    "tmp_table_size": 16777216,
                    "max_heap_table_size": 16777216,
                    "innodb_flush_log_at_trx_commit": 1,
                    "innodb_lock_wait_timeout": 50,
                    "transaction_isolation": "REPEATABLE-READ",
                    "log_error_verbosity": 3,
                    "audit_log": false,
                    "general_log": false,
                    "max_allowed_packet": 16777216,
                    "server-id": 1,
                    "sql_mode": [
                        "ONLY_FULL_GROUP_BY",
                        "STRICT_TRANS_TABLES",
                        "NO_ZERO_IN_DATE",
                        "NO_ZERO_DATE",
                        "ERROR_FOR_DIVISION_BY_ZERO",
                        "NO_ENGINE_SUBSTITUTION",
                        "NO_DIR_IN_CREATE"
                    ],
                    "innodb_adaptive_hash_index" : true,
                    "innodb_numa_interleave" : false,
                    "innodb_log_buffer_size" : 16777216,
                    "innodb_log_file_size" : 268435456,
                    "innodb_io_capacity" : 200,
                    "innodb_io_capacity_max" : 2000,
                    "innodb_read_io_threads" : 4,
                    "innodb_write_io_threads" : 4,
                    "innodb_purge_threads" : 4,
                    "innodb_thread_concurrency" : 0,
                    "thread_cache_size" : 10,
                    "thread_stack" : 196608,
                    "join_buffer_size" : 262144,
                    "sort_buffer_size" : 262144,
                    "table_definition_cache" : 2000,
                    "table_open_cache" : 4000,
                    "table_open_cache_instances" : 16,
                    "explicit_defaults_for_timestamp" : true,
                    "auto_increment_increment" : 1,
                    "auto_increment_offset" : 1,
                    "sync_binlog" : 1,
                    "binlog_cache_size" : 32768,
                    "binlog_group_commit_sync_delay" : 0,
                    "binlog_row_image" : "FULL",
                    "binlog_rows_query_log_events" : false,
                    "mdb_preserve_binlog_bytes": 1073741824,
                    "mdb_priority_choice_max_lag": 60,
                    "rpl_semi_sync_master_wait_for_slave_count" : 1,
                    "binlog_transaction_dependency_tracking" : "COMMIT_ORDER",
                    "slave_parallel_type" : "DATABASE",
                    "slave_parallel_workers" : 0,
                    "interactive_timeout": 28800,
                    "wait_timeout": 28800,
                    "mdb_offline_mode_enable_lag": 86400,
                    "mdb_offline_mode_disable_lag": 300,
                    "range_optimizer_max_mem_size": 8388608,
                    "innodb_ft_max_token_size": 84,
                    "innodb_ft_min_token_size": 3,
                    "innodb_online_alter_log_max_size": 134217728,
                    "innodb_strict_mode": true,
                    "max_digest_length": 1024,
                    "lower_case_table_names": 0,
                    "innodb_page_size": 16384,
                    "max_sp_recursion_depth": 0,
                    "innodb_compression_level": 6
                },
                "databases": [
                    "testdb"
                ],
                "priority": 7,
                "users": {
                    "admin": {
                        "dbs": {
                            "*": [
                                "ALL PRIVILEGES"
                            ]
                        },
                        "hosts": "__cluster__",
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "connection_limits": {
                            "MAX_USER_CONNECTIONS": 30
                        }
                    },
                    "monitor": {
                        "dbs": {
                            "*": [
                                "REPLICATION CLIENT"
                            ],
                            "mysql": [
                                "SELECT"
                            ]
                        },
                        "hosts": "__cluster__",
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "connection_limits": {
                            "MAX_USER_CONNECTIONS": 10
                        }
                    },
                    "repl": {
                        "dbs": {
                            "*": [
                                "REPLICATION SLAVE"
                            ]
                        },
                        "hosts": "__cluster__",
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "connection_limits": {
                            "MAX_USER_CONNECTIONS": 10
                        }
                    },
                    "test": {
                        "dbs": {
                            "testdb": [
                                "ALL PRIVILEGES"
                            ]
                        },
                        "password": {
                            "data": "test_password",
                            "encryption_version": 0
                        },
                        "services": [
                            "mysql"
                        ]
                    }
                },
                "zk_hosts": [
                    "localhost"
                ],
                "version": {
                    "major_human": "8.0",
                    "major_num": "800"
                }
            },
            "mysql default pillar": true,
            "runlist": [
                "components.mysql_cluster"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
            "sox_audit": false,
            "versions": {
                "mysql": {
                    "edition": "some hidden value",
                    "major_version": "8.0",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                },
                "xtrabackup": {
                    "edition": "some hidden value",
                    "major_version": "some hidden value",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                }

            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """
    And health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
               "status": "Alive"
           }
       ],
       "hosts": [
           {
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mysql",
                       "role": "Master",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mysql",
                       "role": "Replica",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mysql",
                       "role": "Replica",
                       "status": "Alive"
                   }
               ]
           }
       ]
    }
    """
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create MySQL cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker

  Scenario: Cluster creation works
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body equals to "default cluster config" data
    When we GET "/mdb/mysql/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["ALL_PRIVILEGES"]
                    }
                ]
            }
        ]
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 3.0,
        "memoryUsed": 12884901888,
        "ssdSpaceUsed": 32212254720
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body equals to "default iva host config" data

  Scenario Outline: Cluster health deduction works
    Given health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
               "status": "<status>"
           }
       ],
       "hosts": [
           {
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "<my-iva>",
               "services": [
                   {
                       "name": "mysql",
                       "role": "Master",
                       "status": "<my-iva>"
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "<my-myt>",
               "services": [
                   {
                       "name": "mysql",
                       "role": "Replica",
                       "status": "<my-myt>"
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "<my-sas>",
               "services": [
                   {
                       "name": "mysql",
                       "role": "Replica",
                       "status": "<my-sas>"
                   }
               ]
           }
       ]
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "health": "<health>"
    }
    """
    Examples:
      | my-iva | my-myt | my-sas | status   | health   |
      | Dead   | Alive  | Alive  | Degraded | DEGRADED |
      | Dead   | Dead   | Dead   | Dead     | DEAD     |

  Scenario: Config for unmanaged cluster doesn't work
    When we GET "/api/v1.0/config_unmanaged/iva-1.db.yandex.net"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Unknown cluster for host iva-1.db.yandex.net"
    }
    """

  Scenario: Folder operations list works
    When we run query
    """
    DELETE FROM dbaas.worker_queue
    """
    And we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_8_0": {
                "maxConnections": 90
            }
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_8_0": {
                "maxConnections": 100
            }
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_8_0": {
                "maxConnections": 90
            }
        }
    }
    """
    And "worker_task_id4" acquired and finished by worker
    And we run query
    """
    UPDATE dbaas.worker_queue
    SET
        start_ts = null,
        end_ts = null,
        result = null,
        create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    And we GET "/mdb/mysql/1.0/operations?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "operations": [
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify MySQL cluster",
                "done": false,
                "id": "worker_task_id4",
                "metadata": {
                    "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify MySQL cluster",
                "done": false,
                "id": "worker_task_id3",
                "metadata": {
                    "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify MySQL cluster",
                "done": false,
                "id": "worker_task_id2",
                "metadata": {
                    "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            }
        ]
    }
    """

  Scenario: Cluster list works
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_8_0": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "another test cluster"
    }
    """
    And we run query
    """
    UPDATE dbaas.clusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    And we GET "/mdb/mysql/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusters": [
            {
                "config": {
                    "version": "8.0",
                    "access": {
                        "webSql": false,
                        "dataLens": false,
                        "dataTransfer": false,
                        "serverless": false
                    },
                    "backupRetainPeriodDays": 7,
                    "backupWindowStart": {
                        "hours": 22,
                        "minutes": 15,
                        "seconds": 30,
                        "nanos": 100
                    },
                    "performanceDiagnostics": {
                        "enabled": false,
                        "statementsSamplingInterval": 600,
                        "sessionsSamplingInterval": 60
                    },
                    "mysqlConfig_8_0": {
                        "defaultConfig": {
                            "innodbBufferPoolSize": 1610612736,
                            "characterSetServer": "utf8mb4",
                            "collationServer": "utf8mb4_0900_ai_ci",
                            "defaultTimeZone": "Europe/Moscow",
                            "groupConcatMaxLen": 1024,
                            "netReadTimeout": 30,
                            "netWriteTimeout": 60,
                            "tmpTableSize": 16777216,
                            "maxHeapTableSize": 16777216,
                            "innodbFlushLogAtTrxCommit": 1,
                            "innodbLockWaitTimeout": 50,
                            "transactionIsolation": "REPEATABLE_READ",
                            "logErrorVerbosity": 3,
                            "longQueryTime": 0.0,
                            "logSlowRateLimit": 1,
                            "logSlowRateType": "SESSION",
                            "logSlowSpStatements": false,
                            "slowQueryLog": false,
                            "slowQueryLogAlwaysWriteTime": 10,
                            "auditLog": false,
                            "generalLog": false,
                            "maxAllowedPacket": 16777216,
                            "maxConnections": 128,
                            "sqlMode": [
                                "ONLY_FULL_GROUP_BY",
                                "STRICT_TRANS_TABLES",
                                "NO_ZERO_IN_DATE",
                                "NO_ZERO_DATE",
                                "ERROR_FOR_DIVISION_BY_ZERO",
                                "NO_ENGINE_SUBSTITUTION",
                                "NO_DIR_IN_CREATE"
                            ],
                            "innodbAdaptiveHashIndex" : true,
                            "innodbNumaInterleave" : false,
                            "innodbLogBufferSize" : 16777216,
                            "innodbLogFileSize" : 268435456,
                            "innodbIoCapacity" : 200,
                            "innodbIoCapacityMax" : 2000,
                            "innodbReadIoThreads" : 4,
                            "innodbWriteIoThreads" : 4,
                            "innodbPurgeThreads" : 4,
                            "innodbThreadConcurrency" : 0,
                            "threadCacheSize" : 10,
                            "threadStack" : 196608,
                            "joinBufferSize" : 262144,
                            "sortBufferSize" : 262144,
                            "tableDefinitionCache" : 2000,
                            "tableOpenCache" : 4000,
                            "tableOpenCacheInstances" : 16,
                            "explicitDefaultsForTimestamp" : true,
                            "autoIncrementIncrement" : 1,
                            "autoIncrementOffset" : 1,
                            "syncBinlog" : 1,
                            "binlogCacheSize" : 32768,
                            "binlogGroupCommitSyncDelay" : 0,
                            "binlogRowImage" : "FULL",
                            "binlogRowsQueryLogEvents" : false,
                            "mdbPreserveBinlogBytes" : 1073741824,
                            "rplSemiSyncMasterWaitForSlaveCount" : 1,
                            "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                            "slaveParallelType" : "DATABASE",
                            "slaveParallelWorkers" : 0,
                            "defaultAuthenticationPlugin": "CACHING_SHA2_PASSWORD",
                            "interactiveTimeout": 28800,
                            "waitTimeout": 28800,
                            "mdbPriorityChoiceMaxLag": 60,
                            "mdbOfflineModeEnableLag": 86400,
                            "mdbOfflineModeDisableLag": 300,
                            "rangeOptimizerMaxMemSize": 8388608,
                            "innodbOnlineAlterLogMaxSize": 134217728,
                            "innodbFtMinTokenSize": 3,
                            "innodbFtMaxTokenSize": 84,
                            "innodbStrictMode": true,
                            "maxDigestLength": 1024,
                            "lowerCaseTableNames": 0,
                            "innodbPageSize": 16384,
                            "maxSpRecursionDepth": 0,
                            "innodbCompressionLevel": 6
                        },
                        "effectiveConfig": {
                            "innodbBufferPoolSize": 1610612736,
                            "characterSetServer": "utf8mb4",
                            "collationServer": "utf8mb4_0900_ai_ci",
                            "defaultTimeZone": "Europe/Moscow",
                            "groupConcatMaxLen": 1024,
                            "netReadTimeout": 30,
                            "netWriteTimeout": 60,
                            "tmpTableSize": 16777216,
                            "maxHeapTableSize": 16777216,
                            "innodbFlushLogAtTrxCommit": 1,
                            "innodbLockWaitTimeout": 50,
                            "transactionIsolation": "REPEATABLE_READ",
                            "logErrorVerbosity": 3,
                            "longQueryTime": 0.0,
                            "logSlowRateLimit": 1,
                            "logSlowRateType": "SESSION",
                            "logSlowSpStatements": false,
                            "slowQueryLog": false,
                            "slowQueryLogAlwaysWriteTime": 10,
                            "auditLog": false,
                            "generalLog": false,
                            "maxAllowedPacket": 16777216,
                            "maxConnections": 128,
                            "sqlMode": [
                                "ONLY_FULL_GROUP_BY",
                                "STRICT_TRANS_TABLES",
                                "NO_ZERO_IN_DATE",
                                "NO_ZERO_DATE",
                                "ERROR_FOR_DIVISION_BY_ZERO",
                                "NO_ENGINE_SUBSTITUTION",
                                "NO_DIR_IN_CREATE"
                            ],
                            "innodbAdaptiveHashIndex" : true,
                            "innodbNumaInterleave" : false,
                            "innodbLogBufferSize" : 16777216,
                            "innodbLogFileSize" : 268435456,
                            "innodbIoCapacity" : 200,
                            "innodbIoCapacityMax" : 2000,
                            "innodbReadIoThreads" : 4,
                            "innodbWriteIoThreads" : 4,
                            "innodbPurgeThreads" : 4,
                            "innodbThreadConcurrency" : 0,
                            "threadCacheSize" : 10,
                            "threadStack" : 196608,
                            "joinBufferSize" : 262144,
                            "sortBufferSize" : 262144,
                            "tableDefinitionCache" : 2000,
                            "tableOpenCache" : 4000,
                            "tableOpenCacheInstances" : 16,
                            "explicitDefaultsForTimestamp" : true,
                            "autoIncrementIncrement" : 1,
                            "autoIncrementOffset" : 1,
                            "syncBinlog" : 1,
                            "binlogCacheSize" : 32768,
                            "binlogGroupCommitSyncDelay" : 0,
                            "binlogRowImage" : "FULL",
                            "binlogRowsQueryLogEvents" : false,
                            "mdbPreserveBinlogBytes" : 1073741824,
                            "rplSemiSyncMasterWaitForSlaveCount" : 1,
                            "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                            "slaveParallelType" : "DATABASE",
                            "slaveParallelWorkers" : 0,
                            "defaultAuthenticationPlugin": "CACHING_SHA2_PASSWORD",
                            "interactiveTimeout": 28800,
                            "waitTimeout": 28800,
                            "mdbPriorityChoiceMaxLag": 60,
                            "mdbOfflineModeEnableLag": 86400,
                            "mdbOfflineModeDisableLag": 300,
                            "rangeOptimizerMaxMemSize": 8388608,
                            "innodbOnlineAlterLogMaxSize": 134217728,
                            "innodbFtMinTokenSize": 3,
                            "innodbFtMaxTokenSize": 84,
                            "innodbStrictMode": true,
                            "maxDigestLength": 1024,
                            "lowerCaseTableNames": 0,
                            "innodbPageSize": 16384,
                            "maxSpRecursionDepth": 0,
                            "innodbCompressionLevel": 6
                        },
                        "userConfig": {}
                    },
                    "soxAudit": false,
                    "resources": {
                        "diskSize": 10737418240,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
                    }
                },
                "createdAt": "2000-01-01T00:00:00+00:00",
                "deletionProtection": false,
                "description": "test cluster",
                "environment": "PRESTABLE",
                "folderId": "folder1",
                "health": "ALIVE",
                "id": "cid1",
                "labels": {},
                "plannedOperation": null,
                "securityGroupIds": [],
                "hostGroupIds": [],
                "maintenanceWindow": {
                    "anytime": {}
                },
                "monitoring": [
                    {
                        "description": "YaSM (Golovan) charts",
                        "link": "https://yasm.yandex-team.ru/template/panel/dbaas_mysql_metrics/cid=cid1",
                        "name": "YASM"
                    },
                    {
                        "description": "Solomon charts",
                        "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid1&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mysql",
                        "name": "Solomon"
                    },
                    {
                        "description": "Console charts",
                        "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/mysql_cluster/cid1?section=monitoring",
                        "name": "Console"
                    }
                ],
                "name": "test",
                "networkId": "",
                "status": "RUNNING"
            },
            {
                "config": {
                    "version": "8.0",
                    "access": {
                        "webSql": false,
                        "dataLens": false,
                        "dataTransfer": false,
                        "serverless": false
                    },
                    "backupRetainPeriodDays": 7,
                    "backupWindowStart": {
                        "hours": 22,
                        "minutes": 15,
                        "seconds": 30,
                        "nanos": 100
                    },
                    "performanceDiagnostics": {
                        "enabled": false,
                        "statementsSamplingInterval": 600,
                        "sessionsSamplingInterval": 60
                    },
                    "mysqlConfig_8_0": {
                        "defaultConfig": {
                            "innodbBufferPoolSize": 1610612736,
                            "characterSetServer": "utf8mb4",
                            "collationServer": "utf8mb4_0900_ai_ci",
                            "defaultTimeZone": "Europe/Moscow",
                            "groupConcatMaxLen": 1024,
                            "netReadTimeout": 30,
                            "netWriteTimeout": 60,
                            "tmpTableSize": 16777216,
                            "maxHeapTableSize": 16777216,
                            "innodbFlushLogAtTrxCommit": 1,
                            "innodbLockWaitTimeout": 50,
                            "transactionIsolation": "REPEATABLE_READ",
                            "logErrorVerbosity": 3,
                            "longQueryTime": 0.0,
                            "logSlowRateLimit": 1,
                            "logSlowRateType": "SESSION",
                            "logSlowSpStatements": false,
                            "slowQueryLog": false,
                            "slowQueryLogAlwaysWriteTime": 10,
                            "auditLog": false,
                            "generalLog": false,
                            "maxAllowedPacket": 16777216,
                            "maxConnections": 128,
                            "sqlMode": [
                                "ONLY_FULL_GROUP_BY",
                                "STRICT_TRANS_TABLES",
                                "NO_ZERO_IN_DATE",
                                "NO_ZERO_DATE",
                                "ERROR_FOR_DIVISION_BY_ZERO",
                                "NO_ENGINE_SUBSTITUTION",
                                "NO_DIR_IN_CREATE"
                            ],
                            "innodbAdaptiveHashIndex" : true,
                            "innodbNumaInterleave" : false,
                            "innodbLogBufferSize" : 16777216,
                            "innodbLogFileSize" : 268435456,
                            "innodbIoCapacity" : 200,
                            "innodbIoCapacityMax" : 2000,
                            "innodbReadIoThreads" : 4,
                            "innodbWriteIoThreads" : 4,
                            "innodbPurgeThreads" : 4,
                            "innodbThreadConcurrency" : 0,
                            "threadCacheSize" : 10,
                            "threadStack" : 196608,
                            "joinBufferSize" : 262144,
                            "sortBufferSize" : 262144,
                            "tableDefinitionCache" : 2000,
                            "tableOpenCache" : 4000,
                            "tableOpenCacheInstances" : 16,
                            "explicitDefaultsForTimestamp" : true,
                            "autoIncrementIncrement" : 1,
                            "autoIncrementOffset" : 1,
                            "syncBinlog" : 1,
                            "binlogCacheSize" : 32768,
                            "binlogGroupCommitSyncDelay" : 0,
                            "binlogRowImage" : "FULL",
                            "binlogRowsQueryLogEvents" : false,
                            "mdbPreserveBinlogBytes" : 1073741824,
                            "rplSemiSyncMasterWaitForSlaveCount" : 1,
                            "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                            "slaveParallelType" : "DATABASE",
                            "slaveParallelWorkers" : 0,
                            "defaultAuthenticationPlugin": "CACHING_SHA2_PASSWORD",
                            "interactiveTimeout": 28800,
                            "waitTimeout": 28800,
                            "mdbPriorityChoiceMaxLag": 60,
                            "mdbOfflineModeEnableLag": 86400,
                            "mdbOfflineModeDisableLag": 300,
                            "rangeOptimizerMaxMemSize": 8388608,
                            "innodbOnlineAlterLogMaxSize": 134217728,
                            "innodbFtMinTokenSize": 3,
                            "innodbFtMaxTokenSize": 84,
                            "innodbStrictMode": true,
                            "maxDigestLength": 1024,
                            "lowerCaseTableNames": 0,
                            "innodbPageSize": 16384,
                            "maxSpRecursionDepth": 0,
                            "innodbCompressionLevel": 6
                        },
                        "effectiveConfig": {
                            "innodbBufferPoolSize": 1610612736,
                            "characterSetServer": "utf8mb4",
                            "collationServer": "utf8mb4_0900_ai_ci",
                            "defaultTimeZone": "Europe/Moscow",
                            "groupConcatMaxLen": 1024,
                            "netReadTimeout": 30,
                            "netWriteTimeout": 60,
                            "tmpTableSize": 16777216,
                            "maxHeapTableSize": 16777216,
                            "innodbFlushLogAtTrxCommit": 1,
                            "innodbLockWaitTimeout": 50,
                            "transactionIsolation": "REPEATABLE_READ",
                            "logErrorVerbosity": 3,
                            "longQueryTime": 0.0,
                            "logSlowRateLimit": 1,
                            "logSlowRateType": "SESSION",
                            "logSlowSpStatements": false,
                            "slowQueryLog": false,
                            "slowQueryLogAlwaysWriteTime": 10,
                            "auditLog": false,
                            "generalLog": false,
                            "maxAllowedPacket": 16777216,
                            "maxConnections": 128,
                            "sqlMode": [
                                "ONLY_FULL_GROUP_BY",
                                "STRICT_TRANS_TABLES",
                                "NO_ZERO_IN_DATE",
                                "NO_ZERO_DATE",
                                "ERROR_FOR_DIVISION_BY_ZERO",
                                "NO_ENGINE_SUBSTITUTION",
                                "NO_DIR_IN_CREATE"
                            ],
                            "innodbAdaptiveHashIndex" : true,
                            "innodbNumaInterleave" : false,
                            "innodbLogBufferSize" : 16777216,
                            "innodbLogFileSize" : 268435456,
                            "innodbIoCapacity" : 200,
                            "innodbIoCapacityMax" : 2000,
                            "innodbReadIoThreads" : 4,
                            "innodbWriteIoThreads" : 4,
                            "innodbPurgeThreads" : 4,
                            "innodbThreadConcurrency" : 0,
                            "threadCacheSize" : 10,
                            "threadStack" : 196608,
                            "joinBufferSize" : 262144,
                            "sortBufferSize" : 262144,
                            "tableDefinitionCache" : 2000,
                            "tableOpenCache" : 4000,
                            "tableOpenCacheInstances" : 16,
                            "explicitDefaultsForTimestamp" : true,
                            "autoIncrementIncrement" : 1,
                            "autoIncrementOffset" : 1,
                            "syncBinlog" : 1,
                            "binlogCacheSize" : 32768,
                            "binlogGroupCommitSyncDelay" : 0,
                            "binlogRowImage" : "FULL",
                            "binlogRowsQueryLogEvents" : false,
                            "mdbPreserveBinlogBytes" : 1073741824,
                            "rplSemiSyncMasterWaitForSlaveCount" : 1,
                            "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                            "slaveParallelType" : "DATABASE",
                            "slaveParallelWorkers" : 0,
                            "defaultAuthenticationPlugin": "CACHING_SHA2_PASSWORD",
                            "interactiveTimeout": 28800,
                            "waitTimeout": 28800,
                            "mdbPriorityChoiceMaxLag": 60,
                            "mdbOfflineModeEnableLag": 86400,
                            "mdbOfflineModeDisableLag": 300,
                            "rangeOptimizerMaxMemSize": 8388608,
                            "innodbOnlineAlterLogMaxSize": 134217728,
                            "innodbFtMinTokenSize": 3,
                            "innodbFtMaxTokenSize": 84,
                            "innodbStrictMode": true,
                            "maxDigestLength": 1024,
                            "lowerCaseTableNames": 0,
                            "innodbPageSize": 16384,
                            "maxSpRecursionDepth": 0,
                            "innodbCompressionLevel": 6
                        },
                        "userConfig": {}
                    },
                    "soxAudit": false,
                    "resources": {
                        "diskSize": 10737418240,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
                    }
                },
                "createdAt": "2000-01-01T00:00:00+00:00",
                "deletionProtection": false,
                "description": "another test cluster",
                "environment": "PRESTABLE",
                "folderId": "folder1",
                "health": "UNKNOWN",
                "id": "cid2",
                "labels": {},
                "plannedOperation": null,
                "securityGroupIds": [],
                "hostGroupIds": [],
                "maintenanceWindow": {
                    "anytime": {}
                },
                "monitoring": [
                    {
                        "description": "YaSM (Golovan) charts",
                        "link": "https://yasm.yandex-team.ru/template/panel/dbaas_mysql_metrics/cid=cid2",
                        "name": "YASM"
                    },
                    {
                        "description": "Solomon charts",
                        "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid2&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mysql",
                        "name": "Solomon"
                    },
                    {
                        "description": "Console charts",
                        "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/mysql_cluster/cid2?section=monitoring",
                        "name": "Console"
                    }
                ],
                "name": "test2",
                "networkId": "",
                "status": "CREATING"
            }
        ]
    }
    """
    When we GET "/mdb/mysql/1.0/clusters" with params
    """
    {
        "folderId": "folder1",
        "filter": "name = \"test2\""
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "clusters": [
            {
                "config": {
                    "version": "8.0",
                    "access": {
                        "webSql": false,
                        "dataLens": false,
                        "dataTransfer": false,
                        "serverless": false
                    },
                    "backupRetainPeriodDays": 7,
                    "backupWindowStart": {
                        "hours": 22,
                        "minutes": 15,
                        "seconds": 30,
                        "nanos": 100
                    },
                    "performanceDiagnostics": {
                        "enabled": false,
                        "statementsSamplingInterval": 600,
                        "sessionsSamplingInterval": 60
                    },
                    "mysqlConfig_8_0": {
                        "defaultConfig": {
                            "innodbBufferPoolSize": 1610612736,
                            "characterSetServer": "utf8mb4",
                            "collationServer": "utf8mb4_0900_ai_ci",
                            "defaultTimeZone": "Europe/Moscow",
                            "groupConcatMaxLen": 1024,
                            "netReadTimeout": 30,
                            "netWriteTimeout": 60,
                            "tmpTableSize": 16777216,
                            "maxHeapTableSize": 16777216,
                            "innodbFlushLogAtTrxCommit": 1,
                            "innodbLockWaitTimeout": 50,
                            "transactionIsolation": "REPEATABLE_READ",
                            "logErrorVerbosity": 3,
                            "longQueryTime": 0.0,
                            "logSlowRateLimit": 1,
                            "logSlowRateType": "SESSION",
                            "logSlowSpStatements": false,
                            "slowQueryLog": false,
                            "slowQueryLogAlwaysWriteTime": 10,
                            "auditLog": false,
                            "generalLog": false,
                            "maxAllowedPacket": 16777216,
                            "maxConnections": 128,
                            "sqlMode": [
                                "ONLY_FULL_GROUP_BY",
                                "STRICT_TRANS_TABLES",
                                "NO_ZERO_IN_DATE",
                                "NO_ZERO_DATE",
                                "ERROR_FOR_DIVISION_BY_ZERO",
                                "NO_ENGINE_SUBSTITUTION",
                                "NO_DIR_IN_CREATE"
                            ],
                            "innodbAdaptiveHashIndex" : true,
                            "innodbNumaInterleave" : false,
                            "innodbLogBufferSize" : 16777216,
                            "innodbLogFileSize" : 268435456,
                            "innodbIoCapacity" : 200,
                            "innodbIoCapacityMax" : 2000,
                            "innodbReadIoThreads" : 4,
                            "innodbWriteIoThreads" : 4,
                            "innodbPurgeThreads" : 4,
                            "innodbThreadConcurrency" : 0,
                            "threadCacheSize" : 10,
                            "threadStack" : 196608,
                            "joinBufferSize" : 262144,
                            "sortBufferSize" : 262144,
                            "tableDefinitionCache" : 2000,
                            "tableOpenCache" : 4000,
                            "tableOpenCacheInstances" : 16,
                            "explicitDefaultsForTimestamp" : true,
                            "autoIncrementIncrement" : 1,
                            "autoIncrementOffset" : 1,
                            "syncBinlog" : 1,
                            "binlogCacheSize" : 32768,
                            "binlogGroupCommitSyncDelay" : 0,
                            "binlogRowImage" : "FULL",
                            "binlogRowsQueryLogEvents" : false,
                            "mdbPreserveBinlogBytes" : 1073741824,
                            "rplSemiSyncMasterWaitForSlaveCount" : 1,
                            "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                            "slaveParallelType" : "DATABASE",
                            "slaveParallelWorkers" : 0,
                            "defaultAuthenticationPlugin": "CACHING_SHA2_PASSWORD",
                            "interactiveTimeout": 28800,
                            "waitTimeout": 28800,
                            "mdbPriorityChoiceMaxLag": 60,
                            "mdbOfflineModeEnableLag": 86400,
                            "mdbOfflineModeDisableLag": 300,
                            "rangeOptimizerMaxMemSize": 8388608,
                            "innodbOnlineAlterLogMaxSize": 134217728,
                            "innodbFtMinTokenSize": 3,
                            "innodbFtMaxTokenSize": 84,
                            "innodbStrictMode": true,
                            "maxDigestLength": 1024,
                            "lowerCaseTableNames": 0,
                            "innodbPageSize": 16384,
                            "maxSpRecursionDepth": 0,
                            "innodbCompressionLevel": 6
                        },
                        "effectiveConfig": {
                            "innodbBufferPoolSize": 1610612736,
                            "characterSetServer": "utf8mb4",
                            "collationServer": "utf8mb4_0900_ai_ci",
                            "defaultTimeZone": "Europe/Moscow",
                            "groupConcatMaxLen": 1024,
                            "netReadTimeout": 30,
                            "netWriteTimeout": 60,
                            "tmpTableSize": 16777216,
                            "maxHeapTableSize": 16777216,
                            "innodbFlushLogAtTrxCommit": 1,
                            "innodbLockWaitTimeout": 50,
                            "transactionIsolation": "REPEATABLE_READ",
                            "logErrorVerbosity": 3,
                            "longQueryTime": 0.0,
                            "logSlowRateLimit": 1,
                            "logSlowRateType": "SESSION",
                            "logSlowSpStatements": false,
                            "slowQueryLog": false,
                            "slowQueryLogAlwaysWriteTime": 10,
                            "auditLog": false,
                            "generalLog": false,
                            "maxAllowedPacket": 16777216,
                            "maxConnections": 128,
                            "sqlMode": [
                                "ONLY_FULL_GROUP_BY",
                                "STRICT_TRANS_TABLES",
                                "NO_ZERO_IN_DATE",
                                "NO_ZERO_DATE",
                                "ERROR_FOR_DIVISION_BY_ZERO",
                                "NO_ENGINE_SUBSTITUTION",
                                "NO_DIR_IN_CREATE"
                            ],
                            "innodbAdaptiveHashIndex" : true,
                            "innodbNumaInterleave" : false,
                            "innodbLogBufferSize" : 16777216,
                            "innodbLogFileSize" : 268435456,
                            "innodbIoCapacity" : 200,
                            "innodbIoCapacityMax" : 2000,
                            "innodbReadIoThreads" : 4,
                            "innodbWriteIoThreads" : 4,
                            "innodbPurgeThreads" : 4,
                            "innodbThreadConcurrency" : 0,
                            "threadCacheSize" : 10,
                            "threadStack" : 196608,
                            "joinBufferSize" : 262144,
                            "sortBufferSize" : 262144,
                            "tableDefinitionCache" : 2000,
                            "tableOpenCache" : 4000,
                            "tableOpenCacheInstances" : 16,
                            "explicitDefaultsForTimestamp" : true,
                            "autoIncrementIncrement" : 1,
                            "autoIncrementOffset" : 1,
                            "syncBinlog" : 1,
                            "binlogCacheSize" : 32768,
                            "binlogGroupCommitSyncDelay" : 0,
                            "binlogRowImage" : "FULL",
                            "binlogRowsQueryLogEvents" : false,
                            "mdbPreserveBinlogBytes" : 1073741824,
                            "rplSemiSyncMasterWaitForSlaveCount" : 1,
                            "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                            "slaveParallelType" : "DATABASE",
                            "slaveParallelWorkers" : 0,
                            "defaultAuthenticationPlugin": "CACHING_SHA2_PASSWORD",
                            "interactiveTimeout": 28800,
                            "waitTimeout": 28800,
                            "mdbPriorityChoiceMaxLag": 60,
                            "mdbOfflineModeEnableLag": 86400,
                            "mdbOfflineModeDisableLag": 300,
                            "rangeOptimizerMaxMemSize": 8388608,
                            "innodbOnlineAlterLogMaxSize": 134217728,
                            "innodbFtMinTokenSize": 3,
                            "innodbFtMaxTokenSize": 84,
                            "innodbStrictMode": true,
                            "maxDigestLength": 1024,
                            "lowerCaseTableNames": 0,
                            "innodbPageSize": 16384,
                            "maxSpRecursionDepth": 0,
                            "innodbCompressionLevel": 6
                        },
                        "userConfig": {}
                    },
                    "soxAudit": false,
                    "resources": {
                        "diskSize": 10737418240,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
                    }
                },
                "createdAt": "2000-01-01T00:00:00+00:00",
                "deletionProtection": false,
                "description": "another test cluster",
                "environment": "PRESTABLE",
                "folderId": "folder1",
                "health": "UNKNOWN",
                "id": "cid2",
                "labels": {},
                "plannedOperation": null,
                "securityGroupIds": [],
                "hostGroupIds": [],
                "maintenanceWindow": {
                    "anytime": {}
                },
                "monitoring": [
                    {
                        "description": "YaSM (Golovan) charts",
                        "link": "https://yasm.yandex-team.ru/template/panel/dbaas_mysql_metrics/cid=cid2",
                        "name": "YASM"
                    },
                    {
                        "description": "Solomon charts",
                        "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid2&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mysql",
                        "name": "Solomon"
                    },
                    {
                        "description": "Console charts",
                        "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/mysql_cluster/cid2?section=monitoring",
                        "name": "Console"
                    }
                ],
                "name": "test2",
                "networkId": "",
                "status": "CREATING"
            }
        ]
    }
    """

  Scenario Outline: Cluster list with invalid filter fails
    When we GET "/mdb/mysql/1.0/clusters" with params
    """
    {
        "folderId": "folder1",
        "filter": "<filter>"
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | filter          | status | error code | message                                                                                            |
      | name = 1        | 422    | 3          | Filter 'name = 1' has wrong 'name' attribute type. Expected a string.                              |
      | my = 1          | 422    | 3          | Filter by 'my' ('my = 1') is not supported.                                                        |
      | name =          | 422    | 3          | The request is invalid.\nfilter: Filter syntax error (missing value) at or near 6.\nname =\n     ^ |
      | name < \"test\" | 501    | 12         | Operator '<' not implemented.                                                                      |

  Scenario: Host list works
    When we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "backupPriority": 0,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 7,
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "backupPriority": 5,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 0,
                "role": "REPLICA",
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "backupPriority": 0,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 0,
                "role": "REPLICA",
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas"
            }
        ]
    }
    """

  @replication_source
  Scenario: Modifying host with setting replication source works
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify hosts in MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.UpdateClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "sas-1.db.yandex.net"
            ]
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "update_host_specs":  [{
                 "host_name": "sas-1.db.yandex.net",
                 "replication_source": "iva-1.db.yandex.net"
             }]
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    Then we get response with status 200
    And each path on body evaluates to
      | $.hosts[0].name               | "iva-1.db.yandex.net" |
      | $.hosts[1].name               | "myt-1.db.yandex.net" |
      | $.hosts[2].name               | "sas-1.db.yandex.net" |
      | $.hosts[2].replicationSource  | "iva-1.db.yandex.net" |
    And body does not contain following paths
      | $.hosts[0].replicationSource  |
      | $.hosts[1].replicationSource  |
    When we GET "/api/v1.0/config/sas-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "backup": {
                "retain_period": 7,
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                },
                "use_backup_service": true
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "dbaas": {
                "assign_public_ip": false,
                "on_dedicated_host": false,
                "cloud": {
                    "cloud_ext_id": "cloud1"
                },
                "cluster": {
                    "subclusters": {
                        "subcid1": {
                            "hosts": {
                                "iva-1.db.yandex.net": {"geo": "iva"},
                                "myt-1.db.yandex.net": {"geo": "myt"},
                                "sas-1.db.yandex.net": {"geo": "sas"}
                            },
                            "name": "test",
                            "roles": [
                                "mysql_cluster"
                            ],
                            "shards": {}
                        }
                    }
                },
                "cluster_hosts": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "cluster_id": "cid1",
                "cluster_name": "test",
                "cluster_type": "mysql_cluster",
                "disk_type_id": "local-ssd",
                "flavor": {
                    "cpu_guarantee": 1.0,
                    "cpu_limit": 1.0,
                    "cpu_fraction": 100,
                    "gpu_limit": 0,
                    "description": "s1.porto.1",
                    "id": "00000000-0000-0000-0000-000000000000",
                    "io_limit": 20971520,
                    "io_cores_limit": 0,
                    "memory_guarantee": 4294967296,
                    "memory_limit": 4294967296,
                    "name": "s1.porto.1",
                    "network_guarantee": 16777216,
                    "network_limit": 16777216,
                    "type": "standard",
                    "generation": 1,
                    "platform_id": "mdb-v1",
                    "vtype": "porto"
                },
                "folder": {
                    "folder_ext_id": "folder1"
                },
                "fqdn": "sas-1.db.yandex.net",
                "geo": "sas",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "shard_id": null,
                "shard_name": null,
                "space_limit": 10737418240,
                "subcluster_id": "subcid1",
                "subcluster_name": "test",
                "vtype": "porto",
                "vtype_id": null
            },
            "cluster_nodes": {
                "ha": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net"
                ]
            },
            "default pillar": true,
            "mysql": {
                "_max_server_id": 3,
                "backup_priority": 0,
                "config": {
                    "long_query_time": 0,
                    "log_slow_rate_limit": 1,
                    "log_slow_rate_type": "session",
                    "log_slow_sp_statements": false,
                    "slow_query_log": false,
                    "slow_query_log_always_write_time": 10,
                    "default_time_zone": "Europe/Moscow",
                    "group_concat_max_len": 1024,
                    "net_read_timeout": 30,
                    "net_write_timeout": 60,
                    "query_cache_limit": 1048576,
                    "query_cache_size": 0,
                    "query_cache_type": 0,
                    "tmp_table_size": 16777216,
                    "max_heap_table_size": 16777216,
                    "innodb_flush_log_at_trx_commit": 1,
                    "innodb_lock_wait_timeout": 50,
                    "transaction_isolation": "REPEATABLE-READ",
                    "log_error_verbosity": 3,
                    "audit_log": false,
                    "general_log": false,
                    "max_allowed_packet": 16777216,
                    "server-id": 3,
                    "sql_mode": [
                        "ONLY_FULL_GROUP_BY",
                        "STRICT_TRANS_TABLES",
                        "NO_ZERO_IN_DATE",
                        "NO_ZERO_DATE",
                        "ERROR_FOR_DIVISION_BY_ZERO",
                        "NO_ENGINE_SUBSTITUTION",
                        "NO_DIR_IN_CREATE"
                    ],
                    "innodb_adaptive_hash_index" : true,
                    "innodb_numa_interleave" : false,
                    "innodb_log_buffer_size" : 16777216,
                    "innodb_log_file_size" : 268435456,
                    "innodb_io_capacity" : 200,
                    "innodb_io_capacity_max" : 2000,
                    "innodb_read_io_threads" : 4,
                    "innodb_write_io_threads" : 4,
                    "innodb_purge_threads" : 4,
                    "innodb_thread_concurrency" : 0,
                    "thread_cache_size" : 10,
                    "thread_stack" : 196608,
                    "join_buffer_size" : 262144,
                    "sort_buffer_size" : 262144,
                    "table_definition_cache" : 2000,
                    "table_open_cache" : 4000,
                    "table_open_cache_instances" : 16,
                    "explicit_defaults_for_timestamp" : true,
                    "auto_increment_increment" : 1,
                    "auto_increment_offset" : 1,
                    "sync_binlog" : 1,
                    "binlog_cache_size" : 32768,
                    "binlog_group_commit_sync_delay" : 0,
                    "binlog_row_image" : "FULL",
                    "binlog_rows_query_log_events" : false,
                    "mdb_preserve_binlog_bytes": 1073741824,
                    "mdb_priority_choice_max_lag": 60,
                    "rpl_semi_sync_master_wait_for_slave_count" : 1,
                    "binlog_transaction_dependency_tracking" : "COMMIT_ORDER",
                    "slave_parallel_type" : "DATABASE",
                    "slave_parallel_workers" : 0,
                    "interactive_timeout": 28800,
                    "wait_timeout": 28800,
                    "mdb_offline_mode_enable_lag": 86400,
                    "mdb_offline_mode_disable_lag": 300,
                    "range_optimizer_max_mem_size": 8388608,
                    "innodb_ft_max_token_size": 84,
                    "innodb_ft_min_token_size": 3,
                    "innodb_online_alter_log_max_size": 134217728,
                    "innodb_strict_mode": true,
                    "max_digest_length": 1024,
                    "lower_case_table_names": 0,
                    "innodb_page_size": 16384,
                    "max_sp_recursion_depth": 0,
                    "innodb_compression_level": 6
                },
                "databases": [
                    "testdb"
                ],
                "priority": 0,
                "replication_source": "iva-1.db.yandex.net",
                "users": {
                    "admin": {
                        "dbs": {
                            "*": [
                                "ALL PRIVILEGES"
                            ]
                        },
                        "hosts": "__cluster__",
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "connection_limits": {
                            "MAX_USER_CONNECTIONS": 30
                        }
                    },
                    "monitor": {
                        "dbs": {
                            "*": [
                                "REPLICATION CLIENT"
                            ],
                            "mysql": [
                                "SELECT"
                            ]
                        },
                        "hosts": "__cluster__",
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "connection_limits": {
                            "MAX_USER_CONNECTIONS": 10
                        }
                    },
                    "repl": {
                        "dbs": {
                            "*": [
                                "REPLICATION SLAVE"
                            ]
                        },
                        "hosts": "__cluster__",
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "connection_limits": {
                            "MAX_USER_CONNECTIONS": 10
                        }
                    },
                    "test": {
                        "dbs": {
                            "testdb": [
                                "ALL PRIVILEGES"
                            ]
                        },
                        "password": {
                            "data": "test_password",
                            "encryption_version": 0
                        },
                        "services": [
                            "mysql"
                        ]
                    }
                },
                "zk_hosts": [
                    "localhost"
                ],
                "version": {
                    "major_human": "8.0",
                    "major_num": "800"
                }
            },
            "mysql default pillar": true,
            "runlist": [
                "components.mysql_cluster"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
            "sox_audit": false,
            "versions": {
                "mysql": {
                    "edition": "some hidden value",
                    "major_version": "8.0",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                },
                "xtrabackup": {
                    "edition": "some hidden value",
                    "major_version": "some hidden value",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                }
            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """

  @replication_source
  Scenario Outline: Modifying host with unsetting replication source with empty-string/null works
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": <EmptyValue>
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify hosts in MySQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "backupPriority": 0,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 7,
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "backupPriority": 5,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "priority": 0,
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "backupPriority": 0,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "priority": 0,
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas"
            }
        ]
    }
    """
    Examples:
      | EmptyValue |
      | null       |
      | ""         |

  @replication_source
  Scenario Outline: Modifying host with invalid params fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "man-1.db.yandex.net"
            ]
        }
    }
    """
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": <specs>
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | specs                                                                                                       | status | error code | message                                                                         |
      | []                                                                                                          | 400    | 3          | No changes detected                                                             |
      | [{"hostName": "sas-1.db.yandex.net"}, {"hostName": "myt-1.db.yandex.net"}]                                  | 501    | 12         | Updating multiple hosts at once is not supported yet                            |
      | [{"hostName": "vla-1.db.yandex.net"}]                                                                       | 404    | 5          | Host 'vla-1.db.yandex.net' does not exist                                       |
      | [{"hostName": "sas-1.db.yandex.net"}]                                                                       | 400    | 3          | No changes detected                                                             |
      | [{"hostName": "sas-1.db.yandex.net", "replicationSource": "nohost.db.yandex.net"}]                          | 404    | 5          | Host 'nohost.db.yandex.net' does not exist                                      |
      | [{"hostName": "sas-1.db.yandex.net", "replicationSource": null}]                                            | 400    | 3          | No changes detected                                                             |
      | [{"hostName": "sas-1.db.yandex.net", "replicationSource": ""}]                                              | 400    | 3          | No changes detected                                                             |
      | [{"hostName": "man-1.db.yandex.net", "replicationSource": "sas-1.db.yandex.net"}]                           | 400    | 3          | No changes detected                                                             |
      | [{"hostName": "sas-1.db.yandex.net", "replicationSource": "man-1.db.yandex.net"}]                           | 422    | 3          | Replication chain should end on HA host                                         |

  @replication_source
  Scenario: Partial replication cycles are not possible
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    And we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "iva-1.db.yandex.net",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Replication chain should end on HA host"
    }
    """

  Scenario: Full replication cycles are not possible
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "iva-1.db.yandex.net",
            "replicationSource": "myt-1.db.yandex.net"
        }]
    }
    """
    And we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "myt-1.db.yandex.net",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "At least one HA host is required"
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    Then we get response with status 200
    And each path on body evaluates to
      | $.hosts[0].name               | "iva-1.db.yandex.net" |
      | $.hosts[0].replicationSource  | "myt-1.db.yandex.net" |
      | $.hosts[1].name               | "myt-1.db.yandex.net" |
      | $.hosts[2].name               | "sas-1.db.yandex.net" |
      | $.hosts[2].replicationSource  | "iva-1.db.yandex.net" |
    And body does not contain following paths
      | $.hosts[1].replicationSource  |

  @replication_source
  Scenario: Test rpl_semi_sync_master_wait_for_slave_count disallows to remove hosts
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_8_0": {
                "rplSemiSyncMasterWaitForSlaveCount": 2
            }
        }
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "not enough HA replicas remains! Should be not less than rpl_semi_sync_master_wait_for_slave_count (now 2)"
    }
    """
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "not enough HA replicas remains! Should be not less than rpl_semi_sync_master_wait_for_slave_count (now 2)"
    }
    """

  @replication_source
  Scenario: Test rpl_semi_sync_master_wait_for_slave_count with insufficient HA hosts
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_8_0": {
                "rplSemiSyncMasterWaitForSlaveCount": 2
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "rpl_semi_sync_master_wait_for_slave_count should be less or equal than number of HA replicas"
    }
    """

  @events
  Scenario: Modifying host with setting backup priority works
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "backupPriority": 10
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify hosts in MySQL cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.UpdateClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    Then we get response with status 200
    And each path on body evaluates to
      | $.hosts[0].name               | "iva-1.db.yandex.net" |
      | $.hosts[0].backupPriority     | 0                     |
      | $.hosts[1].name               | "myt-1.db.yandex.net" |
      | $.hosts[1].backupPriority     | 5                     |
      | $.hosts[2].name               | "sas-1.db.yandex.net" |
      | $.hosts[2].backupPriority     | 10                    |
    When we GET "/api/v1.0/config/sas-1.db.yandex.net"
    Then we get response with status 200 and body equals to "default sas host config" data with following changes
      | $.data.mysql.backup_priority | 10 |

  @events
  Scenario: Modifying host with setting priority works
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "priority": 15
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify hosts in MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.UpdateClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "sas-1.db.yandex.net"
            ]
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "update_host_specs":  [{
                "host_name": "sas-1.db.yandex.net",
                "priority": 15
             }]
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    Then we get response with status 200
    And each path on body evaluates to
      | $.hosts[0].name         | "iva-1.db.yandex.net" |
      | $.hosts[0].priority     | 7                     |
      | $.hosts[1].name         | "myt-1.db.yandex.net" |
      | $.hosts[1].priority     | 0                     |
      | $.hosts[2].name         | "sas-1.db.yandex.net" |
      | $.hosts[2].priority     | 15                    |
    When we GET "/api/v1.0/config/sas-1.db.yandex.net"
    Then we get response with status 200 and body equals to "default sas host config" data with following changes
      | $.data.mysql.priority | 15 |

  @events
  Scenario: Modifying host with setting negative priority fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "priority": -10
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nupdateHostSpecs.0.priority: Must be between 0 and 100."
    }
    """

  Scenario Outline: Modifying host with setting to same value fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "<fieldName>": <fieldValue>
        }]
    }
    """
    And we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "<fieldName>": <fieldValue>
        }]
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """
    Examples:
      | fieldName         | fieldValue            |
      | backupPriority    | 5                     |
      | priority          | 5                     |
      | replicationSource | "iva-1.db.yandex.net" |

  @events
  Scenario Outline: Modifying host with setting incorrect value fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "<fieldName>": <fieldValue>
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "<message>"
    }
    """
    Examples:
      | fieldName         | fieldValue | message                                                                               |
      | backupPriority    | -10        | The request is invalid.\nupdateHostSpecs.0.backupPriority: Must be between 0 and 100. |

  @replication_source
  Scenario: Modifying host parameters doesn't reset each other
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net",
            "backupPriority": 15,
            "priority": 8
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id2"
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "backupPriority": 10
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id3"
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "backupPriority": 0,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 7,
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "backupPriority": 5,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 0,
                "role": "REPLICA",
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "backupPriority": 0,
                "clusterId": "cid1",
                "backupPriority": 10,
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 8,
                "role": "REPLICA",
                "replicationSource": "iva-1.db.yandex.net",
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas"
            }
        ]
    }
    """

  Scenario: Adding host works
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "man-1.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "backupPriority": 0,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 7,
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "backupPriority": 0,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 0,
                "role": "UNKNOWN",
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [],
                "subnetId": "",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "backupPriority": 5,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 0,
                "role": "REPLICA",
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "backupPriority": 0,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "priority": 0,
                "role": "REPLICA",
                "replicaType": "UNKNOWN",
                "replicaUpstream": null,
                "replicaLag": null,
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "MYSQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas"
            }
        ]
    }
    """


  @replication_source
  Scenario: Adding cascade replica works
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "man-1.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    Then we get response with status 200
    And each path on body evaluates to
      | $.hosts[0].name               | "iva-1.db.yandex.net" |
      | $.hosts[1].name               | "man-1.db.yandex.net" |
      | $.hosts[1].replicationSource  | "sas-1.db.yandex.net" |
      | $.hosts[2].name               | "myt-1.db.yandex.net" |
      | $.hosts[3].name               | "sas-1.db.yandex.net" |
    And body does not contain following paths
      | $.hosts[0].replicationSource  |
      | $.hosts[2].replicationSource  |
      | $.hosts[3].replicationSource  |


  Scenario: Adding host with backup priority works
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "backupPriority": 15
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "man-1.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    Then we get response with status 200
    And each path on body evaluates to
      | $.hosts[0].name               | "iva-1.db.yandex.net" |
      | $.hosts[0].backupPriority     | 0                     |
      | $.hosts[1].name               | "man-1.db.yandex.net" |
      | $.hosts[1].backupPriority     | 15                    |
      | $.hosts[2].name               | "myt-1.db.yandex.net" |
      | $.hosts[2].backupPriority     |  5                    |
      | $.hosts[3].name               | "sas-1.db.yandex.net" |
      | $.hosts[3].backupPriority     | 0                     |


  Scenario Outline: Adding host with invalid params fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": <hosts>
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | hosts                                                            | status | error code | message                                                                                                               |
      | []                                                               | 422    | 3          | No hosts to add are specified                                                                                         |
      | [{"zoneId": "man"}, {"zoneId": "man"}]                           | 501    | 12         | Adding multiple hosts at once is not supported yet                                                                    |
      | [{"zoneId": "nodc"}]                                             | 422    | 3          | The request is invalid.\nhostSpecs.0.zoneId: Invalid value, valid value is one of ['iva', 'man', 'myt', 'sas', 'vla'] |
      | [{"zoneId": "vla", "replicationSource": "nohost.db.yandex.net"}] | 404    | 5          | Host 'nohost.db.yandex.net' does not exist                                                                            |
      | [{"zoneId": "vla", "priority": -10}]                             | 422    | 3          | The request is invalid.\nhostSpecs.0.priority: Must be between 0 and 100.                                             |
      | [{"zoneId": "vla", "backupPriority": -10}]                       | 422    | 3          | The request is invalid.\nhostSpecs.0.backupPriority: Must be between 0 and 100.                                       |

  Scenario: Deleting host works
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete hosts from MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.DeleteClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    And each path on body evaluates to
      | $.hosts[0].name               | "iva-1.db.yandex.net" |
      | $.hosts[1].name               | "myt-1.db.yandex.net" |
    And body does not contain following paths
      | $.hosts[2].name  |

  Scenario Outline: Deleting host with invalid params fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": <host list>
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | host list                                      | status | error code | message                                              |
      | []                                             | 422    | 3          | No hosts to delete are specified                     |
      | ["sas-1.db.yandex.net", "myt-1.db.yandex.net"] | 501    | 12         | Deleting multiple hosts at once is not supported yet |
      | ["nohost"]                                     | 404    | 5          | Host 'nohost' does not exist                         |

  Scenario: Deleting host with replicas fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "myt-1.db.yandex.net"
        }]
    }
    """
    And we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["myt-1.db.yandex.net"]
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Host 'myt-1.db.yandex.net' has active replicas 'sas-1.db.yandex.net'"
    }
    """

  Scenario: User list works
    When we GET "/mdb/mysql/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["ALL_PRIVILEGES"]
                    }
                ]
            }
        ]
    }
    """

  Scenario: User get works
    When we GET "/mdb/mysql/1.0/clusters/cid1/users/test"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "permissions": [
            {
                "databaseName": "testdb",
                "roles": ["ALL_PRIVILEGES"]
            }
        ]
    }
    """

  Scenario: Nonexistent user get fails
    When we GET "/mdb/mysql/1.0/clusters/cid1/users/test2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "User 'test2' does not exist"
    }
    """

  Scenario Outline: Adding user <name> with invalid params fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "<name>",
            "password": "<password>",
            "permissions": [{
                "databaseName": "<database>",
                "roles": ["<role>"]
            }]
        }
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<error message>"
    }
    """
    Examples:
      | name          | password | database | role           | status | error code | error message                                                                                                                                                                                                                                                                                                 |
      | test          | password | testdb   | ALL_PRIVILEGES | 409    | 6          | User 'test' already exists                                                                                                                                                                                                                                                                                    |
      | test2         | password | nodb     | ALL_PRIVILEGES | 404    | 5          | Database 'nodb' does not exist                                                                                                                                                                                                                                                                                |
      | test3         | password | testdb   | norole         | 422    | 3          | The request is invalid.\nuserSpec.permissions.0.roles.0: Invalid value 'norole', allowed values: ALL_PRIVILEGES, ALTER, ALTER_ROUTINE, CREATE, CREATE_ROUTINE, CREATE_TEMPORARY_TABLES, CREATE_VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK_TABLES, REFERENCES, SELECT, SHOW_VIEW, TRIGGER, UPDATE |
      | b@dn@me!      | password | testdb   | ALL_PRIVILEGES | 422    | 3          | The request is invalid.\nuserSpec.name: User name 'b@dn@me!' does not conform to naming rules                                                                                                                                                                                                                 |
      | user          | short    | testdb   | ALL_PRIVILEGES | 422    | 3          | The request is invalid.\nuserSpec.password: Password must be between 8 and 128 characters long                                                                                                                                                                                                                |
      | root          | password | testdb   | ALL_PRIVILEGES | 422    | 3          | The request is invalid.\nuserSpec.name: User name 'root' is not allowed                                                                                                                                                                                                                                       |
      | mysql.sys     | password | testdb   | ALL_PRIVILEGES | 422    | 3          | The request is invalid.\nuserSpec.name: User name 'mysql.sys' does not conform to naming rules                                                                                                                                                                                                                |
      | mysql.session | password | testdb   | ALL_PRIVILEGES | 422    | 3          | The request is invalid.\nuserSpec.name: User name 'mysql.session' does not conform to naming rules                                                                                                                                                                                                            |
      | name          | b%dpassd | testdb   | ALL_PRIVILEGES | 422    | 3          | The request is invalid.\nuserSpec.password: Password 'b%dpassd' does not conform to naming rules                                                                                                                                                                                                              |
      | name          | b'dpassd | testdb   | ALL_PRIVILEGES | 422    | 3          | The request is invalid.\nuserSpec.password: Password 'b'dpassd' does not conform to naming rules                                                                                                                                                                                                              |

  Scenario: Adding user with empty database acl works
    When we POST "/mdb/mysql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": []
    }
    """

  @events
  Scenario: Adding and modifying user with params works
    When we POST "/mdb/mysql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test3",
            "password": "password",
            "globalPermissions": ["REPLICATION_SLAVE"],
            "connectionLimits": {
                "maxQuestionsPerHour": 5,
                "maxUpdatesPerHour": 6,
                "maxConnectionsPerHour": 42,
                "maxUserConnections": 33
            },
            "authenticationPlugin": "SHA256_PASSWORD"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test3"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.CreateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test3"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "user_spec": {
                "name": "test3",
                "global_permissions": ["REPLICATION_SLAVE"],
                "connection_limits": {
                    "max_questions_per_hour": 5,
                    "max_updates_per_hour": 6,
                    "max_connections_per_hour": 42,
                    "max_user_connections": 33
                },
                "authentication_plugin": "SHA256_PASSWORD"
            }
        }
    }
    """
    And event body for event "worker_task_id2" does not contain following paths
      | $.request_parameters.user_spec.password  |
    When we GET "/api/v1.0/config/sas-1.db.yandex.net"
    Then we get response with status 200 and body equals to "default sas host config" data with following changes
      | $.data.mysql.users.test3.dbs.*[0]                                   | REPLICATION SLAVE |
      | $.data.mysql.users.test3.dbs.testdb[0]                              | ALL PRIVILEGES    |
      | $.data.mysql.users.test3.password.data                              | password          |
      | $.data.mysql.users.test3.password.encryption_version                | 0                 |
      | $.data.mysql.users.test3.services[0]                                | mysql             |
      | $.data.mysql.users.test3.connection_limits.MAX_QUERIES_PER_HOUR     | 5                 |
      | $.data.mysql.users.test3.connection_limits.MAX_UPDATES_PER_HOUR     | 6                 |
      | $.data.mysql.users.test3.connection_limits.MAX_CONNECTIONS_PER_HOUR | 42                |
      | $.data.mysql.users.test3.connection_limits.MAX_USER_CONNECTIONS     | 33                |
      | $.data.mysql.users.test3.plugin                                     | sha256_password   |
    When we GET "/mdb/mysql/1.0/clusters/cid1/users/test3"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test3",
        "permissions": [{
            "databaseName": "testdb",
            "roles": ["ALL_PRIVILEGES"]
        }],
        "globalPermissions": ["REPLICATION_SLAVE"],
        "connectionLimits": {
            "maxQuestionsPerHour": 5,
            "maxUpdatesPerHour": 6,
            "maxConnectionsPerHour": 42,
            "maxUserConnections": 33
        },
        "authenticationPlugin": "SHA256_PASSWORD"
    }
    """
    When we PATCH "/mdb/mysql/1.0/clusters/cid1/users/test3" with data
    """
    {
        "password": "password",
        "permissions": [],
        "globalPermissions": ["REPLICATION_CLIENT"],
        "connectionLimits": {
            "maxQuestionsPerHour": 2,
            "maxUpdatesPerHour": 10,
            "maxUserConnections": 20
        },
        "authenticationPlugin": "CACHING_SHA2_PASSWORD"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in MySQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test3"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.mysql.UpdateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test3"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "user_name": "test3",
            "permissions": [],
            "global_permissions": ["REPLICATION_CLIENT"],
            "connection_limits": {
                "max_questions_per_hour": 2,
                "max_updates_per_hour": 10,
                "max_user_connections": 20
            },
            "authentication_plugin": "CACHING_SHA2_PASSWORD"
        }
    }
    """
    And event body for event "worker_task_id2" does not contain following paths
      | $.request_parameters.password   |
    And we GET "/mdb/mysql/1.0/clusters/cid1/users/test3"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test3",
        "permissions": [],
        "globalPermissions": ["REPLICATION_CLIENT"],
        "connectionLimits": {
            "maxQuestionsPerHour": 2,
            "maxUpdatesPerHour": 10,
            "maxConnectionsPerHour": 42,
            "maxUserConnections": 20
        },
        "authenticationPlugin": "CACHING_SHA2_PASSWORD"
    }
    """
    When we GET "/api/v1.0/config/sas-1.db.yandex.net"
    Then we get response with status 200 and body equals to "default sas host config" data with following changes
      | $.data.mysql.users.test3.dbs.*[0]                                   | REPLICATION CLIENT    |
      | $.data.mysql.users.test3.password.data                              | password              |
      | $.data.mysql.users.test3.password.encryption_version                | 0                     |
      | $.data.mysql.users.test3.services[0]                                | mysql                 |
      | $.data.mysql.users.test3.connection_limits.MAX_QUERIES_PER_HOUR     | 2                     |
      | $.data.mysql.users.test3.connection_limits.MAX_UPDATES_PER_HOUR     | 10                    |
      | $.data.mysql.users.test3.connection_limits.MAX_CONNECTIONS_PER_HOUR | 42                    |
      | $.data.mysql.users.test3.connection_limits.MAX_USER_CONNECTIONS     | 20                    |
      | $.data.mysql.users.test3.plugin                                     | caching_sha2_password |


  Scenario: Adding user with default database acl works
    When we POST "/mdb/mysql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb",
            "roles": ["ALL_PRIVILEGES"]
        }]
    }
    """

  Scenario: Modifying user with permission for database works
    When we POST "/mdb/mysql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/mysql/1.0/clusters/cid1/users/test2" with data
    """
    {
        "permissions": [{
            "databaseName": "testdb",
            "roles": ["SELECT", "SHOW_VIEW"]
        }]
    }
    """
    And "worker_task_id3" acquired and finished by worker
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in MySQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb",
            "roles": ["SELECT", "SHOW_VIEW"]
        }]
    }
    """
    When we GET "/mdb/mysql/1.0/operations/worker_task_id3"
    Then we get response with status 200 and body contains
    """
    {
        "response": {
            "@type":"yandex.cloud.mdb.mysql.v1.User",
            "clusterId":"cid1",
            "name":"test2",
            "permissions": [{
                 "databaseName": "testdb",
                 "roles": ["SELECT", "SHOW_VIEW"]
            }]
        }
    }
    """
    When we PATCH "/mdb/mysql/1.0/clusters/cid1/users/test2" with data
    """
    {
        "permissions": []
    }
    """
    And we GET "/mdb/mysql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": []
    }
    """

  Scenario: Backup window start option change works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "backupWindowStart": {
                "hours": 23,
                "minutes": 10
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body equals to "default cluster config" data with following changes
    | $.status                               | UPDATING |
    | $.config.backupWindowStart.hours       | 23       |
    | $.config.backupWindowStart.minutes     | 10       |
    | $.config.backupWindowStart.seconds     | 0        |
    | $.config.backupWindowStart.nanos       | 0        |

  Scenario: Allow access for webSql
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "access": {
                "webSql": true
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body equals to "default cluster config" data with following changes
    | $.config.access.webSql | true |

  Scenario: Allow access for DataLens
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "access": {
                "dataLens": true
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body equals to "default cluster config" data with following changes
    | $.config.access.dataLens | true |
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body equals to "default iva host config" data with following changes
    | $.data.access.data_lens | true |

  Scenario: Allow access for webSql and next for DataLens
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "access": {
                "webSql": true
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body equals to "default cluster config" data with following changes
    | $.config.access.webSql | true |
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "access": {
                "dataLens": true
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body equals to "default cluster config" data with following changes
    | $.config.access.webSql   | true |
    | $.config.access.dataLens | true |
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "access": {
                "webSql": false
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id4" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body equals to "default cluster config" data with following changes
    | $.config.access.dataLens | true |
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "access": {
                "dataLens": false
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "done": false,
        "id": "worker_task_id5",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id5" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body equals to "default cluster config" data

  @events
  Scenario: Adding permission for database works
    When we POST "/mdb/mysql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test3",
            "password": "password",
            "permissions": []
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mysql/1.0/clusters/cid1/users/test3:grantPermission" with data
    """
    {
        "permission": {
            "databaseName": "testdb",
            "roles": ["SELECT"]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Grant permission to user in MySQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.GrantUserPermissionMetadata",
            "clusterId": "cid1",
            "userName": "test3"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.mysql.GrantUserPermission" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test3"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "user_name": "test3",
            "permission": {
                "database_name": "testdb",
                "roles": ["SELECT"]
            }
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1/users/test3"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test3",
        "permissions": [{
            "databaseName": "testdb",
            "roles": ["SELECT"]
        }]
    }
    """
    When we POST "/mdb/mysql/1.0/clusters/cid1/users/test3:grantPermission" with data
    """
    {
        "permission": {
            "databaseName": "testdb",
            "roles": ["UPDATE"]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Grant permission to user in MySQL cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.GrantUserPermissionMetadata",
            "clusterId": "cid1",
            "userName": "test3"
        }
    }
    """
    When "worker_task_id4" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1/users/test3"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test3",
        "permissions": [{
            "databaseName": "testdb",
            "roles": ["SELECT", "UPDATE"]
        }]
    }
    """

  Scenario Outline: Adding permission for database with invalid params fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/users/<name>:grantPermission" with data
    """
    {
        "permission": {
            "databaseName": "<database>",
            "roles": ["<role>"]
        }
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | name   | database | role   | status | error code | message                                                                                                                                                                                                                                                                                           |
      | nouser | testdb   | SELECT | 404    | 5          | User 'nouser' does not exist                                                                                                                                                                                                                                                                      |
      | test   | nodb     | SELECT | 404    | 5          | Database 'nodb' does not exist                                                                                                                                                                                                                                                                    |
      | test   | testdb   | norole | 422    | 3          | The request is invalid.\npermission.roles.0: Invalid value 'norole', allowed values: ALL_PRIVILEGES, ALTER, ALTER_ROUTINE, CREATE, CREATE_ROUTINE, CREATE_TEMPORARY_TABLES, CREATE_VIEW, DELETE, DROP, EVENT, EXECUTE, INDEX, INSERT, LOCK_TABLES, REFERENCES, SELECT, SHOW_VIEW, TRIGGER, UPDATE |

  Scenario: Revoking permission for database works
    When we POST "/mdb/mysql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": [{
                "databaseName": "testdb",
                "roles": ["SELECT", "UPDATE"]
            }]
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mysql/1.0/clusters/cid1/users/test2:revokePermission" with data
    """
    {
        "permission": {
            "databaseName": "testdb",
            "roles": ["UPDATE"]
        }

    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Revoke permission from user in MySQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.RevokeUserPermissionMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.mysql.RevokeUserPermission" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "user_name": "test2",
            "permission": {
                "database_name": "testdb",
                "roles": ["UPDATE"]
            }
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb",
            "roles": ["SELECT"]
        }]
    }
    """
    When we POST "/mdb/mysql/1.0/clusters/cid1/users/test2:revokePermission" with data
    """
    {
        "permission": {
            "databaseName": "testdb",
            "roles": ["SELECT"]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Revoke permission from user in MySQL cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.RevokeUserPermissionMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When "worker_task_id4" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": []
    }
    """

  Scenario Outline: Revoking permission for database with invalid params fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mysql/1.0/clusters/cid1/users/<name>:revokePermission" with data
    """
    {
        "permission": {
            "databaseName": "<database>",
            "roles": ["SELECT"]
        }
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | name   | database | status | error code | message                                                     |
      | nouser | testdb   | 404    | 5          | User 'nouser' does not exist                                |
      | test   | nodb     | 404    | 5          | Database 'nodb' does not exist                              |
      | test2  | testdb   | 409    | 6          | User 'test2' has no access to the database 'testdb'         |


  Scenario: Revoking role with invalid params fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": [{
                "databaseName": "testdb",
                "roles": ["SELECT"]
            }]
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mysql/1.0/clusters/cid1/users/test2:revokePermission" with data
    """
    {
        "permission": {
            "databaseName": "testdb",
            "roles": ["EXECUTE"]
        }
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "User 'test2' has no role EXECUTE in database 'testdb'"
    }
    """

  @events
  Scenario: Deleting user works
    When we POST "/mdb/mysql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "permissions": []
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we DELETE "/mdb/mysql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete user from MySQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.DeleteUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.mysql.DeleteUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb",
                        "roles": ["ALL_PRIVILEGES"]
                    }
                ]
            }
        ]
    }
    """

  Scenario: Deleting nonexistent user fails
    When we DELETE "/mdb/mysql/1.0/clusters/cid1/users/test2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "User 'test2' does not exist"
    }
    """

  Scenario Outline: Deleting system user <name> fails
    When we DELETE "/mdb/mysql/1.0/clusters/cid1/users/<name>"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "<message>"
    }
    """
    Examples:
      | name        | message                       |
      | admin       | User 'admin' does not exist   |
      | monitor     | User 'monitor' does not exist |
      | root        | User 'root' does not exist    |
      | repl        | User 'repl' does not exist    |

  Scenario: Changing user password works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1/users/test" with data
    """
    {
        "password": "changed password"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test"
        }
    }
    """

  Scenario Outline: Changing system user <name> password fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1/users/<name>" with data
    """
    {
        "password": "changed password"
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "<message>"
    }
    """
    Examples:
      | name        | message                       |
      | admin       | User 'admin' does not exist   |
      | monitor     | User 'monitor' does not exist |
      | root        | User 'root' does not exist    |
      | repl        | User 'repl' does not exist    |

  Scenario: Database get works
    When we GET "/mdb/mysql/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "testdb"
    }
    """

  Scenario: Nonexistent database get fails
    When we GET "/mdb/mysql/1.0/clusters/cid1/databases/testdb2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Database 'testdb2' does not exist"
    }
    """

  Scenario: Database list works
    When we GET "/mdb/mysql/1.0/clusters/cid1/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": [{
            "clusterId": "cid1",
            "name": "testdb"
        }]
    }
    """

  Scenario Outline: Adding database <name> with invalid params fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/databases" with data
    """
    {
       "databaseSpec": {
           "name": "<name>"
       }
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | name    | status | error code | message                                                                                              |
      | testdb  | 409    | 6          | Database 'testdb' already exists                                                                     |
      | b@dN@me | 422    | 3          | The request is invalid.\ndatabaseSpec.name: Database name 'b@dN@me' does not conform to naming rules |
      | sys     | 422    | 3          | The request is invalid.\ndatabaseSpec.name: Database name 'sys' is not allowed                       |

  @events
  Scenario: Adding database works
    When we POST "/mdb/mysql/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb2"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add database to MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.CreateDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.CreateDatabase" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "database_name": "testdb2"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "database_spec": {
                "name": "testdb2"
            }
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/databases/testdb2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "testdb2"
    }
    """

  @events
  Scenario: Deleting database works
    When we DELETE "/mdb/mysql/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete database from MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.DeleteDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.DeleteDatabase" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "database_name": "testdb"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "database_name": "testdb"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": []
    }
    """

  Scenario: Deleting nonexistent database fails
    When we DELETE "/mdb/mysql/1.0/clusters/cid1/databases/nodb"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Database 'nodb' does not exist"
    }
    """

  Scenario: Label set works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "labels": {
            "acid": "yes"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Update MySQL cluster metadata",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "labels": {
                "acid": "yes"
            }
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "labels": {
            "acid": "yes"
        }
    }
    """

  Scenario: Description set works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "description": "my cool description"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "my cool description"
    }
    """

  Scenario: Change disk size to invalid value fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 1
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Invalid disk_size, must be between or equal 10737418240 and 2199023255552"
    }
    """

  Scenario: Change disk size to same value fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 10737418240
            }
        }
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """

  Scenario: Change disk size to valid value works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 21474836480
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 3.0,
        "memoryUsed": 12884901888,
        "ssdSpaceUsed": 64424509440
    }
    """

  Scenario: Scaling cluster up works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.porto.2"
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 6.0,
        "memoryUsed": 25769803776,
        "ssdSpaceUsed": 32212254720
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body equals to "default cluster config" data with following changes
    | $.config.resources.resourcePresetId                           | s1.porto.2 |
    | $.config.mysqlConfig_8_0.defaultConfig.innodbBufferPoolSize   | 4294967296 |
    | $.config.mysqlConfig_8_0.defaultConfig.maxConnections         | 256        |
    | $.config.mysqlConfig_8_0.effectiveConfig.innodbBufferPoolSize | 4294967296 |
    | $.config.mysqlConfig_8_0.effectiveConfig.maxConnections       | 256        |

  Scenario: Scaling cluster down works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.porto.2"
            }
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 3.0,
        "memoryUsed": 12884901888,
        "ssdSpaceUsed": 32212254720
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body equals to "default cluster config" data with following changes
    | $.config.resources.resourcePresetId | s1.porto.1 |

  Scenario: Cluster name change works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "name": "changed"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Update MySQL cluster metadata",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "name": "changed"
    }
    """

  Scenario: Cluster name change to same value fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "name": "test"
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """

  Scenario: Cluster name change with duplicate value fails
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_8_0": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "another test cluster"
    }
    """
    And we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "name": "test2"
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Cluster 'test2' already exists"
    }
    """

  @events
  Scenario: Cluster move works
    When we POST "/mdb/mysql/1.0/clusters/cid1:move" with data
    """
    {
        "destinationFolderId": "folder2"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Move MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.MoveClusterMetadata",
            "clusterId": "cid1",
            "destinationFolderId": "folder2",
            "sourceFolderId": "folder1"
        }
    }
    """
    When we GET "/mdb/1.0/operations/worker_task_id3"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Move MySQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.MoveClusterMetadata",
            "clusterId": "cid1",
            "destinationFolderId": "folder2",
            "sourceFolderId": "folder1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.MoveCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "destination_folder_id": "folder2"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder2"
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 0,
        "cpuUsed": 0.0,
        "memoryUsed": 0,
        "ssdSpaceUsed": 0
    }
    """
    When we GET "/mdb/v1/quota/cloud2"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud2",
        "clustersUsed": 1,
        "cpuUsed": 3.0,
        "memoryUsed": 12884901888,
        "ssdSpaceUsed": 32212254720
    }
    """

  @events
  Scenario: Manual failover works
    When we POST "/mdb/mysql/1.0/clusters/cid1:startFailover"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start manual failover on MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.StartClusterFailoverMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.StartClusterFailover" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.mysql.v1.Cluster",
        "id": "cid1",
        "name": "test",
        "config": {
            "access": {
                "dataLens": false,
                "webSql": false,
                "dataTransfer": false,
                "serverless": false
            },
            "backupRetainPeriodDays": 7,
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "nanos": 100,
                "seconds": 30
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "mysqlConfig_8_0": {
                "defaultConfig": {
                    "innodbBufferPoolSize": 1610612736,
                    "characterSetServer": "utf8mb4",
                    "collationServer": "utf8mb4_0900_ai_ci",
                    "defaultTimeZone": "Europe/Moscow",
                    "groupConcatMaxLen": 1024,
                    "netReadTimeout": 30,
                    "netWriteTimeout": 60,
                    "tmpTableSize": 16777216,
                    "maxHeapTableSize": 16777216,
                    "innodbFlushLogAtTrxCommit": 1,
                    "innodbLockWaitTimeout": 50,
                    "transactionIsolation": "REPEATABLE_READ",
                    "logErrorVerbosity": 3,
                    "longQueryTime": 0.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
                    "auditLog": false,
                    "generalLog": false,
                    "maxAllowedPacket": 16777216,
                    "maxConnections": 128,
                    "sqlMode": [
                        "ONLY_FULL_GROUP_BY",
                        "STRICT_TRANS_TABLES",
                        "NO_ZERO_IN_DATE",
                        "NO_ZERO_DATE",
                        "ERROR_FOR_DIVISION_BY_ZERO",
                        "NO_ENGINE_SUBSTITUTION",
                        "NO_DIR_IN_CREATE"
                    ],
                    "innodbAdaptiveHashIndex" : true,
                    "innodbNumaInterleave" : false,
                    "innodbLogBufferSize" : 16777216,
                    "innodbLogFileSize" : 268435456,
                    "innodbIoCapacity" : 200,
                    "innodbIoCapacityMax" : 2000,
                    "innodbReadIoThreads" : 4,
                    "innodbWriteIoThreads" : 4,
                    "innodbPurgeThreads" : 4,
                    "innodbThreadConcurrency" : 0,
                    "threadCacheSize" : 10,
                    "threadStack" : 196608,
                    "joinBufferSize" : 262144,
                    "sortBufferSize" : 262144,
                    "tableDefinitionCache" : 2000,
                    "tableOpenCache" : 4000,
                    "tableOpenCacheInstances" : 16,
                    "explicitDefaultsForTimestamp" : true,
                    "autoIncrementIncrement" : 1,
                    "autoIncrementOffset" : 1,
                    "syncBinlog" : 1,
                    "binlogCacheSize" : 32768,
                    "binlogGroupCommitSyncDelay" : 0,
                    "binlogRowImage" : "FULL",
                    "binlogRowsQueryLogEvents" : false,
                    "mdbPreserveBinlogBytes" : 1073741824,
                    "rplSemiSyncMasterWaitForSlaveCount" : 1,
                    "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                    "slaveParallelType" : "DATABASE",
                    "slaveParallelWorkers" : 0,
                    "defaultAuthenticationPlugin": "CACHING_SHA2_PASSWORD",
                    "interactiveTimeout": 28800,
                    "waitTimeout": 28800,
                    "mdbPriorityChoiceMaxLag": 60,
                    "mdbOfflineModeEnableLag": 86400,
                    "mdbOfflineModeDisableLag": 300,
                    "rangeOptimizerMaxMemSize": 8388608,
                    "innodbOnlineAlterLogMaxSize": 134217728,
                    "innodbFtMinTokenSize": 3,
                    "innodbFtMaxTokenSize": 84,
                    "innodbStrictMode": true,
                    "maxDigestLength": 1024,
                    "lowerCaseTableNames": 0,
                    "innodbPageSize": 16384,
                    "maxSpRecursionDepth": 0,
                    "innodbCompressionLevel": 6
                },
                "effectiveConfig": {
                    "innodbBufferPoolSize": 1610612736,
                    "characterSetServer": "utf8mb4",
                    "collationServer": "utf8mb4_0900_ai_ci",
                    "defaultTimeZone": "Europe/Moscow",
                    "groupConcatMaxLen": 1024,
                    "netReadTimeout": 30,
                    "netWriteTimeout": 60,
                    "tmpTableSize": 16777216,
                    "maxHeapTableSize": 16777216,
                    "innodbFlushLogAtTrxCommit": 1,
                    "innodbLockWaitTimeout": 50,
                    "transactionIsolation": "REPEATABLE_READ",
                    "logErrorVerbosity": 3,
                    "longQueryTime": 0.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
                    "auditLog": false,
                    "generalLog": false,
                    "maxAllowedPacket": 16777216,
                    "maxConnections": 128,
                    "sqlMode": [
                        "ONLY_FULL_GROUP_BY",
                        "STRICT_TRANS_TABLES",
                        "NO_ZERO_IN_DATE",
                        "NO_ZERO_DATE",
                        "ERROR_FOR_DIVISION_BY_ZERO",
                        "NO_ENGINE_SUBSTITUTION",
                        "NO_DIR_IN_CREATE"
                    ],
                    "innodbAdaptiveHashIndex" : true,
                    "innodbNumaInterleave" : false,
                    "innodbLogBufferSize" : 16777216,
                    "innodbLogFileSize" : 268435456,
                    "innodbIoCapacity" : 200,
                    "innodbIoCapacityMax" : 2000,
                    "innodbReadIoThreads" : 4,
                    "innodbWriteIoThreads" : 4,
                    "innodbPurgeThreads" : 4,
                    "innodbThreadConcurrency" : 0,
                    "threadCacheSize" : 10,
                    "threadStack" : 196608,
                    "joinBufferSize" : 262144,
                    "sortBufferSize" : 262144,
                    "tableDefinitionCache" : 2000,
                    "tableOpenCache" : 4000,
                    "tableOpenCacheInstances" : 16,
                    "explicitDefaultsForTimestamp" : true,
                    "autoIncrementIncrement" : 1,
                    "autoIncrementOffset" : 1,
                    "syncBinlog" : 1,
                    "binlogCacheSize" : 32768,
                    "binlogGroupCommitSyncDelay" : 0,
                    "binlogRowImage" : "FULL",
                    "binlogRowsQueryLogEvents" : false,
                    "mdbPreserveBinlogBytes" : 1073741824,
                    "rplSemiSyncMasterWaitForSlaveCount" : 1,
                    "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                    "slaveParallelType" : "DATABASE",
                    "slaveParallelWorkers" : 0,
                    "defaultAuthenticationPlugin": "CACHING_SHA2_PASSWORD",
                    "interactiveTimeout": 28800,
                    "waitTimeout": 28800,
                    "mdbPriorityChoiceMaxLag": 60,
                    "mdbOfflineModeEnableLag": 86400,
                    "mdbOfflineModeDisableLag": 300,
                    "rangeOptimizerMaxMemSize": 8388608,
                    "innodbOnlineAlterLogMaxSize": 134217728,
                    "innodbFtMinTokenSize": 3,
                    "innodbFtMaxTokenSize": 84,
                    "innodbStrictMode": true,
                    "maxDigestLength": 1024,
                    "lowerCaseTableNames": 0,
                    "innodbPageSize": 16384,
                    "maxSpRecursionDepth": 0,
                    "innodbCompressionLevel": 6
                },
                "userConfig": {}
            },
            "soxAudit": false,
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            },
            "version": "8.0"
        }
    }
    """

  @events
  Scenario: Manual failover with target host works
    When we POST "/mdb/mysql/1.0/clusters/cid1:startFailover" with data
    """
    {
        "hostName": "sas-1.db.yandex.net"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start manual failover on MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.StartClusterFailoverMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.StartClusterFailover" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "host_name": "sas-1.db.yandex.net"
        }
    }
    """

  Scenario: Manual failover with not existing target host fails
    When we POST "/mdb/mysql/1.0/clusters/cid1:startFailover" with data
    """
    {
        "hostName": "bla-1.db.yandex.net"
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Host 'bla-1.db.yandex.net' does not exist"
    }
    """

  @replication_source
  Scenario: Manual failover with not HA target host fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    When we POST "/mdb/mysql/1.0/clusters/cid1:startFailover" with data
    """
    {
        "hostName": "sas-1.db.yandex.net"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Target host 'sas-1.db.yandex.net' is not in HA group"
    }
    """

  @replication_source
  Scenario: Manual failover with all cascade replicas fails
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "myt-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id3" acquired and finished by worker
    When we POST "/mdb/mysql/1.0/clusters/cid1:startFailover"
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "HA group is too small to perform failover"
    }
    """

  @delete
  Scenario: Cluster removal works
    When we DELETE "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.DeleteClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 0,
        "cpuUsed": 0.0,
        "memoryUsed": 0,
        "ssdSpaceUsed": 0
    }
    """
    When we GET "/mdb/mysql/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusters": []
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 403

  @delete
  Scenario: After cluster delete cluster.name can be reused
    When we DELETE "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200

  @delete
  Scenario: Cluster with running operations can not be deleted
    When we run query
    """
    UPDATE dbaas.worker_queue
       SET result = NULL,
           end_ts = NULL
    """
    And we DELETE "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Cluster 'cid1' has active tasks"
    }
    """

  @delete
  Scenario: Cluster with failed operations can be deleted
    When we run query
    """
    UPDATE dbaas.worker_queue
       SET result = false
    """
    And we DELETE "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200

  @delete @operations
  Scenario: After cluster delete cluster operations are shown
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "description": "changed"
    }
    """
    Then we get response with status 200
    When we DELETE "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200
    When we run query
    """
    UPDATE dbaas.worker_queue
       SET start_ts = CASE WHEN start_ts ISNULL THEN null
                      ELSE TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
                      END,
             end_ts = CASE WHEN end_ts ISNULL THEN null
                      ELSE TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
                      END,
          create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we GET "/mdb/mysql/1.0/operations?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "operations": [
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Delete MySQL cluster",
                "done": false,
                "id": "worker_task_id3",
                "metadata": {
                    "@type": "yandex.cloud.mdb.mysql.v1.DeleteClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify MySQL cluster",
                "done": true,
                "id": "worker_task_id2",
                "metadata": {
                    "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00",
                "response": {}
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Create MySQL cluster",
                "done": true,
                "id": "worker_task_id1",
                "metadata": {
                    "@type": "yandex.cloud.mdb.mysql.v1.CreateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00",
                "response": {}
            }
        ]
    }
    """

  @decommission @geo
  Scenario: Create cluster host in decommissioning geo
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "ugr"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "No new resources could be created in zone 'ugr'"
    }
    """

  @stop
  Scenario: Stop cluster not implemented
    When we POST "/mdb/mysql/1.0/clusters/cid1:stop"
    Then we get response with status 501 and body contains
    """
    {
        "code": 12,
        "message": "Stop for mysql_cluster not implemented in that installation"
    }
    """

  @status
  Scenario: Cluster status
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "UPDATING"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """

  Scenario: Database creation sets charset correct:
    When we GET "/mdb/mysql/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "testdb"
    }
    """



  Scenario: Create DB with uppercase name in cluster with lowerCaseTableNames 1 fails
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "lowercase_test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_8_0": {
                "lowerCaseTableNames": 1
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "lowercase test cluster"
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mysql/1.0/clusters/cid2/databases" with data
    """
    {
        "databaseSpec": {
            "name": "TESTDB"
        }
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Database 'testdb' already exists"
    }
    """


  Scenario: Create DB with uppercase name in cluster with lowerCaseTableNames 0 succeeds
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "lowercase_test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_8_0": {
                "lowerCaseTableNames": 0
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "lowercase test cluster"
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mysql/1.0/clusters/cid2/databases" with data
    """
    {
        "databaseSpec": {
            "name": "TESTDB"
        }
    }
    """
    Then we get response with status 200

  Scenario: Decreasing innodbBufferPoolSize requires restart
    And we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_8_0": {
                "innodbBufferPoolSize": 1610612736
            }
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id without args "restart"
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_8_0": {
                "innodbBufferPoolSize": 1342177280
            }
        }
    }
    """
    And in worker_queue exists "worker_task_id3" id with args "restart" set to "true"
    And "worker_task_id3" acquired and finished by worker

  Scenario: Slow log options change works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_8_0": {
                "longQueryTime": 5,
                "logSlowRateLimit": 10,
                "logSlowRateType": "QUERY",
                "logSlowSpStatements": true,
                "slowQueryLog": true,
                "slowQueryLogAlwaysWriteTime": 15
            }
        }
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body equals to "default cluster config" data with following changes
    | $.config.mysqlConfig_8_0.userConfig.longQueryTime                    | 5     |
    | $.config.mysqlConfig_8_0.userConfig.logSlowRateLimit                 | 10    |
    | $.config.mysqlConfig_8_0.userConfig.logSlowRateType                  | QUERY |
    | $.config.mysqlConfig_8_0.userConfig.logSlowSpStatements              | true  |
    | $.config.mysqlConfig_8_0.userConfig.slowQueryLog                     | true  |
    | $.config.mysqlConfig_8_0.userConfig.slowQueryLogAlwaysWriteTime      | 15    |
    | $.config.mysqlConfig_8_0.effectiveConfig.longQueryTime               | 5     |
    | $.config.mysqlConfig_8_0.effectiveConfig.logSlowRateLimit            | 10    |
    | $.config.mysqlConfig_8_0.effectiveConfig.logSlowRateType             | QUERY |
    | $.config.mysqlConfig_8_0.effectiveConfig.logSlowSpStatements         | true  |
    | $.config.mysqlConfig_8_0.effectiveConfig.slowQueryLog                | true  |
    | $.config.mysqlConfig_8_0.effectiveConfig.slowQueryLogAlwaysWriteTime | 15    |

  Scenario: Mdb priority choice max lag change works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_8_0": {
                "mdbPriorityChoiceMaxLag": 99
            }
        }
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body equals to "default cluster config" data with following changes
    | $.config.mysqlConfig_8_0.userConfig.mdbPriorityChoiceMaxLag      | 99 |
    | $.config.mysqlConfig_8_0.effectiveConfig.mdbPriorityChoiceMaxLag | 99 |

  @events
  Scenario: Non-configspec changes should have zk_hosts in task_args
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "zk_hosts" containing:
      |localhost|


  Scenario: Adding and modifying user with FLUSH_TABLES permission works
    When we POST "/mdb/mysql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test4",
            "password": "password",
            "globalPermissions": ["REPLICATION_SLAVE"],
            "connectionLimits": {
                "maxQuestionsPerHour": 5,
                "maxUpdatesPerHour": 6,
                "maxConnectionsPerHour": 42,
                "maxUserConnections": 33
            },
            "authenticationPlugin": "SHA256_PASSWORD"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test4"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/api/v1.0/config/sas-1.db.yandex.net"
    Then we get response with status 200 and body equals to "default sas host config" data with following changes
      | $.data.mysql.users.test4.dbs.testdb[0]                              | ALL PRIVILEGES    |
      | $.data.mysql.users.test4.dbs.*[0]                                   | REPLICATION SLAVE |
      | $.data.mysql.users.test4.password.data                              | password          |
      | $.data.mysql.users.test4.password.encryption_version                | 0                 |
      | $.data.mysql.users.test4.services[0]                                | mysql             |
      | $.data.mysql.users.test4.connection_limits.MAX_QUERIES_PER_HOUR     | 5                 |
      | $.data.mysql.users.test4.connection_limits.MAX_UPDATES_PER_HOUR     | 6                 |
      | $.data.mysql.users.test4.connection_limits.MAX_CONNECTIONS_PER_HOUR | 42                |
      | $.data.mysql.users.test4.connection_limits.MAX_USER_CONNECTIONS     | 33                |
      | $.data.mysql.users.test4.plugin                                     | sha256_password   |
    And we run query
    """
    SELECT code.easy_update_pillar('cid1', 'data:mysql:users:test4:dbs:*', '["FLUSH_TABLES", "REPLICATION SLAVE"]'::jsonb)
    """
    
    When we GET "/mdb/mysql/1.0/clusters/cid1/users/test4"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test4",
        "permissions": [{
            "databaseName": "testdb",
            "roles": ["ALL_PRIVILEGES"]
        }],
        "globalPermissions": ["REPLICATION_SLAVE"],
        "connectionLimits": {
            "maxQuestionsPerHour": 5,
            "maxUpdatesPerHour": 6,
            "maxConnectionsPerHour": 42,
            "maxUserConnections": 33
        },
        "authenticationPlugin": "SHA256_PASSWORD"
    }
    """
    And cluster "cid1" pillar has value '["FLUSH_TABLES", "REPLICATION SLAVE"]' on path "data:mysql:users:test4:dbs:*"
    When we PATCH "/mdb/mysql/1.0/clusters/cid1/users/test4" with data
    """
    {
        "permissions": [],
        "globalPermissions": ["PROCESS"],
        "connectionLimits": {
            "maxQuestionsPerHour": 2,
            "maxUpdatesPerHour": 10,
            "maxUserConnections": 20
        },
        "authenticationPlugin": "CACHING_SHA2_PASSWORD"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in MySQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test4"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1/users/test4"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test4",
        "permissions": [],
        "globalPermissions": ["PROCESS"],
        "connectionLimits": {
            "maxQuestionsPerHour": 2,
            "maxUpdatesPerHour": 10,
            "maxConnectionsPerHour": 42,
            "maxUserConnections": 20
        },
        "authenticationPlugin": "CACHING_SHA2_PASSWORD"
    }
    """
    When we GET "/api/v1.0/config/sas-1.db.yandex.net"
    Then we get response with status 200 and body equals to "default sas host config" data with following changes
      | $.data.mysql.users.test4.dbs.*[1]                                   | PROCESS               |
      | $.data.mysql.users.test4.dbs.*[0]                                   | FLUSH_TABLES               |
      | $.data.mysql.users.test4.password.data                              | password              |
      | $.data.mysql.users.test4.password.encryption_version                | 0                     |
      | $.data.mysql.users.test4.services[0]                                | mysql                 |
      | $.data.mysql.users.test4.connection_limits.MAX_QUERIES_PER_HOUR     | 2                     |
      | $.data.mysql.users.test4.connection_limits.MAX_UPDATES_PER_HOUR     | 10                    |
      | $.data.mysql.users.test4.connection_limits.MAX_CONNECTIONS_PER_HOUR | 42                    |
      | $.data.mysql.users.test4.connection_limits.MAX_USER_CONNECTIONS     | 20                    |
      | $.data.mysql.users.test4.plugin                                     | caching_sha2_password |
    And cluster "cid1" pillar has value '["FLUSH_TABLES", "PROCESS"]' on path "data:mysql:users:test4:dbs:*"
