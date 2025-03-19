@kafka
@grpc_api
Feature: Update Compute Apache Kafka Cluster Validation
  Background:
    Given default headers
    And we add default feature flag "MDB_KAFKA_CLUSTER"

  Scenario: Update message_max_bytes to invalid value
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
          },
          "kafka_config_2_8": {
            "message_max_bytes": 500000
          }
        },
        "version": "2.8",
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      }
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
        "version": "2.8",
        "zone_id": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_size": "107374182400",
            "disk_type_id": "network-ssd"
          },
          "kafka_config_2_8": {
            "message_max_bytes": "500000"
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": [
            "config_spec.kafka.kafka_config_2_8.message_max_bytes"
        ]
      },
      "config_spec": {
        "kafka": {
          "kafka_config_2_8": {
            "message_max_bytes": 2000000
          }
        }
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "For multi-node kafka cluster, broker setting "replica.fetch.max.bytes" value(1048576) must be equal or greater then broker setting "message.max.bytes" value(2000000) - record log overhead size(12)."

  Scenario: Update ssl_cipher_suites to invalid value
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
          },
          "kafka_config_2_8": {
            "message_max_bytes": 500000
          }
        },
        "version": "2.8",
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      }
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
        "version": "2.8",
        "zone_id": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_size": "107374182400",
            "disk_type_id": "network-ssd"
          },
          "kafka_config_2_8": {
            "message_max_bytes": "500000"
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": [
            "config_spec.kafka.kafka_config_2_8.ssl_cipher_suites"
        ]
      },
      "config_spec": {
        "kafka": {
          "kafka_config_2_8": {
            "ssl_cipher_suites": ["blank", "abc"]
          }
        }
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "these suites are invalid: [abc blank]. List of valid suites: [TLS_AKE_WITH_AES_128_GCM_SHA256,TLS_AKE_WITH_AES_256_GCM_SHA384,TLS_AKE_WITH_CHACHA20_POLY1305_SHA256,TLS_DHE_RSA_WITH_AES_128_CBC_SHA,TLS_DHE_RSA_WITH_AES_128_CBC_SHA256,TLS_DHE_RSA_WITH_AES_128_GCM_SHA256,TLS_DHE_RSA_WITH_AES_256_CBC_SHA,TLS_DHE_RSA_WITH_AES_256_CBC_SHA256,TLS_DHE_RSA_WITH_AES_256_GCM_SHA384,TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256,TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384,TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,TLS_RSA_WITH_AES_128_CBC_SHA,TLS_RSA_WITH_AES_128_CBC_SHA256,TLS_RSA_WITH_AES_128_GCM_SHA256,TLS_RSA_WITH_AES_256_CBC_SHA,TLS_RSA_WITH_AES_256_CBC_SHA256,TLS_RSA_WITH_AES_256_GCM_SHA384]."
