@kafka
@grpc_api
Feature: Create Apache Kafka users
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
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "users": [
        {
          "name": "consumer",
          "cluster_id": "cid1",
          "permissions": [
            {
              "topic_name": "topic1",
              "role": "ACCESS_ROLE_CONSUMER",
              "host": "",
              "group": ""
            }
          ]
        },
        {
          "name": "producer",
          "cluster_id": "cid1",
          "permissions": [
            {
              "topic_name": "topic1",
              "role": "ACCESS_ROLE_PRODUCER",
              "host": "",
              "group": ""
            }
          ]
        }
      ]
    }
    """

  Scenario: Delete nonexistent user returns not found
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "UserNotFound"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "user "UserNotFound" not found"

  Scenario: User creation and delete works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "test",
        "password": "TestPassword",
        "permissions": [
          {
            "topic_name": "topic1",
            "role": "ACCESS_ROLE_PRODUCER"
          },
          {
            "topic_name": "topic2",
            "role": "ACCESS_ROLE_CONSUMER"
          }
        ]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create user in Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateUserMetadata",
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
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "users": [
        {
          "name": "consumer",
          "cluster_id": "cid1",
          "permissions": [
            {
              "topic_name": "topic1",
              "role": "ACCESS_ROLE_CONSUMER",
              "host": "",
              "group": ""
            }
          ]
        },
        {
          "name": "producer",
          "cluster_id": "cid1",
          "permissions": [
            {
              "topic_name": "topic1",
              "role": "ACCESS_ROLE_PRODUCER",
              "host": "",
              "group": ""
            }
          ]
        },
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
              "role": "ACCESS_ROLE_CONSUMER",
              "host": "",
              "group": ""
            }
          ]
        }
      ]
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "consumer"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete user from Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.DeleteUserMetadata",
        "cluster_id": "cid1",
        "user_name": "consumer"
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
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "users": [
        {
          "name": "producer",
          "cluster_id": "cid1",
          "permissions": [
            {
              "topic_name": "topic1",
              "role": "ACCESS_ROLE_PRODUCER",
              "host": "",
              "group": ""
            }
          ]
        },
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
              "role": "ACCESS_ROLE_CONSUMER",
              "host": "",
              "group": ""
            }
          ]
        }
      ]
    }
    """

  Scenario: User list pagination works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "test",
        "password": "TestPassword",
        "permissions": [
          {
            "topic_name": "topic1",
            "role": "ACCESS_ROLE_PRODUCER"
          },
          {
            "topic_name": "topic2",
            "role": "ACCESS_ROLE_CONSUMER"
          }
        ]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create user in Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateUserMetadata",
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
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "page_size": 2
    }
    """
    Then we get gRPC response with body
    """
    {
      "users": [
        {
          "name": "consumer",
          "cluster_id": "cid1",
          "permissions": [
            {
              "topic_name": "topic1",
              "role": "ACCESS_ROLE_CONSUMER",
              "host": "",
              "group": ""
            }
          ]
        },
        {
          "name": "producer",
          "cluster_id": "cid1",
          "permissions": [
            {
              "topic_name": "topic1",
              "role": "ACCESS_ROLE_PRODUCER",
              "host": "",
              "group": ""
            }
          ]
        }
      ],
      "next_page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
