@elasticsearch
@grpc_api
Feature: ElasticSearch user operations adhere to naming rules
  Background:
    Given default headers
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "user_specs": [{
            "name": "user",
            "password": "coolpassword"
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

  Scenario: Users with special prefix are not allowed
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "mdb_user",
            "password": "coolpassword"
        }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "invalid user name "mdb_user""

  Scenario: Users with special semantic are not allowed
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "elastic",
            "password": "coolpassword"
        }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "invalid user name "elastic""

  Scenario: Users with special symbols are not allowed
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "user!#",
            "password": "coolpassword"
        }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "user name "user!#" has invalid symbols"

  Scenario: Short password are not allowed
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "user2",
            "password": "shortpw"
        }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password is too short"

  Scenario: User admin is predefined and cannot be added
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "admin",
        "password": "TestPassword"
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "invalid user name "admin""

  Scenario: User admin cannot be deleted
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "admin"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "user "admin" not found"
