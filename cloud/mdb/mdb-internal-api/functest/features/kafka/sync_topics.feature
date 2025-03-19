@kafka
@grpc_api
Feature: Sync topic changes made via Kafka Admin API
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
        "kafka": {
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_size": "107374182400",
            "disk_type_id": "network-ssd"
          }
        },
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      },
      "topic_specs": [
        {
          "name": "topic1",
          "partitions": 12,
          "replication_factor": 1,
          "topic_config_3_0": {
            "delete_retention_ms": 1000
          }
        },
        {
          "name": "topic2",
          "partitions": 12,
          "replication_factor": 1
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

  Scenario: List topics to sync
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.inner.TopicService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "update_allowed": true,
      "revision": "3",
      "topics": [
        "{\"name\":\"topic1\",\"partitions\":12,\"replication_factor\":1,\"config\":{\"delete.retention.ms\":1000}}",
        "{\"name\":\"topic2\",\"partitions\":12,\"replication_factor\":1,\"config\":{}}"
      ]
    }
    """

  Scenario: List returns update is not allowed when there's running task
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["name"]
      },
      "name": "new-name"
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.inner.TopicService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "update_allowed": false
    }
    """

  Scenario: List returns update is not allowed when cluster is in error state
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["name"]
      },
      "name": "new-name"
    }
    """
    And "worker_task_id2" acquired and failed by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.inner.TopicService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "update_allowed": false
    }
    """

  Scenario: Topic changes received via Update method of inner.TopicService are saved successfully
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.inner.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "revision": "3",
      "changed_topics": [
        "{\"name\":\"topic1\",\"partitions\":12,\"replication_factor\":2,\"config\":{\"delete.retention.ms\":2000}}",
        "{\"name\":\"topic3\",\"partitions\":12,\"replication_factor\":3,\"config\":{\"max.message.bytes\":1024}}"
      ],
      "deleted_topics": ["topic2"]
    }
    """
    Then we get gRPC response with body
    """
    {
      "update_accepted": true
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "topics": [
        {
          "name": "topic1",
          "cluster_id": "cid1",
          "partitions": "12",
          "replication_factor": "2",
          "topic_config_3_0": {
            "delete_retention_ms": "2000"
          }
        },
        {
          "name": "topic3",
          "cluster_id": "cid1",
          "partitions": "12",
          "replication_factor": "3",
          "topic_config_3_0": {
            "max_message_bytes": "1024"
          }
        }
      ]
    }
    """

  Scenario: Topic changes received via Update method of inner.TopicService are not accepted if revision is outdated
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.inner.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "revision": "2",
      "changed_topics": [
        "{\"name\":\"topic1\",\"partitions\":12,\"replication_factor\":2,\"config\":{\"delete.retention.ms\":2000}}",
        "{\"name\":\"topic3\",\"partitions\":12,\"replication_factor\":3,\"config\":{\"max.message.bytes\":1024}}"
      ],
      "deleted_topics": ["topic2"]
    }
    """
    Then we get gRPC response with body
    """
    {
      "update_accepted": false
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "topics": [
        {
          "name": "topic1",
          "cluster_id": "cid1",
          "partitions": "12",
          "replication_factor": "1",
          "topic_config_3_0": {
            "delete_retention_ms": "1000"
          }
        },
        {
          "name": "topic2",
          "cluster_id": "cid1",
          "partitions": "12",
          "replication_factor": "1",
          "topic_config_3_0": {}
        }
      ]
    }
    """

  Scenario: Topic changes received via Update method of inner.TopicService are not accepted if there's running task
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["name"]
      },
      "name": "new-name"
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.inner.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "revision": "5",
      "changed_topics": [
        "{\"name\":\"topic1\",\"partitions\":12,\"replication_factor\":2,\"config\":{\"delete.retention.ms\":2000}}",
        "{\"name\":\"topic3\",\"partitions\":12,\"replication_factor\":3,\"config\":{\"max.message.bytes\":1024}}"
      ],
      "deleted_topics": ["topic2"]
    }
    """
    Then we get gRPC response with body
    """
    {
      "update_accepted": false
    }
    """

  Scenario: Topic changes received via Update method of inner.TopicService are not accepted if cluster is in error state
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["name"]
      },
      "name": "new-name"
    }
    """
    And "worker_task_id2" acquired and failed by worker
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.inner.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "revision": "5",
      "changed_topics": [
        "{\"name\":\"topic1\",\"partitions\":12,\"replication_factor\":2,\"config\":{\"delete.retention.ms\":2000}}",
        "{\"name\":\"topic3\",\"partitions\":12,\"replication_factor\":3,\"config\":{\"max.message.bytes\":1024}}"
      ],
      "deleted_topics": ["topic2"]
    }
    """
    Then we get gRPC response with body
    """
    {
      "update_accepted": false
    }
    """


  Scenario: Error is reported when no changes detected within Update request
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.inner.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "revision": "3",
      "changed_topics": [
        "{\"name\":\"topic1\",\"partitions\":12,\"replication_factor\":1,\"config\":{\"delete.retention.ms\":1000}}"
      ],
      "deleted_topics": ["topic3"]
    }
    """
    Then we get gRPC response error with code ALREADY_EXISTS and message "no changes made"
