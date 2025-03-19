@kafka
@grpc_api
Feature: Apache Kafka v3.0
  Background:
    Given default headers
    When we add default feature flag "MDB_KAFKA_CLUSTER"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRODUCTION",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "kafka": {
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_size": "107374182400",
            "disk_type_id": "network-ssd"
          },
          "kafka_config_3_0": {
            "compression_type":                "COMPRESSION_TYPE_UNCOMPRESSED",
            "log_flush_interval_messages":     5000,
            "log_flush_interval_ms":           5001,
            "log_flush_scheduler_interval_ms": 5002,
            "log_retention_bytes":             5003,
            "log_retention_hours":             5004,
            "log_retention_minutes":           5005,
            "log_retention_ms":                5006,
            "log_segment_bytes":               5007,
            "log_preallocate":                 false,
            "socket_send_buffer_bytes":        5008,
            "socket_receive_buffer_bytes":     5009,
            "num_partitions":                  5010,
            "default_replication_factor":      1
          }
        },
        "version": "3.0",
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
            "retention_ms":          1007,
            "segment_bytes":         1008,
            "preallocate":           false
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "config": {
        "access":{},
        "brokers_count": "1",
        "version": "3.0",
        "zone_id": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_size": "107374182400",
            "disk_type_id": "network-ssd"
          },
          "kafka_config_3_0": {
            "compression_type":                "COMPRESSION_TYPE_UNCOMPRESSED",
            "log_flush_interval_messages":     "5000",
            "log_flush_interval_ms":           "5001",
            "log_flush_scheduler_interval_ms": "5002",
            "log_retention_bytes":             "5003",
            "log_retention_hours":             "5004",
            "log_retention_minutes":           "5005",
            "log_retention_ms":                "5006",
            "log_segment_bytes":               "5007",
            "log_preallocate":                 false,
            "socket_send_buffer_bytes":        "5008",
            "socket_receive_buffer_bytes":     "5009",
            "num_partitions":                  "5010",
            "default_replication_factor":      "1"
          }
        },
        "zookeeper": {
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        }
      }
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
            "retention_ms":          "1007",
            "segment_bytes":         "1008",
            "preallocate":           false
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


  Scenario: Change kafka config 3.0
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": [
            "config_spec.unmanaged_topics",
            "config_spec.schema_registry",
            "config_spec.kafka.kafka_config_3_0.compression_type",
            "config_spec.kafka.kafka_config_3_0.log_flush_interval_messages",
            "config_spec.kafka.kafka_config_3_0.log_flush_interval_ms",
            "config_spec.kafka.kafka_config_3_0.log_flush_scheduler_interval_ms",
            "config_spec.kafka.kafka_config_3_0.log_retention_bytes",
            "config_spec.kafka.kafka_config_3_0.log_retention_hours",
            "config_spec.kafka.kafka_config_3_0.log_retention_minutes",
            "config_spec.kafka.kafka_config_3_0.log_retention_ms",
            "config_spec.kafka.kafka_config_3_0.log_segment_bytes",
            "config_spec.kafka.kafka_config_3_0.socket_send_buffer_bytes",
            "config_spec.kafka.kafka_config_3_0.socket_receive_buffer_bytes",
            "config_spec.kafka.kafka_config_3_0.auto_create_topics_enable",
            "config_spec.kafka.kafka_config_3_0.num_partitions",
            "config_spec.kafka.kafka_config_3_0.default_replication_factor"
        ]
      },
      "config_spec": {
        "unmanaged_topics": true,
        "schema_registry": true,
        "kafka": {
          "kafka_config_3_0": {
            "compression_type":                "COMPRESSION_TYPE_GZIP",
            "log_flush_interval_messages":     6000,
            "log_flush_interval_ms":           6001,
            "log_flush_scheduler_interval_ms": 6002,
            "log_retention_bytes":             6003,
            "log_retention_hours":             6004,
            "log_retention_minutes":           6005,
            "log_retention_ms":                6006,
            "log_segment_bytes":               6007,
            "socket_send_buffer_bytes":        6008,
            "socket_receive_buffer_bytes":     6009,
            "auto_create_topics_enable":       true,
            "num_partitions":                  6010,
            "default_replication_factor":      1
          }
        }
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Modify Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateClusterMetadata",
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
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "config": {
        "access":{},
        "brokers_count": "1",
        "version": "3.0",
        "zone_id": ["myt", "sas"],
        "unmanaged_topics": true,
        "schema_registry": true,
        "kafka": {
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_size": "107374182400",
            "disk_type_id": "network-ssd"
          },
          "kafka_config_3_0": {
            "compression_type":                "COMPRESSION_TYPE_GZIP",
            "log_flush_interval_messages":     "6000",
            "log_flush_interval_ms":           "6001",
            "log_flush_scheduler_interval_ms": "6002",
            "log_retention_bytes":             "6003",
            "log_retention_hours":             "6004",
            "log_retention_minutes":           "6005",
            "log_retention_ms":                "6006",
            "log_segment_bytes":               "6007",
            "log_preallocate":                 false,
            "socket_send_buffer_bytes":        "6008",
            "socket_receive_buffer_bytes":     "6009",
            "auto_create_topics_enable":       true,
            "num_partitions":                  "6010",
            "default_replication_factor":      "1"
          }
        },
        "zookeeper": {
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        }
      }
    }
    """


  Scenario: Topic config 3.0
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
          "retention_ms":          2007,
          "segment_bytes":         2008,
          "preallocate":           false
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_name": "topic3"
    }
    """
    Then we get gRPC response with body
    """
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
        "retention_ms":          "2007",
        "segment_bytes":         "2008",
        "preallocate":           false
      }
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
          "retention_ms":          3007,
          "segment_bytes":         3008
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
              "topic_spec.topic_config_3_0.retention_ms",
              "topic_spec.topic_config_3_0.segment_bytes"
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
        "retention_ms":          "3007",
        "segment_bytes":         "3008",
        "preallocate":           false
      }
    }
    """
