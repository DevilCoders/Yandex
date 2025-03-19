Feature: MySQL perfidag

Background:
    Given default headers
    And "create" data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "5.7",
           "mysqlConfig_5_7": {},
           "performanceDiagnostics": {
                "enabled": true
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
       "description": "test cluster",
       "networkId": "IN-PORTO-NO-NETWORK-API"
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
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "5.7",
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
                "enabled": true,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "mysqlConfig_5_7": {
                "defaultConfig": {
                    "innodbBufferPoolSize": 1610612736,
                    "characterSetServer": "latin1",
                    "collationServer": "latin1_swedish_ci",
                    "defaultTimeZone": "Europe/Moscow",
                    "groupConcatMaxLen": 1024,
                    "netReadTimeout": 30,
                    "netWriteTimeout": 60,
                    "queryCacheLimit": 1048576,
                    "queryCacheSize": 0,
                    "queryCacheType": 0,
                    "tmpTableSize": 16777216,
                    "maxHeapTableSize": 16777216,
                    "innodbFlushLogAtTrxCommit": 1,
                    "innodbLockWaitTimeout": 50,
                    "transactionIsolation": "REPEATABLE_READ",
                    "logErrorVerbosity": 3,
                    "maxConnections": 128,
                    "longQueryTime": 0.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
                    "generalLog": false,
                    "auditLog": false,
                    "maxAllowedPacket": 16777216,
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
                    "showCompatibility56": false,
                    "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                    "slaveParallelType" : "DATABASE",
                    "slaveParallelWorkers" : 0,
                    "defaultAuthenticationPlugin": "MYSQL_NATIVE_PASSWORD",
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
                    "characterSetServer": "latin1",
                    "collationServer": "latin1_swedish_ci",
                    "defaultTimeZone": "Europe/Moscow",
                    "groupConcatMaxLen": 1024,
                    "netReadTimeout": 30,
                    "netWriteTimeout": 60,
                    "queryCacheLimit": 1048576,
                    "queryCacheSize": 0,
                    "queryCacheType": 0,
                    "tmpTableSize": 16777216,
                    "maxHeapTableSize": 16777216,
                    "innodbFlushLogAtTrxCommit": 1,
                    "innodbLockWaitTimeout": 50,
                    "transactionIsolation": "REPEATABLE_READ",
                    "logErrorVerbosity": 3,
                    "maxConnections": 128,
                    "longQueryTime": 0.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
                    "generalLog": false,
                    "auditLog": false,
                    "maxAllowedPacket": 16777216,
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
                    "showCompatibility56": false,
                    "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                    "slaveParallelType" : "DATABASE",
                    "slaveParallelWorkers" : 0,
                    "defaultAuthenticationPlugin": "MYSQL_NATIVE_PASSWORD",
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
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
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
            "perf_diag": {
                "enabled": true
            },
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
                    "major_human": "5.7",
                    "major_num": "507"
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
                    "major_version": "5.7",
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


Scenario: Changing perf_diag interval works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "performanceDiagnostics": {
                "sessionsSamplingInterval": 666
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
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "5.7",
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
                "enabled": true,
                "sessionsSamplingInterval": 666,
                "statementsSamplingInterval": 600
            },
            "mysqlConfig_5_7": {
                "defaultConfig": {
                    "innodbBufferPoolSize": 1610612736,
                    "characterSetServer": "latin1",
                    "collationServer": "latin1_swedish_ci",
                    "defaultTimeZone": "Europe/Moscow",
                    "groupConcatMaxLen": 1024,
                    "netReadTimeout": 30,
                    "netWriteTimeout": 60,
                    "queryCacheLimit": 1048576,
                    "queryCacheSize": 0,
                    "queryCacheType": 0,
                    "tmpTableSize": 16777216,
                    "maxHeapTableSize": 16777216,
                    "innodbFlushLogAtTrxCommit": 1,
                    "innodbLockWaitTimeout": 50,
                    "transactionIsolation": "REPEATABLE_READ",
                    "logErrorVerbosity": 3,
                    "maxConnections": 128,
                    "longQueryTime": 0.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
                    "generalLog": false,
                    "auditLog": false,
                    "maxAllowedPacket": 16777216,
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
                    "showCompatibility56": false,
                    "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                    "slaveParallelType" : "DATABASE",
                    "slaveParallelWorkers" : 0,
                    "defaultAuthenticationPlugin": "MYSQL_NATIVE_PASSWORD",
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
                    "characterSetServer": "latin1",
                    "collationServer": "latin1_swedish_ci",
                    "defaultTimeZone": "Europe/Moscow",
                    "groupConcatMaxLen": 1024,
                    "netReadTimeout": 30,
                    "netWriteTimeout": 60,
                    "queryCacheLimit": 1048576,
                    "queryCacheSize": 0,
                    "queryCacheType": 0,
                    "tmpTableSize": 16777216,
                    "maxHeapTableSize": 16777216,
                    "innodbFlushLogAtTrxCommit": 1,
                    "innodbLockWaitTimeout": 50,
                    "transactionIsolation": "REPEATABLE_READ",
                    "logErrorVerbosity": 3,
                    "maxConnections": 128,
                    "longQueryTime": 0.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
                    "generalLog": false,
                    "auditLog": false,
                    "maxAllowedPacket": 16777216,
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
                    "showCompatibility56": false,
                    "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                    "slaveParallelType" : "DATABASE",
                    "slaveParallelWorkers" : 0,
                    "defaultAuthenticationPlugin": "MYSQL_NATIVE_PASSWORD",
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

Scenario: Disable perf_diag interval works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "performanceDiagnostics": {
                "enabled": false,
                "sessionsSamplingInterval": 777,
                "statementsSamplingInterval": 111
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
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "5.7",
            "access": {
                "webSql": false,
                "dataLens": false,
                "dataTransfer": false,
                "serverless": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 111,
                "sessionsSamplingInterval": 777
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
                "statementsSamplingInterval": 111,
                "sessionsSamplingInterval": 777
            },
            "mysqlConfig_5_7": {
                "defaultConfig": {
                    "innodbBufferPoolSize": 1610612736,
                    "characterSetServer": "latin1",
                    "collationServer": "latin1_swedish_ci",
                    "defaultTimeZone": "Europe/Moscow",
                    "groupConcatMaxLen": 1024,
                    "netReadTimeout": 30,
                    "netWriteTimeout": 60,
                    "queryCacheLimit": 1048576,
                    "queryCacheSize": 0,
                    "queryCacheType": 0,
                    "tmpTableSize": 16777216,
                    "maxHeapTableSize": 16777216,
                    "innodbFlushLogAtTrxCommit": 1,
                    "innodbLockWaitTimeout": 50,
                    "transactionIsolation": "REPEATABLE_READ",
                    "logErrorVerbosity": 3,
                    "maxConnections": 128,
                    "longQueryTime": 0.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
                    "generalLog": false,
                    "auditLog": false,
                    "maxAllowedPacket": 16777216,
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
                    "showCompatibility56": false,
                    "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                    "slaveParallelType" : "DATABASE",
                    "slaveParallelWorkers" : 0,
                    "defaultAuthenticationPlugin": "MYSQL_NATIVE_PASSWORD",
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
                    "characterSetServer": "latin1",
                    "collationServer": "latin1_swedish_ci",
                    "defaultTimeZone": "Europe/Moscow",
                    "groupConcatMaxLen": 1024,
                    "netReadTimeout": 30,
                    "netWriteTimeout": 60,
                    "queryCacheLimit": 1048576,
                    "queryCacheSize": 0,
                    "queryCacheType": 0,
                    "tmpTableSize": 16777216,
                    "maxHeapTableSize": 16777216,
                    "innodbFlushLogAtTrxCommit": 1,
                    "innodbLockWaitTimeout": 50,
                    "transactionIsolation": "REPEATABLE_READ",
                    "logErrorVerbosity": 3,
                    "maxConnections": 128,
                    "longQueryTime": 0.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
                    "generalLog": false,
                    "auditLog": false,
                    "maxAllowedPacket": 16777216,
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
                    "showCompatibility56": false,
                    "binlogTransactionDependencyTracking" : "COMMIT_ORDER",
                    "slaveParallelType" : "DATABASE",
                    "slaveParallelWorkers" : 0,
                    "defaultAuthenticationPlugin": "MYSQL_NATIVE_PASSWORD",
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
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "performanceDiagnostics": {
                "enabled": false,
                "sessionsSamplingInterval": 1844,
                "statementsSamplingInterval": 111
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
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "performanceDiagnostics": {
                "enabled": false,
                "sessionsSamplingInterval": 1844,
                "statementsSamplingInterval": 111
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
