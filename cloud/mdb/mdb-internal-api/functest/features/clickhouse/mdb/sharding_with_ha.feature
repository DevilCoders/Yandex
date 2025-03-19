Feature: Management of Sharded ClickHouse Cluster with HA

  Background:
    Given default headers
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
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
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
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
        }, {
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "iva"
        }],
        "description": "test cluster",
        "network_id": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker

  @shards @get
  Scenario: Shard get works
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards/shard1"
    Then we get response with status 200 and body contains
    """
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
    }
    """
    When we "GetShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "cluster_id": "cid1",
        "name": "shard1",
        "config": {
            "clickhouse": {
                "config": {
                    "default_config": {
                        "builtin_dictionaries_reload_interval": "3600",
                        "keep_alive_timeout": "3",
                        "log_level": "INFORMATION",
                        "mark_cache_size": "5368709120",
                        "max_concurrent_queries": "500",
                        "max_connections": "4096",
                        "max_table_size_to_drop": "53687091200",
                        "max_partition_size_to_drop": "53687091200",
                        "timezone": "Europe/Moscow",
                        "merge_tree": {
                            "replicated_deduplication_window": "100",
                            "replicated_deduplication_window_seconds": "604800"
                        },
                        "kafka": {},
                        "rabbitmq": {},
                        "uncompressed_cache_size": "8589934592"
                    },
                    "effective_config": {
                        "builtin_dictionaries_reload_interval": "3600",
                        "keep_alive_timeout": "3",
                        "log_level": "INFORMATION",
                        "mark_cache_size": "5368709120",
                        "max_concurrent_queries": "500",
                        "max_connections": "4096",
                        "max_table_size_to_drop": "53687091200",
                        "max_partition_size_to_drop": "53687091200",
                        "timezone": "Europe/Moscow",
                        "merge_tree": {
                            "enable_mixed_granularity_parts": true,
                            "replicated_deduplication_window": "100",
                            "replicated_deduplication_window_seconds": "604800"
                        },
                        "kafka": {},
                        "rabbitmq": {},
                        "uncompressed_cache_size": "8589934592"
                    },
                    "user_config": {
                        "merge_tree": {
                            "enable_mixed_granularity_parts": true
                        },
                        "kafka": {},
                        "rabbitmq": {}
                    }
                },
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "weight": "100"
            }
        }
    }
    """

  @shards @list
  Scenario: Shard list works
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards"
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
            }
        ]
    }
    """
    When we "ListShards" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "shards": [{
            "cluster_id": "cid1",
            "name": "shard1",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "effective_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {}
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    },
                    "weight": "100"
                }
            }
        }]
    }
    """

  @shards @create
  Scenario: Shard creation with single host in HA configuration works
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
    Given for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.AddClusterShard" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "shard_name": "shard2"
        }
    }
    """
    Given "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_at": "**IGNORE**",
        "created_by": "user",
        "description": "Add shard to ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.AddClusterShardMetadata",
            "cluster_id": "cid1",
            "shard_name": "shard2"
        },
        "modified_at": "**IGNORE**",
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Shard",
            "cluster_id": "cid1",
            "name": "shard2",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": "**IGNORE**",
                        "user_config": "**IGNORE**",
                        "effective_config": "**IGNORE**"
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    },
                    "weight": "100"
                }
            }
        }
    }
    """

  @shards @create
  Scenario: Shard creation with dedicated zookeeper hosts fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
            }, {
                "type": "ZOOKEEPER",
                "zoneId": "iva"
            }, {
                "type": "ZOOKEEPER",
                "zoneId": "man"
            }, {
                "type": "ZOOKEEPER",
                "zoneId": "myt"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Shard cannot have dedicated ZooKeeper hosts"
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
                "zone_id": "vla"
            }, {
                "type": "ZOOKEEPER",
                "zone_id": "iva"
            }, {
                "type": "ZOOKEEPER",
                "zone_id": "man"
            }, {
                "type": "ZOOKEEPER",
                "zone_id": "myt"
            }
        ]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "shard cannot have dedicated ZooKeeper hosts"

  @shards @create
  Scenario: Shard creation with name conflict fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard1",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Shard 'shard1' already exists"
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard1",
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            },
            {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
            }
        ]
    }
    """
    Then we get gRPC response error with code ALREADY_EXISTS and message "cannot create shard "shard1": shard with name "shard1" already exists"

  @shards @create
  Scenario: Creating a shard with a name that differs only by case from the existing fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "Shard1",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Cannot create shard 'Shard1': shard with name 'shard1' already exists"
    }
    """
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "Shard1",
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            },
            {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
            }
        ]
    }
    """
    Then we get gRPC response error with code ALREADY_EXISTS and message "cannot create shard "Shard1": shard with name "shard1" already exists"

  @shards @create
  Scenario: Shard creation works
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
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
            }
        ]
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
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
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
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards"
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
            }
        ]
    }
    """
    When we "ListShards" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "shards": [{
            "cluster_id": "cid1",
            "name": "shard1",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "effective_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {}
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    },
                    "weight": "100"
                }
            }
        }, {
            "cluster_id": "cid1",
            "name": "shard2",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "effective_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {}
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.2"
                    },
                    "weight": "100"
                }
            }
        }]
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "cid1",
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
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                }
            },
            "cloudStorage": {
                "enabled": false
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
        "cpuUsed": 9.0,
        "memoryUsed": 38654705664,
        "ssdSpaceUsed": 75161927680
    }
    """

  @shards @create
  Scenario: Setting shard config works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    # language=json
    """
    {
        "shardName": "shard2",
        "configSpec": {
            "clickhouse": {
                "config": {
                    "mergeTree": {
                        "replicatedDeduplicationWindow": 100,
                        "replicatedDeduplicationWindowSeconds": 604800
                    },
                    "geobaseUri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                    "kafka": {
                        "securityProtocol": "SECURITY_PROTOCOL_SSL",
                        "saslMechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                        "saslUsername": "kafka_username",
                        "saslPassword": "kafka_pass"
                    },
                    "kafkaTopics": [{
                        "name": "kafka_topic",
                        "settings": {
                            "securityProtocol": "SECURITY_PROTOCOL_SSL",
                            "saslMechanism": "SASL_MECHANISM_GSSAPI",
                            "saslUsername": "topic_username",
                            "saslPassword": "topic_pass"
                        }
                    }],
                    "rabbitmq": {
                        "username": "test_user",
                        "password": "test_password"
                    },
                    "builtinDictionariesReloadInterval": 3600,
                    "keepAliveTimeout": 3,
                    "logLevel": "INFORMATION",
                    "markCacheSize": 5368709120,
                    "maxConcurrentQueries": 500,
                    "maxConnections": 4096,
                    "maxPartitionSizeToDrop": 53687091200,
                    "maxTableSizeToDrop": 53687091200,
                    "timezone": "Europe/Moscow",
                    "uncompressedCacheSize": 8589934592
                },
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
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 200
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2",
        "config_spec": {
            "clickhouse": {
                "config": {
                    "merge_tree": {
                        "replicated_deduplication_window": "100",
                        "replicated_deduplication_window_seconds": "604800"
                    },
                    "geobase_uri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                    "kafka": {
                        "security_protocol": "SECURITY_PROTOCOL_SSL",
                        "sasl_mechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                        "sasl_username": "kafka_username",
                        "sasl_password": "kafka_pass"
                    },
                    "kafka_topics": [{
                        "name": "kafka_topic",
                        "settings": {
                            "security_protocol": "SECURITY_PROTOCOL_SSL",
                            "sasl_mechanism": "SASL_MECHANISM_GSSAPI",
                            "sasl_username": "topic_username",
                            "sasl_password": "topic_pass"
                        }
                    }],
                    "rabbitmq": {
                        "username": "test_user",
                        "password": "test_password"
                    },
                    "builtin_dictionaries_reload_interval": "3600",
                    "keep_alive_timeout": "3",
                    "log_level": "INFORMATION",
                    "mark_cache_size": "5368709120",
                    "max_concurrent_queries": "500",
                    "max_connections": "4096",
                    "max_partition_size_to_drop": "53687091200",
                    "max_table_size_to_drop": "53687091200",
                    "timezone": "Europe/Moscow",
                    "uncompressed_cache_size": "8589934592"
                },
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
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
            }
        ]
    }
    """
    Then we get gRPC response OK
    Given "worker_task_id2" acquired and finished by worker
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards"
    Then we get response with status 200 and body contains
    # language=json
    """
    {
        "shards": [
            {
                "clusterId": "cid1",
                "name": "shard1",
                "config": {
                    "clickhouse": {
                        "config": {
                            "defaultConfig": "**IGNORE**",
                            "effectiveConfig": "**IGNORE**",
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
                            "defaultConfig": "**IGNORE**",
                            "effectiveConfig": "**IGNORE**",
                            "userConfig": {
                                "mergeTree": {
                                    "replicatedDeduplicationWindow": 100,
                                    "replicatedDeduplicationWindowSeconds": 604800
                                },
                                "geobaseUri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                                "kafka": {
                                    "securityProtocol": "SECURITY_PROTOCOL_SSL",
                                    "saslMechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                                    "saslUsername": "kafka_username"
                                },
                                "kafkaTopics": [{
                                    "name": "kafka_topic",
                                    "settings": {
                                        "securityProtocol": "SECURITY_PROTOCOL_SSL",
                                        "saslMechanism": "SASL_MECHANISM_GSSAPI",
                                        "saslUsername": "topic_username"
                                    }
                                }],
                                "rabbitmq": {
                                    "username": "test_user"
                                },
                                "builtinDictionariesReloadInterval": 3600,
                                "keepAliveTimeout": 3,
                                "logLevel": "INFORMATION",
                                "markCacheSize": 5368709120,
                                "maxConcurrentQueries": 500,
                                "maxConnections": 4096,
                                "maxPartitionSizeToDrop": 53687091200,
                                "maxTableSizeToDrop": 53687091200,
                                "timezone": "Europe/Moscow",
                                "uncompressedCacheSize": 8589934592
                            }
                        },
                        "resources": "**IGNORE**",
                        "weight": "**IGNORE**"
                    }
                }
            }
        ]
    }
    """
    When we "ListShards" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    # language=json
    """
    {
        "shards": [{
            "cluster_id": "cid1",
            "name": "shard1",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": "**IGNORE**",
                        "effective_config": "**IGNORE**",
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {}
                        }
                    },
                    "resources": "**IGNORE**",
                    "weight": "**IGNORE**"
                }
            }
        }, {
            "cluster_id": "cid1",
            "name": "shard2",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": "**IGNORE**",
                        "effective_config": "**IGNORE**",
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "geobase_uri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                            "kafka": {
                                "security_protocol": "SECURITY_PROTOCOL_SSL",
                                "sasl_mechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                                "sasl_username": "kafka_username"
                            },
                            "kafka_topics": [{
                                "name": "kafka_topic",
                                "settings": {
                                    "security_protocol": "SECURITY_PROTOCOL_SSL",
                                    "sasl_mechanism": "SASL_MECHANISM_GSSAPI",
                                    "sasl_username": "topic_username"
                                }
                            }],
                            "rabbitmq": {
                                "username": "test_user"
                            },
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_partition_size_to_drop": "53687091200",
                            "max_table_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "uncompressed_cache_size": "8589934592"
                        }
                    },
                    "resources": "**IGNORE**",
                    "weight": "**IGNORE**"
                }
            }
        }]
    }
    """

  @shards @update @events
  Scenario: Setting max connections for shard works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
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
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1/shards/shard1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "maxConnections": 30
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse shard",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard1"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.UpdateClusterShard" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "shard_name": "shard1"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id3"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.clickhouse.v1.Shard",
        "clusterId": "cid1",
        "name": "shard1"
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards"
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
                                "maxConnections": 30,
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
                                "mergeTree": {},
                                "maxConnections": 30
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
                            "resourcePresetId": "s1.porto.1"
                        },
                        "weight": 100
                    }
                }
            }
        ]
    }
    """
    When we "ListShards" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "shards": [{
            "cluster_id": "cid1",
            "name": "shard1",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "effective_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "30",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "max_connections": "30"
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    },
                    "weight": "100"
                }
            }
        }, {
            "cluster_id": "cid1",
            "name": "shard2",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "effective_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {}
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    },
                    "weight": "100"
                }
            }
        }]
    }
    """

  @shards @update
  Scenario: Scaling shard up works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
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
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1/shards/shard1" with data
    """
    {
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
        "description": "Modify ClickHouse shard",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard1"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards"
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
                            "resourcePresetId": "s1.porto.2"
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
                            "resourcePresetId": "s1.porto.1"
                        },
                        "weight": 100
                    }
                }
            }
        ]
    }
    """
    When we "ListShards" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "shards": [{
            "cluster_id": "cid1",
            "name": "shard1",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "effective_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {}
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.2"
                    },
                    "weight": "100"
                }
            }
        }, {
            "cluster_id": "cid1",
            "name": "shard2",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "effective_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {}
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    },
                    "weight": "100"
                }
            }
        }]
    }
    """

  @shards @update
  Scenario: Scaling shard down works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.3",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
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
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.3",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1/shards/shard2" with data
    """
    {
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
        "description": "Modify ClickHouse shard",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard2"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards"
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
            }
        ]
    }
    """
    When we "ListShards" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "shards": [{
            "cluster_id": "cid1",
            "name": "shard1",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "effective_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {}
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    },
                    "weight": "100"
                }
            }
        }, {
            "cluster_id": "cid1",
            "name": "shard2",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "effective_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {}
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.2"
                    },
                    "weight": "100"
                }
            }
        }]
    }
    """

  @clusters @update
  Scenario: Scaling shards simultaneously up and down fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.3",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
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
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.3",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.2"
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Upscale and downscale of shards cannot be mixed in one operation."
    }
    """

  @shards @update
  Scenario: Setting shard weight works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
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
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1/shards/shard1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "weight": 200
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse shard",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard1"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards"
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
                        "weight": 200
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
                            "resourcePresetId": "s1.porto.1"
                        },
                        "weight": 100
                    }
                }
            }
        ]
    }
    """
    When we "ListShards" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "shards": [{
            "cluster_id": "cid1",
            "name": "shard1",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "effective_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {}
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    },
                    "weight": "200"
                }
            }
        }, {
            "cluster_id": "cid1",
            "name": "shard2",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "effective_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {}
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    },
                    "weight": "100"
                }
            }
        }]
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse.shards" contains
    """
    {
        "shard_id1": {
            "weight": 200
        }
    }
    """

  @shards @delete
  Scenario: Nonexistent shard deletion fails
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards/shard2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Shard 'shard2' does not exist"
    }
    """
    When we "DeleteShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "shard "shard2" does not exist"

  @shards @delete
  Scenario: Last shard deletion fails
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards/shard1"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Last shard in cluster cannot be removed"
    }
    """
    When we "DeleteShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "last shard in cluster cannot be removed"

  @shards @create
  Scenario: Attempt to exceed max host count limit while adding new shard fails
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
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
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
        "message": "ClickHouse shard with resource preset 's1.porto.2' and disk type 'local-ssd' allows at most 7 hosts"
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
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }
        ]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "subcluster ClickHouse with resource preset "s1.porto.2" and disk type "local-ssd" allows at most 7 hosts"

  @hosts @create
  Scenario: Max host count limit is checked against shard host count and not cluster host count (in add host)
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
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
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard3",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
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
        "shard_name": "shard3",
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id3" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "shardName": "shard3",
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }]
    }
    """
    Then we get response with status 200
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "sas",
            "shard_name": "shard3"
        }]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add hosts to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.AddClusterHostsMetadata",
            "cluster_id": "cid1",
            "host_names": ["sas-10.db.yandex.net"]
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """

  @hosts @delete
  Scenario: Max host count limit is checked against shard host count and not cluster host count (in delete host)
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
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
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard3",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
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
        "shard_name": "shard3",
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id3" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-8.db.yandex.net"]
    }
    """
    Then we get response with status 200
    When we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["sas-8.db.yandex.net"]
    }
    """
    Then we get gRPC response OK

  @shards @delete @events
  Scenario: Shard deletion works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
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
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards/shard2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete shard from ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.DeleteClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard2"
        }
    }
    """
    When we "DeleteShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2"
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.DeleteClusterShard" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "shard_name": "shard2"
        }
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 5.0,
        "memoryUsed": 21474836480,
        "ssdSpaceUsed": 53687091200
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards"
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
            }
        ]
    }
    """
    When we "ListShards" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "shards": [{
            "cluster_id": "cid1",
            "name": "shard1",
            "config": {
                "clickhouse": {
                    "config": {
                        "default_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "effective_config": {
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_table_size_to_drop": "53687091200",
                            "max_partition_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "uncompressed_cache_size": "8589934592"
                        },
                        "user_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true
                            },
                            "kafka": {},
                            "rabbitmq": {}
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    },
                    "weight": "100"
                }
            }
        }]
    }
    """

  @shards @delete
  Scenario: Deleting host in 2-hosts HA-shard works only with feature flag
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard4",
        "hostSpecs": [
            {
                "type": "CLICKHOUSE",
                "zoneId": "man"
            }, {
                "type": "CLICKHOUSE",
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 200
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard4",
        "host_specs": [
            {
                "type": "CLICKHOUSE",
                "zone_id": "man"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "vla"
            }
        ]
    }
    """
    Then we get gRPC response OK
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["man-2.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Shard cannot have less than 2 ClickHouse hosts in HA cluster configuration"
    }
    """
    When we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["man-2.db.yandex.net"]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "shard cannot have less than 2 ClickHouse hosts in HA cluster configuration"
    Given feature flags
    """
    ["MDB_CLICKHOUSE_DISABLE_CLUSTER_CONFIGURATION_CHECKS"]
    """
    Given "worker_task_id2" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["man-2.db.yandex.net"]
    }
    """
    Then we get response with status 200
    When we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["man-2.db.yandex.net"]
    }
    """
    Then we get gRPC response OK

