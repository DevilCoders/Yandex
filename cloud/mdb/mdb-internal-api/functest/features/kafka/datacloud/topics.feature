@kafka
@grpc_api
Feature: Manage Apache Kafka topics in datacloud

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
    When "worker_task_id1" acquired and finished by worker
    When we "Create" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "topic1",
        "partitions": 12,
        "replication_factor": 1,
        "topic_config_3_0": {
          "cleanup_policy":        "CLEANUP_POLICY_COMPACT",
          "compression_type":      "COMPRESSION_TYPE_LZ4",
          "retention_bytes":       1006,
          "retention_ms":          1007
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
            "cleanup_policy":        "CLEANUP_POLICY_COMPACT",
            "compression_type":      "COMPRESSION_TYPE_LZ4",
            "retention_bytes":       "1006",
            "retention_ms":          "1007"
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

  Scenario: Delete nonexistent topic returns not found
    When we "Delete" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_name": "TopicNotFound"
    }
    """
    Then we get DataCloud response error with code NOT_FOUND and message "topic "TopicNotFound" not found"

  Scenario: Delete for topic within nonexistent cluster returns FAILED_PRECONDITION
    When we "Delete" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "nonexistent_cluster_id",
      "topic_name": "topic1"
    }
    """
    Then we get DataCloud response error with code FAILED_PRECONDITION and message "cluster id "nonexistent_cluster_id" not found"

  Scenario: Get for nonexistent cluster returns FAILED_PRECONDITION
    When we "Get" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "nonexistent_cluster_id"
    }
    """
    Then we get DataCloud response error with code FAILED_PRECONDITION and message "cluster id "nonexistent_cluster_id" not found"

  Scenario: Topic creation with wrong name fails
    When we "Create" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "topic_wrong_name[*]",
        "partitions": 3,
        "replication_factor": 1
      }
    }
    """
    Then we get DataCloud response error with code INVALID_ARGUMENT and message "topic name "topic_wrong_name[*]" has invalid symbols"

  Scenario: Consumer offsets topic creation fails
    When we "Create" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "__consumer_offsets"
      }
    }
    """
    Then we get DataCloud response error with code INVALID_ARGUMENT and message "cannot create topic with reserved name "__consumer_offsets""

  Scenario: Transaction state topic creation fails
    When we "Create" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "__transaction_state"
      }
    }
    """
    Then we get DataCloud response error with code INVALID_ARGUMENT and message "cannot create topic with reserved name "__transaction_state""



  Scenario: Topic creation with dot in name works
    When we "Create" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "topic_complex-name.example",
        "partitions": 3,
        "replication_factor": 1
      }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id4"
    }
    """

  Scenario: Topic creation, modification and delete works
    When we "Create" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "topic3",
        "partitions": 3,
        "replication_factor": 1,
        "topic_config_3_0": {
          "cleanup_policy":        "CLEANUP_POLICY_COMPACT_AND_DELETE",
          "compression_type":      "COMPRESSION_TYPE_SNAPPY",
          "retention_bytes":       2006,
          "retention_ms":          2007
        }
      }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id4"
    }
    """
    And "worker_task_id4" acquired and finished by worker
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
            "cleanup_policy":        "CLEANUP_POLICY_COMPACT",
            "compression_type":      "COMPRESSION_TYPE_LZ4",
            "retention_bytes":       "1006",
            "retention_ms":          "1007"
          }
        },
        {
          "name": "topic2",
          "cluster_id": "cid1",
          "partitions": "12",
          "replication_factor": "1",
          "topic_config_3_0": {}
        },
        {
          "name": "topic3",
          "cluster_id": "cid1",
          "partitions": "3",
          "replication_factor": "1",
          "topic_config_3_0": {
            "cleanup_policy":        "CLEANUP_POLICY_COMPACT_AND_DELETE",
            "compression_type":      "COMPRESSION_TYPE_SNAPPY",
            "retention_bytes":       "2006",
            "retention_ms":          "2007"
          }
        }
      ]
    }
    """
    When we "Update" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_name": "topic3",
      "topic_spec": {
        "partitions": 4,
        "replication_factor": 2,
        "topic_config_3_0": {
          "cleanup_policy":        "CLEANUP_POLICY_DELETE",
          "compression_type":      "COMPRESSION_TYPE_PRODUCER",
          "retention_bytes":       3006,
          "retention_ms":          3007
        }
      }
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id5"
    }
    """
    And "worker_task_id5" acquired and finished by worker
    When we "Get" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_name": "topic3"
    }
    """
    Then we get DataCloud response with body ignoring empty
    """
    {
      "name": "topic3",
      "cluster_id": "cid1",
      "partitions": "4",
      "replication_factor": "2",
      "topic_config_3_0": {
        "cleanup_policy":        "CLEANUP_POLICY_DELETE",
        "compression_type":      "COMPRESSION_TYPE_PRODUCER",
        "retention_bytes":       "3006",
        "retention_ms":          "3007"
      }
    }
    """
    When we "Delete" via DataCloud at "datacloud.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_name": "topic2"
    }
    """
    Then we get DataCloud response with body
    """
    {
        "operation_id": "worker_task_id6"
    }
    """
    And "worker_task_id6" acquired and finished by worker
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
            "cleanup_policy":        "CLEANUP_POLICY_COMPACT",
            "compression_type":      "COMPRESSION_TYPE_LZ4",
            "retention_bytes":       "1006",
            "retention_ms":          "1007"
          }
        },
        {
          "name": "topic3",
          "cluster_id": "cid1",
          "partitions": "4",
          "replication_factor": "2",
          "topic_config_3_0": {
            "cleanup_policy":        "CLEANUP_POLICY_DELETE",
            "compression_type":      "COMPRESSION_TYPE_PRODUCER",
            "retention_bytes":       "3006",
            "retention_ms":          "3007"
          }
        }
      ]
    }
    """