Feature: Create/Modify Compute MySQL Cluster

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
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "network-ssd",
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
            "zoneId": "vla"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "network1",
        "securityGroupIds": ["sg_id1", "sg_id2"],
        "deletionProtection": false
    }
    """
    And "worker_task_id1" acquired and finished by worker
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" containing:
      |sg_id1|
      |sg_id2|
    And worker set "sg_id1,sg_id2" security groups on "cid1"
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """


  @events
  Scenario: Cluster creation works
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
                "userConfig": {}
            },
            "soxAudit": false,
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "network-ssd",
                "resourcePresetId": "s1.compute.1"
            }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "id": "cid1",
        "labels": {},
        "name": "test",
        "networkId": "network1",
        "status": "RUNNING"
    }
    """
    And for "worker_task_id1" exists "yandex.cloud.events.mdb.mysql.CreateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "folder_id": "folder1",
            "name": "test",
            "environment": "PRESTABLE",
            "config_spec": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            },
            "database_specs": [{
                "name": "testdb"
            }],
            "user_specs": [{
                "name": "test"
            }],
            "host_specs": [{
                "zone_id": "myt"
            }, {
                "zone_id": "vla"
            }, {
                "zone_id": "sas"
            }],
            "network_id": "network1",
            "security_group_ids": ["sg_id1", "sg_id2"],
            "deletion_protection": false
        }
    }
    """
    And event body for event "worker_task_id1" does not contain following paths
      | $.request_parameters.user_spec.0.password  |
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

  Scenario: Billing cost estimation works
    When we POST "/mdb/mysql/1.0/console/clusters:estimate?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "local-nvme",
                "diskSize": 107374182400
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
            "zoneId": "vla"
        }, {
            "assignPublicIp": true,
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "mysql_cluster",
                    "disk_size": 107374182400,
                    "disk_type_id": "local-nvme",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "mysql_cluster"
                    ],
                    "software_accelerated_network_cores": 0
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "mysql_cluster",
                    "disk_size": 107374182400,
                    "disk_type_id": "local-nvme",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "mysql_cluster"
                    ],
                    "software_accelerated_network_cores": 0
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "mysql_cluster",
                    "disk_size": 107374182400,
                    "disk_type_id": "local-nvme",
                    "online": 1,
                    "public_ip": 1,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "mysql_cluster"
                    ],
                    "software_accelerated_network_cores": 0
                }
            }
        ]
    }
    """

  Scenario: Billing cost estimation works for dedicated host
    When we POST "/mdb/mysql/1.0/console/clusters:estimate?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "local-nvme",
                "diskSize": 107374182400
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostGroupIds": [
            "hg4"
        ],
        "hostSpecs": [{
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "mysql_cluster",
                    "disk_size": 107374182400,
                    "disk_type_id": "local-nvme",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 1,
                    "roles": [
                        "mysql_cluster"
                    ],
                    "software_accelerated_network_cores": 0
                }
            }
        ]
    }
    """

  Scenario: Modify to burstable on multi-host cluster fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "b2.compute.3"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "MySQL cluster with resource preset 'b2.compute.3' and disk type 'network-ssd' allows at most 1 host"
    }
    """

  @events
  Scenario: Resources modify on network-ssd disk works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.compute.2"
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
            "config_spec": {
                "resources": {
                    "resource_preset_id": "s1.compute.2"
                }
            }
        }
    }
    """

  Scenario: Disk scale up on network-ssd disk works
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

  Scenario: Disk scale down on network-ssd disk without feature flag fails
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
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 10737418240
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Requested feature is not available"
    }
    """

  Scenario: Disk scale down on network-ssd disk with feature flag works
    Given feature flags
    """
    ["MDB_NETWORK_DISK_TRUNCATE"]
    """
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
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 10737418240
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

  Scenario: Resources modify on local-nvme disk without feature flag fails
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "local-nvme",
                "diskSize": 107374182400
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
            "zoneId": "vla"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    And we PATCH "/mdb/mysql/1.0/clusters/cid2" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.compute.2"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Requested feature is not available"
    }
    """

  @stop @events
  Scenario: Stop cluster works
    When we POST "/mdb/mysql/1.0/clusters/cid1:stop"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Stop MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.StopClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.StopCluster" event with
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
    When we GET "/mdb/1.0/operations/worker_task_id2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Stop MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.StopClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STOPPING"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STOPPED"
    }
    """

  @stop
  Scenario: Modifying stopped cluster fails
    When we POST "/mdb/mysql/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  @stop
  Scenario: Labels are no longer metadata-only operations
    When we POST "/mdb/mysql/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "labels": {
            "acid": "yes"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  @stop
  Scenario: Stop stopped cluster fails
    When we POST "/mdb/mysql/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mysql/1.0/clusters/cid1:stop"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  @stop
  Scenario: Delete stopped cluster works
    When we POST "/mdb/mysql/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we DELETE "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200

  @start @events
  Scenario: Start stopped cluster works
    When we POST "/mdb/mysql/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mysql/1.0/clusters/cid1:start"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start MySQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.StartClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.mysql.StartCluster" event with
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
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STARTING"
    }
    """
    When we GET "/mdb/1.0/operations/worker_task_id3"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start MySQL cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.StartClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """

  @start
  Scenario: Start running cluster fails
    When we POST "/mdb/mysql/1.0/clusters/cid1:start"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  @security_groups
  Scenario: Removing security groups works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": []
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" is empty list
    And "worker_task_id2" acquired and finished by worker
    And worker clear security groups for "cid1"
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": null
    }
    """

  @security_groups
  Scenario: Modify security groups works
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": ["sg_id3", "sg_id2", "sg_id3"]
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
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" containing:
      |sg_id2|
      |sg_id3|
    And worker clear security groups for "cid1"
    And worker set "sg_id2,sg_id3" security groups on "cid1"
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": ["sg_id2", "sg_id3"]
    }
    """

  @security_groups
  Scenario: Modify security groups with same groups show no changes
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": ["sg_id1", "sg_id2", "sg_id1", "sg_id2"]
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """

  @security_groups
  Scenario: Modify security groups with more than limit show error
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": ["sg_id1", "sg_id2", "sg_id3", "sg_id4", "sg_id5", "sg_id6", "sg_id7", "sg_id8", "sg_id9", "sg_id10", "sg_id11"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "too many security groups (10 is the maximum)"
    }
    """

  @assign_public_ip
  Scenario: Modify assign_public_ip works
    When we POST "/mdb/mysql/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
          "hostName": "sas-1.df.cloud.yandex.net",
          "assignPublicIp": true
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
                "sas-1.df.cloud.yandex.net"
            ]
        }
    }
    """
    And we GET "/mdb/mysql/1.0/clusters/cid1/hosts"
    And body at path "$.hosts[1]" contains
    """
    {
      "name": "sas-1.df.cloud.yandex.net",
      "assignPublicIp": true
    }
    """
    And "worker_task_id2" acquired and finished by worker
