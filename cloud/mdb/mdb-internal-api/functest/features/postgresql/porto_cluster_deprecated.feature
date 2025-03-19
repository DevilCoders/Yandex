Feature: Create/Restore Porto PostgreSQL Cluster with deprecated version

  Background:
    Given default headers
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
       "description": "",
       "networkId": "IN-PORTO-NO-NETWORK-API",
       "monitoringCloudId": "cloud1"
    }
    """

  Scenario: Create cluster with deprecated version fails
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Can't create cluster, version '10' is deprecated"
    }
    """

  Scenario: Create cluster with deprecated version works under feature flag
    Given feature flags
    """
    ["MDB_PG_ALLOW_DEPRECATED_VERSIONS"]
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

  Scenario: Create cluster with deprecated version works with restore
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Can't create cluster, version '10' is deprecated"
    }
    """
    When we run query
    """
    UPDATE dbaas.default_versions
    SET is_deprecated = false 
    WHERE
    type = 'postgresql_cluster' AND component = 'postgres' AND major_version='10'
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
    And we run query
    """
    UPDATE dbaas.default_versions
    SET is_deprecated = true 
    WHERE
    type = 'postgresql_cluster' AND component = 'postgres' AND major_version='10'
    """
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
	And I successfully execute query
	"""
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:01.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:01.000001+00:00'), 'SCHEDULE', '{"name": "auto01","start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb),
		('auto02', 'DONE', '1970-01-01T00:00:01.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:01.000001+00:00'), 'SCHEDULE', '{"name": "auto02", "start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    When we POST "/mdb/postgresql/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto01",
        "time": "1970-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new PostgreSQL cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "restore-from" containing map:
      |      cid                         |           cid1            |
      |    backup-id                     |          auto01           |
      |      time                        | 1970-01-01T00:00:02+00:00 |
      | time-inclusive                   |           false           |
      | restore-latest                   |           false           |
      | use_backup_service_at_latest_rev |           true            |
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.RestoreCluster" event with
    """
    {
        "details": {
            "backup_id": "cid1:auto01",
            "cluster_id": "cid2"
        },
        "request_parameters": {
            "name": "test_restored",
            "environment": "PRESTABLE",
            "config_spec": {
                "resources": {
                    "disk_size": 10737418240,
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                }
            },
            "host_specs": [{
                "zone_id": "myt"
            }, {
                "zone_id": "iva"
            }, {
                "zone_id": "sas"
            }],
            "backup_id": "cid1:auto01",
            "time": "1970-01-01T00:00:02+00:00"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/postgresql/1.0/clusters/cid2"
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
            "monitoringCloudId": null,
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
        "description": "",
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
        "name": "test_restored",
        "networkId": "",
        "status": "RUNNING"
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 2,
        "cpuUsed": 6,
        "memoryUsed": 25769803776,
        "ssdSpaceUsed": 64424509440
    }
    """
    When we GET "/api/v1.0/config/iva-2.db.yandex.net"
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
                    "iva-2.db.yandex.net",
                    "myt-2.db.yandex.net",
                    "sas-2.db.yandex.net"
                ]
            },
            "cluster_private_key": {
                "data": "2",
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
                        "subcid2": {
                            "hosts": {
                                "iva-2.db.yandex.net": {"geo": "iva"},
                                "myt-2.db.yandex.net": {"geo": "myt"},
                                "sas-2.db.yandex.net": {"geo": "sas"}
                            },
                            "name": "test_restored",
                            "roles": [
                                "postgresql_cluster"
                            ],
                            "shards": {}
                        }
                    }
                },
                "cluster_hosts": [
                    "iva-2.db.yandex.net",
                    "myt-2.db.yandex.net",
                    "sas-2.db.yandex.net"
                ],
                "cluster_id": "cid2",
                "cluster_name": "test_restored",
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
                "fqdn": "iva-2.db.yandex.net",
                "geo": "iva",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "iva-2.db.yandex.net",
                    "myt-2.db.yandex.net",
                    "sas-2.db.yandex.net"
                ],
                "shard_id": null,
                "shard_name": null,
                "space_limit": 10737418240,
                "subcluster_id": "subcid2",
                "subcluster_name": "test_restored",
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
            "s3_bucket": "yandexcloud-dbaas-cid2",
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
