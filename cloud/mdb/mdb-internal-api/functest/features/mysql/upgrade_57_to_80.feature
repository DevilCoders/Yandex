Feature: Upgrade Porto MySQL Cluster to 8 version


  Background:
    Given default headers
    And feature flags
    """
    ["MDB_MYSQL_8_0", "MDB_MYSQL_5_7_TO_8_0_UPGRADE"]
    """
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_5_7": {
                "sqlMode": ["NO_AUTO_CREATE_USER", "NO_ZERO_IN_DATE", "NO_ZERO_DATE"]
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
    Then we get response with status 200
    And each path on body evaluates to
      | $.config.mysqlConfig_5_7.userConfig.sqlMode | ["NO_AUTO_CREATE_USER", "NO_ZERO_IN_DATE", "NO_ZERO_DATE"] |

  @events
  Scenario: Upgrade 57 to 80 works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "8.0"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Upgrade MySQL cluster to 8.0 version",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "config_spec": {
                "version": "8.0"
            }
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
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
                    "innodbFlushLogAtTrxCommit": 1,
                    "innodbLockWaitTimeout": 50,
                    "transactionIsolation": "REPEATABLE_READ",
                    "logErrorVerbosity": 3,
                    "auditLog": false,
                    "generalLog": false,
                    "maxAllowedPacket": 16777216,
                    "longQueryTime": 0.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
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
                    "auditLog": false,
                    "generalLog": false,
                    "maxAllowedPacket": 16777216,
                    "longQueryTime": 0.0,
                    "logSlowRateLimit": 1,
                    "logSlowRateType": "SESSION",
                    "logSlowSpStatements": false,
                    "slowQueryLog": false,
                    "slowQueryLogAlwaysWriteTime": 10,
                    "maxConnections": 128,
                    "sqlMode": [
                        "NO_ZERO_IN_DATE",
                        "NO_ZERO_DATE"
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
                "userConfig": {
                    "sqlMode": [
                        "NO_ZERO_IN_DATE",
                        "NO_ZERO_DATE"
                    ]
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
            "version": "5.7"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Version downgrade detected"
    }
    """
