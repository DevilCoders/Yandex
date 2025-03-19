@kafka
@grpc_api
Feature: Manage Apache Kafka topics
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
            "cleanup_policy":        "CLEANUP_POLICY_COMPACT",
            "compression_type":      "COMPRESSION_TYPE_LZ4",
            "delete_retention_ms":   1000,
            "file_delete_delay_ms":  1001,
            "flush_messages":        1002,
            "flush_ms":              1003,
            "max_message_bytes":     1004,
            "min_compaction_lag_ms": 1005,
            "min_insync_replicas":   1,
            "retention_bytes":       1006,
            "retention_ms":          1007
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
            "delete_retention_ms":   "1000",
            "file_delete_delay_ms":  "1001",
            "flush_messages":        "1002",
            "flush_ms":              "1003",
            "max_message_bytes":     "1004",
            "min_compaction_lag_ms": "1005",
            "min_insync_replicas":   "1",
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
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_name": "TopicNotFound"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "topic "TopicNotFound" not found"


  Scenario: Get for topic within nonexistent cluster returns FAILED_PRECONDITION
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "nonexistent_cluster_id",
      "topic_name": "topic1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "cluster id "nonexistent_cluster_id" not found"


  Scenario: Get for nonexistent cluster returns NOT_FOUND
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "nonexistent_cluster_id"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "cluster id "nonexistent_cluster_id" not found"


  Scenario: Topic creation with wrong config version fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "topic3",
        "partitions": 3,
        "replication_factor": 1,
        "topic_config_2_1": {
          "cleanup_policy":        "CLEANUP_POLICY_COMPACT_AND_DELETE",
          "compression_type":      "COMPRESSION_TYPE_SNAPPY",
          "delete_retention_ms":   2000
        }
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "wrong topic config version 2.1 for topic topic3, must be: 3.0"

  Scenario: Topic creation with wrong name fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "topic name "topic_wrong_name[*]" has invalid symbols"

  Scenario: Create topic in multi broker configuration with max.message.bytes with less of equal than replica.fetch.max.bytes should work
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "topic_with_max_message_bytes_bigger_less_or_equal_than_replica_fetch_max_bytes",
        "partitions": 3,
        "replication_factor": 1,
        "topic_config_3_0": {
          "max_message_bytes": 1048588
        }
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Add topic to Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateTopicMetadata",
        "cluster_id": "cid1",
        "topic_name": "topic_with_max_message_bytes_bigger_less_or_equal_than_replica_fetch_max_bytes"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """

  Scenario: Create topic in multi broker configuration with max.message.bytes greater than replica.fetch.max.bytes should fails fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "topic_with_max_message_bytes_bigger_than_replica_fetch_max_bytes",
        "partitions": 3,
        "replication_factor": 1,
        "topic_config_3_0": {
          "max_message_bytes": 1048589
        }
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "For multi-node kafka cluster, broker setting "replica.fetch.max.bytes" value(1048576) must be equal or greater then topic ("topic_with_max_message_bytes_bigger_than_replica_fetch_max_bytes") setting "max.message.bytes" value(1048589) - record log overhead size(12)."


  Scenario: Consumer offsets topic creation fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "__consumer_offsets"
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "cannot create topic with reserved name "__consumer_offsets""

  Scenario: Transaction state topic creation fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "__transaction_state"
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "cannot create topic with reserved name "__transaction_state""

  Scenario: Topic creation with dot in name works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
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
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Add topic to Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateTopicMetadata",
        "cluster_id": "cid1",
        "topic_name": "topic_complex-name.example"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """

  Scenario: Topic creation, modification and delete works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
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
          "delete_retention_ms":   2000,
          "file_delete_delay_ms":  2001,
          "flush_messages":        2002,
          "flush_ms":              2003,
          "max_message_bytes":     2004,
          "min_compaction_lag_ms": 2005,
          "min_insync_replicas":   1,
          "retention_bytes":       2006,
          "retention_ms":          2007
        }
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Add topic to Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateTopicMetadata",
        "cluster_id": "cid1",
        "topic_name": "topic3"
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
            "delete_retention_ms":   "1000",
            "file_delete_delay_ms":  "1001",
            "flush_messages":        "1002",
            "flush_ms":              "1003",
            "max_message_bytes":     "1004",
            "min_compaction_lag_ms": "1005",
            "min_insync_replicas":   "1",
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
            "delete_retention_ms":   "2000",
            "file_delete_delay_ms":  "2001",
            "flush_messages":        "2002",
            "flush_ms":              "2003",
            "max_message_bytes":     "2004",
            "min_compaction_lag_ms": "2005",
            "min_insync_replicas":   "1",
            "retention_bytes":       "2006",
            "retention_ms":          "2007"
          }
        }
      ]
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
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
          "delete_retention_ms":   3000,
          "file_delete_delay_ms":  3001,
          "flush_messages":        3002,
          "flush_ms":              3003,
          "max_message_bytes":     3004,
          "min_compaction_lag_ms": 3005,
          "min_insync_replicas":   2,
          "retention_bytes":       3006,
          "retention_ms":          3007
        }
      },
      "updateMask": {
          "paths": [
              "topic_spec.partitions",
              "topic_spec.replication_factor",
              "topic_spec.topic_config_3_0.cleanup_policy",
              "topic_spec.topic_config_3_0.compression_type",
              "topic_spec.topic_config_3_0.delete_retention_ms",
              "topic_spec.topic_config_3_0.file_delete_delay_ms",
              "topic_spec.topic_config_3_0.flush_messages",
              "topic_spec.topic_config_3_0.flush_ms",
              "topic_spec.topic_config_3_0.max_message_bytes",
              "topic_spec.topic_config_3_0.min_compaction_lag_ms",
              "topic_spec.topic_config_3_0.min_insync_replicas",
              "topic_spec.topic_config_3_0.retention_bytes",
              "topic_spec.topic_config_3_0.retention_ms"
          ]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Modify topic in Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateTopicMetadata",
        "cluster_id": "cid1",
        "topic_name": "topic3"
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_name": "topic3"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "name": "topic3",
      "cluster_id": "cid1",
      "partitions": "4",
      "replication_factor": "2",
      "topic_config_3_0": {
        "cleanup_policy":        "CLEANUP_POLICY_DELETE",
        "compression_type":      "COMPRESSION_TYPE_PRODUCER",
        "delete_retention_ms":   "3000",
        "file_delete_delay_ms":  "3001",
        "flush_messages":        "3002",
        "flush_ms":              "3003",
        "max_message_bytes":     "3004",
        "min_compaction_lag_ms": "3005",
        "min_insync_replicas":   "2",
        "retention_bytes":       "3006",
        "retention_ms":          "3007"
      }
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_name": "topic2"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete topic from Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id4",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.DeleteTopicMetadata",
        "cluster_id": "cid1",
        "topic_name": "topic2"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id4" acquired and finished by worker
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
            "delete_retention_ms":   "1000",
            "file_delete_delay_ms":  "1001",
            "flush_messages":        "1002",
            "flush_ms":              "1003",
            "max_message_bytes":     "1004",
            "min_compaction_lag_ms": "1005",
            "min_insync_replicas":   "1",
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
            "delete_retention_ms":   "3000",
            "file_delete_delay_ms":  "3001",
            "flush_messages":        "3002",
            "flush_ms":              "3003",
            "max_message_bytes":     "3004",
            "min_compaction_lag_ms": "3005",
            "min_insync_replicas":   "2",
            "retention_bytes":       "3006",
            "retention_ms":          "3007"
          }
        }
      ]
    }
    """

  Scenario: Reset topic config value to default works
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_name": "topic1",
      "topic_spec": {
        "topic_config_3_0": {}
      },
      "updateMask": {
          "paths": [
              "topic_spec.topic_config_3_0.delete_retention_ms"
          ]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Modify topic in Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateTopicMetadata",
        "cluster_id": "cid1",
        "topic_name": "topic1"
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_name": "topic1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "name": "topic1",
      "cluster_id": "cid1",
      "partitions": "12",
      "replication_factor": "1",
      "topic_config_3_0": {
        "cleanup_policy":        "CLEANUP_POLICY_COMPACT",
        "compression_type":      "COMPRESSION_TYPE_LZ4",
        "file_delete_delay_ms":  "1001",
        "flush_messages":        "1002",
        "flush_ms":              "1003",
        "max_message_bytes":     "1004",
        "min_compaction_lag_ms": "1005",
        "min_insync_replicas":   "1",
        "retention_bytes":       "1006",
        "retention_ms":          "1007"
      }
    }
    """
