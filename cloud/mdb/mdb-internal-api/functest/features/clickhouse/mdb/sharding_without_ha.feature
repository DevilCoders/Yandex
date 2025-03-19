Feature: Management of Sharded ClickHouse Cluster without HA

  Background:
    Given default headers
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
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

  @shards @create
  Scenario: Shard creation with multiple hosts in non-HA configuration fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            },
            {
                "type": "CLICKHOUSE",
                "zoneId": "myt"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Shard cannot have more than 1 host in non-HA cluster configuration"
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2",
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            }
        ]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "shard cannot have more than 1 host in non-HA cluster configuration"

  @shards @create
  Scenario: Shard creation with invalid disk type fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "no-disk-type",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "diskTypeId 'no-disk-type' is not valid"
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "no-disk-type",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }
        ]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "disk type "no-disk-type" is not valid"

  @shards @create
  Scenario: Shard creation with "_" in name fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard_2",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "no-disk-type",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nshardName: Shard name 'shard_2' does not conform to naming rules"
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard_2",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "no-disk-type",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }
        ]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "shard name "shard_2" has invalid symbols"

  @shards @create
  Scenario: Shard creation with explicit parameters works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.2",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }
        ],
        "copySchema": true
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add shard to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.AddClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard2"
        }
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.2",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }
        ]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add shard to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.AddClusterShardMetadata",
            "cluster_id": "cid1",
            "shard_name": "shard2"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.clickhouse.v1.Shard",
        "clusterId": "cid1",
        "name": "shard2"
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/shards"
    Then we get response with status 200 and body contains
    """
    {
        "shards": [
            {
                "clusterId": "cid1",
                "name": "shard1",
                "config": {
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
                        },
                        "weight": 100
                    }
                }
            }, {
                "clusterId": "cid1",
                "name": "shard2",
                "config": {
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
                            "resourcePresetId": "s1.porto.2",
                            "diskTypeId": "local-ssd",
                            "diskSize": 10737418240
                        },
                        "weight": 100
                    }
                }
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
        "ssdSpaceUsed": 21474836480
    }
    """

  @shards @create
  Scenario: Shard creation with default disk parameters works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.2"
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add shard to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.AddClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard2"
        }
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.2"
                }
            }
        }
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.clickhouse.v1.Shard",
        "clusterId": "cid1",
        "name": "shard2"
    }
    """
    And body at path "$.response.config.clickhouse" contains
    """
    {
        "resources": {
            "diskSize": 10737418240,
            "diskTypeId": "local-ssd",
            "resourcePresetId": "s1.porto.2"
        }
    }
    """

  @shards @create
  Scenario: Default resources and host parameters in shard creation are taken from the oldest shard
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.2",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "man"
            }
        ]
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.2",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "man"
            }
        ]
    }
    """
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard3"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add shard to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.AddClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard3"
        }
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard3"
    }
    """
    Then we get gRPC response OK
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/shards"
    Then we get response with status 200 and body contains
    """
    {
        "shards": [
            {
                "clusterId": "cid1",
                "name": "shard1",
                "config": {
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
                        },
                        "weight": 100
                    }
                }
            }, {
                "clusterId": "cid1",
                "name": "shard2",
                "config": {
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
                            "resourcePresetId": "s1.porto.2"
                        },
                        "weight": 100
                    }
                }
            }, {
                "clusterId": "cid1",
                "name": "shard3",
                "config": {
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
                        },
                        "weight": 100
                    }
                }
            }
        ]
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "clusterId": "cid1",
                "name": "man-1.db.yandex.net",
                "zoneId": "man",
                "shardName": "shard2",
                "type": "CLICKHOUSE",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.2"
                },
                "assignPublicIp": false,
                "subnetId": "",
                "health": "UNKNOWN",
                "services": []
            },
            {
                "clusterId": "cid1",
                "name": "myt-1.db.yandex.net",
                "zoneId": "myt",
                "shardName": "shard1",
                "type": "CLICKHOUSE",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "assignPublicIp": false,
                "subnetId": "",
                "health": "UNKNOWN",
                "services": []
            },
            {
                "clusterId": "cid1",
                "name": "myt-2.db.yandex.net",
                "zoneId": "myt",
                "shardName": "shard3",
                "type": "CLICKHOUSE",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "assignPublicIp": false,
                "subnetId": "",
                "health": "UNKNOWN",
                "services": []
            }
        ]
    }
    """

  @shards @create
  Scenario: Attempt to exceed shard count limit fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2"
    }
    """
    Then we get response with status 200
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard3"
    }
    """
    Then we get response with status 200
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard3"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id3" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard4"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Cluster can have up to a maximum of 3 ClickHouse shards"
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard4"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "cluster can have up to a maximum of 3 ClickHouse shards"

  @shards @create
  Scenario: Attempt to exceed shard count limit with enabled MDB_CLICKHOUSE_UNLIMITED_SHARD_COUNT succeeds
    Given feature flags
    """
    ["MDB_CLICKHOUSE_UNLIMITED_SHARD_COUNT"]
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2"
    }
    """
    Then we get response with status 200
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard3"
    }
    """
    Then we get response with status 200
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard3"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id3" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard4"
    }
    """
    Then we get response with status 200
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard4"
    }
    """
    Then we get gRPC response OK

  @shards @create
  Scenario: Attempt to exceed min host count limit while adding new shard fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "local-nvme",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "ClickHouse shard with resource preset 's1.compute.1' and disk type 'local-nvme' requires at least 2 hosts"
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "local-nvme",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }
        ]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "subcluster ClickHouse with resource preset "s1.compute.1" and disk type "local-nvme" requires at least 2 hosts"

  @hosts @create
  Scenario: Adding host to sharded cluster without ZooKeeper fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }
        ]
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2",
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When we run query
    """
    DELETE FROM dbaas.worker_queue
    """
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "man",
            "shardName": "shard1"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Shard cannot have more than 1 host in non-HA cluster configuration"
    }
    """
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "man",
            "shard_name": "shard1"
        }],
        "copy_schema": true
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "shard cannot have more than 1 host in non-HA cluster configuration"

  @hosts @delete
  Scenario: Deleting the last host in shard fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 200
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2",
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When we run query
    """
    DELETE FROM dbaas.worker_queue
    """
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Last host in shard cannot be removed"
    }
    """
    When we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["sas-1.db.yandex.net"]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "last host in shard cannot be removed"

