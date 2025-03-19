@kafka
@grpc_api
Feature: Create Apache Kafka users validation
  Background:
    Given default headers
    When we add default feature flag "MDB_KAFKA_CLUSTER"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
          "name": "producer",
          "password": "ProducerPassword",
          "permissions": [{
              "topic_name": "topic1",
              "role": "ACCESS_ROLE_PRODUCER"
          }]
        },
        {
          "name": "consumer",
          "password": "ConsumerPassword",
          "permissions": [{
              "topic_name": "topic1",
              "role": "ACCESS_ROLE_CONSUMER"
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


  Scenario: User creation with mdb prefix fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "mdb_test",
        "password": "TestPassword",
        "permissions": [
          {
            "topic_name": "topic2",
            "role": "ACCESS_ROLE_CONSUMER"
          }
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "invalid user name "mdb_test""

  Scenario: User creation with empty permission topic fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "test",
        "password": "TestPassword",
        "permissions": [
          {
            "role": "ACCESS_ROLE_CONSUMER"
          }
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "topic name "" is too short"

  Scenario: User creation with invalid topic name fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "producer",
        "password": "TestPassword",
        "permissions": [
          {
            "topic_name": "*invalid*",
            "role": "ACCESS_ROLE_CONSUMER"
          }
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "topic name "*invalid*" has invalid symbols"

  Scenario: User creation with permission for topic pattern works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "test",
        "password": "TestPassword",
        "permissions": [
          {
            "topic_name": "prefix_*",
            "role": "ACCESS_ROLE_CONSUMER"
          }
        ]
      }
    }
    """
    Then we get gRPC response OK

  Scenario: User creation with permission for all topics works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "test",
        "password": "TestPassword",
        "permissions": [
          {
            "topic_name": "*",
            "role": "ACCESS_ROLE_CONSUMER"
          }
        ]
      }
    }
    """
    Then we get gRPC response OK

Scenario: User update with invalid topic name fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "userName": "producer",
      "updateMask": {
          "paths": [
              "permissions"
          ]
      },
      "permissions": [
        {
          "topic_name": "*invalid*",
          "role": "ACCESS_ROLE_CONSUMER"
        }
      ]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "topic name "*invalid*" has invalid symbols"

Scenario: Grant user permissions with invalid topic name fails
    When we "GrantPermission" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
      {
        "cluster_id": "cid1",
        "userName": "producer",
        "permission": {
          "topic_name": "*invalid*",
          "role": "ACCESS_ROLE_PRODUCER"
        }
      }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "topic name "*invalid*" has invalid symbols"

    Scenario: Create user without password fails
        When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "without_password",
        "permissions": [
          {
            "topic_name": "*",
            "role": "ACCESS_ROLE_CONSUMER"

          }
        ]
      }
    }
    """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "user password cannot be empty"

    Scenario: Create user with empty password fails
        When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "empty_password",
        "password": "",
        "permissions": [
          {
            "topic_name": "*",
            "role": "ACCESS_ROLE_CONSUMER"

          }
        ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "user password cannot be empty"

Scenario: Create user with invalid password fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "empty_password",
        "password": "qwerty["
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password should not contain following symbols: '[]"

Scenario: User update with empty password fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "userName": "producer",
      "updateMask": {
          "paths": [
              "password"
          ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "user password cannot be empty"

Scenario: User update with invalid password fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "userName": "producer",
      "password": "qwerty]",
      "updateMask": {
          "paths": [
              "password"
          ]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "password should not contain following symbols: '[]"
