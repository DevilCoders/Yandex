Feature: CLickHouse cluster config validation

  Background:
    Given default headers

  Scenario: Config validation on cluster creation works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                },
                "config": {
                    "mergeTree": {
                        "numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge": 32
                    }
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Value of merge_tree.number_of_free_entries_in_pool_to_lower_max_size_of_merge (32) is greater than background_pool_size (16)"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                },
                "config": {
                    "merge_tree": {
                        "number_of_free_entries_in_pool_to_lower_max_size_of_merge": 32
                    }
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
        "description": "test cluster",
        "network_id": "network1"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "value of merge_tree.number_of_free_entries_in_pool_to_lower_max_size_of_merge (32) greater than background_pool_size (16)"

  Scenario: Config validation on cluster update works
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                },
                "config": {
                    "backgroundPoolSize": 32
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
    When "worker_task_id1" acquired and finished by worker
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "mergeTree": {
                        "numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge": 64
                    }
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Value of merge_tree.number_of_free_entries_in_pool_to_lower_max_size_of_merge (64) is greater than background_pool_size (32)"
    }
    """

  Scenario: Config validation on shard creation works
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                },
                "config": {
                    "backgroundPoolSize": 32
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
    When "worker_task_id1" acquired and finished by worker
    And we POST "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "configSpec": {
            "clickhouse": {
                "config": {
                    "mergeTree": {
                        "numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge": 64
                    }
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Value of merge_tree.number_of_free_entries_in_pool_to_lower_max_size_of_merge (64) is greater than background_pool_size (32)"
    }
    """

  Scenario: Config validation on shard update works
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                },
                "config": {
                    "backgroundPoolSize": 32
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
    When "worker_task_id1" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
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
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1/shards/shard2" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "mergeTree": {
                        "numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge": 64
                    }
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Value of merge_tree.number_of_free_entries_in_pool_to_lower_max_size_of_merge (64) is greater than background_pool_size (32)"
    }
    """
