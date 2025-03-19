@kafka
@grpc_api
Feature: Update Compute Apache Kafka users
  Background:
    Given default headers
    When we add default feature flag "MDB_KAFKA_CLUSTER"
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      },
      "topic_specs": [
        {
          "name": "topic1",
          "partitions": 12,
          "replication_factor": 1
        },
        {
          "name": "topic2",
          "partitions": 12,
          "replication_factor": 1
        }
      ],
      "user_specs": [
        {
          "name": "test",
          "password": "TestPassword",
          "permissions": [{
              "topic_name": "topic1",
              "role": "ACCESS_ROLE_PRODUCER"
          }]
        }
      ]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateClusterMetadata",
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
    And "worker_task_id1" acquired and finished by worker

  Scenario: Update with no changes doesn't work
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "no fields to change in update request"

  Scenario: Update changes user info
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "userName": "test",
      "updateMask": {
          "paths": [
              "password",
              "permissions"
          ]
      },
      "password": "newPassword",
      "permissions": [
          {
              "topicName": "topic1",
              "role": "ACCESS_ROLE_CONSUMER"
          },
          {
              "topicName": "topic1",
              "role": "ACCESS_ROLE_PRODUCER"
          }
      ]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Modify user in Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateUserMetadata",
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "userName": "test"
    }
    """
    Then we get gRPC response with body
    """
    {
      "name": "test",
      "cluster_id": "cid1",
      "permissions": [
        {
          "topic_name": "topic1",
          "role": "ACCESS_ROLE_CONSUMER",
          "host": "",
          "group": ""
        },
        {
          "topic_name": "topic1",
          "role": "ACCESS_ROLE_PRODUCER",
          "host": "",
          "group": ""
        }
      ]
    }
    """

  Scenario: Update user can erase permissions
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "userName": "test",
      "updateMask": {
          "paths": [
              "permissions"
          ]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Modify user in Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateUserMetadata",
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "userName": "test"
    }
    """
    Then we get gRPC response with body
    """
    {
      "name": "test",
      "cluster_id": "cid1",
      "permissions": []
    }
    """

  Scenario: Grant empty permission doesn't work
    When we "GrantPermission" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
      {
        "cluster_id": "cid1",
        "userName": "test"
      }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "permission must be non empty"

  Scenario: Grant existing permission doesn't work
    When we "GrantPermission" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
      {
        "cluster_id": "cid1",
        "userName": "test",
        "permission": {
          "topic_name": "topic1",
          "role": "ACCESS_ROLE_PRODUCER"
        }
      }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "no changes found in request"

  Scenario: Revoke empty permission doesn't work
    When we "RevokePermission" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
      {
        "cluster_id": "cid1",
        "userName": "test"
      }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "permission must be non empty"

  Scenario: Revoke absent permission doesn't work
    When we "RevokePermission" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
      {
        "cluster_id": "cid1",
        "userName": "test",
        "permission": {
          "topic_name": "topic2",
          "role": "ACCESS_ROLE_PRODUCER"
        }
      }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "no changes found in request"



  Scenario: Grant and revoke user permissions works
    When we "GrantPermission" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
      {
        "cluster_id": "cid1",
        "userName": "test",
        "permission": {
          "topic_name": "topic2",
          "role": "ACCESS_ROLE_PRODUCER"
        }
      }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Grant permission to user in Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateUserMetadata",
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "userName": "test"
    }
    """
    Then we get gRPC response with body
    """
    {
      "name": "test",
      "cluster_id": "cid1",
      "permissions": [
        {
          "topic_name": "topic1",
          "role": "ACCESS_ROLE_PRODUCER",
          "host": "",
          "group": ""
        },
        {
          "topic_name": "topic2",
          "role": "ACCESS_ROLE_PRODUCER",
          "host": "",
          "group": ""
        }
      ]
    }
    """
    When we "RevokePermission" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
      {
        "cluster_id": "cid1",
        "userName": "test",
        "permission": {
          "topic_name": "topic1",
          "role": "ACCESS_ROLE_PRODUCER"
        }
      }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Revoke permission from user in Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateUserMetadata",
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
    And "worker_task_id3" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "userName": "test"
    }
    """
    Then we get gRPC response with body
    """
    {
      "name": "test",
      "cluster_id": "cid1",
      "permissions": [
        {
          "topic_name": "topic2",
          "role": "ACCESS_ROLE_PRODUCER",
          "host": "",
          "group": ""
        }
      ]
    }
    """
