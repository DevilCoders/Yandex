Feature: Create/Modify Porto Non-HA ClickHouse Cluster

  Background:
    Given default headers
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
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "clickhouse",
                       "status": "Alive"
                   }
               ]
           }
       ]
    }
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
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
        }],
        "description": "test cluster"
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
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
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
        "description": "test cluster"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker

  Scenario: Cluster creation works
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.3",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
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
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
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
                "enabled": false
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
        "cpuUsed": 1.0,
        "memoryUsed": 4294967296,
        "ssdSpaceUsed": 10737418240
    }
    """

  Scenario: Adding host to cluster without ZooKeeper fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Cluster cannot have more than 1 host in non-HA configuration"
    }
    """
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "man"
        }]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "cluster cannot have more than 1 host in non-HA configuration"

  Scenario: Deleting the last host in cluster fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["myt-1.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Last host in cluster cannot be removed"
    }
    """
    When we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["myt-1.db.yandex.net"]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "last host in cluster cannot be removed"

  Scenario: Add ZooKeeper with single host fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Cluster should have 3 ZooKeeper hosts"
    }
    """
    When we "AddZookeeper" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "shard ZooKeeper with resource preset "s2.porto.1" and disk type "local-ssd" requires at least 3 hosts"

  Scenario: Add ZooKeeper with invalid subnet fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva",
            "subnetId": "nosubnet"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Unable to find subnet with id 'nosubnet' in zone 'iva'"
    }
    """
    When we "AddZookeeper" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "sas",
            "subnet_id": "nosubnet"
        }]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "subnet "nosubnet" not found"

  @events
  Scenario: Add ZooKeeper with default resources works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add ZooKeeper to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.AddClusterZookeeperMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "AddZookeeper" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "iva"
        }]
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.AddClusterZookeeper" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.clickhouse.v1.Cluster",
        "id": "cid1"
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "clusterId": "cid1",
                "name": "iva-1.db.yandex.net",
                "assignPublicIp": false,
                "subnetId": "",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "health": "UNKNOWN",
                "services": [],
                "shardName": null,
                "type": "ZOOKEEPER",
                "zoneId": "iva"
            },
            {
                "clusterId": "cid1",
                "name": "man-1.db.yandex.net",
                "assignPublicIp": false,
                "subnetId": "",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "health": "UNKNOWN",
                "services": [],
                "shardName": null,
                "type": "ZOOKEEPER",
                "zoneId": "man"
            },
            {
                "clusterId": "cid1",
                "name": "myt-1.db.yandex.net",
                "assignPublicIp": false,
                "subnetId": "",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "health": "ALIVE",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shardName": "shard1",
                "type": "CLICKHOUSE",
                "zoneId": "myt"
            },
            {
                "clusterId": "cid1",
                "name": "vla-1.db.yandex.net",
                "assignPublicIp": false,
                "subnetId": "",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "health": "UNKNOWN",
                "services": [],
                "shardName": null,
                "type": "ZOOKEEPER",
                "zoneId": "vla"
            }
        ]
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "cluster_id": "cid1",
                "name": "iva-1.db.yandex.net",
                "assign_public_ip": false,
                "subnetId": "",
                "resources": {
                    "disk_size": 10737418240,
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "health": "UNKNOWN",
                "services": [],
                "shard_name": null,
                "type": "ZOOKEEPER",
                "zoneId": "iva",
                "system": null
            },
            {
                "cluster_id": "cid1",
                "name": "man-1.db.yandex.net",
                "assign_public_ip": false,
                "subnetId": "",
                "resources": {
                    "disk_size": 10737418240,
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "health": "UNKNOWN",
                "services": [],
                "shard_name": null,
                "type": "ZOOKEEPER",
                "zone_id": "man",
                "system": null
            },
            {
                "cluster_id": "cid1",
                "name": "myt-1.db.yandex.net",
                "assign_public_ip": false,
                "subnet_id": "",
                "resources": {
                    "disk_size": 10737418240,
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "health": "ALIVE",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shard_name": "shard1",
                "type": "CLICKHOUSE",
                "zone_id": "myt",
                "system": null
            },
            {
                "cluster_id": "cid1",
                "name": "vla-1.db.yandex.net",
                "assign_public_ip": false,
                "subnet_id": "",
                "resources": {
                    "disk_size": 10737418240,
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "health": "UNKNOWN",
                "services": [],
                "shard_name": null,
                "type": "ZOOKEEPER",
                "zone_id": "vla",
                "system": null
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
        "cpuUsed": 4.0,
        "memoryUsed": 17179869184,
        "ssdSpaceUsed": 42949672960
    }
    """

  Scenario: Add ZooKeeper with explicit resources works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "resources": {
            "resourcePresetId": "s1.porto.2",
            "diskTypeId": "local-ssd",
            "diskSize": 21474836480
        },
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add ZooKeeper to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.AddClusterZookeeperMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "AddZookeeper" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "resources": {
            "resource_preset_id": "s1.porto.2",
            "disk_type_id": "local-ssd",
            "disk_size": 21474836480
        },
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "iva"
        }]
    }
    """
    Then we get gRPC response OK
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.zookeeper" contains
    """
    {
        "resources": {
            "resourcePresetId": "s1.porto.2",
            "diskTypeId": "local-ssd",
            "diskSize": 21474836480
        }
    }
    """

  Scenario: Add ZooKeeper with default disk parameters works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "resources": {
            "resourcePresetId": "s1.porto.2"
        },
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add ZooKeeper to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.AddClusterZookeeperMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "AddZookeeper" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "resources": {
            "resource_preset_id": "s1.porto.2"
        },
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "iva"
        }]
    }
    """
    Then we get gRPC response OK
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.zookeeper" contains
    """
    {
        "resources": {
            "resourcePresetId": "s1.porto.2",
            "diskTypeId": "local-ssd",
            "diskSize": 10737418240
        }
    }
    """

  Scenario: Downgrading ClickHouse to allowed version works
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "21.2"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config" contains
    """
    {
        "version": "21.2"
    }
    """

  Scenario: Downgrading ClickHouse to not allowed version fails
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "21.1"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Can't create cluster, version '21.1' is deprecated"
    }
    """
