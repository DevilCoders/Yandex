Feature: Deletion Protection

  Background:
    Given default headers
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
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
            "version": "21.7",
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
        "network_id": "network1"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "deletionProtection": false
    }
    """

  Scenario: delete_protection works with python-api
    #
    # ENABLE deletion_protection
    #
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "deletionProtection": true
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": true
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "deletionProtection": true
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "deletion_protection": true
    }
    """
    #
    # TEST deletion_protection
    #
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 422
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "The operation was rejected because cluster has 'deletion_protection' = ON"
    #
    # DISABLE deletion_protection
    #
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "deletionProtection": false
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": true
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "deletionProtection": false
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "deletion_protection": false
    }
    """
    #
    # TEST deletion_protection
    #
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete ClickHouse cluster",
        "id": "worker_task_id4",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.DeleteClusterMetadata",
            "cluster_id": "cid1"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And for "worker_task_id4" exists "yandex.cloud.events.mdb.clickhouse.DeleteCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """

  Scenario: Resource Reaper can overcome delete_protection
    #
    # ENABLE deletion_protection
    #
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "deletionProtection": true
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": true
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "deletionProtection": true
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "deletion_protection": true
    }
    """
    #
    # TEST deletion_protection
    #
    When default headers with "resource-reaper-service-token" token
    And we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "resource-reaper",
        "description": "Delete ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.DeleteClusterMetadata",
            "cluster_id": "cid1"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.DeleteCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """

#
# Note: Modifies doesn't implemented in Go-API
#       Uncomment and fix it when API will be ready
#
#Scenario: delete_protection works with go-api
#    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
#    """
#    {
#        "folder_id": "folder1",
#        "name": "test2",
#        "environment": "PRESTABLE",
#        "config_spec": {
#            "version": "21.3",
#            "clickhouse": {
#                "resources": {
#                    "resource_preset_id": "s2.porto.1",
#                    "disk_type_id": "local-ssd",
#                    "disk_size": 10737418240
#                }
#            }
#        },
#        "database_specs": [{
#            "name": "testdb"
#        }],
#        "user_specs": [{
#            "name": "test",
#            "password": "test_password"
#        }],
#        "host_specs": [{
#            "type": "CLICKHOUSE",
#            "zone_id": "myt"
#        }, {
#            "type": "CLICKHOUSE",
#            "zone_id": "sas"
#        }],
#        "description": "test cluster"
#    }
#    """
#    Then we get gRPC response with body
#    """
#    {
#      "done": false,
#      "id": "worker_task_id1",
#      "metadata": {
#        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterMetadata",
#        "cluster_id": "cid1"
#      }
#    }
#    """
#    When "worker_task_id1" acquired and finished by worker
#    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
#    """
#    {
#      "cluster_id": "cid1"
#    }
#    """
#    Then we get gRPC response with body
#    """
#    {
#        "id": "cid1",
#        "deletion_protection": false
#    }
#    """
#    #
#    # ENABLE deletion_protection
#    #
#    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
#    """
#    {
#      "cluster_id": "cid1",
#      "update_mask": {
#        "paths": [
#            "deletion_protection"
#        ]
#      },
#      "deletion_protection": true
#    }
#    """
#    Then we get gRPC response OK
#
#    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
#    """
#    {
#      "cluster_id": "cid1"
#    }
#    """
#    Then we get gRPC response with body
#    """
#    {
#        "id": "cid1",
#        "deletion_protection": true
#    }
#    """
#    #
#    # TEST deletion_protection
#    #
#    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
#    """
#    {
#      "cluster_id": "cid1"
#    }
#    """
#    Then we get gRPC response error with code FAILED_PRECONDITION and message "The operation was rejected because cluster has 'deletion_protection' = ON"
#    #
#    # DISABLE deletion_protection
#    #
#    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
#    """
#    {
#      "cluster_id": "cid1",
#      "update_mask": {
#        "paths": [
#            "deletion_protection"
#        ]
#      },
#      "deletion_protection": false
#    }
#    """
#    Then we get gRPC response OK
#
#    """
#    {
#        "deletion_protection": false
#    }
#    """
#    #
#    # TEST deletion_protection
#    #
#    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
#    """
#    {
#      "cluster_id": "cid1"
#    }
#    """
#    Then we get gRPC response OK
