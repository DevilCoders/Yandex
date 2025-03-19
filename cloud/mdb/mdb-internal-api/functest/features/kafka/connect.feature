@kafka
@grpc_api
Feature: Managed Kafka Connect connectors feature
  Background:
    Given health response
    """
    {
      "clusters": [
        {
          "cid": "cid1",
          "status": "Alive"
        }
      ]
    }
    """
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

  Scenario: Create and get connectors works correctly
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_spec": {
        "name": "cluster-migration",
        "tasks_max": 3,
        "connector_config_mirrormaker": {
          "topics": "payload-*",
          "replication_factor": 1,
          "source_cluster": {
            "alias": "source",
            "this_cluster": {}
          },
          "target_cluster": {
            "alias": "target",
            "external_cluster": {
              "bootstrap_servers": "rc1c-ixxxxxxxxxxxehbpid4.db.yandex.net:9091",
              "sasl_username": "external-broker-user",
              "sasl_password": "external-broker-password",
              "sasl_mechanism": "SCRAM-SHA-512",
              "security_protocol": "SASL_SSL",
              "ssl_truststore_certificates": "-----CERT-----"
            }
          }
        },
        "properties": {
          "offset-syncs.topic.replication.factor": "1"
        }
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create managed connector on Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateConnectorMetadata",
        "cluster_id": "cid1",
        "connector_name": "cluster-migration"
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "name": "cluster-migration",
      "cluster_id": "cid1",
      "tasks_max": "3",
      "connector_config_mirrormaker": {
        "topics": "payload-*",
        "replication_factor": "1",
        "source_cluster": {
          "alias": "source",
          "this_cluster": {}
        },
        "target_cluster": {
          "alias": "target",
          "external_cluster": {
            "bootstrap_servers": "rc1c-ixxxxxxxxxxxehbpid4.db.yandex.net:9091",
            "sasl_username": "external-broker-user",
            "sasl_mechanism": "SCRAM-SHA-512",
            "security_protocol": "SASL_SSL"
          }
        }
      },
      "properties": {
        "offset-syncs.topic.replication.factor": "1"
      }
    }
    """

  Scenario: List and delete handlers works correctly
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "connectors": []
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_spec": {
        "name": "cluster-migration",
        "tasks_max": 3,
        "connector_config_mirrormaker": {
          "topics": "payload-*",
          "replication_factor": 1,
          "source_cluster": {
            "alias": "source",
            "this_cluster": {}
          },
          "target_cluster": {
            "alias": "target",
            "external_cluster": {
              "bootstrap_servers": "asdsad-asdsad-asdas2.kafka-egor-yegeraskin13-d951.aivencloud7.com:12373",
              "sasl_username": "external-broker-user",
              "sasl_password": "external-broker-password",
              "sasl_mechanism": "SCRAM-SHA-512",
              "security_protocol": "SASL_SSL",
              "ssl_truststore_certificates": "-----CERT-----"
            }
          }
        },
        "properties": {
          "offset-syncs.topic.replication.factor": "1"
        }
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create managed connector on Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateConnectorMetadata",
        "cluster_id": "cid1",
        "connector_name": "cluster-migration"
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
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "connectors": [
        {
          "name": "cluster-migration",
          "cluster_id": "cid1",
          "tasks_max": "3",
          "connector_config_mirrormaker": {
            "topics": "payload-*",
            "replication_factor": "1",
            "source_cluster": {
              "alias": "source",
              "this_cluster": {}
            },
            "target_cluster": {
              "alias": "target",
              "external_cluster": {
                "bootstrap_servers": "asdsad-asdsad-asdas2.kafka-egor-yegeraskin13-d951.aivencloud7.com:12373",
                "sasl_username": "external-broker-user",
                "sasl_mechanism": "SCRAM-SHA-512",
                "security_protocol": "SASL_SSL"
              }
            }
          },
          "properties": {
            "offset-syncs.topic.replication.factor": "1"
          }
        }
      ]
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete managed connector on Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.DeleteConnectorMetadata",
        "cluster_id": "cid1",
        "connector_name": "cluster-migration"
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
    When we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "connectors": []
    }
    """

  Scenario: Pause and resume handlers works correctly
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_spec": {
        "name": "cluster-migration",
        "tasks_max": 3,
        "connector_config_mirrormaker": {
          "topics": "payload-*",
          "replication_factor": 1,
          "source_cluster": {
            "alias": "source",
            "this_cluster": {}
          },
          "target_cluster": {
            "alias": "target",
            "external_cluster": {
              "bootstrap_servers": "some-external-broker:9091",
              "sasl_username": "external-broker-user",
              "sasl_password": "external-broker-password",
              "sasl_mechanism": "SCRAM-SHA-512",
              "security_protocol": "SASL_SSL",
              "ssl_truststore_certificates": "-----CERT-----"
            }
          }
        },
        "properties": {
          "offset-syncs.topic.replication.factor": "1"
        }
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create managed connector on Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateConnectorMetadata",
        "cluster_id": "cid1",
        "connector_name": "cluster-migration"
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "name": "cluster-migration",
      "cluster_id": "cid1",
      "tasks_max": "3",
      "connector_config_mirrormaker": {
        "topics": "payload-*",
        "replication_factor": "1",
        "source_cluster": {
          "alias": "source",
          "this_cluster": {}
        },
        "target_cluster": {
          "alias": "target",
          "external_cluster": {
            "bootstrap_servers": "some-external-broker:9091",
            "sasl_username": "external-broker-user",
            "sasl_mechanism": "SCRAM-SHA-512",
            "security_protocol": "SASL_SSL"
          }
        }
      },
      "properties": {
        "offset-syncs.topic.replication.factor": "1"
      }
    }
    """
    When we "Pause" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Pause managed connector on Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.PauseConnectorMetadata",
        "cluster_id": "cid1",
        "connector_name": "cluster-migration"
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "name": "cluster-migration",
      "cluster_id": "cid1",
      "tasks_max": "3",
      "connector_config_mirrormaker": {
        "topics": "payload-*",
        "replication_factor": "1",
        "source_cluster": {
          "alias": "source",
          "this_cluster": {}
        },
        "target_cluster": {
          "alias": "target",
          "external_cluster": {
            "bootstrap_servers": "some-external-broker:9091",
            "sasl_username": "external-broker-user",
            "sasl_mechanism": "SCRAM-SHA-512",
            "security_protocol": "SASL_SSL"
          }
        }
      },
      "properties": {
        "offset-syncs.topic.replication.factor": "1"
      }
    }
    """
    When we "Resume" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Resume managed connector on Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id4",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.ResumeConnectorMetadata",
        "cluster_id": "cid1",
        "connector_name": "cluster-migration"
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "name": "cluster-migration",
      "cluster_id": "cid1",
      "tasks_max": "3",
      "connector_config_mirrormaker": {
        "topics": "payload-*",
        "replication_factor": "1",
        "source_cluster": {
          "alias": "source",
          "this_cluster": {}
        },
        "target_cluster": {
          "alias": "target",
          "external_cluster": {
            "bootstrap_servers": "some-external-broker:9091",
            "sasl_username": "external-broker-user",
            "sasl_mechanism": "SCRAM-SHA-512",
            "security_protocol": "SASL_SSL"
          }
        }
      },
      "properties": {
        "offset-syncs.topic.replication.factor": "1"
      }
    }
    """

  Scenario: Create mirrormaker without source fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_spec": {
        "name": "cluster-migration",
        "tasks_max": 3,
        "connector_config_mirrormaker": {
          "topics": "payload-*",
          "replication_factor": 1,
          "target_cluster": {
            "alias": "target",
            "external_cluster": {
              "bootstrap_servers": "some-external-broker:9091",
              "sasl_username": "external-broker-user",
              "sasl_password": "external-broker-password",
              "sasl_mechanism": "SCRAM-SHA-512",
              "security_protocol": "SASL_SSL",
              "ssl_truststore_certificates": "-----CERT-----"
            }
          }
        }
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "cannot create connector without source or target type - this cluster"

  Scenario: Create mirrormaker with external cluster with empty "bootstrap_server" property
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_spec": {
        "name": "cluster-migration1",
        "tasks_max": 3,
        "connector_config_mirrormaker": {
          "topics": "payload-*",
          "replication_factor": 1,
          "source_cluster": {
            "alias": "source",
            "this_cluster": {}
          },
          "target_cluster": {
            "alias": "target",
            "external_cluster": {
              "bootstrap_servers": "",
              "sasl_username": "external-broker-user",
              "sasl_password": "external-broker-password",
              "sasl_mechanism": "SCRAM-SHA-512",
              "security_protocol": "SASL_SSL",
              "ssl_truststore_certificates": "-----CERT-----"
            }
          }
        },
        "properties": {
          "offset-syncs.topic.replication.factor": "1"
        }
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "bootstrap servers cannot be empty"

  Scenario: Create and get connector with ip address in bootstrap_servers works correctly
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_spec": {
        "name": "cluster-migration",
        "tasks_max": 3,
        "connector_config_mirrormaker": {
          "topics": "payload-*",
          "replication_factor": 1,
          "source_cluster": {
            "alias": "source",
            "this_cluster": {}
          },
          "target_cluster": {
            "alias": "target",
            "external_cluster": {
              "bootstrap_servers": "255.255.255.255:9091",
              "sasl_username": "external-broker-user",
              "sasl_password": "external-broker-password",
              "sasl_mechanism": "SCRAM-SHA-512",
              "security_protocol": "SASL_SSL",
              "ssl_truststore_certificates": "-----CERT-----"
            }
          }
        },
        "properties": {
          "offset-syncs.topic.replication.factor": "1"
        }
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create managed connector on Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateConnectorMetadata",
        "cluster_id": "cid1",
        "connector_name": "cluster-migration"
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "name": "cluster-migration",
      "cluster_id": "cid1",
      "tasks_max": "3",
      "connector_config_mirrormaker": {
        "topics": "payload-*",
        "replication_factor": "1",
        "source_cluster": {
          "alias": "source",
          "this_cluster": {}
        },
        "target_cluster": {
          "alias": "target",
          "external_cluster": {
            "bootstrap_servers": "255.255.255.255:9091",
            "sasl_username": "external-broker-user",
            "sasl_mechanism": "SCRAM-SHA-512",
            "security_protocol": "SASL_SSL"
          }
        }
      },
      "properties": {
        "offset-syncs.topic.replication.factor": "1"
      }
    }
    """

  Scenario: Create, get and update s3 sink connector works correctly
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_spec": {
        "name": "s3-sink",
        "tasks_max": 3,
        "connector_config_s3_sink": {
          "s3_connection": {
            "bucket_name": "egor.geraskin.bucket.3",
            "external_s3": {
              "access_key_id": "access_id",
              "secret_access_key": "secret",
              "endpoint": "storage.yandexcloud.net",
              "region": "us-east-1"
            }
          },
          "topics": "payload-*",
          "file_compression_type": "gzip",
          "file_max_records": "1"
        },
        "properties": {
          "offset-syncs.topic.replication.factor": "1"
        }
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create managed connector on Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateConnectorMetadata",
        "cluster_id": "cid1",
        "connector_name": "s3-sink"
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "s3-sink"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "name": "s3-sink",
      "cluster_id": "cid1",
      "tasks_max": "3",
      "connector_config_s3_sink": {
        "topics": "payload-*",
        "file_compression_type": "gzip",
        "file_max_records": "1",
        "s3_connection": {
          "bucket_name": "egor.geraskin.bucket.3",
          "external_s3": {
            "access_key_id": "access_id",
            "endpoint": "storage.yandexcloud.net",
            "region": "us-east-1"
          }
        }
      },
      "properties": {
        "offset-syncs.topic.replication.factor": "1"
      }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "s3-sink",
      "updateMask": {
          "paths": [
              "connector_spec.tasks_max",
              "connector_spec.connector_config_s3_sink.topics",
              "connector_spec.connector_config_s3_sink.file_max_records",
              "connector_spec.connector_config_s3_sink.s3_connection.bucket_name",
              "connector_spec.connector_config_s3_sink.s3_connection.external_s3.access_key_id",
              "connector_spec.connector_config_s3_sink.s3_connection.external_s3.secret_access_key",
              "connector_spec.connector_config_s3_sink.s3_connection.external_s3.endpoint",
              "connector_spec.connector_config_s3_sink.s3_connection.external_s3.region",
              "connector_spec.properties.offset-syncs.topic.replication.factor",
              "connector_spec.properties.new_property_key"
          ]
      },
      "connector_spec": {
        "tasks_max": 2,
        "connector_config_s3_sink": {
          "topics": "topic1,topic2",
          "file_max_records": 100,
          "s3_connection": {
            "bucket_name": "new.bucket.name",
            "external_s3": {
              "access_key_id": "new_access_key_id",
              "secret_access_key": "new_secret_access_key",
              "endpoint": "new.endpoint.ru",
              "region": "ru-central-1"
            }
          }
        },
        "properties": {
          "offset-syncs.topic.replication.factor": "2",
          "new_property_key": "new_property_value"
        }
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Update managed connector on Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateConnectorMetadata",
        "cluster_id": "cid1",
        "connector_name": "s3-sink"
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "s3-sink"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
      "name": "s3-sink",
      "cluster_id": "cid1",
      "tasks_max": "2",
      "connector_config_s3_sink": {
        "topics": "topic1,topic2",
        "file_compression_type": "gzip",
        "file_max_records": "100",
        "s3_connection": {
          "bucket_name": "new.bucket.name",
          "external_s3": {
            "access_key_id": "new_access_key_id",
            "endpoint": "new.endpoint.ru",
            "region": "ru-central-1"
          }
        }
      },
      "properties": {
        "offset-syncs.topic.replication.factor": "2",
        "new_property_key": "new_property_value"
      }
    }
    """
