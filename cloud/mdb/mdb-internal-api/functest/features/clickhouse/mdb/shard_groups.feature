@grpc_api
@shard_groups
Feature: Management of ClickHouse shard groups

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
        "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards" with data
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
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker

  Scenario: Adding shard group works
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add shard group in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterShardGroupMetadata",
            "cluster_id": "cid1",
            "shard_group_name": "test_group"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/1.0/operations/worker_task_id3"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add shard group in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterShardGroupMetadata",
          "clusterId": "cid1",
          "shardGroupName": "test_group"
        },
        "response": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.ShardGroup",
            "clusterId": "cid1",
            "name": "test_group",
            "description": "Test shard group",
            "shardNames": ["shard1", "shard2"]
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id3"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_by": "user",
        "description": "Add shard group in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterShardGroupMetadata",
          "cluster_id": "cid1",
          "shard_group_name": "test_group"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.ShardGroup",
            "cluster_id": "cid1",
            "name": "test_group",
            "description": "Test shard group",
            "shard_names": ["shard1", "shard2"]
        }
    }
    """
    When we "GetShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    When we "ListShardGroups" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "shard_groups": [{
            "cluster_id": "cid1",
            "name": "test_group",
            "description": "Test shard group",
            "shard_names": ["shard1", "shard2"]
        }]
    }
    """

  @errors
  Scenario: Adding shard group with invalid name fails
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "Invalid Name!",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "shard group name "Invalid Name!" has invalid symbols"

  @errors
  Scenario: Adding shard group with the name same to cid fails
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "cid1",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "shard group name must not be euqal to cluster ID"

  Scenario: Adding multiple shard groups works
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add shard group in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterShardGroupMetadata",
            "cluster_id": "cid1",
            "shard_group_name": "test_group"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group2",
        "description": "Test shard group2",
        "shard_names": ["shard2"]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add shard group in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterShardGroupMetadata",
            "cluster_id": "cid1",
            "shard_group_name": "test_group2"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When "worker_task_id4" acquired and finished by worker
    And we "ListShardGroups" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "shard_groups": [
            {
                "cluster_id": "cid1",
                "name": "test_group",
                "description": "Test shard group",
                "shard_names": ["shard1", "shard2"]
            },
            {
                "cluster_id": "cid1",
                "name": "test_group2",
                "description": "Test shard group2",
                "shard_names": ["shard2"]
            }
        ]
    }
    """

  @errors
  Scenario: Adding duplicate shard groups fails
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add shard group in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterShardGroupMetadata",
            "cluster_id": "cid1",
            "shard_group_name": "test_group"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    Then we get gRPC response error with code ALREADY_EXISTS and message "shard group "test_group" already exists"

  Scenario: Adding shard group with duplicated shards works
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2", "shard1"]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add shard group in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterShardGroupMetadata",
            "cluster_id": "cid1",
            "shard_group_name": "test_group"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we "GetShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """

  Scenario: Update of the shard group works
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we "UpdateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Updated shard group description",
        "shard_names": ["shard2"],
        "update_mask": {
            "paths": ["description", "shard_names"]
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Update shard group in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateClusterShardGroupMetadata",
            "cluster_id": "cid1",
            "shard_group_name": "test_group"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    Given "worker_task_id4" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id4"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_by": "user",
        "description": "Update shard group in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id4",
        "metadata": {
          "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateClusterShardGroupMetadata",
          "cluster_id": "cid1",
          "shard_group_name": "test_group"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.ShardGroup",
            "cluster_id": "cid1",
            "name": "test_group",
            "description": "Updated shard group description",
            "shard_names": ["shard2"]
        }
    }
    """
    When we "GetShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test_group",
        "description": "Updated shard group description",
        "shard_names": ["shard2"]
    }
    """

  @errors
  Scenario: Update of the shard group without diff fails
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we "UpdateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"],
        "update_mask": {
            "paths": ["description", "shard_names"]
        }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "nothing to update"

  @errors
  Scenario: Updating nonexistent shard group fails
    And we "UpdateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Updated shard group description",
        "shard_names": ["shard1"],
        "update_mask": {
            "paths": ["description", "shard_names"]
        }
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "shard group "test_group" not found"

  @errors
  Scenario: Updating shard group with invalid update mask fails
    And we "UpdateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Updated shard group description",
        "shard_names": ["shard3", "shard4"],
        "update_mask": {
            "paths": ["description..asd", "shard_names"]
        }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "invalid field mask for UpdateClusterShardGroupRequest"

  Scenario: Deleting shard group works
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add shard group in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterShardGroupMetadata",
            "cluster_id": "cid1",
            "shard_group_name": "test_group"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we "DeleteShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete shard group in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.DeleteClusterShardGroupMetadata",
            "cluster_id": "cid1",
            "shard_group_name": "test_group"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When we "ListShardGroups" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "shard_groups": []
    }
    """

  @errors
  Scenario: Deleting nonexistent shard group fails
    When we "DeleteShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "shard group "test_group" not found"

  @errors
  Scenario: Adding shard group to nonexistent cluster fails
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "cluster id "cid2" not found"

  @errors
  Scenario: Adding shard group with empty shard list fails
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": []
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "shard list can not be empty"

  Scenario: Remove shard that belong to group
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards/shard2"
    When we "DeleteShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id4" acquired and finished by worker
    And we "GetShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1"]
    }
    """

  @errors
  Scenario: Remove last shard that belong to group fails
    And we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard2"]
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/shards/shard2"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Last shard in shard group \"test_group\" cannot be removed"
    }
    """
    When we "DeleteShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "last shard in shard group "test_group" cannot be removed"

  Scenario: List shard groups with pagination works
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1", "shard2"]
    }
    """
    Then we get gRPC response OK
    Given "worker_task_id3" acquired and finished by worker
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group2",
        "description": "Test shard group2",
        "shard_names": ["shard2"]
    }
    """
    Then we get gRPC response OK
    Given "worker_task_id4" acquired and finished by worker
    When we "ListShardGroups" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 1
    }
    """
    Then we get gRPC response with body
    """
    {
        "shard_groups": [
            {
                "cluster_id": "cid1",
                "name": "test_group",
                "description": "Test shard group",
                "shard_names": ["shard1", "shard2"]
            }
        ],
        "next_page_token": "eyJPZmZzZXQiOjF9"
    }
    """
    When we "ListShardGroups" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 1,
        "page_token": "eyJPZmZzZXQiOjF9"
    }
    """
    Then we get gRPC response with body
    """
    {
        "shard_groups": [
            {
                "cluster_id": "cid1",
                "name": "test_group2",
                "description": "Test shard group2",
                "shard_names": ["shard2"]
            }
        ],
        "next_page_token": ""
    }
    """
    When we "ListShardGroups" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
    Then we get gRPC response with body
    """
    {
        "shard_groups": []
    }
    """
