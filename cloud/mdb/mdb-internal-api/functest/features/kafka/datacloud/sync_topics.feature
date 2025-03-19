@kafka
@grpc_api
Feature: Sync topic changes made via Kafka Admin API in datacloud
  Background:
    Given headers
    """
    {
        "X-YaCloud-SubjectToken": "rw-token",
        "Accept": "application/json",
        "Content-Type": "application/json",
        "Access-Id": "00000000-0000-0000-0000-000000000000",
        "Access-Secret": "dummy"
    }
    """
    When we "Create" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
        "project_id": "folder1",
        "cloud_type": "aws",
        "region_id": "eu-central1",
        "name": "test_cluster",
        "description": "test cluster",
        "version": "3.0",
        "resources": {
            "kafka": {
                "resource_preset_id": "a1.aws.1",
                "disk_size": 34359738368,
                "broker_count": 1,
                "zone_count": 3
            }
        }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "cluster_id": "cid1",
        "operation_id": "worker_task_id1"
    }
    """
    And "worker_task_id1" acquired and finished by worker
    When we "Create" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "topic1",
        "partitions": 12,
        "replication_factor": 1,
        "topic_config_3_0": {
          "retention_ms": 1000
        }
      }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we "Create" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "topic2",
        "partitions": 12,
        "replication_factor": 1
      }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id3"
    }
    """
    When "worker_task_id3" acquired and finished by worker

  Scenario: List topics to sync
    When we "List" via DataCloud at "datacloud.kafka.inner.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body
    """
    {
      "update_allowed": true,
      "revision": "9",
      "topics": [
        "{\"name\":\"topic1\",\"partitions\":12,\"replication_factor\":1,\"config\":{\"retention.ms\":1000}}",
        "{\"name\":\"topic2\",\"partitions\":12,\"replication_factor\":1,\"config\":{}}"
      ]
    }
    """

  Scenario: List returns update is not allowed when there's running task
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "name": "new-name"
    }
    """
    When we "List" via DataCloud at "datacloud.kafka.inner.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body
    """
    {
      "update_allowed": false
    }
    """

  Scenario: List returns update is not allowed when cluster is in error state
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "name": "new-name"
    }
    """
    And "worker_task_id4" acquired and failed by worker
    When we "List" via DataCloud at "datacloud.kafka.inner.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body
    """
    {
      "update_allowed": false
    }
    """

  Scenario: Topic changes received via Update method of inner.TopicService are saved successfully
    When we "Update" via DataCloud at "datacloud.kafka.inner.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "revision": "9",
      "changed_topics": [
        "{\"name\":\"topic1\",\"partitions\":12,\"replication_factor\":2,\"config\":{\"retention.ms\":2000}}",
        "{\"name\":\"topic3\",\"partitions\":12,\"replication_factor\":3,\"config\":{\"retention.bytes\":1024}}"
      ],
      "deleted_topics": ["topic2"]
    }
    """
    Then we get DataCloud response with body
    """
    {
      "update_accepted": true
    }
    """
    When we "List" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body ignoring empty
    """
    {
      "topics": [
        {
          "name": "topic1",
          "cluster_id": "cid1",
          "partitions": "12",
          "replication_factor": "2",
          "topic_config_3_0": {
            "retention_ms": "2000"
          }
        },
        {
          "name": "topic3",
          "cluster_id": "cid1",
          "partitions": "12",
          "replication_factor": "3",
          "topic_config_3_0": {
            "retention_bytes": "1024"
          }
        }
      ]
    }
    """

  Scenario: Topic changes received via Update method of inner.TopicService are not accepted if revision is outdated
    When we "Update" via DataCloud at "datacloud.kafka.inner.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "revision": "8",
      "changed_topics": [
        "{\"name\":\"topic1\",\"partitions\":12,\"replication_factor\":2,\"config\":{\"retention.ms\":2000}}",
        "{\"name\":\"topic3\",\"partitions\":12,\"replication_factor\":3,\"config\":{\"retention.bytes\":1024}}"
      ],
      "deleted_topics": ["topic2"]
    }
    """
    Then we get DataCloud response with body
    """
    {
      "update_accepted": false
    }
    """
    When we "List" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get DataCloud response with body ignoring empty
    """
    {
      "topics": [
        {
          "name": "topic1",
          "cluster_id": "cid1",
          "partitions": "12",
          "replication_factor": "1",
          "topic_config_3_0": {
            "retention_ms": "1000"
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
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "name": "new-name"
    }
    """
    When we "Update" via DataCloud at "datacloud.kafka.inner.v1.TopicService" with data
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
    Then we get DataCloud response with body
    """
    {
      "update_accepted": false
    }
    """

  Scenario: Topic changes received via Update method of inner.TopicService are not accepted if cluster is in error state
    When we "Update" via DataCloud at "datacloud.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "name": "new-name"
    }
    """
    And "worker_task_id4" acquired and failed by worker
    When we "Update" via DataCloud at "datacloud.kafka.inner.v1.TopicService" with data
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
    Then we get DataCloud response with body
    """
    {
      "update_accepted": false
    }
    """

  Scenario: Error is reported when no changes detected within Update request
    When we "Update" via DataCloud at "datacloud.kafka.inner.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "revision": "9",
      "changed_topics": [
        "{\"name\":\"topic1\",\"partitions\":12,\"replication_factor\":1,\"config\":{\"retention.ms\":1000}}"
      ],
      "deleted_topics": ["topic3"]
    }
    """
    Then we get DataCloud response error with code ALREADY_EXISTS and message "no changes made"
