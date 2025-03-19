Feature: Management of ClickHouse clusters with Cloud Storage

  Background:
    Given default headers
    Given we use following default resources for "clickhouse_cluster" with role "clickhouse_cluster"
    # language=json
    """
    {
        "resource_preset_id": "s1.compute.1",
        "disk_type_id": "network-ssd",
        "disk_size": 10737418240,
        "generation": 1
    }
    """

  @cloud_storage
  Scenario: Create cluster with enabled cloud storage
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "22.3",
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "cloudStorage": {
                "enabled": true
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
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            },
            "cloud_storage": {
                "enabled": true
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }],
        "description": "test cluster",
        "network_id": "network1"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "access": {
                "webSql": false,
                "dataLens": false,
                "metrika": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "version": "22.3",
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "serviceAccountId": null,
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
                    "userConfig": {
                        "mergeTree": {}
                    }
                },
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "network-ssd",
                    "resourcePresetId": "s1.compute.1"
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 0,
                    "diskTypeId": null,
                    "resourcePresetId": null
                }
            },
            "cloudStorage": {
                "enabled": true
            }
        }
    }
    """

  @cloud_storage
  Scenario: Attempt to create cluster with cloud storage and version lower 22.3 fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "cloudStorage": {
                "enabled": true
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [
        {
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Minimum required version for clusters with cloud storage is 22.3"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            },
            "cloud_storage": {
                "enabled": true
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }],
        "description": "test cluster",
        "network_id": "network1"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "minimum required version for clusters with cloud storage is 22.3"

  @cloud_storage
  Scenario: Create HA cluster with cloud storage
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "cloudStorage": {
                "enabled": true
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [
        {
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        },
        {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        },
        {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        },
        {
            "type": "ZOOKEEPER",
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            },
            "cloud_storage": {
                "enabled": true
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            },
            {
                "type": "ZOOKEEPER",
                "zone_id": "myt"
            },
            {
                "type": "ZOOKEEPER",
                "zone_id": "vla"
            },
            {
                "type": "ZOOKEEPER",
                "zone_id": "sas"
            }
        ],
        "description": "test cluster",
        "network_id": "network1"
    }
    """
    Then we get gRPC response OK

  @cloud_storage
  Scenario: Attempt to create HA cluster with cloud storage and version lower 22.3 fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "cloudStorage": {
                "enabled": true
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [
        {
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        },
        {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Minimum required version for clusters with cloud storage is 22.3"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            },
            "cloud_storage": {
                "enabled": true
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            },
            {
                "type": "ZOOKEEPER",
                "zone_id": "myt"
            },
            {
                "type": "ZOOKEEPER",
                "zone_id": "vla"
            },
            {
                "type": "ZOOKEEPER",
                "zone_id": "sas"
            }
        ],
        "description": "test cluster",
        "network_id": "network1"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "minimum required version for clusters with cloud storage is 22.3"

  @cloud_storage
  Scenario: Restore to cluster with enabled cloud storage
    Given default headers
    And s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "cloudStorage": {
                "enabled": true
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
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "description": "test cluster"
    }
    """
    Then "worker_task_id1" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "cloudStorage": {
                "enabled": true
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 200
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "cloud_storage": {
                "enabled": true
            }
        },
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }],
        "backup_id": "cid1:0"
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid2"
    Then we get response with status 200
    And body at path "$.config.cloudStorage" contains
    """
    {
        "enabled": true
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "source_cloud_storage_s3_bucket" set to "cloud-storage-cid1"

  @cloud_storage
  Scenario: Attempt to restore to cluster with enabled cloud storage and version lower 22.3 fails
    Given default headers
    And s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
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
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "description": "test cluster"
    }
    """
    Then "worker_task_id1" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When we POST "/mdb/clickhouse/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "cloudStorage": {
                "enabled": true
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Minimum required version for clusters with cloud storage is 22.3"
    }
    """

  @cloud_storage
  Scenario: Enable cloud storage
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
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
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "cloudStorage": {
                "enabled": true
            }
        }
    }
    """
    Then we get response with status 200
    And "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.cloudStorage" contains
    """
    {
        "enabled": true
    }
    """

  @cloud_storage
  Scenario: Attempt to enable cloud storage on cluster with version lower 22.3 fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
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
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "cloudStorage": {
                "enabled": true
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Minimum required version for clusters with cloud storage is 22.3"
    }
    """

  @cloud_storage
  Scenario: Attempt to disable cloud storage fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
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
                "enabled": true
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
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "cloudStorage": {
                "enabled": false
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Cloud storage cannot be turned-off after cluster creation."
    }
    """

  @cloud_storage
  Scenario: Attempt to downgrade cluster with enabled cloud storage to version lower 22.3 fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
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
                "enabled": true
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
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 200
    And "worker_task_id1" acquired and finished by worker
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "22.1"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "message": "Minimum required version for clusters with cloud storage is 22.3"
    }
    """

  @cloud_storage
  Scenario: Attempt turn off cloud storage on restore fails
    Given headers
    """
    {
        "X-YaCloud-SubjectToken": "rw-token",
        "Accept": "application/json",
        "Content-Type": "application/json",
        "Access-Id": "00000000-0000-0000-0000-000000000000",
        "Access-Secret": "dummy"
    }
    """
    And s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test_cloud_storage",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "cloudStorage": {
                "enabled": true
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
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_cloud_storage",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "cloud_storage": {
                "enabled": true
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }],
        "description": "test cluster",
        "network_id": "network1"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When we POST "/mdb/clickhouse/1.0/clusters:restore" with data
    """
    {
        "name": "test_restore",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "cloudStorage": {
                "enabled": false
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Backup with hybrid storage can not be restored to cluster without hybrid storage"
    }
    """
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "22.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "cloud_storage": {
                "enabled": false
            }
        },
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }],
        "backup_id": "cid1:0"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "Backup with hybrid storage can not be restored to cluster without hybrid storage"


