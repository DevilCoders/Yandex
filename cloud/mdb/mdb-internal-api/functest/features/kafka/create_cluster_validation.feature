@kafka
@grpc_api
Feature: Create Compute Apache Kafka Cluster Validation
  Background:
    Given default headers
    And we add default feature flag "MDB_KAFKA_CLUSTER"

  Scenario: Cluster creation doesn't work with not valid version
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
        "zoneId": ["myt", "sas"],
        "version": "1.1"
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "unknown Apache Kafka version"

  Scenario: Cluster creation doesn't work too big replication factor
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
        "zoneId": ["myt", "sas"],
        "kafka": {
          "kafka_config_2_8": {
            "log_segment_bytes": 1048576
          }
        }
      },
      "topic_specs": [
        {
          "name": "test",
          "partitions": 12,
          "replication_factor": 3
        }
      ]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "topic "test" has too big replication factor: 3. maximum is brokers quantity: 2"

  Scenario: Cluster creation doesn't work without subnets when needed
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network2",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "vla"]
      }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "geo "vla" has multiple subnets, need to specify one"

  Scenario: Cluster creation doesn't work with subnets in wrong zones
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network2",
      "subnetId": ["network2-myt","network2-vla"],
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "no subnets in geo "sas""

  Scenario: Cluster creation doesn't work with wrong cluster name
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "wrong name",
      "description": "test cluster",
      "networkId": "network2",
      "subnetId": ["network2-myt","network2-sas"],
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 10737418240
          }
        }
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "cluster name "wrong name" has invalid symbols"

  Scenario: Cluster creation doesn't work with wrong topic name
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network2",
      "subnetId": ["network2-myt","network2-sas"],
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 32212254720
          }
        }
      },
      "topic_specs": [
        {
          "name": "wrong topic",
          "partitions": 12,
          "replication_factor": 1
        }
      ]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "topic name "wrong topic" has invalid symbols"

  Scenario: Cluster creation doesn't work with small disk
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
        "zoneId": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 10737418240
          }
        }
      },
      "topic_specs": [
        {
          "name": "test",
          "partitions": 12,
          "replication_factor": 1
        }
      ]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "disk size must be at least 12884901888 according to topics partitions number and replication factor but size is 10737418240"

  Scenario: Cluster creation doesn't work with empty resource preset
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
        "zoneId": ["myt", "sas"],
        "kafka": {
          "resources": {
            "diskTypeId": "network-ssd",
            "diskSize": 10737418240
          }
        }
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "resource preset must be specified"

  Scenario: Cluster creation doesn't work with empty disk type
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
        "zoneId": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskSize": 10737418240
          }
        }
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "disk type must be specified"

  Scenario: Cluster creation doesn't work when config version differs with cluster version
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
          "kafka_config_2_1": {
            "compression_type":                "COMPRESSION_TYPE_UNCOMPRESSED",
            "log_flush_interval_messages":     5000
          }
        },
        "version": "3.0",
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "the config version 2.1 does not match the cluster version 3.0"

  Scenario: Cluster creation doesn't work when version is deprecated
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "version": "2.6",
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "version 2.6 is deprecated"

  Scenario: Unable to create single-broker cluster if zookeeper config is specified
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "networkId": "network1",
      "configSpec": {
        "zookeeper": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 10737418240
          }
        },
        "brokersCount": 1,
        "zoneId": ["myt"]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "zookeeper resources should not be specified for single-broker cluster"

  Scenario: Unable to create 3.2 in stable ennvironment
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRODUCTION",
      "name": "test",
      "networkId": "network1",
      "configSpec": {
        "version": "3.2",
        "brokersCount": 1,
        "zoneId": ["myt"]
      }
    }
    """
    Then we get gRPC response error with code UNIMPLEMENTED and message "version 3.2 can be created only in prestable environment"

  Scenario: Cluster creation doesn't work with not valid message_max_bytes
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
            "replica_fetch_max_bytes": 5011
          }
        },
        "version": "2.8",
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "For multi-node kafka cluster, broker setting "replica.fetch.max.bytes" value(5011) must be equal or greater then broker setting "message.max.bytes" value(1048588) - record log overhead size(12)."

  Scenario: Cluster creation doesn't work with not valid ssl_cipher_suites
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
            "ssl_cipher_suites": ["TLS_AKE_WITH_AES_128_GCM_SHA2566", "TLS_RSA_WITH_AES_256_CBC_SHA256"]
          }
        },
        "version": "2.8",
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "these suites are invalid: [TLS_AKE_WITH_AES_128_GCM_SHA2566]. List of valid suites: [TLS_AKE_WITH_AES_128_GCM_SHA256,TLS_AKE_WITH_AES_256_GCM_SHA384,TLS_AKE_WITH_CHACHA20_POLY1305_SHA256,TLS_DHE_RSA_WITH_AES_128_CBC_SHA,TLS_DHE_RSA_WITH_AES_128_CBC_SHA256,TLS_DHE_RSA_WITH_AES_128_GCM_SHA256,TLS_DHE_RSA_WITH_AES_256_CBC_SHA,TLS_DHE_RSA_WITH_AES_256_CBC_SHA256,TLS_DHE_RSA_WITH_AES_256_GCM_SHA384,TLS_DHE_RSA_WITH_CHACHA20_POLY1305_SHA256,TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA,TLS_ECDHE_RSA_WITH_AES_128_CBC_SHA256,TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA,TLS_ECDHE_RSA_WITH_AES_256_CBC_SHA384,TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,TLS_RSA_WITH_AES_128_CBC_SHA,TLS_RSA_WITH_AES_128_CBC_SHA256,TLS_RSA_WITH_AES_128_GCM_SHA256,TLS_RSA_WITH_AES_256_CBC_SHA,TLS_RSA_WITH_AES_256_CBC_SHA256,TLS_RSA_WITH_AES_256_GCM_SHA384]."

  Scenario: Cluster creation doesn't work with not valid offsets_retention_minutes
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
            "offsets_retention_minutes": 0
          }
        },
        "version": "2.8",
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "wrong value for offsets.retention.minutes: 0"

Scenario: Cluster creation doesn't work on the 1st gen
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
        "zoneId": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s1.compute.1",
            "diskSize": 10737418240,
            "disk_type_id": "network-ssd"
          }
        }
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "creating hosts on Intel Broadwell is impossible"
