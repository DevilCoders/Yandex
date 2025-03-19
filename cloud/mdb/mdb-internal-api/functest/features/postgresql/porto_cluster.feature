Feature: Create/Modify Porto PostgreSQL Cluster

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_PG_ALLOW_DEPRECATED_VERSIONS"]
    """
    And "create" data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "10",
           "postgresqlConfig_10": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "myt",
           "priority": 9,
           "configSpec": {
                "postgresqlConfig_10": {
                    "workMem": 65536
                }
           }
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster",
       "networkId": "IN-PORTO-NO-NETWORK-API",
       "monitoringCloudId": "cloud1"
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
                       "name": "pgbouncer",
                       "role": "Unknown",
                       "status": "Alive"
                   },
                   {
                       "name": "pg_replication",
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
                       "name": "pgbouncer",
                       "role": "Unknown",
                       "status": "Alive"
                   },
                   {
                       "name": "pg_replication",
                       "role": "Replica",
                       "replicatype": "Sync",
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
                       "name": "pgbouncer",
                       "role": "Unknown",
                       "status": "Alive"
                   },
                   {
                       "name": "pg_replication",
                       "role": "Replica",
                       "replicatype": "Async",
                       "status": "Alive"
                   }
               ]
           }
       ]
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker

  @events
  Scenario: Cluster creation works
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "backupRetainPeriodDays": 7,
            "monitoringCloudId": "cloud1",
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
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
                "link": "https://yasm.yandex-team.ru/template/panel/dbaas_postgres_metrics/cid=cid1;dbname=testdb",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid1&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-postgres",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/postgresql_cluster/cid1?section=monitoring",
                "name": "Console"
            }
        ],
        "name": "test",
        "networkId": "",
        "status": "RUNNING"
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 3,
        "memoryUsed": 12884901888,
        "ssdSpaceUsed": 32212254720
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "backup": {
                "retain_period": 7,
                "use_backup_service": true,
                "max_incremental_steps": 6,
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                }
            },
            "cluster_nodes": {
                "ha": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ]
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "config": {
                "array_nulls": true,
                "archive_timeout": "10min",
                "auto_explain_log_min_duration": -1,
                "auto_explain_log_analyze": false,
                "auto_explain_log_buffers": false,
                "auto_explain_log_timing": false,
                "auto_explain_log_triggers": false,
                "auto_explain_log_verbose": false,
                "auto_explain_log_nested_statements": false,
                "auto_explain_sample_rate": 1.0,
                "pg_hint_plan_enable_hint": true,
                "pg_hint_plan_enable_hint_table": false,
                "pg_hint_plan_message_level":  "log",
                "pg_hint_plan_debug_print": "off",
                "user_shared_preload_libraries": [],
                "effective_io_concurrency": 1,
                "effective_cache_size": "100GB",
                "auto_kill_timeout": "12 hours",
                "autovacuum_analyze_scale_factor": 0.1,
                "autovacuum_naptime": "15s",
                "autovacuum_vacuum_scale_factor": 0.2,
                "autovacuum_work_mem": -1,
                "backend_flush_after": 0,
                "backslash_quote": "safe_encoding",
                "bgwriter_delay": "200ms",
                "bgwriter_flush_after": "512kB",
                "bgwriter_lru_maxpages": 100,
                "bgwriter_lru_multiplier": 2.0,
                "bytea_output": "hex",
                "checkpoint_completion_target": 0.5,
                "checkpoint_flush_after": "256kB",
                "checkpoint_timeout": "5min",
                "client_min_messages": "notice",
                "constraint_exclusion": "partition",
                "cursor_tuple_fraction": 0.1,
                "deadlock_timeout": "1s",
                "default_statistics_target": 100,
                "default_transaction_isolation": "read committed",
                "default_transaction_read_only": false,
                "default_with_oids": false,
                "enable_bitmapscan": true,
                "enable_hashagg": true,
                "enable_hashjoin": true,
                "enable_indexonlyscan": true,
                "enable_indexscan": true,
                "enable_material": true,
                "enable_mergejoin": true,
                "enable_nestloop": true,
                "enable_parallel_append": true,
                "enable_parallel_hash": true,
                "enable_partition_pruning": true,
                "enable_partitionwise_aggregate": false,
                "enable_partitionwise_join": false,
                "enable_seqscan": true,
                "enable_sort": true,
                "enable_tidscan": true,
                "escape_string_warning": true,
                "exit_on_error": false,
                "force_parallel_mode": "off",
                "from_collapse_limit": 8,
                "gin_pending_list_limit": "4MB",
                "idle_in_transaction_session_timeout": 0,
                "jit": false,
                "join_collapse_limit": 8,
                "lo_compat_privileges": false,
                "lock_timeout": 0,
                "log_checkpoints": false,
                "log_connections": false,
                "log_disconnections": false,
                "log_duration": false,
                "log_error_verbosity": "default",
                "log_lock_waits": false,
                "log_min_duration_statement": -1,
                "log_min_error_statement": "error",
                "log_min_messages": "warning",
                "log_statement": "none",
                "log_temp_files": -1,
                "log_transaction_sample_rate": 0,
                "track_activity_query_size": 1024,
                "maintenance_work_mem": "64MB",
                "max_client_pool_conn": 8000,
                "max_locks_per_transaction": 64,
                "max_logical_replication_workers": 4,
                "max_parallel_maintenance_workers": 2,
                "max_parallel_workers": 8,
                "max_parallel_workers_per_gather": 2,
                "max_pred_locks_per_transaction": 64,
                "max_prepared_transactions": 0,
                "max_replication_slots": 20,
                "max_slot_wal_keep_size": -1,
                "max_standby_streaming_delay": "30s",
                "max_wal_senders": 20,
                "max_worker_processes": 8,
                "old_snapshot_threshold": -1,
                "operator_precedence_warning": false,
                "parallel_leader_participation": true,
                "pgbouncer": {
                    "override_pool_mode": {}
                },
                "pgusers": {
                    "admin": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": true,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod"
                         }
                    },
                    "monitor": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": true,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod"
                        }
                    },
                    "postgres": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": true,
                        "superuser": true,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "lock_timeout": 0,
                            "log_statement": "mod",
                            "synchronous_commit": "local",
                            "temp_file_limit": -1
                        }
                    },
                    "repl": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": true,
                        "superuser": false,
                        "settings": {}
                    },
                    "mdb_admin": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "mdb_replication": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "mdb_monitor": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "test": {
                        "allow_db": "*",
                        "allow_port": 6432,
                        "bouncer": true,
                        "conn_limit": 50,
                        "connect_dbs": [
                            "testdb"
                        ],
                        "create": true,
                        "password": {
                            "data": "test_password",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false,
                        "settings": {},
                        "login": true,
                        "grants": []
                    }
                },
                "plan_cache_mode": "auto",
                "quote_all_identifiers": false,
                "random_page_cost": 1.0,
                "replacement_sort_tuples": 150000,
                "row_security": true,
                "search_path": "\"$user\", public",
                "seq_page_cost": 1.0,
                "server_reset_query_always": 1,
                "shared_preload_libraries": "pg_stat_statements,pg_stat_kcache,repl_mon",
                "sql_inheritance": true,
                "standard_conforming_strings": true,
                "statement_timeout": 0,
                "stats_temp_directory": "/dev/shm/pg_stat_tmp",
                "synchronize_seqscans": true,
                "synchronous_commit": "on",
                "temp_buffers": "8MB",
                "temp_file_limit": -1,
                "timezone": "Europe/Moscow",
                "transform_null_equals": false,
                "vacuum_cleanup_index_scale_factor": 0.1,
                "vacuum_cost_delay": 0,
                "vacuum_cost_limit": 200,
                "vacuum_cost_page_dirty": 20,
                "vacuum_cost_page_hit": 1,
                "vacuum_cost_page_miss": 10,
                "wal_level": "logical",
                "wal_log_hints": "on",
                "work_mem": "4MB",
                "xmlbinary": "base64",
                "xmloption": "content"
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
                                "postgresql_cluster"
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
                "cluster_type": "postgresql_cluster",
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
            "default pillar": true,
            "perf_diag": {"enable": false},
            "pgbouncer": {
                "custom_user_params": [],
                "custom_user_pool": true
            },
            "pgsync": {
                "autofailover": true,
                "priority": 10,
                "zk_hosts": "localhost",
                "zk_id": "test"
            },
            "postgresql default pillar": true,
            "runlist": [
                "components.postgresql_cluster"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
                        "unmanaged_dbs": [
                {
                    "testdb": {
                        "extensions": [],
                        "lc_collate": "C",
                        "lc_ctype": "C",
                        "user": "test"
                    }
                }
            ],
            "use_wale": false,
            "use_walg": true,
            "walg": {},
            "versions": {
                "postgres": {
                    "edition": "default",
                    "major_version": "10",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                },
                "odyssey": {
                    "major_version": "some hidden value",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value",
                    "edition": "some hidden value"
                }
            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """
    And for "worker_task_id1" exists "yandex.cloud.events.mdb.postgresql.CreateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
             "name": "test",
             "environment": "PRESTABLE",
             "config_spec": {
                 "version": "10",
                 "postgresql_config_10": {},
                 "resources": {
                     "resource_preset_id": "s1.porto.1",
                     "disk_type_id": "local-ssd",
                     "disk_size": 10737418240
                 }
             },
             "database_specs": [{
                 "name": "testdb",
                 "owner": "test"
             }],
             "folder_id": "folder1",
             "user_specs": [{
                 "name": "test"
             }],
             "host_specs": [{
                 "zone_id": "myt",
                 "priority": 9,
                 "config_spec": {
                    "postgresql_config_10": {
                        "work_mem": 65536
                    }
                 }
             }, {
                 "zone_id": "iva"
             }, {
                 "zone_id": "sas"
             }],
             "network_id": "IN-PORTO-NO-NETWORK-API"
        }
    }
    """
    And event body for event "worker_task_id1" does not contain following paths
      | $.request_parameters.user_spec.0.password  |
      | $.request_parameters.description           |


  @events
  Scenario: Cluster creation with SOX works
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test-sox",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "10",
           "postgresqlConfig_10": {},
           "soxAudit": true,
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "myt",
           "priority": 9,
           "configSpec": {
                "postgresqlConfig_10": {
                    "workMem": 65536
                }
           }
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test sox cluster",
       "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
            "clusterId": "cid2"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": true,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": null,
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "description": "test sox cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "UNKNOWN",
        "id": "cid2",
        "labels": {},
        "monitoring": [
            {
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm.yandex-team.ru/template/panel/dbaas_postgres_metrics/cid=cid2;dbname=testdb",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid2&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-postgres",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/postgresql_cluster/cid2?section=monitoring",
                "name": "Console"
            }
        ],
        "name": "test-sox",
        "networkId": "",
        "status": "CREATING"
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.CreateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid2"
        },
        "request_parameters": {
           "name": "test-sox",
           "environment": "PRESTABLE",
           "config_spec": {
               "version": "10",
               "postgresql_config_10": {},
               "resources": {
                   "resource_preset_id": "s1.porto.1",
                   "disk_type_id": "local-ssd",
                   "disk_size": 10737418240
               }
           },
           "database_specs": [{
               "name": "testdb",
               "owner": "test"
           }],
           "folder_id": "folder1",
           "user_specs": [{
               "name": "test"
           }],
           "host_specs": [{
               "zone_id": "myt",
               "priority": 9,
               "config_spec": {
                  "postgresql_config_10": {
                      "work_mem": 65536
                  }
               }
           }, {
               "zone_id": "iva"
           }, {
               "zone_id": "sas"
           }],
           "network_id": "IN-PORTO-NO-NETWORK-API"
        }
    }
    """
    And event body for event "worker_task_id2" does not contain following paths
      | $.request_parameters.user_spec.0.password  |
      | $.request_parameters.description           |
    When "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/postgresql/1.0/clusters/cid2/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "connLimit": 10,
            "permissions": [],
            "grants": ["reader", "writer"]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateUserMetadata",
            "clusterId": "cid2",
            "userName": "test2"
        }
    }
    """

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
               "status": "<status>",
               "services": [
                   {
                       "name": "pgbouncer",
                       "role": "Unknown",
                       "status": "<bouncer-iva>"
                   },
                   {
                       "name": "pg_replication",
                       "role": "Master",
                       "status": "<pg-iva>"
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "<status>",
               "services": [
                   {
                       "name": "pgbouncer",
                       "role": "Unknown",
                       "status": "<bouncer-myt>"
                   },
                   {
                       "name": "pg_replication",
                       "role": "Replica",
                       "replicatype": "Sync",
                       "status": "<pg-myt>"
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "<status>",
               "services": [
                   {
                       "name": "pgbouncer",
                       "role": "Unknown",
                       "status": "<bouncer-sas>"
                   },
                   {
                       "name": "pg_replication",
                       "role": "Replica",
                       "replicatype": "Async",
                       "status": "<pg-sas>"
                   }
               ]
           }
       ]
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "health": "<health>"
    }
    """
    Examples:
      | bouncer-iva | bouncer-myt | bouncer-sas | pg-iva | pg-myt | pg-sas | status   | health   |
      | Alive       | Dead        | Alive       | Alive  | Alive  | Alive  | Degraded | DEGRADED |
      | Alive       | Alive       | Alive       | Dead   | Alive  | Alive  | Degraded | DEGRADED |
      | Alive       | Alive       | Alive       | Dead   | Dead   | Dead   | Dead     | DEAD     |
      | Dead        | Dead        | Dead        | Alive  | Alive  | Alive  | Degraded | DEGRADED |
      | Dead        | Dead        | Dead        | Dead   | Dead   | Dead   | Dead     | DEAD     |

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
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "logMinDurationStatement": 100
            }
        }
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "logMinDurationStatement": 1000
            }
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "logMinDurationStatement": 10
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
    And we GET "/mdb/postgresql/1.0/operations?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "operations": [
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify PostgreSQL cluster",
                "done": false,
                "id": "worker_task_id4",
                "metadata": {
                    "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify PostgreSQL cluster",
                "done": false,
                "id": "worker_task_id3",
                "metadata": {
                    "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify PostgreSQL cluster",
                "done": false,
                "id": "worker_task_id2",
                "metadata": {
                    "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            }
        ]
    }
    """

  Scenario: Cluster list works
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test2",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "10",
           "postgresqlConfig_10": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
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
    And we GET "/mdb/postgresql/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusters": [
            {
                "config": {
                    "version": "10",
                    "autofailover": true,
                    "soxAudit": false,
                    "access": {
                        "webSql": false,
                        "dataLens": false,
                        "serverless": false,
                        "dataTransfer": false
                    },
                    "performanceDiagnostics": {
                        "enabled": false,
                        "statementsSamplingInterval": 600,
                        "sessionsSamplingInterval": 60
                    },
                    "backupWindowStart": {
                        "hours": 22,
                        "minutes": 15,
                        "seconds": 30,
                        "nanos": 100
                    },
                    "backupRetainPeriodDays": 7,
                    "postgresqlConfig_10": {
                        "defaultConfig": {
                            "archiveTimeout": 600000,
                            "autoExplainLogAnalyze": false,
                            "autoExplainLogBuffers": false,
                            "autoExplainLogMinDuration": -1,
                            "autoExplainLogNestedStatements": false,
                            "autoExplainLogTiming": false,
                            "autoExplainLogTriggers": false,
                            "autoExplainLogVerbose": false,
                            "autoExplainSampleRate": 1.0,
                            "pgHintPlanEnableHint": true,
                            "pgHintPlanEnableHintTable": false,
                            "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                            "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                            "sharedPreloadLibraries": [],
                            "effectiveIoConcurrency": 1,
                            "effectiveCacheSize": 107374182400,
                            "arrayNulls": true,
                            "autovacuumAnalyzeScaleFactor": 0.1,
                            "autovacuumMaxWorkers": 3,
                            "autovacuumNaptime": 15000,
                            "autovacuumVacuumCostDelay": 50,
                            "autovacuumVacuumCostLimit": 550,
                            "autovacuumVacuumScaleFactor": 0.2,
                            "autovacuumWorkMem": -1,
                            "backendFlushAfter": 0,
                            "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                            "bgwriterDelay": 200,
                            "bgwriterLruMaxpages": 100,
                            "bgwriterLruMultiplier": 2.0,
                            "byteaOutput": "BYTEA_OUTPUT_HEX",
                            "checkpointCompletionTarget": 0.5,
                            "checkpointTimeout": 300000,
                            "clientMinMessages": "LOG_LEVEL_NOTICE",
                            "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                            "cursorTupleFraction": 0.1,
                            "deadlockTimeout": 1000,
                            "defaultStatisticsTarget": 100,
                            "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                            "defaultTransactionReadOnly": false,
                            "defaultWithOids": false,
                            "enableBitmapscan": true,
                            "enableHashagg": true,
                            "enableHashjoin": true,
                            "enableIndexonlyscan": true,
                            "enableIndexscan": true,
                            "enableMaterial": true,
                            "enableMergejoin": true,
                            "enableNestloop": true,
                            "enableSeqscan": true,
                            "enableSort": true,
                            "enableTidscan": true,
                            "escapeStringWarning": true,
                            "exitOnError": false,
                            "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                            "fromCollapseLimit": 8,
                            "ginPendingListLimit": 4194304,
                            "idleInTransactionSessionTimeout": 0,
                            "joinCollapseLimit": 8,
                            "loCompatPrivileges": false,
                            "lockTimeout": 0,
                            "logCheckpoints": false,
                            "logConnections": false,
                            "logDisconnections": false,
                            "logDuration": false,
                            "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                            "logLockWaits": false,
                            "logMinDurationStatement": -1,
                            "logMinErrorStatement": "LOG_LEVEL_ERROR",
                            "logMinMessages": "LOG_LEVEL_WARNING",
                            "logStatement": "LOG_STATEMENT_NONE",
                            "logTempFiles": -1,
                            "maintenanceWorkMem": 67108864,
                            "maxConnections": 200,
                            "maxLocksPerTransaction": 64,
                            "maxLogicalReplicationWorkers": 4,
                            "maxParallelWorkers": 8,
                            "maxParallelWorkersPerGather": 2,
                            "maxPredLocksPerTransaction": 64,
                            "maxPreparedTransactions": 0,
                            "maxReplicationSlots": 20,
                            "maxStandbyStreamingDelay": 30000,
                            "maxWalSenders": 20,
                            "maxWalSize": 1073741824,
                            "maxWorkerProcesses": 8,
                            "minWalSize": 536870912,
                            "oldSnapshotThreshold": -1,
                            "operatorPrecedenceWarning": false,
                            "quoteAllIdentifiers": false,
                            "randomPageCost": 1.0,
                            "replacementSortTuples": 150000,
                            "rowSecurity": true,
                            "searchPath": "\"$user\", public",
                            "seqPageCost": 1.0,
                            "sharedBuffers": 1073741824,
                            "standardConformingStrings": true,
                            "statementTimeout": 0,
                            "synchronizeSeqscans": true,
                            "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                            "tempBuffers": 8388608,
                            "tempFileLimit": -1,
                            "timezone": "Europe/Moscow",
                            "trackActivityQuerySize": 1024,
                            "transformNullEquals": false,
                            "vacuumCostDelay": 0,
                            "vacuumCostLimit": 200,
                            "vacuumCostPageDirty": 20,
                            "vacuumCostPageHit": 1,
                            "vacuumCostPageMiss": 10,
                            "walLevel": "WAL_LEVEL_LOGICAL",
                            "workMem": 4194304,
                            "xmlbinary": "XML_BINARY_BASE64",
                            "xmloption": "XML_OPTION_CONTENT"
                        },
                        "effectiveConfig": {
                            "archiveTimeout": 600000,
                            "autoExplainLogAnalyze": false,
                            "autoExplainLogBuffers": false,
                            "autoExplainLogMinDuration": -1,
                            "autoExplainLogNestedStatements": false,
                            "autoExplainLogTiming": false,
                            "autoExplainLogTriggers": false,
                            "autoExplainLogVerbose": false,
                            "autoExplainSampleRate": 1.0,
                            "pgHintPlanEnableHint": true,
                            "pgHintPlanEnableHintTable": false,
                            "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                            "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                            "sharedPreloadLibraries": [],
                            "effectiveIoConcurrency": 1,
                            "effectiveCacheSize": 107374182400,
                            "arrayNulls": true,
                            "autovacuumAnalyzeScaleFactor": 0.1,
                            "autovacuumMaxWorkers": 3,
                            "autovacuumNaptime": 15000,
                            "autovacuumVacuumCostDelay": 50,
                            "autovacuumVacuumCostLimit": 550,
                            "autovacuumVacuumScaleFactor": 0.2,
                            "autovacuumWorkMem": -1,
                            "backendFlushAfter": 0,
                            "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                            "bgwriterDelay": 200,
                            "bgwriterLruMaxpages": 100,
                            "bgwriterLruMultiplier": 2.0,
                            "byteaOutput": "BYTEA_OUTPUT_HEX",
                            "checkpointCompletionTarget": 0.5,
                            "checkpointTimeout": 300000,
                            "clientMinMessages": "LOG_LEVEL_NOTICE",
                            "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                            "cursorTupleFraction": 0.1,
                            "deadlockTimeout": 1000,
                            "defaultStatisticsTarget": 100,
                            "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                            "defaultTransactionReadOnly": false,
                            "defaultWithOids": false,
                            "enableBitmapscan": true,
                            "enableHashagg": true,
                            "enableHashjoin": true,
                            "enableIndexonlyscan": true,
                            "enableIndexscan": true,
                            "enableMaterial": true,
                            "enableMergejoin": true,
                            "enableNestloop": true,
                            "enableSeqscan": true,
                            "enableSort": true,
                            "enableTidscan": true,
                            "escapeStringWarning": true,
                            "exitOnError": false,
                            "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                            "fromCollapseLimit": 8,
                            "ginPendingListLimit": 4194304,
                            "idleInTransactionSessionTimeout": 0,
                            "joinCollapseLimit": 8,
                            "loCompatPrivileges": false,
                            "lockTimeout": 0,
                            "logCheckpoints": false,
                            "logConnections": false,
                            "logDisconnections": false,
                            "logDuration": false,
                            "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                            "logLockWaits": false,
                            "logMinDurationStatement": -1,
                            "logMinErrorStatement": "LOG_LEVEL_ERROR",
                            "logMinMessages": "LOG_LEVEL_WARNING",
                            "logStatement": "LOG_STATEMENT_NONE",
                            "logTempFiles": -1,
                            "maintenanceWorkMem": 67108864,
                            "maxConnections": 200,
                            "maxLocksPerTransaction": 64,
                            "maxLogicalReplicationWorkers": 4,
                            "maxParallelWorkers": 8,
                            "maxParallelWorkersPerGather": 2,
                            "maxPredLocksPerTransaction": 64,
                            "maxPreparedTransactions": 0,
                            "maxReplicationSlots": 20,
                            "maxStandbyStreamingDelay": 30000,
                            "maxWalSenders": 20,
                            "maxWalSize": 1073741824,
                            "maxWorkerProcesses": 8,
                            "minWalSize": 536870912,
                            "oldSnapshotThreshold": -1,
                            "operatorPrecedenceWarning": false,
                            "quoteAllIdentifiers": false,
                            "randomPageCost": 1.0,
                            "replacementSortTuples": 150000,
                            "rowSecurity": true,
                            "searchPath": "\"$user\", public",
                            "seqPageCost": 1.0,
                            "sharedBuffers": 1073741824,
                            "standardConformingStrings": true,
                            "statementTimeout": 0,
                            "synchronizeSeqscans": true,
                            "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                            "tempBuffers": 8388608,
                            "tempFileLimit": -1,
                            "timezone": "Europe/Moscow",
                            "trackActivityQuerySize": 1024,
                            "transformNullEquals": false,
                            "vacuumCostDelay": 0,
                            "vacuumCostLimit": 200,
                            "vacuumCostPageDirty": 20,
                            "vacuumCostPageHit": 1,
                            "vacuumCostPageMiss": 10,
                            "walLevel": "WAL_LEVEL_LOGICAL",
                            "workMem": 4194304,
                            "xmlbinary": "XML_BINARY_BASE64",
                            "xmloption": "XML_OPTION_CONTENT"
                        },
                        "userConfig": {}
                    },
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
                        "link": "https://yasm.yandex-team.ru/template/panel/dbaas_postgres_metrics/cid=cid1;dbname=testdb",
                        "name": "YASM"
                    },
                    {
                        "description": "Solomon charts",
                        "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid1&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-postgres",
                        "name": "Solomon"
                    },
                    {
                        "description": "Console charts",
                        "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/postgresql_cluster/cid1?section=monitoring",
                        "name": "Console"
                    }
                ],
                "name": "test",
                "networkId": "",
                "status": "RUNNING"
            },
            {
                "config": {
                    "version": "10",
                    "autofailover": true,
                    "soxAudit": false,
                    "access": {
                        "webSql": false,
                        "dataLens": false,
                        "serverless": false,
                        "dataTransfer": false
                    },
                    "performanceDiagnostics": {
                        "enabled": false,
                        "statementsSamplingInterval": 600,
                        "sessionsSamplingInterval": 60
                    },
                    "backupWindowStart": {
                        "hours": 22,
                        "minutes": 15,
                        "seconds": 30,
                        "nanos": 100
                    },
                    "backupRetainPeriodDays": 7,
                    "postgresqlConfig_10": {
                        "defaultConfig": {
                            "archiveTimeout": 600000,
                            "autoExplainLogAnalyze": false,
                            "autoExplainLogBuffers": false,
                            "autoExplainLogMinDuration": -1,
                            "autoExplainLogNestedStatements": false,
                            "autoExplainLogTiming": false,
                            "autoExplainLogTriggers": false,
                            "autoExplainLogVerbose": false,
                            "autoExplainSampleRate": 1.0,
                            "pgHintPlanEnableHint": true,
                            "pgHintPlanEnableHintTable": false,
                            "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                            "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                            "sharedPreloadLibraries": [],
                            "effectiveIoConcurrency": 1,
                            "effectiveCacheSize": 107374182400,
                            "arrayNulls": true,
                            "autovacuumAnalyzeScaleFactor": 0.1,
                            "autovacuumMaxWorkers": 3,
                            "autovacuumNaptime": 15000,
                            "autovacuumVacuumCostDelay": 50,
                            "autovacuumVacuumCostLimit": 550,
                            "autovacuumVacuumScaleFactor": 0.2,
                            "autovacuumWorkMem": -1,
                            "backendFlushAfter": 0,
                            "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                            "bgwriterDelay": 200,
                            "bgwriterLruMaxpages": 100,
                            "bgwriterLruMultiplier": 2.0,
                            "byteaOutput": "BYTEA_OUTPUT_HEX",
                            "checkpointCompletionTarget": 0.5,
                            "checkpointTimeout": 300000,
                            "clientMinMessages": "LOG_LEVEL_NOTICE",
                            "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                            "cursorTupleFraction": 0.1,
                            "deadlockTimeout": 1000,
                            "defaultStatisticsTarget": 100,
                            "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                            "defaultTransactionReadOnly": false,
                            "defaultWithOids": false,
                            "enableBitmapscan": true,
                            "enableHashagg": true,
                            "enableHashjoin": true,
                            "enableIndexonlyscan": true,
                            "enableIndexscan": true,
                            "enableMaterial": true,
                            "enableMergejoin": true,
                            "enableNestloop": true,
                            "enableSeqscan": true,
                            "enableSort": true,
                            "enableTidscan": true,
                            "escapeStringWarning": true,
                            "exitOnError": false,
                            "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                            "fromCollapseLimit": 8,
                            "ginPendingListLimit": 4194304,
                            "idleInTransactionSessionTimeout": 0,
                            "joinCollapseLimit": 8,
                            "loCompatPrivileges": false,
                            "lockTimeout": 0,
                            "logCheckpoints": false,
                            "logConnections": false,
                            "logDisconnections": false,
                            "logDuration": false,
                            "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                            "logLockWaits": false,
                            "logMinDurationStatement": -1,
                            "logMinErrorStatement": "LOG_LEVEL_ERROR",
                            "logMinMessages": "LOG_LEVEL_WARNING",
                            "logStatement": "LOG_STATEMENT_NONE",
                            "logTempFiles": -1,
                            "maintenanceWorkMem": 67108864,
                            "maxConnections": 200,
                            "maxLocksPerTransaction": 64,
                            "maxLogicalReplicationWorkers": 4,
                            "maxParallelWorkers": 8,
                            "maxParallelWorkersPerGather": 2,
                            "maxPredLocksPerTransaction": 64,
                            "maxPreparedTransactions": 0,
                            "maxReplicationSlots": 20,
                            "maxStandbyStreamingDelay": 30000,
                            "maxWalSenders": 20,
                            "maxWalSize": 1073741824,
                            "maxWorkerProcesses": 8,
                            "minWalSize": 536870912,
                            "oldSnapshotThreshold": -1,
                            "operatorPrecedenceWarning": false,
                            "quoteAllIdentifiers": false,
                            "randomPageCost": 1.0,
                            "replacementSortTuples": 150000,
                            "rowSecurity": true,
                            "searchPath": "\"$user\", public",
                            "seqPageCost": 1.0,
                            "sharedBuffers": 1073741824,
                            "standardConformingStrings": true,
                            "statementTimeout": 0,
                            "synchronizeSeqscans": true,
                            "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                            "tempBuffers": 8388608,
                            "tempFileLimit": -1,
                            "timezone": "Europe/Moscow",
                            "trackActivityQuerySize": 1024,
                            "transformNullEquals": false,
                            "vacuumCostDelay": 0,
                            "vacuumCostLimit": 200,
                            "vacuumCostPageDirty": 20,
                            "vacuumCostPageHit": 1,
                            "vacuumCostPageMiss": 10,
                            "walLevel": "WAL_LEVEL_LOGICAL",
                            "workMem": 4194304,
                            "xmlbinary": "XML_BINARY_BASE64",
                            "xmloption": "XML_OPTION_CONTENT"
                        },
                        "effectiveConfig": {
                            "archiveTimeout": 600000,
                            "autoExplainLogAnalyze": false,
                            "autoExplainLogBuffers": false,
                            "autoExplainLogMinDuration": -1,
                            "autoExplainLogNestedStatements": false,
                            "autoExplainLogTiming": false,
                            "autoExplainLogTriggers": false,
                            "autoExplainLogVerbose": false,
                            "autoExplainSampleRate": 1.0,
                            "pgHintPlanEnableHint": true,
                            "pgHintPlanEnableHintTable": false,
                            "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                            "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                            "sharedPreloadLibraries": [],
                            "effectiveIoConcurrency": 1,
                            "effectiveCacheSize": 107374182400,
                            "arrayNulls": true,
                            "autovacuumAnalyzeScaleFactor": 0.1,
                            "autovacuumMaxWorkers": 3,
                            "autovacuumNaptime": 15000,
                            "autovacuumVacuumCostDelay": 50,
                            "autovacuumVacuumCostLimit": 550,
                            "autovacuumVacuumScaleFactor": 0.2,
                            "autovacuumWorkMem": -1,
                            "backendFlushAfter": 0,
                            "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                            "bgwriterDelay": 200,
                            "bgwriterLruMaxpages": 100,
                            "bgwriterLruMultiplier": 2.0,
                            "byteaOutput": "BYTEA_OUTPUT_HEX",
                            "checkpointCompletionTarget": 0.5,
                            "checkpointTimeout": 300000,
                            "clientMinMessages": "LOG_LEVEL_NOTICE",
                            "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                            "cursorTupleFraction": 0.1,
                            "deadlockTimeout": 1000,
                            "defaultStatisticsTarget": 100,
                            "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                            "defaultTransactionReadOnly": false,
                            "defaultWithOids": false,
                            "enableBitmapscan": true,
                            "enableHashagg": true,
                            "enableHashjoin": true,
                            "enableIndexonlyscan": true,
                            "enableIndexscan": true,
                            "enableMaterial": true,
                            "enableMergejoin": true,
                            "enableNestloop": true,
                            "enableSeqscan": true,
                            "enableSort": true,
                            "enableTidscan": true,
                            "escapeStringWarning": true,
                            "exitOnError": false,
                            "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                            "fromCollapseLimit": 8,
                            "ginPendingListLimit": 4194304,
                            "idleInTransactionSessionTimeout": 0,
                            "joinCollapseLimit": 8,
                            "loCompatPrivileges": false,
                            "lockTimeout": 0,
                            "logCheckpoints": false,
                            "logConnections": false,
                            "logDisconnections": false,
                            "logDuration": false,
                            "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                            "logLockWaits": false,
                            "logMinDurationStatement": -1,
                            "logMinErrorStatement": "LOG_LEVEL_ERROR",
                            "logMinMessages": "LOG_LEVEL_WARNING",
                            "logStatement": "LOG_STATEMENT_NONE",
                            "logTempFiles": -1,
                            "maintenanceWorkMem": 67108864,
                            "maxConnections": 200,
                            "maxLocksPerTransaction": 64,
                            "maxLogicalReplicationWorkers": 4,
                            "maxParallelWorkers": 8,
                            "maxParallelWorkersPerGather": 2,
                            "maxPredLocksPerTransaction": 64,
                            "maxPreparedTransactions": 0,
                            "maxReplicationSlots": 20,
                            "maxStandbyStreamingDelay": 30000,
                            "maxWalSenders": 20,
                            "maxWalSize": 1073741824,
                            "maxWorkerProcesses": 8,
                            "minWalSize": 536870912,
                            "oldSnapshotThreshold": -1,
                            "operatorPrecedenceWarning": false,
                            "quoteAllIdentifiers": false,
                            "randomPageCost": 1.0,
                            "replacementSortTuples": 150000,
                            "rowSecurity": true,
                            "searchPath": "\"$user\", public",
                            "seqPageCost": 1.0,
                            "sharedBuffers": 1073741824,
                            "standardConformingStrings": true,
                            "statementTimeout": 0,
                            "synchronizeSeqscans": true,
                            "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                            "tempBuffers": 8388608,
                            "tempFileLimit": -1,
                            "timezone": "Europe/Moscow",
                            "trackActivityQuerySize": 1024,
                            "transformNullEquals": false,
                            "vacuumCostDelay": 0,
                            "vacuumCostLimit": 200,
                            "vacuumCostPageDirty": 20,
                            "vacuumCostPageHit": 1,
                            "vacuumCostPageMiss": 10,
                            "walLevel": "WAL_LEVEL_LOGICAL",
                            "workMem": 4194304,
                            "xmlbinary": "XML_BINARY_BASE64",
                            "xmloption": "XML_OPTION_CONTENT"
                        },
                        "userConfig": {}
                    },
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
                        "link": "https://yasm.yandex-team.ru/template/panel/dbaas_postgres_metrics/cid=cid2;dbname=testdb",
                        "name": "YASM"
                    },
                    {
                        "description": "Solomon charts",
                        "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid2&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-postgres",
                        "name": "Solomon"
                    },
                    {
                        "description": "Console charts",
                        "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/postgresql_cluster/cid2?section=monitoring",
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
    When we GET "/mdb/postgresql/1.0/clusters" with params
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
                    "version": "10",
                    "autofailover": true,
                    "soxAudit": false,
                    "access": {
                        "webSql": false,
                        "dataLens": false,
                        "serverless": false,
                        "dataTransfer": false
                    },
                    "performanceDiagnostics": {
                        "enabled": false,
                        "statementsSamplingInterval": 600,
                        "sessionsSamplingInterval": 60
                    },
                    "backupWindowStart": {
                        "hours": 22,
                        "minutes": 15,
                        "seconds": 30,
                        "nanos": 100
                    },
                    "backupRetainPeriodDays": 7,
                    "postgresqlConfig_10": {
                        "defaultConfig": {
                            "archiveTimeout": 600000,
                            "autoExplainLogAnalyze": false,
                            "autoExplainLogBuffers": false,
                            "autoExplainLogMinDuration": -1,
                            "autoExplainLogNestedStatements": false,
                            "autoExplainLogTiming": false,
                            "autoExplainLogTriggers": false,
                            "autoExplainLogVerbose": false,
                            "autoExplainSampleRate": 1.0,
                            "pgHintPlanEnableHint": true,
                            "pgHintPlanEnableHintTable": false,
                            "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                            "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                            "sharedPreloadLibraries": [],
                            "effectiveIoConcurrency": 1,
                            "effectiveCacheSize": 107374182400,
                            "arrayNulls": true,
                            "autovacuumAnalyzeScaleFactor": 0.1,
                            "autovacuumMaxWorkers": 3,
                            "autovacuumNaptime": 15000,
                            "autovacuumVacuumCostDelay": 50,
                            "autovacuumVacuumCostLimit": 550,
                            "autovacuumVacuumScaleFactor": 0.2,
                            "autovacuumWorkMem": -1,
                            "backendFlushAfter": 0,
                            "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                            "bgwriterDelay": 200,
                            "bgwriterLruMaxpages": 100,
                            "bgwriterLruMultiplier": 2.0,
                            "byteaOutput": "BYTEA_OUTPUT_HEX",
                            "checkpointCompletionTarget": 0.5,
                            "checkpointTimeout": 300000,
                            "clientMinMessages": "LOG_LEVEL_NOTICE",
                            "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                            "cursorTupleFraction": 0.1,
                            "deadlockTimeout": 1000,
                            "defaultStatisticsTarget": 100,
                            "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                            "defaultTransactionReadOnly": false,
                            "defaultWithOids": false,
                            "enableBitmapscan": true,
                            "enableHashagg": true,
                            "enableHashjoin": true,
                            "enableIndexonlyscan": true,
                            "enableIndexscan": true,
                            "enableMaterial": true,
                            "enableMergejoin": true,
                            "enableNestloop": true,
                            "enableSeqscan": true,
                            "enableSort": true,
                            "enableTidscan": true,
                            "escapeStringWarning": true,
                            "exitOnError": false,
                            "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                            "fromCollapseLimit": 8,
                            "ginPendingListLimit": 4194304,
                            "idleInTransactionSessionTimeout": 0,
                            "joinCollapseLimit": 8,
                            "loCompatPrivileges": false,
                            "lockTimeout": 0,
                            "logCheckpoints": false,
                            "logConnections": false,
                            "logDisconnections": false,
                            "logDuration": false,
                            "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                            "logLockWaits": false,
                            "logMinDurationStatement": -1,
                            "logMinErrorStatement": "LOG_LEVEL_ERROR",
                            "logMinMessages": "LOG_LEVEL_WARNING",
                            "logStatement": "LOG_STATEMENT_NONE",
                            "logTempFiles": -1,
                            "maintenanceWorkMem": 67108864,
                            "maxConnections": 200,
                            "maxLocksPerTransaction": 64,
                            "maxLogicalReplicationWorkers": 4,
                            "maxParallelWorkers": 8,
                            "maxParallelWorkersPerGather": 2,
                            "maxPredLocksPerTransaction": 64,
                            "maxPreparedTransactions": 0,
                            "maxReplicationSlots": 20,
                            "maxStandbyStreamingDelay": 30000,
                            "maxWalSenders": 20,
                            "maxWalSize": 1073741824,
                            "maxWorkerProcesses": 8,
                            "minWalSize": 536870912,
                            "oldSnapshotThreshold": -1,
                            "operatorPrecedenceWarning": false,
                            "quoteAllIdentifiers": false,
                            "randomPageCost": 1.0,
                            "replacementSortTuples": 150000,
                            "rowSecurity": true,
                            "searchPath": "\"$user\", public",
                            "seqPageCost": 1.0,
                            "sharedBuffers": 1073741824,
                            "standardConformingStrings": true,
                            "statementTimeout": 0,
                            "synchronizeSeqscans": true,
                            "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                            "tempBuffers": 8388608,
                            "tempFileLimit": -1,
                            "timezone": "Europe/Moscow",
                            "trackActivityQuerySize": 1024,
                            "transformNullEquals": false,
                            "vacuumCostDelay": 0,
                            "vacuumCostLimit": 200,
                            "vacuumCostPageDirty": 20,
                            "vacuumCostPageHit": 1,
                            "vacuumCostPageMiss": 10,
                            "walLevel": "WAL_LEVEL_LOGICAL",
                            "workMem": 4194304,
                            "xmlbinary": "XML_BINARY_BASE64",
                            "xmloption": "XML_OPTION_CONTENT"
                        },
                        "effectiveConfig": {
                            "archiveTimeout": 600000,
                            "autoExplainLogAnalyze": false,
                            "autoExplainLogBuffers": false,
                            "autoExplainLogMinDuration": -1,
                            "autoExplainLogNestedStatements": false,
                            "autoExplainLogTiming": false,
                            "autoExplainLogTriggers": false,
                            "autoExplainLogVerbose": false,
                            "autoExplainSampleRate": 1.0,
                            "pgHintPlanEnableHint": true,
                            "pgHintPlanEnableHintTable": false,
                            "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                            "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                            "sharedPreloadLibraries": [],
                            "effectiveIoConcurrency": 1,
                            "effectiveCacheSize": 107374182400,
                            "arrayNulls": true,
                            "autovacuumAnalyzeScaleFactor": 0.1,
                            "autovacuumMaxWorkers": 3,
                            "autovacuumNaptime": 15000,
                            "autovacuumVacuumCostDelay": 50,
                            "autovacuumVacuumCostLimit": 550,
                            "autovacuumVacuumScaleFactor": 0.2,
                            "autovacuumWorkMem": -1,
                            "backendFlushAfter": 0,
                            "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                            "bgwriterDelay": 200,
                            "bgwriterLruMaxpages": 100,
                            "bgwriterLruMultiplier": 2.0,
                            "byteaOutput": "BYTEA_OUTPUT_HEX",
                            "checkpointCompletionTarget": 0.5,
                            "checkpointTimeout": 300000,
                            "clientMinMessages": "LOG_LEVEL_NOTICE",
                            "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                            "cursorTupleFraction": 0.1,
                            "deadlockTimeout": 1000,
                            "defaultStatisticsTarget": 100,
                            "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                            "defaultTransactionReadOnly": false,
                            "defaultWithOids": false,
                            "enableBitmapscan": true,
                            "enableHashagg": true,
                            "enableHashjoin": true,
                            "enableIndexonlyscan": true,
                            "enableIndexscan": true,
                            "enableMaterial": true,
                            "enableMergejoin": true,
                            "enableNestloop": true,
                            "enableSeqscan": true,
                            "enableSort": true,
                            "enableTidscan": true,
                            "escapeStringWarning": true,
                            "exitOnError": false,
                            "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                            "fromCollapseLimit": 8,
                            "ginPendingListLimit": 4194304,
                            "idleInTransactionSessionTimeout": 0,
                            "joinCollapseLimit": 8,
                            "loCompatPrivileges": false,
                            "lockTimeout": 0,
                            "logCheckpoints": false,
                            "logConnections": false,
                            "logDisconnections": false,
                            "logDuration": false,
                            "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                            "logLockWaits": false,
                            "logMinDurationStatement": -1,
                            "logMinErrorStatement": "LOG_LEVEL_ERROR",
                            "logMinMessages": "LOG_LEVEL_WARNING",
                            "logStatement": "LOG_STATEMENT_NONE",
                            "logTempFiles": -1,
                            "maintenanceWorkMem": 67108864,
                            "maxConnections": 200,
                            "maxLocksPerTransaction": 64,
                            "maxLogicalReplicationWorkers": 4,
                            "maxParallelWorkers": 8,
                            "maxParallelWorkersPerGather": 2,
                            "maxPredLocksPerTransaction": 64,
                            "maxPreparedTransactions": 0,
                            "maxReplicationSlots": 20,
                            "maxStandbyStreamingDelay": 30000,
                            "maxWalSenders": 20,
                            "maxWalSize": 1073741824,
                            "maxWorkerProcesses": 8,
                            "minWalSize": 536870912,
                            "oldSnapshotThreshold": -1,
                            "operatorPrecedenceWarning": false,
                            "quoteAllIdentifiers": false,
                            "randomPageCost": 1.0,
                            "replacementSortTuples": 150000,
                            "rowSecurity": true,
                            "searchPath": "\"$user\", public",
                            "seqPageCost": 1.0,
                            "sharedBuffers": 1073741824,
                            "standardConformingStrings": true,
                            "statementTimeout": 0,
                            "synchronizeSeqscans": true,
                            "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                            "tempBuffers": 8388608,
                            "tempFileLimit": -1,
                            "timezone": "Europe/Moscow",
                            "trackActivityQuerySize": 1024,
                            "transformNullEquals": false,
                            "vacuumCostDelay": 0,
                            "vacuumCostLimit": 200,
                            "vacuumCostPageDirty": 20,
                            "vacuumCostPageHit": 1,
                            "vacuumCostPageMiss": 10,
                            "walLevel": "WAL_LEVEL_LOGICAL",
                            "workMem": 4194304,
                            "xmlbinary": "XML_BINARY_BASE64",
                            "xmloption": "XML_OPTION_CONTENT"
                        },
                        "userConfig": {}
                    },
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
                        "link": "https://yasm.yandex-team.ru/template/panel/dbaas_postgres_metrics/cid=cid2;dbname=testdb",
                        "name": "YASM"
                    },
                    {
                        "description": "Solomon charts",
                        "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid2&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-postgres",
                        "name": "Solomon"
                    },
                    {
                        "description": "Console charts",
                        "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/postgresql_cluster/cid2?section=monitoring",
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
    When we GET "/mdb/postgresql/1.0/clusters" with params
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
    When we GET "/mdb/postgresql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "priority": 10,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "configSpec": {
                    "postgresqlConfig_10": {
                        "workMem": 65536
                    }
                },
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "priority": 9,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "SYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "priority": 5,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "ASYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas"
             }
         ]
    }
    """

  @events
  Scenario: Modifying host with setting replication source and priority works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net",
            "priority": 4
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify hosts in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.UpdateClusterHosts" event with
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
            "update_host_specs": [{
                "host_name": "sas-1.db.yandex.net",
                "replication_source": "iva-1.db.yandex.net",
                "priority": 4
            }]
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "priority": 10,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "configSpec": {
                    "postgresqlConfig_10": {
                        "workMem": 65536
                    }
                },
                "name": "myt-1.db.yandex.net",
                "priority": 9,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "SYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "priority": 4,
                "replicationSource": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "ASYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas"
            }
        ]
    }
    """

  Scenario: Modifying host with unsetting replication source works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": ""
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify hosts in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "priority": 10,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "configSpec": {
                    "postgresqlConfig_10": {
                        "workMem": 65536
                    }
                },
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "priority": 9,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "SYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "priority": 5,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "ASYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas"
            }
        ]
    }
    """

  Scenario: Adding user with maximum (135 = 200 - 15(reserved) - 50(user 'test')) plus one conn_limit fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test_conn_limit",
            "password": "password",
            "connLimit": 136
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "'conn_limit' is too high for user 'test_conn_limit'. Consider increasing 'max_connections'"
    }
    """

  @events
  Scenario: Adding user with maximum conn_limit
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test_conn_limit",
            "password": "password",
            "connLimit": 135
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test_conn_limit"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.CreateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test_conn_limit"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "user_spec": {
                "name": "test_conn_limit",
                "conn_limit": 135
            }
        }
    }
    """
    And event body for event "worker_task_id2" does not contain following paths
      | $.request_parameters.user_spec.password  |
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test_conn_limit"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "connLimit": 135,
        "name": "test_conn_limit"
    }
    """
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/users/test_conn_limit" with data
    """
    {
        "connLimit": 136
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "'conn_limit' is too high for user 'test_conn_limit'. Consider increasing 'max_connections'"
    }
    """

  Scenario Outline: Modifying host with invalid params fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
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
      | specs                                                                                                            | status | error code | message                                                                         |
      | []                                                                                                               | 400    | 3          | No changes detected                                                             |
      | [{"hostName": "sas-1.db.yandex.net"}, {"hostName": "myt-1.db.yandex.net"}]                                       | 501    | 12         | Updating multiple hosts at once is not supported yet                            |
      | [{"hostName": "man-1.db.yandex.net"}]                                                                            | 404    | 5          | Host 'man-1.db.yandex.net' does not exist                                       |
      | [{"hostName": "sas-1.db.yandex.net"}]                                                                            | 400    | 3          | No changes detected                                                             |
      | [{"hostName": "sas-1.db.yandex.net", "priority": -200}]                                                          | 422    | 3          | The request is invalid.\nupdateHostSpecs.0.priority: Must be between 0 and 100. |
      | [{"hostName": "sas-1.db.yandex.net", "priority": 1000000}]                                                       | 422    | 3          | The request is invalid.\nupdateHostSpecs.0.priority: Must be between 0 and 100. |
      | [{"hostName": "sas-1.db.yandex.net", "configSpec": {}}]                                                          | 422    | 3          | Error parsing configSpec: config_spec cannot be empty                           |
      | [{"hostName": "sas-1.db.yandex.net", "configSpec": {"postgresqlConfig_10": {"recoveryMinApplyDelay": 100}}}]     | 422    | 3          | host config apply only on non HA host                                           |
      | [{"hostName": "sas-1.db.yandex.net", "replicationSource": "man-1.db.yandex.net"}]                                | 404    | 5          | Host 'man-1.db.yandex.net' does not exist                                       |

  Scenario: Modifying host with setting config param works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net",
            "configSpec": {
                "postgresqlConfig_10": {
                    "recoveryMinApplyDelay": 3600,
                    "logMinDurationStatement": 100500,
                    "timezone": "GMT"
                }
            }
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify hosts in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "priority": 10,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "configSpec": {
                    "postgresqlConfig_10": {
                        "workMem": 65536
                    }
                },
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "priority": 9,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "SYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "priority": 5,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "ASYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "replicationSource": "iva-1.db.yandex.net",
                "configSpec": {
                    "postgresqlConfig_10": {
                        "recoveryMinApplyDelay": 3600,
                        "logMinDurationStatement": 100500,
                        "timezone": "GMT"
                    }
                },
                "subnetId": "",
                "zoneId": "sas"
            }
        ]
    }
    """
    When we GET "/api/v1.0/config/sas-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "backup": {
                "retain_period": 7,
                "use_backup_service": true,
                "max_incremental_steps": 6,
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                }
            },
            "cluster_nodes": {
                "ha": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net"
                ]
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "config": {
                "array_nulls": true,
                "archive_timeout": "10min",
                "auto_explain_log_min_duration": -1,
                "auto_explain_log_analyze": false,
                "auto_explain_log_buffers": false,
                "auto_explain_log_timing": false,
                "auto_explain_log_triggers": false,
                "auto_explain_log_verbose": false,
                "auto_explain_log_nested_statements": false,
                "auto_explain_sample_rate": 1.0,
                "pg_hint_plan_enable_hint": true,
                "pg_hint_plan_enable_hint_table": false,
                "pg_hint_plan_message_level":  "log",
                "pg_hint_plan_debug_print": "off",
                "user_shared_preload_libraries": [],
                "effective_io_concurrency": 1,
                "effective_cache_size": "100GB",
                "auto_kill_timeout": "12 hours",
                "autovacuum_analyze_scale_factor": 0.1,
                "autovacuum_naptime": "15s",
                "autovacuum_vacuum_scale_factor": 0.2,
                "autovacuum_work_mem": -1,
                "backend_flush_after": 0,
                "backslash_quote": "safe_encoding",
                "bgwriter_delay": "200ms",
                "bgwriter_flush_after": "512kB",
                "bgwriter_lru_maxpages": 100,
                "bgwriter_lru_multiplier": 2.0,
                "bytea_output": "hex",
                "checkpoint_completion_target": 0.5,
                "checkpoint_flush_after": "256kB",
                "checkpoint_timeout": "5min",
                "client_min_messages": "notice",
                "constraint_exclusion": "partition",
                "cursor_tuple_fraction": 0.1,
                "deadlock_timeout": "1s",
                "default_statistics_target": 100,
                "default_transaction_isolation": "read committed",
                "default_transaction_read_only": false,
                "default_with_oids": false,
                "enable_bitmapscan": true,
                "enable_hashagg": true,
                "enable_hashjoin": true,
                "enable_indexonlyscan": true,
                "enable_indexscan": true,
                "enable_material": true,
                "enable_mergejoin": true,
                "enable_nestloop": true,
                "enable_parallel_append": true,
                "enable_parallel_hash": true,
                "enable_partition_pruning": true,
                "enable_partitionwise_aggregate": false,
                "enable_partitionwise_join": false,
                "enable_seqscan": true,
                "enable_sort": true,
                "enable_tidscan": true,
                "escape_string_warning": true,
                "exit_on_error": false,
                "force_parallel_mode": "off",
                "from_collapse_limit": 8,
                "gin_pending_list_limit": "4MB",
                "idle_in_transaction_session_timeout": 0,
                "jit": false,
                "join_collapse_limit": 8,
                "lo_compat_privileges": false,
                "lock_timeout": 0,
                "log_checkpoints": false,
                "log_connections": false,
                "log_disconnections": false,
                "log_duration": false,
                "log_error_verbosity": "default",
                "log_lock_waits": false,
                "log_min_duration_statement": "100500ms",
                "log_min_error_statement": "error",
                "log_min_messages": "warning",
                "log_statement": "none",
                "log_temp_files": -1,
                "log_transaction_sample_rate": 0,
                "track_activity_query_size": 1024,
                "maintenance_work_mem": "64MB",
                "max_client_pool_conn": 8000,
                "max_locks_per_transaction": 64,
                "max_logical_replication_workers": 4,
                "max_parallel_maintenance_workers": 2,
                "max_parallel_workers": 8,
                "max_parallel_workers_per_gather": 2,
                "max_pred_locks_per_transaction": 64,
                "max_prepared_transactions": 0,
                "max_replication_slots": 20,
                "max_slot_wal_keep_size": -1,
                "max_standby_streaming_delay": "30s",
                "max_wal_senders": 20,
                "max_worker_processes": 8,
                "old_snapshot_threshold": -1,
                "operator_precedence_warning": false,
                "parallel_leader_participation": true,
                "pgbouncer": {
                    "override_pool_mode": {}
                },
                "pgusers": {
                    "admin": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": true,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod"
                         }
                    },
                    "monitor": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": true,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod"
                        }
                    },
                    "postgres": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": true,
                        "superuser": true,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "lock_timeout": 0,
                            "log_statement": "mod",
                            "synchronous_commit": "local",
                            "temp_file_limit": -1
                        }
                    },
                    "mdb_admin": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "mdb_replication": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "mdb_monitor": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "repl": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": true,
                        "superuser": false,
                        "settings": {}
                    },
                    "test": {
                        "allow_db": "*",
                        "allow_port": 6432,
                        "bouncer": true,
                        "conn_limit": 50,
                        "connect_dbs": [
                            "testdb"
                        ],
                        "create": true,
                        "password": {
                            "data": "test_password",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false,
                        "settings": {},
                        "login": true,
                        "grants": []
                    }
                },
                "plan_cache_mode": "auto",
                "quote_all_identifiers": false,
                "random_page_cost": 1.0,
                "recovery_min_apply_delay": "3600ms",
                "replacement_sort_tuples": 150000,
                "row_security": true,
                "search_path": "\"$user\", public",
                "seq_page_cost": 1.0,
                "server_reset_query_always": 1,
                "shared_preload_libraries": "pg_stat_statements,pg_stat_kcache,repl_mon",
                "sql_inheritance": true,
                "standard_conforming_strings": true,
                "statement_timeout": 0,
                "stats_temp_directory": "/dev/shm/pg_stat_tmp",
                "synchronize_seqscans": true,
                "synchronous_commit": "on",
                "temp_buffers": "8MB",
                "temp_file_limit": -1,
                "timezone": "GMT",
                "transform_null_equals": false,
                "vacuum_cleanup_index_scale_factor": 0.1,
                "vacuum_cost_delay": 0,
                "vacuum_cost_limit": 200,
                "vacuum_cost_page_dirty": 20,
                "vacuum_cost_page_hit": 1,
                "vacuum_cost_page_miss": 10,
                "wal_level": "logical",
                "wal_log_hints": "on",
                "work_mem": "4MB",
                "xmlbinary": "base64",
                "xmloption": "content"
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
                                "postgresql_cluster"
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
                "cluster_type": "postgresql_cluster",
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
            "default pillar": true,
            "perf_diag": {"enable": false},
            "pgbouncer": {
                "custom_user_params": [],
                "custom_user_pool": true
            },
            "pgsync": {
                "autofailover": true,
                "replication_source": "iva-1.db.yandex.net",
                "priority": 5,
                "zk_hosts": "localhost",
                "zk_id": "test"
            },
            "postgresql default pillar": true,
            "runlist": [
                "components.postgresql_cluster"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
                        "unmanaged_dbs": [
                {
                    "testdb": {
                        "extensions": [],
                        "lc_collate": "C",
                        "lc_ctype": "C",
                        "user": "test"
                    }
                }
            ],
            "use_replication_slots": false,
            "use_wale": false,
            "use_walg": true,
            "walg": {},
            "versions": {
                "postgres": {
                    "edition": "default",
                    "major_version": "10",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                },
                "odyssey": {
                    "major_version": "some hidden value",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value",
                    "edition": "some hidden value"
                }
            }
        }
    }
    """

  Scenario: Modifying host with setting config param to same value fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net",
            "configSpec": {
                "postgresqlConfig_10": {
                    "recoveryMinApplyDelay": 3600
                }
            }
        }]
    }
    """
    And we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "configSpec": {
                "postgresqlConfig_10": {
                    "recoveryMinApplyDelay": 3600
                }
            }
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

  Scenario: Partial replication cycles are not possible
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    And we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
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
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "iva-1.db.yandex.net",
            "replicationSource": "myt-1.db.yandex.net"
        }]
    }
    """
    And we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
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

  Scenario: Modifying host with setting replication source to same value fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    And we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
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

  Scenario: Modifying host with setting priority to same value fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "priority": 1
        }]
    }
    """
    And we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "priority": 1
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

  @events
  Scenario: Adding cascade replica works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
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
        "description": "Add hosts to PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "man-1.db.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.AddClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "man-1.db.yandex.net"
            ]
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "host_specs": [{
                "zone_id": "man",
                "replication_source": "sas-1.db.yandex.net"
            }]
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "priority": 10,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "man-1.db.yandex.net",
                "priority": 0,
                "replicationSource": "sas-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "replicaType": "UNKNOWN",
                "services": [],
                "subnetId": "",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "configSpec": {
                    "postgresqlConfig_10": {
                        "workMem": 65536
                    }
                },
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "priority": 9,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "SYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "priority": 5,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "ASYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas"
            }
        ]
    }
    """

  Scenario: Adding HA host works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
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
        "description": "Add hosts to PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "man-1.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "priority": 10,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "man-1.db.yandex.net",
                "priority": 0,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "replicaType": "UNKNOWN",
                "services": [],
                "subnetId": "",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "configSpec": {
                    "postgresqlConfig_10": {
                        "workMem": 65536
                    }
                },
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "priority": 9,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "SYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "priority": 5,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "ASYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas"
            }
        ]
    }
    """

  Scenario Outline: Adding host with invalid params fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
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

  @events
  Scenario: Deleting host works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete hosts from PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.DeleteClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.DeleteClusterHosts" event with
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
            "host_names": ["sas-1.db.yandex.net"]
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "priority": 10,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "configSpec": {
                    "postgresqlConfig_10": {
                        "workMem": 65536
                    }
                },
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "priority": 9,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "SYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
             }
         ]
     }
     """

  Scenario Outline: Deleting host with invalid params fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchDelete" with data
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

  Scenario: Deleting host with exclusive constraint violation fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    And we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["iva-1.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Conflicting operation worker_task_id2 detected"
    }
    """

  Scenario: Deleting last host fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["iva-1.db.yandex.net"]
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["myt-1.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Last PostgreSQL host cannot be removed"
    }
    """

  Scenario: Deleting host with replicas fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    And we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["iva-1.db.yandex.net"]
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Host 'iva-1.db.yandex.net' has active replicas 'sas-1.db.yandex.net'"
    }
    """

  Scenario: User list works
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "connLimit": 50,
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb"
                    }
                ],
                "settings": {},
                "login": true,
                "grants": []
            }
        ]
    }
    """

  Scenario: User get works
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "connLimit": 50,
        "name": "test",
        "permissions": [
            {
                "databaseName": "testdb"
            }
        ],
        "settings": {}
    }
    """

  Scenario: Nonexistent user get fails
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "User 'test2' does not exist"
    }
    """

  Scenario Outline: Adding system user "<system user>" to cluster fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "<system user>",
            "password": "password"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "<error message>"
    }
    """
    Examples:
      | system user | error message                                                                                    |
      | postgres    | The request is invalid.\nuserSpec.name: User name 'postgres' is not allowed                      |
      | monitor     | The request is invalid.\nuserSpec.name: User name 'monitor' is not allowed                       |
      | admin       | The request is invalid.\nuserSpec.name: User name 'admin' is not allowed                         |
      | repl        | The request is invalid.\nuserSpec.name: User name 'repl' is not allowed                          |

  Scenario: Database owner removal fails
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1/users/test"
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Unable to delete owner of database 'testdb'"
    }
    """

  Scenario Outline: Adding user <name> with invalid params fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "<name>",
            "password": "<password>",
            "connLimit": <conn limit>,
            "permissions": [{
                "databaseName": "<database>"
            }],
            "grants": [<grants>]
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
      | name     | password | conn limit | database | grants     | status | error code | error message                                                                                  |
      | b@dN@me! | password | 20         | testdb   |            | 422    | 3          | The request is invalid.\nuserSpec.name: User name 'b@dN@me!' does not conform to naming rules  |
      | pg_vasya | password | 20         | testdb   |            | 422    | 3          | The request is invalid.\nuserSpec.name: User name 'pg_vasya' does not conform to naming rules  |
      | vasya    | short    | 20         | testdb   |            | 422    | 3          | The request is invalid.\nuserSpec.password: Password must be between 8 and 128 characters long |
      | vasya    | password | -2         | testdb   |            | 422    | 3          | The request is invalid.\nuserSpec.connLimit: Must be at least 0.                               |
      | vasya    | password | 1000       | testdb   |            | 422    | 9          | 'conn_limit' is too high for user 'vasya'. Consider increasing 'max_connections'               |
      | vasya    | password | 20         | nodb     |            | 404    | 5          | Database 'nodb' does not exist                                                                 |
      | test     | password | 20         | testdb   |            | 409    | 6          | User 'test' already exists                                                                     |
      | petya    | password | 20         | testdb   | "asdf"     | 404    | 5          | User 'asdf' does not exist                                                                     |
      | petya    | password | 20         | testdb   | "reader"   | 404    | 5          | User 'reader' does not exist                                                                   |
      | petya    | password | 20         | testdb   | "postgres" | 422    | 3          | The request is invalid.\nuserSpec.grants.0: User name 'postgres' is not allowed                |
      | petya    | password | 20         | testdb   | "petya"    | 404    | 5          | User 'petya' does not exist                                                                   |

  Scenario Outline: Modifying user <name> with invalid params fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/users/<name>" with data
    """
    {
        "password": "<password>",
        "connLimit": <conn limit>,
        "permissions": [{
            "databaseName": "<database>"
        }],
        "grants": [<grants>]
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
      | name  | password | conn limit | database | grants      | status | error code | error message                                                                         |
      | test  | short    | 20         | testdb   |             | 422    | 3          | The request is invalid.\npassword: Password must be between 8 and 128 characters long |
      | test  | password | -2         | testdb   |             | 422    | 3          | The request is invalid.\nconnLimit: Must be at least 0.                               |
      | test  | password | 1000       | testdb   |             | 422    | 9          | 'conn_limit' is too high for user 'test'. Consider increasing 'max_connections'       |
      | test  | password | 20         | nodb     |             | 404    | 5          | Database 'nodb' does not exist                                                        |
      | test2 | password | 20         | testdb   |             | 404    | 5          | User 'test2' does not exist                                                           |
      | test  | password | 20         | testdb   | "asdf"      | 404    | 5          | User 'asdf' does not exist                                                            |
      | test  | password | 20         | testdb   | "reader"    | 404    | 5          | User 'reader' does not exist                                                          |
      | test  | password | 20         | testdb   | "postgres"  | 422    | 3          | The request is invalid.\ngrants.0: User name 'postgres' is not allowed                |
      | test  | password | 20         | testdb   | "test"      | 422    | 3          | User grants has a cycle                                                               |

  @events
  Scenario: Adding user with empty database acl works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "connLimit": 10,
            "permissions": []
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.CreateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "user_spec": {
                "name": "test2",
                "conn_limit": 10,
                "permissions": []
            }
        }
    }
    """
    And event body for event "worker_task_id2" does not contain following paths
      | $.request_parameters.user_spec.password  |
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "connLimit": 10,
        "name": "test2",
        "permissions": [],
        "settings": {}
    }
    """

  Scenario: Adding user with default database acl works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "connLimit": 10
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "connLimit": 10,
        "name": "test2",
        "permissions": [
            {"databaseName": "testdb"}
        ],
        "settings": {}
    }
    """

  Scenario: Adding user with settings
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test3",
            "password": "password",
            "connLimit": 10,
            "settings": {
              "lockTimeout": 0,
              "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
              "logMinDurationStatement": -1,
              "logStatement": "LOG_STATEMENT_MOD",
              "synchronousCommit": "SYNCHRONOUS_COMMIT_REMOTE_WRITE",
              "tempFileLimit": -1,
              "poolingMode": "SESSION",
              "preparedStatementsPooling": true,
              "catchupTimeout": 10
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test3"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test3"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "connLimit": 10,
        "name": "test3",
        "permissions": [
            {"databaseName": "testdb"}
        ],
        "settings": {
            "lockTimeout": 0,
            "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
            "logMinDurationStatement": -1,
            "logStatement": "LOG_STATEMENT_MOD",
            "synchronousCommit": "SYNCHRONOUS_COMMIT_REMOTE_WRITE",
            "tempFileLimit": -1,
            "poolingMode": "SESSION",
            "preparedStatementsPooling": true,
            "catchupTimeout": 10
        }
    }
    """

  Scenario: Adding user with grants and no login
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test4",
            "password": "password",
            "connLimit": 10,
            "login": false,
            "grants": ["test", "mdb_admin", "mdb_replication"]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test4"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test4"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test4",
        "login": false,
        "grants": ["test", "mdb_admin", "mdb_replication"]
    }
    """

  Scenario: Modify user with cyclical grants fails; delete granted role works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test_role",
            "password": "password",
            "login": false
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test6",
            "password": "password",
            "grants": ["test_role"]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test6"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test6"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test6",
        "grants": ["test_role"]
    }
    """
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/users/test_role" with data
    """
    {
        "grants": ["test6"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "User grants has a cycle"
    }
    """
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1/users/test_role"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete user from PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.DeleteUserMetadata",
            "clusterId": "cid1",
            "userName": "test_role"
        }
    }
    """
    When "worker_task_id4" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test6"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test6",
        "grants": []
    }
    """

  Scenario: Adding and modifying user with IDM grants
    When we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_insert(value, '{data,sox_audit}', 'true')
        WHERE cid = 'cid1'
    """
    And we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test5",
            "password": "password",
            "connLimit": 10,
            "grants": ["reader"]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test5"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test5"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test5",
        "grants": ["reader"]
    }
    """
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/users/test5" with data
    """
    {
        "grants": ["reader", "writer"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test5"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.postgresql.UpdateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test5"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "user_name": "test5",
            "grants": ["reader", "writer"]
        }
    }
    """
    And event body for event "worker_task_id3" does not contain following paths
      | $.request_parameters.user_spec.password  |
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test5"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "connLimit": 10,
        "name": "test5",
        "grants": ["reader", "writer"]
    }
    """


  @events
  Scenario: Modifying user with permission for database works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "connLimit": 10
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1/users/test2" with data
    """
    {
        "permissions": [{
            "databaseName": "testdb"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.postgresql.UpdateUser" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "user_name": "test2",
            "permissions": [{
                "database_name": "testdb"
            }]
        }
    }
    """
    And event body for event "worker_task_id3" does not contain following paths
      | $.request_parameters.user_spec.password  |
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "connLimit": 10,
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb"
        }]
    }
    """

  @events
  Scenario: Adding permission for database works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "connLimit": 10,
            "permissions": []
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/postgresql/1.0/clusters/cid1/users/test2:grantPermission" with data
    """
    {
        "permission": {
            "databaseName": "testdb"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Grant permission to user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.GrantUserPermissionMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.postgresql.GrantUserPermission" event with
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
                "database_name": "testdb"
            }
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "connLimit": 10,
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "settings": {}
    }
    """
  @setting
  Scenario: Adding settings for user works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/users/test" with data
    """
    {
        "settings": {
            "lockTimeout": 0,
            "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
            "logMinDurationStatement": -1,
            "logStatement": "LOG_STATEMENT_MOD",
            "synchronousCommit": "SYNCHRONOUS_COMMIT_REMOTE_WRITE",
            "tempFileLimit": -1,
            "poolingMode": "SESSION",
            "preparedStatementsPooling": true,
            "catchupTimeout": 10
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "connLimit": 50,
        "name": "test",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "settings": {
            "lockTimeout": 0,
            "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
            "logMinDurationStatement": -1,
            "logStatement": "LOG_STATEMENT_MOD",
            "synchronousCommit": "SYNCHRONOUS_COMMIT_REMOTE_WRITE",
            "tempFileLimit": -1,
            "poolingMode": "SESSION",
            "preparedStatementsPooling": true,
            "catchupTimeout": 10
        }
    }
    """

  Scenario Outline: Adding permission for database with invalid params fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users/<name>:grantPermission" with data
    """
    {
        "permission": {
            "databaseName": "<database>"
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
      | name   | database | status | error code | message                                                 |
      | nouser | testdb   | 404    | 5          | User 'nouser' does not exist                            |
      | test   | nodb     | 404    | 5          | Database 'nodb' does not exist                          |
      | test   | testdb   | 409    | 6          | User 'test' already has access to the database 'testdb' |

  @events
  Scenario: Revoking permission for database works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "connLimit": 10,
            "permissions": [{
                "databaseName": "testdb"
            }]
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/postgresql/1.0/clusters/cid1/users/test2:revokePermission" with data
    """
    {
        "databaseName": "testdb"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Revoke permission from user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.RevokeUserPermissionMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.postgresql.RevokeUserPermission" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "user_name": "test2"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "user_name": "test2",
            "database_name": "testdb"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "connLimit": 10,
        "name": "test2",
        "permissions": [],
        "settings": {}
    }
    """

  Scenario Outline: Revoking permission for database with invalid params fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "connLimit": 10,
            "permissions": []
        }
    }
    """
    And we POST "/mdb/postgresql/1.0/clusters/cid1/users/<name>:revokePermission" with data
    """
    {
        "databaseName": "<database>"
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
      | test   | testdb   | 422    | 3          | Unable to revoke permission from owner of database 'testdb' |

  @events
  Scenario: Deleting user works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "connLimit": 10,
            "permissions": []
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we DELETE "/mdb/postgresql/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete user from PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.DeleteUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.postgresql.DeleteUser" event with
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
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [
            {
                "clusterId": "cid1",
                "connLimit": 50,
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb"
                    }
                ],
                "settings": {},
                "login": true,
                "grants": []
            }
        ]
    }
    """

  Scenario: Deleting nonexistent user fails
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1/users/test2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "User 'test2' does not exist"
    }
    """

  Scenario Outline: Deleting system user <name> fails
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1/users/<name>"
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "<message>"
    }
    """
    Examples:
      | name        | message                                                  |
      | postgres    | User name 'postgres' is not allowed                      |
      | monitor     | User name 'monitor' is not allowed                       |
      | admin       | User name 'admin' is not allowed                         |
      | repl        | User name 'repl' is not allowed                          |

  Scenario: Changing user password works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/users/test" with data
    """
    {
        "password": "changed password"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test"
        }
    }
    """

  Scenario Outline: Changing system user <name> password fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/users/<name>" with data
    """
    {
        "password": "changed password"
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
      | name        | message                                                  |
      | postgres    | User name 'postgres' is not allowed                      |
      | monitor     | User name 'monitor' is not allowed                       |
      | admin       | User name 'admin' is not allowed                         |
      | repl        | User name 'repl' is not allowed                          |

  Scenario: Changing user connection limit works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/users/test" with data
    """
    {
        "connLimit": 10
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/users/test"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "connLimit": 10,
        "name": "test",
        "permissions": [
            {
                "databaseName": "testdb"
            }
        ],
        "settings": {}
    }
    """

  Scenario: Database get works
    When we GET "/mdb/postgresql/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "extensions": [],
        "lcCollate": "C",
        "lcCtype": "C",
        "name": "testdb",
        "owner": "test"
    }
    """

  Scenario: Nonexistent database get fails
    When we GET "/mdb/postgresql/1.0/clusters/cid1/databases/testdb2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Database 'testdb2' does not exist"
    }
    """

  Scenario: Database list works
    When we GET "/mdb/postgresql/1.0/clusters/cid1/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": [{
            "clusterId": "cid1",
            "extensions": [],
            "lcCollate": "C",
            "lcCtype": "C",
            "name": "testdb",
            "owner": "test"
        }]
    }
    """

  Scenario: Adding invalid extension to database fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/databases/testdb" with data
    """
    {
        "extensions": [{
            "name": "invalid"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Invalid value 'invalid', allowed values: pgvector, pg_trgm, uuid-ossp, bloom, dblink, postgres_fdw, oracle_fdw, orafce, tablefunc, pg_repack, autoinc, pgrowlocks, hstore, cube, citext, earthdistance, btree_gist, xml2, isn, fuzzystrmatch, ltree, btree_gin, intarray, moddatetime, lo, pgcrypto, dict_xsyn, seg, unaccent, dict_int, postgis, postgis_topology, address_standardizer, address_standardizer_data_us, postgis_tiger_geocoder, pgrouting, jsquery, smlar, pg_stat_statements, pg_stat_kcache, pg_partman, pg_tm_aux, amcheck, pg_hint_plan, pgstattuple, pg_buffercache, rum, plv8, hypopg, pg_qualstats, pg_cron"
    }
    """

  Scenario: Adding smlar and rum extensions to database fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/databases/testdb" with data
    """
    {
        "extensions": [
            {
                "name": "smlar"
            },
            {
                "name": "rum"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Cannot use 'smlar' and 'rum' at the same time, because they use a common '%' operator"
    }
    """

  Scenario: Adding extension without necessary dependencies to database fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/databases/testdb" with data
    """
    {
        "extensions": [
            {
                "name": "pg_stat_kcache"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The specified extension 'pg_stat_kcache' requires pg_stat_statements"
    }
    """

  Scenario: Adding extension requires shared_preload_libraries to database fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/databases/testdb" with data
    """
    {
        "extensions": [{
            "name": "pg_hint_plan"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The specified extension 'pg_hint_plan' is not present in shared_preload_libraries."
    }
    """

  Scenario: Adding extension to nonexistent database fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/databases/nodb" with data
    """
    {
        "extensions": [{
            "name": "pgcrypto"
        }]
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Database 'nodb' does not exist"
    }
    """

  @events
  Scenario: Adding extension to database works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/databases/testdb" with data
    """
    {
        "extensions": [{
            "name": "pgcrypto"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify database in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.UpdateDatabase" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "database_name": "testdb"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "database_name": "testdb",
            "extensions": [{
                "name": "pgcrypto"
            }]
        }
    }
    """

  Scenario: Modify database name works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/databases/testdb" with data
    """
    {
      "newDatabaseName": "new_testdb"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify database in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.UpdateDatabase" event with
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
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1/databases/new_testdb"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "extensions": [],
        "lcCollate": "C",
        "lcCtype": "C",
        "name": "new_testdb",
        "owner": "test"
    }
    """

  Scenario: Modify database name throws error
    When we POST "/mdb/postgresql/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb13",
            "owner": "test"
        }
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    Then we POST "/mdb/postgresql/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb14",
            "owner": "test"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/databases/testdb13" with data
    """
    {
      "newDatabaseName": "testdb14"
    }
    """
    Then we get response with status 409 and body contains
    """
    {
      "code": 6,
      "message": "Database 'testdb14' already exists"
    }
    """
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1/databases/testdb13" with data
    """
    {
      "newDatabaseName": "template0"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
      "code": 3,
      "message": "The request is invalid.\nnewDatabaseName: Database name 'template0' is not allowed"
    }
    """


  Scenario: Adding extension and changing params requires shared_preload_libraries to database works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "sharedPreloadLibraries": ["SHARED_PRELOAD_LIBRARIES_AUTO_EXPLAIN", "SHARED_PRELOAD_LIBRARIES_PG_CRON", "SHARED_PRELOAD_LIBRARIES_PG_HINT_PLAN", "SHARED_PRELOAD_LIBRARIES_PG_QUALSTATS"],
                "pgHintPlanEnableHintTable": true
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1/databases/testdb" with data
    """
    {
        "extensions": [{
            "name": "pg_hint_plan"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify database in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.postgresql.UpdateDatabase" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "database_name": "testdb"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "database_name": "testdb",
            "extensions": [{
                "name": "pg_hint_plan"
            }]
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "pgHintPlanEnableHintTable": true
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """

  Scenario: Dropping shared preload libraries works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "sharedPreloadLibraries": ["SHARED_PRELOAD_LIBRARIES_PG_QUALSTATS"]
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "sharedPreloadLibraries": []
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "backupRetainPeriodDays": 7,
            "monitoringCloudId": "cloud1",
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "sharedPreloadLibraries": []
                }
            },
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
                "link": "https://yasm.yandex-team.ru/template/panel/dbaas_postgres_metrics/cid=cid1;dbname=testdb",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid1&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-postgres",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/postgresql_cluster/cid1?section=monitoring",
                "name": "Console"
            }
        ],
        "name": "test",
        "networkId": "",
        "status": "RUNNING"
    }
    """

  Scenario: Decrease max_locks_per_transaction leads to restart in reverse order
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxLocksPerTransaction": 32
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "reverse_order" set to "True"

  Scenario Outline: Adding database <name> with invalid params fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/databases" with data
    """
    {
       "databaseSpec": {
           "name": "<name>",
           "owner": "<owner>"
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
      | name    | owner  | status | error code | message                                                                                              |
      | testdb  | test   | 409    | 6          | Database 'testdb' already exists                                                                     |
      | testdb2 | nouser | 404    | 5          | User 'nouser' does not exist                                                                         |
      | b@dN@me | test   | 422    | 3          | The request is invalid.\ndatabaseSpec.name: Database name 'b@dN@me' does not conform to naming rules |

  Scenario Outline: Adding database with template <template>, lcCollate <lcCollate> and lcCtype <lcCtype> fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/databases" with data
    """
    {
       "databaseSpec": {
           "name": "testdb1",
           "owner": "test",
           "templateDb": "<template>",
           "lcCtype": "<lcCtype>",
           "lcCollate": "<lcCollate>"
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
      | template | lcCollate   | lcCtype     | status | error code | message                                                                                              |
      | foobar   | C           | C           |404     | 5          | Database 'foobar' does not exist                                                                     |
      | testdb   | C           | fi_FI.UTF-8 |422     | 3          | LC_CTYPE of database 'testdb1' and template database 'testdb' should be equal         |
      | testdb   | fi_FI.UTF-8 | C           |422     | 3          | LC_COLLATE of database 'testdb1' and template database 'testdb' should be equal         |

  @events
  Scenario: Adding database works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb2",
            "owner": "test",
            "extensions": [{
                "name": "pgcrypto"
            }],
            "lcCollate": "fi_FI.UTF-8",
            "lcCtype": "fi_FI.UTF-8"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add database to PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.CreateDatabase" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "database_name": "testdb2"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "database_spec": {
                "name": "testdb2",
                "owner": "test",
                "extensions": [{
                    "name": "pgcrypto"
                }],
                "lc_collate": "fi_FI.UTF-8",
                "lc_ctype": "fi_FI.UTF-8"
            }
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/databases/testdb2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "extensions": [
            {"name": "pgcrypto"}
        ],
        "lcCollate": "fi_FI.UTF-8",
        "lcCtype": "fi_FI.UTF-8",
        "name": "testdb2",
        "owner": "test"
    }
    """

  Scenario: Removing template database does not lead to validate exceptions
    When we POST "/mdb/postgresql/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb2",
            "owner": "test",
            "extensions": [{
                "name": "pgcrypto"
            }],
            "lcCollate": "C",
            "lcCtype": "C",
            "templateDb": "testdb"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add database to PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb2"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete database from PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.DeleteDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1/databases/testdb2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "extensions": [
            {"name": "pgcrypto"}
        ],
        "lcCollate": "C",
        "lcCtype": "C",
        "name": "testdb2",
        "templateDb": "testdb",
        "owner": "test"
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb3",
            "owner": "test",
            "extensions": [{
                "name": "pgcrypto"
            }],
            "lcCollate": "C",
            "lcCtype": "C"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add database to PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb3"
        }
    }
    """
    And "worker_task_id4" acquired and finished by worker

  Scenario: Adding database with invalid extension fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb228",
            "owner": "test",
            "extensions": [{
                "name": "definetely_non_existing_extension"
            }]
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Invalid value 'definetely_non_existing_extension', allowed values: pgvector, pg_trgm, uuid-ossp, bloom, dblink, postgres_fdw, oracle_fdw, orafce, tablefunc, pg_repack, autoinc, pgrowlocks, hstore, cube, citext, earthdistance, btree_gist, xml2, isn, fuzzystrmatch, ltree, btree_gin, intarray, moddatetime, lo, pgcrypto, dict_xsyn, seg, unaccent, dict_int, postgis, postgis_topology, address_standardizer, address_standardizer_data_us, postgis_tiger_geocoder, pgrouting, jsquery, smlar, pg_stat_statements, pg_stat_kcache, pg_partman, pg_tm_aux, amcheck, pg_hint_plan, pgstattuple, pg_buffercache, rum, plv8, hypopg, pg_qualstats, pg_cron"
    }
    """

  Scenario: Deleting nonexistent database fails
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1/databases/nodb"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Database 'nodb' does not exist"
    }
    """

  @events
  Scenario: Deleting database works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb2",
            "owner": "test"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we DELETE "/mdb/postgresql/1.0/clusters/cid1/databases/testdb2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete database from PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.DeleteDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb2"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.postgresql.DeleteDatabase" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "database_name": "testdb2"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "database_name": "testdb2"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": [{
            "clusterId": "cid1",
            "extensions": [],
            "lcCollate": "C",
            "lcCtype": "C",
            "name": "testdb",
            "owner": "test"
        }]
    }
    """

  @events
  Scenario: Label set works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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
        "description": "Update PostgreSQL cluster metadata",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.UpdateCluster" event with
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
    And event body for event "worker_task_id2" does not contain following paths
      | $.request_parameters.labels  |
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "labels": {
            "acid": "yes"
        }
    }
    """

  @events
  Scenario: Description set works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "description": "my cool description"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.UpdateCluster" event with
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
    And event body for event "worker_task_id2" does not contain following paths
      | $.request_parameters.description           |
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "my cool description"
    }
    """

  Scenario: Change disk size to invalid value fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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

  @events
  Scenario: Change disk size to valid value works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "config_spec": {
                "resources": {
                    "disk_size": 21474836480
                }
            }
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

  Scenario: Autofailover option change works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "autofailover": false
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": false,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  @events
  @backup_api_opt
  Scenario: Backup window start & retention options change works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "backupWindowStart": {
                "hours": 23,
                "minutes": 10
            },
            "backupRetainPeriodDays": 10
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "config_spec": {
                "backup_window_start": {
                    "hours": 23,
                    "minutes": 10
                },
                "backup_retain_period_days": 10
            }
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 23,
                "minutes": 10,
                "seconds": 0,
                "nanos": 0
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 10,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
  Scenario: Change only retention options works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "backupRetainPeriodDays": 11
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "config_spec": {
                "backup_retain_period_days": 11
            }
        }
    }
   """
  @retention_policy
  Scenario: Change only retention options keep between operations
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "backupRetainPeriodDays": 11
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker

    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 23,
                "minutes": 10,
                "seconds": 0,
                "nanos": 0
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 11,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Changing max_connections beyond resourcePresetId limit fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxConnections": 10000
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "max_connections (10000) is too high for your instance type (max is 200)"
    }
    """

  Scenario: Changing plugins param without sharedPreloadLibraries fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "pgHintPlanEnableHintTable": true
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "pg_hint_plan_enable_hint_table cannot be changed without using pg_hint_plan in shared_preload_libraries"
    }
    """

  Scenario: Changing sharedPreloadLibraries fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "sharedPreloadLibraries": ["FOO"]
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.postgresqlConfig_10.sharedPreloadLibraries.0: Invalid value 'FOO', allowed values: SHARED_PRELOAD_LIBRARIES_AUTO_EXPLAIN, SHARED_PRELOAD_LIBRARIES_PG_CRON, SHARED_PRELOAD_LIBRARIES_PG_HINT_PLAN, SHARED_PRELOAD_LIBRARIES_PG_QUALSTATS, SHARED_PRELOAD_LIBRARIES_TIMESCALEDB"
    }
    """

  Scenario: Changing max_connections to less than sum of user limit value fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxConnections": 10
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "max_connections (10) is less than sum of users connection limit (65)"
    }
    """

  Scenario: Changing checkpoint_timeout fails (deprecated)
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "walLevel": "WAL_LEVEL_REPLICA"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "wal_level is deprecated and no longer supported"
    }
    """

  Scenario: Changing checkpoint_timeout to less than limit value fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "checkpointTimeout": 10
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.postgresqlConfig_10.checkpointTimeout: Must be between 30000 and 86400000."
    }
    """

  Scenario: Reducing max_connections and scaling up fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxConnections": 100
            },
            "resources": {
                "resourcePresetId": "s1.porto.2"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Upscale cannot be mixed with reducing max_connections"
    }
    """

  Scenario: Increasing max_connections and scaling down fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxConnections": 100
            }
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.porto.2"
            }
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxConnections": 200
            },
            "resources": {
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Downscale cannot be mixed with increasing max_connections"
    }
    """

  Scenario: Changing max_connections works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxConnections": 100
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 100,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "maxConnections": 100
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Changing increase max_replication_slots works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxReplicationSlots": 30
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 30,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "maxReplicationSlots": 30
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Changing decrease max_replication_slots fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxReplicationSlots": 30
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 30,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "maxReplicationSlots": 30
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxReplicationSlots": 29
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "max_replication_slots cannot be decreased"
    }
    """

  Scenario: Changing max_replication_slots less than schema min value fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxReplicationSlots": 19
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.postgresqlConfig_10.maxReplicationSlots: Must be between 20 and 100."
    }
    """

  Scenario: Changing max_replication_slots greater than schema max value fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxReplicationSlots": 101
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.postgresqlConfig_10.maxReplicationSlots: Must be between 20 and 100."
    }
    """

  Scenario: Changing max_replication_slots ignored
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxReplicationSlots": 20
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

  Scenario: Changing increase max_wal_senders works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxWalSenders": 30
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 30,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "maxWalSenders": 30
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Changing decrease max_wal_senders works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxWalSenders": 30
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 30,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "maxWalSenders": 30
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxWalSenders": 29
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 29,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "maxWalSenders": 29
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Changing max_wal_senders ignored
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxWalSenders": 20
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

  Scenario: Changing max_wal_senders less than schema min value fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxWalSenders": 19
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.postgresqlConfig_10.maxWalSenders: Must be between 20 and 100."
    }
    """

  Scenario: Changing max_wal_senders greater than schema max value fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxWalSenders": 101
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.postgresqlConfig_10.maxWalSenders: Must be between 20 and 100."
    }
    """

  Scenario: Changing increase max_logical_replication_workers works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxLogicalReplicationWorkers": 7
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 7,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "maxLogicalReplicationWorkers": 7
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Changing decrease max_logical_replication_workers works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxLogicalReplicationWorkers": 7
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 7,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "maxLogicalReplicationWorkers": 7
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxLogicalReplicationWorkers": 6
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 6,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "maxLogicalReplicationWorkers": 6
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Changing max_logical_replication_workers ignored
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxLogicalReplicationWorkers": 4
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

  Scenario: Changing max_logical_replication_workers less than schema min value fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxLogicalReplicationWorkers": 3
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.postgresqlConfig_10.maxLogicalReplicationWorkers: Must be between 4 and 100."
    }
    """

  Scenario: Changing max_logical_replication_workers greater than schema max value fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxLogicalReplicationWorkers": 101
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.postgresqlConfig_10.maxLogicalReplicationWorkers: Must be between 4 and 100."
    }
    """

  Scenario: Changing max_logical_replication_workers greater or equal max_worker_processes fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxLogicalReplicationWorkers": 99
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "max_logical_replication_workers cannot be greater or equal max_worker_processes"
    }
    """

  Scenario: Changing log_min_duration_statement works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "logMinDurationStatement": 100
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": 100,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "logMinDurationStatement": 100
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Changing vacuum, bytea_output, shared_preload_libraries(then add pg_hint_plan) and server_reset_query_always settings
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "autovacuumAnalyzeScaleFactor": 0.001,
                "autovacuumMaxWorkers": 4,
                "autovacuumVacuumCostDelay": 40,
                "autovacuumVacuumCostLimit": 444,
                "autovacuumVacuumScaleFactor": 0.002,
                "autovacuumNaptime": 60000,
                "byteaOutput": "BYTEA_OUTPUT_ESCAPED",
                "sharedPreloadLibraries": ["SHARED_PRELOAD_LIBRARIES_AUTO_EXPLAIN", "SHARED_PRELOAD_LIBRARIES_PG_CRON", "SHARED_PRELOAD_LIBRARIES_PG_HINT_PLAN", "SHARED_PRELOAD_LIBRARIES_PG_QUALSTATS"]
            },
            "poolerConfig": {
                "poolDiscard": false
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "poolerConfig": {
                "poolDiscard": false
            },
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": ["SHARED_PRELOAD_LIBRARIES_AUTO_EXPLAIN", "SHARED_PRELOAD_LIBRARIES_PG_CRON", "SHARED_PRELOAD_LIBRARIES_PG_HINT_PLAN", "SHARED_PRELOAD_LIBRARIES_PG_QUALSTATS"],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.001,
                    "autovacuumMaxWorkers": 4,
                    "autovacuumNaptime": 60000,
                    "autovacuumVacuumCostDelay": 40,
                    "autovacuumVacuumCostLimit": 444,
                    "autovacuumVacuumScaleFactor": 0.002,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_ESCAPED",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "autovacuumAnalyzeScaleFactor": 0.001,
                    "autovacuumMaxWorkers": 4,
                    "autovacuumVacuumCostDelay": 40,
                    "autovacuumVacuumCostLimit": 444,
                    "autovacuumVacuumScaleFactor": 0.002,
                    "autovacuumNaptime": 60000,
                    "byteaOutput": "BYTEA_OUTPUT_ESCAPED",
                    "sharedPreloadLibraries": ["SHARED_PRELOAD_LIBRARIES_AUTO_EXPLAIN", "SHARED_PRELOAD_LIBRARIES_PG_CRON", "SHARED_PRELOAD_LIBRARIES_PG_HINT_PLAN", "SHARED_PRELOAD_LIBRARIES_PG_QUALSTATS"]
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "backup": {
                "retain_period": 7,
                "use_backup_service": true,
                "max_incremental_steps": 6,
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                }
            },
            "cluster_nodes": {
                "ha": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ]
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "config": {
                "array_nulls": true,
                "archive_timeout": "10min",
                "auto_explain_log_min_duration": -1,
                "auto_explain_log_analyze": false,
                "auto_explain_log_buffers": false,
                "auto_explain_log_timing": false,
                "auto_explain_log_triggers": false,
                "auto_explain_log_verbose": false,
                "auto_explain_log_nested_statements": false,
                "auto_explain_sample_rate": 1.0,
                "pg_hint_plan_enable_hint": true,
                "pg_hint_plan_enable_hint_table": false,
                "pg_hint_plan_message_level":  "log",
                "pg_hint_plan_debug_print": "off",
                "user_shared_preload_libraries": ["auto_explain", "pg_cron", "pg_hint_plan", "pg_qualstats"],
                "effective_io_concurrency": 1,
                "effective_cache_size": "100GB",
                "auto_kill_timeout": "12 hours",
                "autovacuum_analyze_scale_factor": 0.001,
                "autovacuum_naptime": "60000ms",
                "autovacuum_vacuum_cost_limit": 444,
                "autovacuum_max_workers": 4,
                "autovacuum_vacuum_cost_delay": 40,
                "autovacuum_vacuum_scale_factor": 0.002,
                "autovacuum_work_mem": -1,
                "backend_flush_after": 0,
                "backslash_quote": "safe_encoding",
                "bgwriter_delay": "200ms",
                "bgwriter_flush_after": "512kB",
                "bgwriter_lru_maxpages": 100,
                "bgwriter_lru_multiplier": 2.0,
                "bytea_output": "escape",
                "checkpoint_completion_target": 0.5,
                "checkpoint_flush_after": "256kB",
                "checkpoint_timeout": "5min",
                "client_min_messages": "notice",
                "constraint_exclusion": "partition",
                "cursor_tuple_fraction": 0.1,
                "deadlock_timeout": "1s",
                "default_statistics_target": 100,
                "default_transaction_isolation": "read committed",
                "default_transaction_read_only": false,
                "default_with_oids": false,
                "enable_bitmapscan": true,
                "enable_hashagg": true,
                "enable_hashjoin": true,
                "enable_indexonlyscan": true,
                "enable_indexscan": true,
                "enable_material": true,
                "enable_mergejoin": true,
                "enable_nestloop": true,
                "enable_parallel_append": true,
                "enable_parallel_hash": true,
                "enable_partition_pruning": true,
                "enable_partitionwise_aggregate": false,
                "enable_partitionwise_join": false,
                "enable_seqscan": true,
                "enable_sort": true,
                "enable_tidscan": true,
                "escape_string_warning": true,
                "exit_on_error": false,
                "force_parallel_mode": "off",
                "from_collapse_limit": 8,
                "gin_pending_list_limit": "4MB",
                "idle_in_transaction_session_timeout": 0,
                "jit": false,
                "join_collapse_limit": 8,
                "lo_compat_privileges": false,
                "lock_timeout": 0,
                "log_checkpoints": false,
                "log_connections": false,
                "log_disconnections": false,
                "log_duration": false,
                "log_error_verbosity": "default",
                "log_lock_waits": false,
                "log_min_duration_statement": -1,
                "log_min_error_statement": "error",
                "log_min_messages": "warning",
                "log_statement": "none",
                "log_temp_files": -1,
                "log_transaction_sample_rate": 0,
                "track_activity_query_size": 1024,
                "maintenance_work_mem": "64MB",
                "max_client_pool_conn": 8000,
                "max_locks_per_transaction": 64,
                "max_logical_replication_workers": 4,
                "max_parallel_maintenance_workers": 2,
                "max_parallel_workers": 8,
                "max_parallel_workers_per_gather": 2,
                "max_pred_locks_per_transaction": 64,
                "max_prepared_transactions": 0,
                "max_replication_slots": 20,
                "max_slot_wal_keep_size": -1,
                "max_standby_streaming_delay": "30s",
                "max_wal_senders": 20,
                "max_worker_processes": 8,
                "old_snapshot_threshold": -1,
                "operator_precedence_warning": false,
                "parallel_leader_participation": true,
                "pgbouncer": {
                    "override_pool_mode": {}
                },
                "pgusers": {
                    "admin": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": true,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod"
                         }
                    },
                    "monitor": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": true,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod"
                        }
                    },
                    "postgres": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": true,
                        "superuser": true,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "lock_timeout": 0,
                            "log_statement": "mod",
                            "synchronous_commit": "local",
                            "temp_file_limit": -1
                        }
                    },
                    "repl": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": true,
                        "superuser": false,
                        "settings": {}
                    },
                    "mdb_admin": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "mdb_replication": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "mdb_monitor": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "test": {
                        "allow_db": "*",
                        "allow_port": 6432,
                        "bouncer": true,
                        "conn_limit": 50,
                        "connect_dbs": [
                            "testdb"
                        ],
                        "create": true,
                        "grants": [],
                        "login": true,
                        "password": {
                            "data": "test_password",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false,
                        "settings": {}
                    }
                },
                "plan_cache_mode": "auto",
                "quote_all_identifiers": false,
                "random_page_cost": 1.0,
                "replacement_sort_tuples": 150000,
                "row_security": true,
                "search_path": "\"$user\", public",
                "seq_page_cost": 1.0,
                "server_reset_query_always": 0,
                "shared_preload_libraries": "pg_stat_statements,pg_stat_kcache,repl_mon",
                "sql_inheritance": true,
                "standard_conforming_strings": true,
                "statement_timeout": 0,
                "stats_temp_directory": "/dev/shm/pg_stat_tmp",
                "synchronize_seqscans": true,
                "synchronous_commit": "on",
                "temp_buffers": "8MB",
                "temp_file_limit": -1,
                "timezone": "Europe/Moscow",
                "transform_null_equals": false,
                "vacuum_cleanup_index_scale_factor": 0.1,
                "vacuum_cost_delay": 0,
                "vacuum_cost_limit": 200,
                "vacuum_cost_page_dirty": 20,
                "vacuum_cost_page_hit": 1,
                "vacuum_cost_page_miss": 10,
                "wal_level": "logical",
                "wal_log_hints": "on",
                "work_mem": "4MB",
                "xmlbinary": "base64",
                "xmloption": "content"
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
                                "postgresql_cluster"
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
                "cluster_type": "postgresql_cluster",
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
            "default pillar": true,
            "perf_diag": {"enable": false},
            "pgbouncer": {
                "custom_user_params": [],
                "custom_user_pool": true
            },
            "pgsync": {
                "autofailover": true,
                "priority": 10,
                "zk_hosts": "localhost",
                "zk_id": "test"
            },
            "postgresql default pillar": true,
            "runlist": [
                "components.postgresql_cluster"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
                        "unmanaged_dbs": [
                {
                    "testdb": {
                        "extensions": [],
                        "lc_collate": "C",
                        "lc_ctype": "C",
                        "user": "test"
                    }
                }
            ],
            "use_wale": false,
            "use_walg": true,
            "walg": {},
            "versions": {
                "postgres": {
                    "edition": "default",
                    "major_version": "10",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                },
                "odyssey": {
                    "major_version": "some hidden value",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value",
                    "edition": "some hidden value"
                }
            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """

  Scenario: Changing default_transaction_read_only setting works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "defaultTransactionReadOnly": true
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": true,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "defaultTransactionReadOnly": true
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "backup": {
                "retain_period": 7,
                "use_backup_service": true,
                "max_incremental_steps": 6,
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                }
            },
            "cluster_nodes": {
                "ha": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ]
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "config": {
                "array_nulls": true,
                "archive_timeout": "10min",
                "auto_explain_log_min_duration": -1,
                "auto_explain_log_analyze": false,
                "auto_explain_log_buffers": false,
                "auto_explain_log_timing": false,
                "auto_explain_log_triggers": false,
                "auto_explain_log_verbose": false,
                "auto_explain_log_nested_statements": false,
                "auto_explain_sample_rate": 1.0,
                "pg_hint_plan_enable_hint": true,
                "pg_hint_plan_enable_hint_table": false,
                "pg_hint_plan_message_level":  "log",
                "pg_hint_plan_debug_print": "off",
                "user_shared_preload_libraries": [],
                "effective_io_concurrency": 1,
                "effective_cache_size": "100GB",
                "auto_kill_timeout": "12 hours",
                "autovacuum_analyze_scale_factor": 0.1,
                "autovacuum_naptime": "15s",
                "autovacuum_vacuum_scale_factor": 0.2,
                "autovacuum_work_mem": -1,
                "backend_flush_after": 0,
                "backslash_quote": "safe_encoding",
                "bgwriter_delay": "200ms",
                "bgwriter_flush_after": "512kB",
                "bgwriter_lru_maxpages": 100,
                "bgwriter_lru_multiplier": 2.0,
                "bytea_output": "hex",
                "checkpoint_completion_target": 0.5,
                "checkpoint_flush_after": "256kB",
                "checkpoint_timeout": "5min",
                "client_min_messages": "notice",
                "constraint_exclusion": "partition",
                "cursor_tuple_fraction": 0.1,
                "deadlock_timeout": "1s",
                "default_statistics_target": 100,
                "default_transaction_isolation": "read committed",
                "default_transaction_read_only": true,
                "default_with_oids": false,
                "enable_bitmapscan": true,
                "enable_hashagg": true,
                "enable_hashjoin": true,
                "enable_indexonlyscan": true,
                "enable_indexscan": true,
                "enable_material": true,
                "enable_mergejoin": true,
                "enable_nestloop": true,
                "enable_parallel_append": true,
                "enable_parallel_hash": true,
                "enable_partition_pruning": true,
                "enable_partitionwise_aggregate": false,
                "enable_partitionwise_join": false,
                "enable_seqscan": true,
                "enable_sort": true,
                "enable_tidscan": true,
                "escape_string_warning": true,
                "exit_on_error": false,
                "force_parallel_mode": "off",
                "from_collapse_limit": 8,
                "gin_pending_list_limit": "4MB",
                "idle_in_transaction_session_timeout": 0,
                "jit": false,
                "join_collapse_limit": 8,
                "lo_compat_privileges": false,
                "lock_timeout": 0,
                "log_checkpoints": false,
                "log_connections": false,
                "log_disconnections": false,
                "log_duration": false,
                "log_error_verbosity": "default",
                "log_lock_waits": false,
                "log_min_duration_statement": -1,
                "log_min_error_statement": "error",
                "log_min_messages": "warning",
                "log_statement": "none",
                "log_temp_files": -1,
                "log_transaction_sample_rate": 0,
                "track_activity_query_size": 1024,
                "maintenance_work_mem": "64MB",
                "max_client_pool_conn": 8000,
                "max_locks_per_transaction": 64,
                "max_logical_replication_workers": 4,
                "max_parallel_maintenance_workers": 2,
                "max_parallel_workers": 8,
                "max_parallel_workers_per_gather": 2,
                "max_pred_locks_per_transaction": 64,
                "max_prepared_transactions": 0,
                "max_replication_slots": 20,
                "max_slot_wal_keep_size": -1,
                "max_standby_streaming_delay": "30s",
                "max_wal_senders": 20,
                "max_worker_processes": 8,
                "old_snapshot_threshold": -1,
                "operator_precedence_warning": false,
                "parallel_leader_participation": true,
                "pgbouncer": {
                    "override_pool_mode": {}
                },
                "pgusers": {
                    "admin": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": true,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod"
                         }
                    },
                    "monitor": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": true,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod"
                        }
                    },
                    "postgres": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": true,
                        "superuser": true,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "lock_timeout": 0,
                            "log_statement": "mod",
                            "synchronous_commit": "local",
                            "temp_file_limit": -1
                        }
                    },
                    "repl": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": true,
                        "superuser": false,
                        "settings": {}
                    },
                    "mdb_admin": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "mdb_replication": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "mdb_monitor": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "test": {
                        "allow_db": "*",
                        "allow_port": 6432,
                        "bouncer": true,
                        "conn_limit": 50,
                        "connect_dbs": [
                            "testdb"
                        ],
                        "create": true,
                        "grants": [],
                        "login": true,
                        "password": {
                            "data": "test_password",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false,
                        "settings": {}
                    }
                },
                "plan_cache_mode": "auto",
                "quote_all_identifiers": false,
                "random_page_cost": 1.0,
                "replacement_sort_tuples": 150000,
                "row_security": true,
                "search_path": "\"$user\", public",
                "seq_page_cost": 1.0,
                "server_reset_query_always": 1,
                "shared_preload_libraries": "pg_stat_statements,pg_stat_kcache,repl_mon",
                "sql_inheritance": true,
                "standard_conforming_strings": true,
                "statement_timeout": 0,
                "stats_temp_directory": "/dev/shm/pg_stat_tmp",
                "synchronize_seqscans": true,
                "synchronous_commit": "on",
                "temp_buffers": "8MB",
                "temp_file_limit": -1,
                "timezone": "Europe/Moscow",
                "transform_null_equals": false,
                "vacuum_cleanup_index_scale_factor": 0.1,
                "vacuum_cost_delay": 0,
                "vacuum_cost_limit": 200,
                "vacuum_cost_page_dirty": 20,
                "vacuum_cost_page_hit": 1,
                "vacuum_cost_page_miss": 10,
                "wal_level": "logical",
                "wal_log_hints": "on",
                "work_mem": "4MB",
                "xmlbinary": "base64",
                "xmloption": "content"
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
                                "postgresql_cluster"
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
                "cluster_type": "postgresql_cluster",
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
            "default pillar": true,
            "perf_diag": {"enable": false},
            "pgbouncer": {
                "custom_user_params": [],
                "custom_user_pool": true
            },
            "pgsync": {
                "autofailover": true,
                "priority": 10,
                "zk_hosts": "localhost",
                "zk_id": "test"
            },
            "postgresql default pillar": true,
            "runlist": [
                "components.postgresql_cluster"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
                        "unmanaged_dbs": [
                {
                    "testdb": {
                        "extensions": [],
                        "lc_collate": "C",
                        "lc_ctype": "C",
                        "user": "test"
                    }
                }
            ],
            "use_wale": false,
            "use_walg": true,
            "walg": {},
            "versions": {
                "postgres": {
                    "edition": "default",
                    "major_version": "10",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                },
                "odyssey": {
                    "major_version": "some hidden value",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value",
                    "edition": "some hidden value"
                }
            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """

  Scenario: Changing replacement_sort_tuples works (removed in PostgreSQL 11)
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "replacementSortTuples": 100
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 100,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {
                    "replacementSortTuples": 100
                }
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Changing PostgreSQL 11 specific settings ignored
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "enableParallelAppend": false,
                "enableParallelHash": false,
                "enablePartitionPruning": false,
                "enablePartitionwiseAggregate": true,
                "enablePartitionwiseJoin": true,
                "jit": true,
                "maxParallelMaintenanceWorkers": 3,
                "parallelLeaderParticipation": false,
                "vacuumCleanupIndexScaleFactor": 0.2
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
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "backup": {
                "retain_period": 7,
                "use_backup_service": true,
                "max_incremental_steps": 6,
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                }
            },
            "cluster_nodes": {
                "ha": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ]
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "config": {
                "array_nulls": true,
                "archive_timeout": "10min",
                "auto_explain_log_min_duration": -1,
                "auto_explain_log_analyze": false,
                "auto_explain_log_buffers": false,
                "auto_explain_log_timing": false,
                "auto_explain_log_triggers": false,
                "auto_explain_log_verbose": false,
                "auto_explain_log_nested_statements": false,
                "auto_explain_sample_rate": 1.0,
                "pg_hint_plan_enable_hint": true,
                "pg_hint_plan_enable_hint_table": false,
                "pg_hint_plan_message_level":  "log",
                "pg_hint_plan_debug_print": "off",
                "user_shared_preload_libraries": [],
                "effective_io_concurrency": 1,
                "effective_cache_size": "100GB",
                "auto_kill_timeout": "12 hours",
                "autovacuum_analyze_scale_factor": 0.1,
                "autovacuum_naptime": "15s",
                "autovacuum_vacuum_scale_factor": 0.2,
                "autovacuum_work_mem": -1,
                "backend_flush_after": 0,
                "backslash_quote": "safe_encoding",
                "bgwriter_delay": "200ms",
                "bgwriter_flush_after": "512kB",
                "bgwriter_lru_maxpages": 100,
                "bgwriter_lru_multiplier": 2.0,
                "bytea_output": "hex",
                "checkpoint_completion_target": 0.5,
                "checkpoint_flush_after": "256kB",
                "checkpoint_timeout": "5min",
                "client_min_messages": "notice",
                "constraint_exclusion": "partition",
                "cursor_tuple_fraction": 0.1,
                "deadlock_timeout": "1s",
                "default_statistics_target": 100,
                "default_transaction_isolation": "read committed",
                "default_transaction_read_only": false,
                "default_with_oids": false,
                "enable_bitmapscan": true,
                "enable_hashagg": true,
                "enable_hashjoin": true,
                "enable_indexonlyscan": true,
                "enable_indexscan": true,
                "enable_material": true,
                "enable_mergejoin": true,
                "enable_nestloop": true,
                "enable_parallel_append": true,
                "enable_parallel_hash": true,
                "enable_partition_pruning": true,
                "enable_partitionwise_aggregate": false,
                "enable_partitionwise_join": false,
                "enable_seqscan": true,
                "enable_sort": true,
                "enable_tidscan": true,
                "escape_string_warning": true,
                "exit_on_error": false,
                "force_parallel_mode": "off",
                "from_collapse_limit": 8,
                "gin_pending_list_limit": "4MB",
                "idle_in_transaction_session_timeout": 0,
                "jit": false,
                "join_collapse_limit": 8,
                "lo_compat_privileges": false,
                "lock_timeout": 0,
                "log_checkpoints": false,
                "log_connections": false,
                "log_disconnections": false,
                "log_duration": false,
                "log_error_verbosity": "default",
                "log_lock_waits": false,
                "log_min_duration_statement": -1,
                "log_min_error_statement": "error",
                "log_min_messages": "warning",
                "log_statement": "none",
                "log_temp_files": -1,
                "log_transaction_sample_rate": 0,
                "track_activity_query_size": 1024,
                "maintenance_work_mem": "64MB",
                "max_client_pool_conn": 8000,
                "max_locks_per_transaction": 64,
                "max_logical_replication_workers": 4,
                "max_parallel_maintenance_workers": 2,
                "max_parallel_workers": 8,
                "max_parallel_workers_per_gather": 2,
                "max_pred_locks_per_transaction": 64,
                "max_prepared_transactions": 0,
                "max_replication_slots": 20,
                "max_slot_wal_keep_size": -1,
                "max_standby_streaming_delay": "30s",
                "max_wal_senders": 20,
                "max_worker_processes": 8,
                "old_snapshot_threshold": -1,
                "operator_precedence_warning": false,
                "parallel_leader_participation": true,
                "pgbouncer": {
                    "override_pool_mode": {}
                },
                "pgusers": {
                    "admin": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": true,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod"
                         }
                    },
                    "monitor": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": true,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod"
                        }
                    },
                    "postgres": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": true,
                        "superuser": true,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "lock_timeout": 0,
                            "log_statement": "mod",
                            "synchronous_commit": "local",
                            "temp_file_limit": -1
                        }
                    },
                    "repl": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": true,
                        "superuser": false,
                        "settings": {}
                    },
                    "mdb_admin": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "mdb_replication": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "mdb_monitor": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "test": {
                        "allow_db": "*",
                        "allow_port": 6432,
                        "bouncer": true,
                        "conn_limit": 50,
                        "connect_dbs": [
                            "testdb"
                        ],
                        "create": true,
                        "grants": [],
                        "login": true,
                        "password": {
                            "data": "test_password",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false,
                        "settings": {}
                    }
                },
                "plan_cache_mode": "auto",
                "quote_all_identifiers": false,
                "random_page_cost": 1.0,
                "replacement_sort_tuples": 150000,
                "row_security": true,
                "search_path": "\"$user\", public",
                "seq_page_cost": 1.0,
                "server_reset_query_always": 1,
                "shared_preload_libraries": "pg_stat_statements,pg_stat_kcache,repl_mon",
                "sql_inheritance": true,
                "standard_conforming_strings": true,
                "statement_timeout": 0,
                "stats_temp_directory": "/dev/shm/pg_stat_tmp",
                "synchronize_seqscans": true,
                "synchronous_commit": "on",
                "temp_buffers": "8MB",
                "temp_file_limit": -1,
                "timezone": "Europe/Moscow",
                "transform_null_equals": false,
                "vacuum_cleanup_index_scale_factor": 0.1,
                "vacuum_cost_delay": 0,
                "vacuum_cost_limit": 200,
                "vacuum_cost_page_dirty": 20,
                "vacuum_cost_page_hit": 1,
                "vacuum_cost_page_miss": 10,
                "wal_level": "logical",
                "wal_log_hints": "on",
                "work_mem": "4MB",
                "xmlbinary": "base64",
                "xmloption": "content"
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
                                "postgresql_cluster"
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
                "cluster_type": "postgresql_cluster",
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
            "default pillar": true,
            "perf_diag": {"enable": false},
            "pgbouncer": {
                "custom_user_params": [],
                "custom_user_pool": true
            },
            "pgsync": {
                "autofailover": true,
                "priority": 10,
                "zk_hosts": "localhost",
                "zk_id": "test"
            },
            "postgresql default pillar": true,
            "runlist": [
                "components.postgresql_cluster"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
                        "unmanaged_dbs": [
                {
                    "testdb": {
                        "extensions": [],
                        "lc_collate": "C",
                        "lc_ctype": "C",
                        "user": "test"
                    }
                }
            ],
            "use_wale": false,
            "use_walg": true,
            "walg": {},
            "versions": {
                "postgres": {
                    "edition": "default",
                    "major_version": "10",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                },
                "odyssey": {
                    "major_version": "some hidden value",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value",
                    "edition": "some hidden value"
                }
            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """

  Scenario: Changing pooler config works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "poolerConfig": {
                "poolingMode": "TRANSACTION",
                "poolDiscard": true
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "poolerConfig": {
                "poolingMode": "TRANSACTION",
                "poolDiscard": true
            },
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Turning off Sox Audit setting does nothing on non-sox cluster
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "soxAudit": false
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

  Scenario: Changing Sox Audit setting
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "soxAudit": true
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "sox-changed" set to "True"
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": true,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test3",
            "password": "password",
            "connLimit": 10,
            "permissions": [],
            "grants": ["reader", "writer"]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test3"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "soxAudit": false
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Cannot turn off sox audit."
    }
    """


  Scenario: Enable sox audit with reader user exists
    When we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "reader",
            "password": "password"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "reader"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "soxAudit": true
        }
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "User 'reader' already exists"
    }
    """

  @events
  Scenario: Allow access for webSql
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "config_spec": {
                "access": {
                    "web_sql": true
                }
            }
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "access": {
                "webSql": true,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "access": {
                "web_sql": true
            },
            "backup": {
                "retain_period": 7,
                "use_backup_service": true,
                "max_incremental_steps": 6,
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                }
            },
            "cluster_nodes": {
                "ha": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ]
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "config": {
                "array_nulls": true,
                "archive_timeout": "10min",
                "auto_explain_log_min_duration": -1,
                "auto_explain_log_analyze": false,
                "auto_explain_log_buffers": false,
                "auto_explain_log_timing": false,
                "auto_explain_log_triggers": false,
                "auto_explain_log_verbose": false,
                "auto_explain_log_nested_statements": false,
                "auto_explain_sample_rate": 1.0,
                "pg_hint_plan_enable_hint": true,
                "pg_hint_plan_enable_hint_table": false,
                "pg_hint_plan_message_level":  "log",
                "pg_hint_plan_debug_print": "off",
                "user_shared_preload_libraries": [],
                "effective_io_concurrency": 1,
                "effective_cache_size": "100GB",
                "auto_kill_timeout": "12 hours",
                "autovacuum_analyze_scale_factor": 0.1,
                "autovacuum_naptime": "15s",
                "autovacuum_vacuum_scale_factor": 0.2,
                "autovacuum_work_mem": -1,
                "backend_flush_after": 0,
                "backslash_quote": "safe_encoding",
                "bgwriter_delay": "200ms",
                "bgwriter_flush_after": "512kB",
                "bgwriter_lru_maxpages": 100,
                "bgwriter_lru_multiplier": 2.0,
                "bytea_output": "hex",
                "checkpoint_completion_target": 0.5,
                "checkpoint_flush_after": "256kB",
                "checkpoint_timeout": "5min",
                "client_min_messages": "notice",
                "constraint_exclusion": "partition",
                "cursor_tuple_fraction": 0.1,
                "deadlock_timeout": "1s",
                "default_statistics_target": 100,
                "default_transaction_isolation": "read committed",
                "default_transaction_read_only": false,
                "default_with_oids": false,
                "enable_bitmapscan": true,
                "enable_hashagg": true,
                "enable_hashjoin": true,
                "enable_indexonlyscan": true,
                "enable_indexscan": true,
                "enable_material": true,
                "enable_mergejoin": true,
                "enable_nestloop": true,
                "enable_parallel_append": true,
                "enable_parallel_hash": true,
                "enable_partition_pruning": true,
                "enable_partitionwise_aggregate": false,
                "enable_partitionwise_join": false,
                "enable_seqscan": true,
                "enable_sort": true,
                "enable_tidscan": true,
                "escape_string_warning": true,
                "exit_on_error": false,
                "force_parallel_mode": "off",
                "from_collapse_limit": 8,
                "gin_pending_list_limit": "4MB",
                "idle_in_transaction_session_timeout": 0,
                "jit": false,
                "join_collapse_limit": 8,
                "lo_compat_privileges": false,
                "lock_timeout": 0,
                "log_checkpoints": false,
                "log_connections": false,
                "log_disconnections": false,
                "log_duration": false,
                "log_error_verbosity": "default",
                "log_lock_waits": false,
                "log_min_duration_statement": -1,
                "log_min_error_statement": "error",
                "log_min_messages": "warning",
                "log_statement": "none",
                "log_temp_files": -1,
                "log_transaction_sample_rate": 0,
                "track_activity_query_size": 1024,
                "maintenance_work_mem": "64MB",
                "max_client_pool_conn": 8000,
                "max_locks_per_transaction": 64,
                "max_logical_replication_workers": 4,
                "max_parallel_maintenance_workers": 2,
                "max_parallel_workers": 8,
                "max_parallel_workers_per_gather": 2,
                "max_pred_locks_per_transaction": 64,
                "max_prepared_transactions": 0,
                "max_replication_slots": 20,
                "max_slot_wal_keep_size": -1,
                "max_standby_streaming_delay": "30s",
                "max_wal_senders": 20,
                "max_worker_processes": 8,
                "old_snapshot_threshold": -1,
                "operator_precedence_warning": false,
                "parallel_leader_participation": true,
                "pgbouncer": {
                    "override_pool_mode": {}
                },
                "pgusers": {
                    "admin": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": true,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod"
                         }
                    },
                    "monitor": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": true,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "log_statement": "mod"
                        }
                    },
                    "postgres": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "connect_dbs": [
                            "*"
                        ],
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": true,
                        "superuser": true,
                        "settings": {
                            "default_transaction_isolation": "read committed",
                            "statement_timeout": 0,
                            "lock_timeout": 0,
                            "log_statement": "mod",
                            "synchronous_commit": "local",
                            "temp_file_limit": -1
                        }
                    },
                    "repl": {
                        "allow_db": "*",
                        "allow_port": "*",
                        "bouncer": false,
                        "create": true,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": true,
                        "superuser": false,
                        "settings": {}
                    },
                    "mdb_admin": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "mdb_replication": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "mdb_monitor": {
                        "conn_limit": 0,
                        "bouncer": false,
                        "create": true,
                        "login": false,
                        "password": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false
                    },
                    "test": {
                        "allow_db": "*",
                        "allow_port": 6432,
                        "bouncer": true,
                        "conn_limit": 50,
                        "connect_dbs": [
                            "testdb"
                        ],
                        "create": true,
                        "grants": [],
                        "login": true,
                        "password": {
                            "data": "test_password",
                            "encryption_version": 0
                        },
                        "replication": false,
                        "superuser": false,
                        "settings": {}
                    }
                },
                "plan_cache_mode": "auto",
                "quote_all_identifiers": false,
                "random_page_cost": 1.0,
                "replacement_sort_tuples": 150000,
                "row_security": true,
                "search_path": "\"$user\", public",
                "seq_page_cost": 1.0,
                "server_reset_query_always": 1,
                "shared_preload_libraries": "pg_stat_statements,pg_stat_kcache,repl_mon",
                "sql_inheritance": true,
                "standard_conforming_strings": true,
                "statement_timeout": 0,
                "stats_temp_directory": "/dev/shm/pg_stat_tmp",
                "synchronize_seqscans": true,
                "synchronous_commit": "on",
                "temp_buffers": "8MB",
                "temp_file_limit": -1,
                "timezone": "Europe/Moscow",
                "transform_null_equals": false,
                "vacuum_cleanup_index_scale_factor": 0.1,
                "vacuum_cost_delay": 0,
                "vacuum_cost_limit": 200,
                "vacuum_cost_page_dirty": 20,
                "vacuum_cost_page_hit": 1,
                "vacuum_cost_page_miss": 10,
                "wal_level": "logical",
                "wal_log_hints": "on",
                "work_mem": "4MB",
                "xmlbinary": "base64",
                "xmloption": "content"
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
                                "postgresql_cluster"
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
                "cluster_type": "postgresql_cluster",
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
            "default pillar": true,
            "perf_diag": {"enable": false},
            "pgbouncer": {
                "custom_user_params": [],
                "custom_user_pool": true
            },
            "pgsync": {
                "autofailover": true,
                "priority": 10,
                "zk_hosts": "localhost",
                "zk_id": "test"
            },
            "postgresql default pillar": true,
            "runlist": [
                "components.postgresql_cluster"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
                        "unmanaged_dbs": [
                {
                    "testdb": {
                        "extensions": [],
                        "lc_collate": "C",
                        "lc_ctype": "C",
                        "user": "test"
                    }
                }
            ],
            "use_wale": false,
            "use_walg": true,
            "walg": {},
            "versions": {
                "postgres": {
                    "edition": "default",
                    "major_version": "10",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                },
                "odyssey": {
                    "major_version": "some hidden value",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value",
                    "edition": "some hidden value"
                }
            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """

  Scenario: Allow access for DataLens
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": true,
                "dataTransfer": false,
                "serverless":false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
  Scenario: Allow access for Serverless
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "access": {
                "serverless": true
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "dataTransfer": false,
                "serverless": true
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Scaling cluster up works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
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
        "cpuUsed": 6.0,
        "memoryUsed": 25769803776,
        "ssdSpaceUsed": 32212254720
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 45,
                    "autovacuumVacuumCostLimit": 700,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 400,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 2147483648,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 45,
                    "autovacuumVacuumCostLimit": 700,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 400,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 2147483648,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.2"
            }
        }
    }
    """

  Scenario: Scaling cluster down works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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
        "description": "Modify PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
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
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Scaling cluster down with large user connections fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1/users/test" with data
    """
    {
        "connLimit": 350
    }
    """
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "max_connections (200) is less than sum of users connection limit (365)"
    }
    """

  Scenario: Scaling max_locks_per_transaction with downscaling max_connections fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxLocksPerTransaction": 80,
                "maxConnections": 100
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Reducing max_connections cannot be mixed with increasing max_locks_per_transaction"
    }
    """

  Scenario: Scaling max_locks_per_transaction with downscaling max_worker_processes fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_10": {
                "maxLocksPerTransaction": 80,
                "maxWorkerProcesses": 6
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Increasing max_locks_per_transaction cannot be mixed with reducing max_worker_processes"
    }
    """

  @events
  Scenario: Cluster name change works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "name": "changed"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Update PostgreSQL cluster metadata",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "name": "changed"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "name": "changed"
    }
    """

  Scenario: Cluster name change to same value fails
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test2",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "10",
           "postgresqlConfig_10": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
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
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
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
    When we POST "/mdb/postgresql/1.0/clusters/cid1:move" with data
    """
    {
        "destinationFolderId": "folder2"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Move PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.MoveClusterMetadata",
            "clusterId": "cid1",
            "destinationFolderId": "folder2",
            "sourceFolderId": "folder1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.MoveCluster" event with
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
    When we GET "/mdb/1.0/operations/worker_task_id3"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Move PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.MoveClusterMetadata",
            "clusterId": "cid1",
            "destinationFolderId": "folder2",
            "sourceFolderId": "folder1"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
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

  @delete @events
  Scenario: Cluster removal works
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.DeleteClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.DeleteCluster" event with
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
    When we GET "/mdb/postgresql/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusters": []
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 403
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body equals
    """
    {}
    """

  @delete
  Scenario: After cluster delete cluster.name can be reused
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200

  @delete
  Scenario: Cluster with running operations can not be deleted
    When we run query
    """
    UPDATE dbaas.worker_queue
       SET result = NULL,
           end_ts = NULL
    """
    And we DELETE "/mdb/postgresql/1.0/clusters/cid1"
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
    And we DELETE "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200

  @delete @operations
  Scenario: After cluster delete cluster operations are shown
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "description": "changed"
    }
    """
    Then we get response with status 200
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1"
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
    When we GET "/mdb/postgresql/1.0/operations?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "operations": [
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Delete PostgreSQL cluster",
                "done": false,
                "id": "worker_task_id3",
                "metadata": {
                    "@type": "yandex.cloud.mdb.postgresql.v1.DeleteClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify PostgreSQL cluster",
                "done": true,
                "id": "worker_task_id2",
                "metadata": {
                    "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00",
                "response": {}
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Create PostgreSQL cluster",
                "done": true,
                "id": "worker_task_id1",
                "metadata": {
                    "@type": "yandex.cloud.mdb.postgresql.v1.CreateClusterMetadata",
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
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
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
    When we POST "/mdb/postgresql/1.0/clusters/cid1:stop"
    Then we get response with status 501 and body contains
    """
    {
        "code": 12,
        "message": "Stop for postgresql_cluster not implemented in that installation"
    }
    """

  @status
  Scenario: Cluster status
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "UPDATING"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """

  @events
  Scenario: Manual failover works
    When we POST "/mdb/postgresql/1.0/clusters/cid1:startFailover"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start manual failover on PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.StartClusterFailoverMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.StartClusterFailover" event with
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
    And we GET "/mdb/postgresql/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.postgresql.v1.Cluster",
        "id": "cid1",
        "name": "test",
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Manual failover with target host works
    When we POST "/mdb/postgresql/1.0/clusters/cid1:startFailover" with data
    """
    {
        "hostName": "sas-1.db.yandex.net"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start manual failover on PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.StartClusterFailoverMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/postgresql/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.postgresql.v1.Cluster",
        "id": "cid1",
        "name": "test",
        "config": {
            "version": "10",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": "cloud1",
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_10": {
                "defaultConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "effectiveConfig": {
                    "archiveTimeout": 600000,
                    "autoExplainLogAnalyze": false,
                    "autoExplainLogBuffers": false,
                    "autoExplainLogMinDuration": -1,
                    "autoExplainLogNestedStatements": false,
                    "autoExplainLogTiming": false,
                    "autoExplainLogTriggers": false,
                    "autoExplainLogVerbose": false,
                    "autoExplainSampleRate": 1.0,
                    "pgHintPlanEnableHint": true,
                    "pgHintPlanEnableHintTable": false,
                    "pgHintPlanMessageLevel":  "LOG_LEVEL_LOG",
                    "pgHintPlanDebugPrint": "PG_HINT_PLAN_DEBUG_PRINT_OFF",
                    "sharedPreloadLibraries": [],
                    "effectiveIoConcurrency": 1,
                    "effectiveCacheSize": 107374182400,
                    "arrayNulls": true,
                    "autovacuumAnalyzeScaleFactor": 0.1,
                    "autovacuumMaxWorkers": 3,
                    "autovacuumNaptime": 15000,
                    "autovacuumVacuumCostDelay": 50,
                    "autovacuumVacuumCostLimit": 550,
                    "autovacuumVacuumScaleFactor": 0.2,
                    "autovacuumWorkMem": -1,
                    "backendFlushAfter": 0,
                    "backslashQuote": "BACKSLASH_QUOTE_SAFE_ENCODING",
                    "bgwriterDelay": 200,
                    "bgwriterLruMaxpages": 100,
                    "bgwriterLruMultiplier": 2.0,
                    "byteaOutput": "BYTEA_OUTPUT_HEX",
                    "checkpointCompletionTarget": 0.5,
                    "checkpointTimeout": 300000,
                    "clientMinMessages": "LOG_LEVEL_NOTICE",
                    "constraintExclusion": "CONSTRAINT_EXCLUSION_PARTITION",
                    "cursorTupleFraction": 0.1,
                    "deadlockTimeout": 1000,
                    "defaultStatisticsTarget": 100,
                    "defaultTransactionIsolation": "TRANSACTION_ISOLATION_READ_COMMITTED",
                    "defaultTransactionReadOnly": false,
                    "defaultWithOids": false,
                    "enableBitmapscan": true,
                    "enableHashagg": true,
                    "enableHashjoin": true,
                    "enableIndexonlyscan": true,
                    "enableIndexscan": true,
                    "enableMaterial": true,
                    "enableMergejoin": true,
                    "enableNestloop": true,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "joinCollapseLimit": 8,
                    "loCompatPrivileges": false,
                    "lockTimeout": 0,
                    "logCheckpoints": false,
                    "logConnections": false,
                    "logDisconnections": false,
                    "logDuration": false,
                    "logErrorVerbosity": "LOG_ERROR_VERBOSITY_DEFAULT",
                    "logLockWaits": false,
                    "logMinDurationStatement": -1,
                    "logMinErrorStatement": "LOG_LEVEL_ERROR",
                    "logMinMessages": "LOG_LEVEL_WARNING",
                    "logStatement": "LOG_STATEMENT_NONE",
                    "logTempFiles": -1,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
                    "replacementSortTuples": 150000,
                    "rowSecurity": true,
                    "searchPath": "\"$user\", public",
                    "seqPageCost": 1.0,
                    "sharedBuffers": 1073741824,
                    "standardConformingStrings": true,
                    "statementTimeout": 0,
                    "synchronizeSeqscans": true,
                    "synchronousCommit": "SYNCHRONOUS_COMMIT_ON",
                    "tempBuffers": 8388608,
                    "tempFileLimit": -1,
                    "timezone": "Europe/Moscow",
                    "trackActivityQuerySize": 1024,
                    "transformNullEquals": false,
                    "vacuumCostDelay": 0,
                    "vacuumCostLimit": 200,
                    "vacuumCostPageDirty": 20,
                    "vacuumCostPageHit": 1,
                    "vacuumCostPageMiss": 10,
                    "walLevel": "WAL_LEVEL_LOGICAL",
                    "workMem": 4194304,
                    "xmlbinary": "XML_BINARY_BASE64",
                    "xmloption": "XML_OPTION_CONTENT"
                },
                "userConfig": {}
            },
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Manual failover with not existing target host fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1:startFailover" with data
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

  Scenario: Manual failover with not HA target host fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters/cid1:startFailover" with data
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

  Scenario: Manual failover in HA-group of size 1 fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
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
        "description": "Modify hosts in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "myt-1.db.yandex.net",
            "replicationSource": "iva-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify hosts in PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.UpdateClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "myt-1.db.yandex.net"
            ]
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    When we GET "/mdb/postgresql/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
        {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "priority": 10,
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "replicaType": "UNKNOWN",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "configSpec": {
                    "postgresqlConfig_10": {
                        "workMem": 65536
                    }
                },
                "name": "myt-1.db.yandex.net",
                "priority": 9,
                "replicationSource": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "SYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "priority": 5,
                "replicationSource": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "replicaType": "ASYNC",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "POOLER"
                    },
                    {
                        "health": "ALIVE",
                        "type": "POSTGRESQL"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas"
            }
        ]
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters/cid1:startFailover"
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "HA group is too small to perform failover"
    }
    """
  Scenario: Adding HA host above limit fails
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id3" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id4" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id5" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id6" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id7" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id8" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id9" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id10" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id11" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id12" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id13" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id14" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id15" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "PostgreSQL cluster allows at most 17 HA hosts"
    }
    """

  Scenario: Adding 19 non HA hosts works
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"

        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id3" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id4" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id5" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id6" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id7" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id8" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id9" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id10" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id11" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id12" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id13" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id14" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id15" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id16" acquired and finished by worker
     When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id17" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id18" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id19" acquired and finished by worker
    When we POST "/mdb/postgresql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "replicationSource": "sas-1.db.yandex.net"
        }]
    }
    """
    Then we get response with status 200
    And "worker_task_id20" acquired and finished by worker
