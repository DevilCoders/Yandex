@elasticsearch
@grpc_api
Feature: ElasticSearch user operations
  Background:
    Given default headers
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "user_specs": [{
            "name": "donn",
            "password": "nomanisanisland100500"
        }, {
            "name": "frost",
            "password": "2roadsd1v3rgedInAYelloWood"
        }],
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "elasticsearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: An individual user properties can be examined
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "frost"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "frost"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "donn"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "donn"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "nonexistent_user"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "user "nonexistent_user" not found"

  Scenario: Users and their properties can be listed
    When we "List" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "users": [{
            "name": "admin",
            "cluster_id": "cid1"
        }, {
            "name": "donn",
            "cluster_id": "cid1"
        }, {
            "name": "frost",
            "cluster_id": "cid1"
        }]
    }
    """

  Scenario: Delete nonexistent user returns not found
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "UserNotFound"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "user "UserNotFound" not found"

  Scenario: User creation and deletion works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
          "name": "test",
          "password": "TestPassword"
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create user in ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.CreateUserMetadata",
            "cluster_id": "cid1",
            "user_name": "test"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "users": [{
            "name": "admin",
            "cluster_id": "cid1"
        }, {
            "name": "donn",
            "cluster_id": "cid1"
        }, {
            "name": "frost",
            "cluster_id": "cid1"
        }, {
            "name": "test",
            "cluster_id": "cid1"
        }]
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "donn"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete user from ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.DeleteUserMetadata",
            "cluster_id": "cid1",
            "user_name": "donn"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "users": [{
            "name": "admin",
            "cluster_id": "cid1"
        }, {
            "name": "frost",
            "cluster_id": "cid1"
        }, {
            "name": "test",
            "cluster_id": "cid1"
        }]
    }
    """
