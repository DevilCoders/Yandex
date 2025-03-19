Feature: Create/Modify Porto ClickHouse Cluster with decommissioning flavor

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_ALLOW_DECOMMISSIONED_FLAVOR_USE"]
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.legacy",
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
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create ClickHouse cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
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
                    "resource_preset_id": "s1.porto.legacy",
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
    When "worker_task_id1" acquired and finished by worker
    Given feature flags
    """
    []
    """

  Scenario: Cluster with decommissioning can be changed
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.legacy"
                },
                "config": {
                    "logLevel": "TRACE"
                }
            }
        }
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1/shards/shard1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.legacy",
                    "diskSize": 21474836480
                }
            }
        }
    }
    """
    Then we get response with status 200

  Scenario: Cluster with decommissioning can be changed to non-decommissioning flavor
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """ 
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1"
                }
            }
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

  Scenario: Can not change cluster flavor to decommissioning one
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """ 
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1"
                }
            }
        }
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """ 
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.legacy"
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Resource preset 's1.porto.legacy' is decommissioned"
    }
    """

  Scenario: Can not create cluster with decommissioning flavor
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.legacy",
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Resource preset 's1.porto.legacy' is decommissioned"
    }
    """

  Scenario: Can not create shard with decommissioning flavor
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test3",
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
    And "worker_task_id2" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid2/shards" with data
    """
    {
        "shardName": "shard2",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.legacy",
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Resource preset 's1.porto.legacy' is decommissioned"
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
                    "resource_preset_id": "s1.porto.legacy",
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "resource preset "s1.porto.legacy" is decommissioned"

  @delete
  Scenario: Cluster with decommissioning flavor removal works
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.DeleteCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """

  Scenario: Adding host to cluster with decommissioning flavor works
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "man-2.db.yandex.net"
            ]
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1/hosts"
    Then we get response with status 200

  Scenario: Create cluster with implicit ZK with feature flag uses actual flavor
    Given feature flags
    """
    ["MDB_ALLOW_DECOMMISSIONED_FLAVOR_USE"]
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.legacy",
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
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test2",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.legacy",
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
        }],
        "description": "test cluster",
        "network_id": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    When we GET "/mdb/clickhouse/1.0/clusters/cid2"
    Then we get response with status 200
    And body at path "$.config.zookeeper" contains
    """
    {
        "resources": {
            "resourcePresetId": "s2.porto.1",
            "diskTypeId": "local-ssd",
            "diskSize": 10737418240
        }
    }
    """