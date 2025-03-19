Feature: Create MySQL with different configuration options
  Background:
    Given default headers
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_5_7": {
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


  Scenario: Cluster creation with default config options works
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
                "enabled": false,
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
                "userConfig": {
                }
            },
            "soxAudit": false,
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  @events
  Scenario: Cluster modification with all valid config options works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "innodbBufferPoolSize": 1610612736,
                "characterSetServer": "utf8",
                "collationServer": "utf8_general_ci",
                "defaultTimeZone": "Europe/Berlin",
                "groupConcatMaxLen": 2048,
                "netReadTimeout": 20,
                "netWriteTimeout": 20,
                "queryCacheLimit": 1048576,
                "queryCacheSize": 104857600,
                "queryCacheType": 1,
                "tmpTableSize": 33554432,
                "maxHeapTableSize": 33554432,
                "innodbFlushLogAtTrxCommit": 2,
                "innodbLockWaitTimeout": 120,
                "transactionIsolation": "READ_COMMITTED",
                "logErrorVerbosity": 2,
                "maxConnections": 150,
                "longQueryTime": 1.0,
                "generalLog": true,
                "auditLog": true,
                "maxAllowedPacket": 33554432,
                "sqlMode": [
                    "TRADITIONAL",
                    "REAL_AS_FLOAT"
                ],
                "innodbAdaptiveHashIndex" : false,
                "innodbNumaInterleave" : true,
                "innodbLogBufferSize" : 33554432,
                "innodbLogFileSize" : 536870912,
                "innodbIoCapacity" : 400,
                "innodbIoCapacityMax" : 4000,
                "innodbReadIoThreads" : 2,
                "innodbWriteIoThreads" : 2,
                "innodbPurgeThreads" : 2,
                "innodbThreadConcurrency" : 100,
                "innodbTempDataFileMaxSize" : 10737418240,
                "threadCacheSize" : 20,
                "threadStack" : 262144,
                "joinBufferSize" : 524288,
                "sortBufferSize" : 524288,
                "tableDefinitionCache" : 1000,
                "tableOpenCache" : 2000,
                "tableOpenCacheInstances" : 8,
                "explicitDefaultsForTimestamp" : true,
                "autoIncrementIncrement" : 5,
                "autoIncrementOffset" : 2,
                "syncBinlog" : 1000,
                "binlogCacheSize" : 65536,
                "binlogGroupCommitSyncDelay" : 1000,
                "binlogRowImage" : "NOBLOB",
                "binlogRowsQueryLogEvents" : true,
                "mdbPreserveBinlogBytes" : 1073741824,
                "rplSemiSyncMasterWaitForSlaveCount" : 2,
                "showCompatibility56": false,
                "binlogTransactionDependencyTracking" : "WRITESET",
                "slaveParallelType" : "LOGICAL_CLOCK",
                "slaveParallelWorkers" : 16,
                "defaultAuthenticationPlugin": "SHA256_PASSWORD",
                "interactiveTimeout": 28800,
                "waitTimeout": 28800,
                "mdbPriorityChoiceMaxLag": 50,
                "mdbOfflineModeEnableLag": 86400,
                "mdbOfflineModeDisableLag": 300,
                "rangeOptimizerMaxMemSize": 8388608,
                "innodbOnlineAlterLogMaxSize": 134217728,
                "innodbFtMinTokenSize": 3,
                "innodbFtMaxTokenSize": 84,
                "innodbStrictMode": false,
                "maxDigestLength": 2048,
                "lowerCaseTableNames": 0,
                "innodbPageSize": 16384,
                "maxSpRecursionDepth": 5,
                "innodbCompressionLevel": 8
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "config_spec": {}
        }
    }
    """
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
                "enabled": false,
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
                    "characterSetServer": "utf8",
                    "collationServer": "utf8_general_ci",
                    "defaultTimeZone": "Europe/Berlin",
                    "groupConcatMaxLen": 2048,
                    "netReadTimeout": 20,
                    "netWriteTimeout": 20,
                    "queryCacheLimit": 1048576,
                    "queryCacheSize": 104857600,
                    "queryCacheType": 1,
                    "tmpTableSize": 33554432,
                    "maxHeapTableSize": 33554432,
                    "innodbFlushLogAtTrxCommit": 2,
                    "innodbLockWaitTimeout": 120,
                    "transactionIsolation": "READ_COMMITTED",
                    "logErrorVerbosity": 2,
                    "maxConnections": 150,
                    "longQueryTime": 1.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
                    "generalLog": true,
                    "auditLog": true,
                    "maxAllowedPacket": 33554432,
                    "sqlMode": [
                        "TRADITIONAL",
                        "REAL_AS_FLOAT"
                    ],
                    "innodbAdaptiveHashIndex" : false,
                    "innodbNumaInterleave" : true,
                    "innodbLogBufferSize" : 33554432,
                    "innodbLogFileSize" : 536870912,
                    "innodbIoCapacity" : 400,
                    "innodbIoCapacityMax" : 4000,
                    "innodbReadIoThreads" : 2,
                    "innodbWriteIoThreads" : 2,
                    "innodbPurgeThreads" : 2,
                    "innodbThreadConcurrency" : 100,
                    "innodbTempDataFileMaxSize" : 10737418240,
                    "threadCacheSize" : 20,
                    "threadStack" : 262144,
                    "joinBufferSize" : 524288,
                    "sortBufferSize" : 524288,
                    "tableDefinitionCache" : 1000,
                    "tableOpenCache" : 2000,
                    "tableOpenCacheInstances" : 8,
                    "explicitDefaultsForTimestamp" : true,
                    "autoIncrementIncrement" : 5,
                    "autoIncrementOffset" : 2,
                    "syncBinlog" : 1000,
                    "binlogCacheSize" : 65536,
                    "binlogGroupCommitSyncDelay" : 1000,
                    "binlogRowImage" : "NOBLOB",
                    "binlogRowsQueryLogEvents" : true,
                    "mdbPreserveBinlogBytes" : 1073741824,
                    "rplSemiSyncMasterWaitForSlaveCount" : 2,
                    "showCompatibility56": false,
                    "binlogTransactionDependencyTracking" : "WRITESET",
                    "slaveParallelType" : "LOGICAL_CLOCK",
                    "slaveParallelWorkers" : 16,
                    "defaultAuthenticationPlugin": "SHA256_PASSWORD",
                    "interactiveTimeout": 28800,
                    "waitTimeout": 28800,
                    "mdbPriorityChoiceMaxLag": 50,
                    "mdbOfflineModeEnableLag": 86400,
                    "mdbOfflineModeDisableLag": 300,
                    "rangeOptimizerMaxMemSize": 8388608,
                    "innodbOnlineAlterLogMaxSize": 134217728,
                    "innodbFtMinTokenSize": 3,
                    "innodbFtMaxTokenSize": 84,
                    "innodbStrictMode": false,
                    "maxDigestLength": 2048,
                    "lowerCaseTableNames": 0,
                    "innodbPageSize": 16384,
                    "maxSpRecursionDepth": 5,
                    "innodbCompressionLevel": 8
                },
                "userConfig": {
                    "innodbBufferPoolSize": 1610612736,
                    "characterSetServer": "utf8",
                    "collationServer": "utf8_general_ci",
                    "defaultTimeZone": "Europe/Berlin",
                    "groupConcatMaxLen": 2048,
                    "netReadTimeout": 20,
                    "netWriteTimeout": 20,
                    "queryCacheLimit": 1048576,
                    "queryCacheSize": 104857600,
                    "queryCacheType": 1,
                    "tmpTableSize": 33554432,
                    "maxHeapTableSize": 33554432,
                    "innodbFlushLogAtTrxCommit": 2,
                    "innodbLockWaitTimeout": 120,
                    "transactionIsolation": "READ_COMMITTED",
                    "logErrorVerbosity": 2,
                    "maxConnections": 150,
                    "longQueryTime": 1.0,
                    "generalLog": true,
                    "auditLog": true,
                    "maxAllowedPacket": 33554432,
                    "sqlMode": [
                        "TRADITIONAL",
                        "REAL_AS_FLOAT"
                    ],
                    "innodbAdaptiveHashIndex" : false,
                    "innodbNumaInterleave" : true,
                    "innodbLogBufferSize" : 33554432,
                    "innodbLogFileSize" : 536870912,
                    "innodbIoCapacity" : 400,
                    "innodbIoCapacityMax" : 4000,
                    "innodbReadIoThreads" : 2,
                    "innodbWriteIoThreads" : 2,
                    "innodbPurgeThreads" : 2,
                    "innodbThreadConcurrency" : 100,
                    "innodbTempDataFileMaxSize" : 10737418240,
                    "threadCacheSize" : 20,
                    "threadStack" : 262144,
                    "joinBufferSize" : 524288,
                    "sortBufferSize" : 524288,
                    "tableDefinitionCache" : 1000,
                    "tableOpenCache" : 2000,
                    "tableOpenCacheInstances" : 8,
                    "explicitDefaultsForTimestamp" : true,
                    "autoIncrementIncrement" : 5,
                    "autoIncrementOffset" : 2,
                    "syncBinlog" : 1000,
                    "binlogCacheSize" : 65536,
                    "binlogGroupCommitSyncDelay" : 1000,
                    "binlogRowImage" : "NOBLOB",
                    "binlogRowsQueryLogEvents" : true,
                    "mdbPreserveBinlogBytes" : 1073741824,
                    "rplSemiSyncMasterWaitForSlaveCount" : 2,
                    "showCompatibility56": false,
                    "binlogTransactionDependencyTracking" : "WRITESET",
                    "slaveParallelType" : "LOGICAL_CLOCK",
                    "slaveParallelWorkers" : 16,
                    "defaultAuthenticationPlugin": "SHA256_PASSWORD",
                    "interactiveTimeout": 28800,
                    "waitTimeout": 28800,
                    "mdbPriorityChoiceMaxLag": 50,
                    "mdbOfflineModeEnableLag": 86400,
                    "mdbOfflineModeDisableLag": 300,
                    "rangeOptimizerMaxMemSize": 8388608,
                    "innodbOnlineAlterLogMaxSize": 134217728,
                    "innodbFtMinTokenSize": 3,
                    "innodbFtMaxTokenSize": 84,
                    "innodbStrictMode": false,
                    "maxDigestLength": 2048,
                    "lowerCaseTableNames": 0,
                    "innodbPageSize": 16384,
                    "maxSpRecursionDepth": 5,
                    "innodbCompressionLevel": 8
                }
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
        "id": "cid1",
        "labels": {},
        "name": "test",
        "networkId": ""
    }
    """


  Scenario: Cluster modification with invalid sql_mode fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "sqlMode": [
                    "ALLOW_INVALID_DATES",
                    "INVALID"
                ]
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.sqlMode.1: Invalid value 'INVALID', allowed values: ALLOW_INVALID_DATES, ANSI_QUOTES, ERROR_FOR_DIVISION_BY_ZERO, HIGH_NOT_PRECEDENCE, IGNORE_SPACE, NO_AUTO_CREATE_USER, NO_AUTO_VALUE_ON_ZERO, NO_BACKSLASH_ESCAPES, NO_DIR_IN_CREATE, NO_ENGINE_SUBSTITUTION, NO_UNSIGNED_SUBTRACTION, NO_ZERO_DATE, NO_ZERO_IN_DATE, NO_FIELD_OPTIONS, NO_KEY_OPTIONS, NO_TABLE_OPTIONS, ONLY_FULL_GROUP_BY, PAD_CHAR_TO_FULL_LENGTH, PIPES_AS_CONCAT, REAL_AS_FLOAT, STRICT_ALL_TABLES, STRICT_TRANS_TABLES, ANSI, TRADITIONAL, DB2, MAXDB, MSSQL, MYSQL323, MYSQL40, ORACLE, POSTGRESQL"
    }
    """


  Scenario: Cluster modification with max_connections below limit succeeds
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "maxConnections": 512
            }
        }
    }
    """
    Then we get response with status 200


  Scenario: Cluster modification with too high max_connections fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "maxConnections": 513
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "max_connections: 513 should be less than or equal to 512"
    }
    """


  Scenario: Cluster modification with invalid charset fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "characterSetServer": "surgik",
                "collationServer": "latin1_swedish_ci"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3
    }
    """


  Scenario: Cluster modification with invalid collation fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "characterSetServer": "latin1",
                "collationServer": "surgik_ci"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3
    }
    """


  Scenario: Cluster modification with only charset fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "characterSetServer": "utf8"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "character_set_server and collation_server should be specified together"
    }
    """


  Scenario: Cluster modification with only collation fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "collationServer": "utf8_general_ci"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "character_set_server and collation_server should be specified together"
    }
    """


  Scenario: Cluster modification with invalid charset and collation combination fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "characterSetServer": "utf8",
                "collationServer": "utf8mb4_general_ci"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "collation_server should correspond to character_set_server"
    }
    """


  Scenario: Cluster modification with invalid timezone fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "defaultTimeZone": "Europe/NewVasyuki"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3
    }
    """


  Scenario: Cluster modification with max_allowed_packet not multiple to 1k fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "maxAllowedPacket": 1048577
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.maxAllowedPacket: 1048577 should be divisible by 1024."
    }
    """

  Scenario: Cluster modification with too high max_allowed_packet fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "maxAllowedPacket": 2147483648
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.maxAllowedPacket: Must be between 1048576 and 1073741824."
    }
    """


  Scenario: Cluster modification with too high group_concat_max_len fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "groupConcatMaxLen": 33554433
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.groupConcatMaxLen: Must be between 4 and 33554432."
    }
    """


  Scenario: Cluster modification with too high net_read_timeout fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "netReadTimeout": 1201
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.netReadTimeout: Must be between 1 and 1200."
    }
    """


  Scenario: Cluster modification with too high net_write_timeout fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "netWriteTimeout": 1201
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.netWriteTimeout: Must be between 1 and 1200."
    }
    """


  Scenario: Cluster modification with too high tmp_table_size fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "tmpTableSize": 1073741824
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.tmpTableSize: Must be between 1024 and 536870912."
    }
    """


  Scenario: Cluster modification with too high max_heap_table_size fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "maxHeapTableSize": 1073741824
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.maxHeapTableSize: Must be between 16384 and 536870912."
    }
    """


  Scenario: Cluster modification with too high innodb_lock_wait_timeout fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "innodbLockWaitTimeout": 28801
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.innodbLockWaitTimeout: Must be between 1 and 28800."
    }
    """


  Scenario: Cluster modification with invalid transaction_isolation fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "transactionIsolation": "READ_UNCOMMITTED"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.transactionIsolation: Invalid value 'READ_UNCOMMITTED', allowed values: READ_COMMITTED, REPEATABLE_READ, SERIALIZABLE"
    }
    """

  Scenario: Cluster modification with invalid high log_error_verbosity fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "logErrorVerbosity": 1
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.logErrorVerbosity: Must be between 2 and 3."
    }
    """

  Scenario: Cluster modification with too high max_digest_length fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "maxDigestLength": 16384
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.maxDigestLength: Must be between 1024 and 8192."
    }
    """

  Scenario: Cluster modification with invalid max_digest_length fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "maxDigestLength": 5000
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.maxDigestLength: 5000 should be divisible by 1024."
    }
    """

  Scenario: Cluster modification with too high innodb_buffer_pool_size fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "innodbBufferPoolSize": 3865470567
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "innodb_buffer_pool_size: 3865470567 should be less than or equal to 2684354560"
    }
    """

  Scenario: Cluster modification with too high query_cache_size fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "queryCacheSize": 3865470567
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "query_cache_size: 3865470567 should be less than or equal to 429496729"
    }
    """

  Scenario: Cluster modification with invalid innodb_flush_log_at_trx_commit fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "innodbFlushLogAtTrxCommit": 0
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.innodbFlushLogAtTrxCommit: Invalid value '0', allowed values: 1, 2"
    }
    """

    Scenario: Cluster modification with mdb_preserve_binlog_bytes less than 1GB fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "mdbPreserveBinlogBytes" : 1000000000
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.mdbPreserveBinlogBytes: Must be between 1073741824 and 1099511627776."
    }
    """

    Scenario: Cluster modification with mdb_preserve_binlog_bytes more than 1TB fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "mdbPreserveBinlogBytes" : 2199023255552
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.mysqlConfig_5_7.mdbPreserveBinlogBytes: Must be between 1073741824 and 1099511627776."
    }
    """

    Scenario: Cluster modification with reversed mdb_offline_mode enable-disable lag fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "mdbOfflineModeEnableLag": 1000,
                "mdbOfflineModeDisableLag": 2000
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "mdb_offline_mode_enable_lag must be greater than mdb_offline_mode_disable_lag"
    }
    """

  Scenario: Cluster modification with empty mdb_offline_mode_enable_lag fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    # in this test mdb_offline_mode_enable_lag = 86400
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "mdbOfflineModeDisableLag": 86400
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "mdb_offline_mode_enable_lag must be greater than mdb_offline_mode_disable_lag"
    }
    """


    Scenario: Cluster modification with correct mdb_offline_mode_enable_lag is ok
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "mdbOfflineModeDisableLag": 300
            }
        }
    }
    """
    Then we get response with status 200


  Scenario: Cluster modification with same lowerCaseTableNames succeeds
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "lowerCaseTableNames": 0
            }
        }
    }
    """
    Then we get response with status 200


  Scenario: Cluster modification with changed lowerCaseTableNames fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "lowerCaseTableNames": 1
            }
        }
    }
    """
    Then we get response with status 422


    Scenario: Cluster modification with same innodb_page_size succeeds
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "innodbPageSize": 16384
            }
        }
    }
    """
    Then we get response with status 200


    Scenario: Cluster modification with changed innodb_page_size fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "innodbPageSize": 32768
            }
        }
    }
    """
    Then we get response with status 422


  Scenario: Cluster modification with empty sql_mode sets it empty
    """
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mysqlConfig_5_7": {
                "sqlMode": []
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
                "enabled": false,
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
                    "sqlMode": [],
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
                "userConfig": {
                    "sqlMode": []
                }
            },
            "soxAudit": false,
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
