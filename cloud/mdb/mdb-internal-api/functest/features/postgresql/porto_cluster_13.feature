Feature: Create/Modify/Backup Porto PostgreSQL Cluster version 13

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_POSTGRESQL_13"]
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
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "13",
           "postgresqlConfig_13": {},
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
       "description": "test cluster"
    }
    """
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

  Scenario: Cluster creation works
    When we GET "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "performanceDiagnostics": {
                "enabled": false,
                "statementsSamplingInterval": 600,
                "sessionsSamplingInterval": 60
            },
            "version": "13",
            "autofailover": true,
            "soxAudit": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "monitoringCloudId": null,
            "backupRetainPeriodDays": 7,
            "postgresqlConfig_13": {
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
                    "enableParallelAppend": true,
                    "enableParallelHash": true,
                    "enablePartitionPruning": true,
                    "enablePartitionwiseAggregate": false,
                    "enablePartitionwiseJoin": false,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "jit": false,
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
                    "logTransactionSampleRate": 0,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelMaintenanceWorkers": 2,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxSlotWalKeepSize": -1,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "maxWalSenders": 20,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "parallelLeaderParticipation": true,
                    "planCacheMode": "PLAN_CACHE_MODE_AUTO",
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
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
                    "vacuumCleanupIndexScaleFactor": 0.1,
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
                    "enableParallelAppend": true,
                    "enableParallelHash": true,
                    "enablePartitionPruning": true,
                    "enablePartitionwiseAggregate": false,
                    "enablePartitionwiseJoin": false,
                    "enableSeqscan": true,
                    "enableSort": true,
                    "enableTidscan": true,
                    "escapeStringWarning": true,
                    "exitOnError": false,
                    "forceParallelMode": "FORCE_PARALLEL_MODE_OFF",
                    "fromCollapseLimit": 8,
                    "ginPendingListLimit": 4194304,
                    "idleInTransactionSessionTimeout": 0,
                    "jit": false,
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
                    "logTransactionSampleRate": 0,
                    "maintenanceWorkMem": 67108864,
                    "maxConnections": 200,
                    "maxLocksPerTransaction": 64,
                    "maxLogicalReplicationWorkers": 4,
                    "maxParallelMaintenanceWorkers": 2,
                    "maxParallelWorkers": 8,
                    "maxParallelWorkersPerGather": 2,
                    "maxPredLocksPerTransaction": 64,
                    "maxPreparedTransactions": 0,
                    "maxReplicationSlots": 20,
                    "maxSlotWalKeepSize": -1,
                    "maxStandbyStreamingDelay": 30000,
                    "maxWalSenders": 20,
                    "maxWalSize": 1073741824,
                    "maxWorkerProcesses": 8,
                    "minWalSize": 536870912,
                    "oldSnapshotThreshold": -1,
                    "operatorPrecedenceWarning": false,
                    "parallelLeaderParticipation": true,
                    "planCacheMode": "PLAN_CACHE_MODE_AUTO",
                    "quoteAllIdentifiers": false,
                    "randomPageCost": 1.0,
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
                    "vacuumCleanupIndexScaleFactor": 0.1,
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
        "cpuUsed": 3.0,
        "memoryUsed": 12884901888,
        "ssdSpaceUsed": 32212254720
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.versions.postgres" contains
    """
    {
        "major_version": "13"
    }
    """


  Scenario: Changing PostgreSQL 12 specific settings works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_13": {
                "logTransactionSampleRate": 0.5,
                "planCacheMode": "PLAN_CACHE_MODE_FORCE_GENERIC_PLAN"
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
    Then we get response with status 200
    And body at path "$.config.postgresqlConfig_13.userConfig" contains
    """
    {
        "logTransactionSampleRate": 0.5,
        "planCacheMode": "PLAN_CACHE_MODE_FORCE_GENERIC_PLAN"
    }
    """
    And body at path "$.config.postgresqlConfig_13.effectiveConfig" contains
    """
    {
        "logTransactionSampleRate": 0.5,
        "planCacheMode": "PLAN_CACHE_MODE_FORCE_GENERIC_PLAN"
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.config" contains
    """
    {
        "log_transaction_sample_rate": 0.5,
        "plan_cache_mode": "force_generic_plan"
    }
    """

  Scenario: Changing plan_cache_mode to invalid value doesn't work
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_13": {
                "planCacheMode": "force_nonexisting_plan"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.postgresqlConfig_13.planCacheMode: Invalid value 'force_nonexisting_plan', allowed values: PLAN_CACHE_MODE_AUTO, PLAN_CACHE_MODE_FORCE_CUSTOM_PLAN, PLAN_CACHE_MODE_FORCE_GENERIC_PLAN"
    }
    """

  Scenario: Changing PostgreSQL 13 specific settings works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "postgresqlConfig_13": {
                "maxSlotWalKeepSize": 1073741824
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
    Then we get response with status 200
    And body at path "$.config.postgresqlConfig_13.userConfig" contains
    """
    {
        "maxSlotWalKeepSize": 1073741824
    }
    """
    And body at path "$.config.postgresqlConfig_13.effectiveConfig" contains
    """
    {
        "maxSlotWalKeepSize": 1073741824
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.config" contains
    """
    {
        "max_slot_wal_keep_size": "1024MB"
    }
    """

  Scenario: Create pg13 cluster with wal_keep_size greater then DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT of space limit fails
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "13",
           "postgresqlConfig_13": {
               "walKeepSize": 6442450944
           },
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
           "password": "test_password",
           "connLimit": 10
       }],
       "hostSpecs": [{
           "zoneId": "myt"
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "wal_keep_size (6442450944) is greater than 0.5 of disk_size (10737418240)"
    }
    """

  Scenario: Create pg13 cluster with max_slot_wal_keep_size greater then DEFAULT_FRAC_DISK_SIZE_WAL_LIMIT of space limit fails
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "13",
           "postgresqlConfig_13": {
               "maxSlotWalKeepSize": 6442450944
           },
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
           "password": "test_password",
           "connLimit": 10
       }],
       "hostSpecs": [{
           "zoneId": "myt"
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "max_slot_wal_keep_size (6442450944) is greater than 0.5 of disk_size (10737418240)"
    }
    """

  Scenario: Create pg13 cluster with max_slot_wal_keep_size less then DEFAULT_MIN_WAL_SIZE_IN_MEGABYTE fails
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "13",
           "postgresqlConfig_13": {
               "maxSlotWalKeepSize": 14680064
           },
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
           "password": "test_password",
           "connLimit": 10
       }],
       "hostSpecs": [{
           "zoneId": "myt"
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "max_slot_wal_keep_size (14680064) is less than minimum allowed 64 MEGABYTES"
    }
    """
