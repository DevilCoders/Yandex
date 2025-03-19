@kafka
@grpc_api
Feature: Managed Kafka Connect connectors update feature
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


  Scenario: Update connector full change works correctly
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration",
      "updateMask": {
          "paths": [
              "connector_spec.tasks_max",
              "connector_spec.connector_config_mirrormaker.topics",
              "connector_spec.connector_config_mirrormaker.replication_factor",
              "connector_spec.connector_config_mirrormaker.source_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.bootstrap_servers",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.security_protocol",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_mechanism",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_username",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_password",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.ssl_truststore_certificates",
              "connector_spec.properties.offset-syncs.topic.replication.factor",
              "connector_spec.properties.new_property_key"
          ]
      },
      "connector_spec": {
        "tasks_max": 2,
        "connector_config_mirrormaker": {
          "topics": "topic1,topic2",
          "replication_factor": 2,
          "source_cluster": {
            "alias": "source_alias"
          },
          "target_cluster": {
            "alias": "target_alias",
            "external_cluster": {
              "bootstrap_servers": "rc1c-tlna06henahos0fj.db.yandex.net:9091,rc1c-rrkmpvc2rg3porhh.db.yandex.net:9091",
              "security_protocol": "sasl_ssl",
              "sasl_mechanism": "scram-sha-512",
              "sasl_username": "sasl_username_test",
              "sasl_password": "new_test_password",
              "ssl_truststore_certificates": "-----NEW-CERT-----"
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
      "tasks_max": "2",
      "connector_config_mirrormaker": {
        "topics": "topic1,topic2",
        "replication_factor": "2",
        "source_cluster": {
          "alias": "source_alias",
          "this_cluster": {}
        },
        "target_cluster": {
          "alias": "target_alias",
          "external_cluster": {
            "bootstrap_servers": "rc1c-tlna06henahos0fj.db.yandex.net:9091,rc1c-rrkmpvc2rg3porhh.db.yandex.net:9091",
            "security_protocol": "SASL_SSL",
            "sasl_mechanism": "SCRAM-SHA-512",
            "sasl_username": "sasl_username_test"
          }
        }
      },
      "properties": {
        "offset-syncs.topic.replication.factor": "2",
        "new_property_key": "new_property_value"
      }
    }
    """

  Scenario: Update connector properties overwrite works correctly
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration",
      "updateMask": {
          "paths": [
              "connector_spec.properties"
          ]
      },
      "connector_spec": {
        "properties": {
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
            "bootstrap_servers": "rc1c-ixxxxxxxxxxxehbpid4.db.yandex.net:9091",
            "security_protocol": "SASL_SSL",
            "sasl_mechanism": "SCRAM-SHA-512",
            "sasl_username": "external-broker-user"
          }
        }
      },
      "properties": {
        "new_property_key": "new_property_value"
      }
    }
    """

  Scenario: Update connector without cluster_id follows error
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "connector_name": "cluster-migration",
      "updateMask": {
          "paths": [
              "connector_spec.tasks_max",
              "connector_spec.connector_config_mirrormaker.topics",
              "connector_spec.connector_config_mirrormaker.replication_factor",
              "connector_spec.connector_config_mirrormaker.source_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.bootstrap_servers",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.security_protocol",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_mechanism",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_username",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_password",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.ssl_truststore_certificates",
              "connector_spec.properties.offset-syncs.topic.replication.factor",
              "connector_spec.properties.new_property_key"
          ]
      },
      "connector_spec": {
        "tasks_max": 2,
        "connector_config_mirrormaker": {
          "topics": "topic1,topic2",
          "replication_factor": 2,
          "source_cluster": {
            "alias": "source_alias"
          },
          "target_cluster": {
            "alias": "target_alias",
            "external_cluster": {
              "bootstrap_servers": "rc1c-tlna06henahos0fj.db.yandex.net:9091,rc1c-rrkmpvc2rg3porhh.db.yandex.net:9091",
              "security_protocol": "sasl_ssl",
              "sasl_mechanism": "scram-sha-512",
              "sasl_username": "sasl_username_test",
              "sasl_password": "new_test_password",
              "ssl_truststore_certificates": "-----NEW-CERT-----"
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "missing required argument: cluster_id"

  Scenario: Update connector without connector name follows error
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "updateMask": {
          "paths": [
              "connector_spec.tasks_max",
              "connector_spec.connector_config_mirrormaker.topics",
              "connector_spec.connector_config_mirrormaker.replication_factor",
              "connector_spec.connector_config_mirrormaker.source_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.bootstrap_servers",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.security_protocol",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_mechanism",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_username",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_password",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.ssl_truststore_certificates",
              "connector_spec.properties.offset-syncs.topic.replication.factor",
              "connector_spec.properties.new_property_key"
          ]
      },
      "connector_spec": {
        "tasks_max": 2,
        "connector_config_mirrormaker": {
          "topics": "topic1,topic2",
          "replication_factor": 2,
          "source_cluster": {
            "alias": "source_alias"
          },
          "target_cluster": {
            "alias": "target_alias",
            "external_cluster": {
              "bootstrap_servers": "rc1c-tlna06henahos0fj.db.yandex.net:9091,rc1c-rrkmpvc2rg3porhh.db.yandex.net:9091",
              "security_protocol": "sasl_ssl",
              "sasl_mechanism": "scram-sha-512",
              "sasl_username": "sasl_username_test",
              "sasl_password": "new_test_password",
              "ssl_truststore_certificates": "-----NEW-CERT-----"
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "missing required argument: connector_name"

  Scenario: Update connector with no fields to update follows error
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration",
      "updateMask": {
          "paths": []
      },
      "connector_spec": {}
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "no fields to change in update connector request"

  Scenario: Update connector with junk fields follows error
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration",
      "updateMask": {
          "paths": [
              "junk_field",
              "connector_spec.tasks_max",
              "connector_spec.connector_config_mirrormaker.topics",
              "connector_spec.connector_config_mirrormaker.replication_factor",
              "connector_spec.connector_config_mirrormaker.source_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.bootstrap_servers",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.security_protocol",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_mechanism",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_username",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_password",
              "connector_spec.properties.offset-syncs.topic.replication.factor",
              "connector_spec.properties.new_property_key"
          ]
      },
      "connector_spec": {
        "tasks_max": 2,
        "connector_config_mirrormaker": {
          "topics": "topic1,topic2",
          "replication_factor": 2,
          "source_cluster": {
            "alias": "source_alias"
          },
          "target_cluster": {
            "alias": "target_alias",
            "external_cluster": {
              "bootstrap_servers": "rc1c-tlna06henahos0fj.db.yandex.net:9091,rc1c-rrkmpvc2rg3porhh.db.yandex.net:9091",
              "security_protocol": "sasl_ssl",
              "sasl_mechanism": "scram-sha-512",
              "sasl_username": "sasl_username_test",
              "sasl_password": "new_test_password"
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "unknown field paths: junk_field"

  Scenario: Update connector without not existing connector name follows error
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration-2",
      "updateMask": {
          "paths": [
              "connector_spec.tasks_max",
              "connector_spec.connector_config_mirrormaker.topics",
              "connector_spec.connector_config_mirrormaker.replication_factor",
              "connector_spec.connector_config_mirrormaker.source_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.bootstrap_servers",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.security_protocol",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_mechanism",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_username",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_password",
              "connector_spec.properties.offset-syncs.topic.replication.factor",
              "connector_spec.properties.new_property_key"
          ]
      },
      "connector_spec": {
        "tasks_max": 2,
        "connector_config_mirrormaker": {
          "topics": "topic1,topic2",
          "replication_factor": 2,
          "source_cluster": {
            "alias": "source_alias"
          },
          "target_cluster": {
            "alias": "target_alias",
            "external_cluster": {
              "bootstrap_servers": "rc1c-tlna06henahos0fj.db.yandex.net:9091,rc1c-rrkmpvc2rg3porhh.db.yandex.net:9091",
              "security_protocol": "sasl_ssl",
              "sasl_mechanism": "scram-sha-512",
              "sasl_username": "sasl_username_test",
              "sasl_password": "new_test_password"
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
    Then we get gRPC response error with code NOT_FOUND and message "connector "cluster-migration-2" not found"

  Scenario: Update connector without any changes follows error
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration",
      "updateMask": {
          "paths": [
              "connector_spec.tasks_max",
              "connector_spec.connector_config_mirrormaker.topics",
              "connector_spec.connector_config_mirrormaker.replication_factor",
              "connector_spec.connector_config_mirrormaker.source_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.bootstrap_servers",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.security_protocol",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_mechanism",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_username",
              "connector_spec.properties.offset-syncs.topic.replication.factor"
          ]
      },
      "connector_spec": {
        "tasks_max": 3,
        "connector_config_mirrormaker": {
          "topics": "payload-*",
          "replication_factor": 1,
          "source_cluster": {
            "alias": "source"
          },
          "target_cluster": {
            "alias": "target",
            "external_cluster": {
              "bootstrap_servers": "rc1c-ixxxxxxxxxxxehbpid4.db.yandex.net:9091",
              "security_protocol": "sasl_ssl",
              "sasl_mechanism": "scram-sha-512",
              "sasl_username": "external-broker-user"
            }
          }
        },
        "properties": {
          "offset-syncs.topic.replication.factor": "1"
        }
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "no fields to update"

  Scenario: Update connector with too small tasks_max follows error
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration",
      "updateMask": {
          "paths": [
              "connector_spec.tasks_max",
              "connector_spec.connector_config_mirrormaker.topics",
              "connector_spec.connector_config_mirrormaker.replication_factor",
              "connector_spec.connector_config_mirrormaker.source_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.bootstrap_servers",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.security_protocol",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_mechanism",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_username",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_password",
              "connector_spec.properties.offset-syncs.topic.replication.factor",
              "connector_spec.properties.new_property_key"
          ]
      },
      "connector_spec": {
        "tasks_max": 0,
        "connector_config_mirrormaker": {
          "topics": "topic1,topic2",
          "replication_factor": 2,
          "source_cluster": {
            "alias": "source_alias"
          },
          "target_cluster": {
            "alias": "target_alias",
            "external_cluster": {
              "bootstrap_servers": "rc1c-tlna06henahos0fj.db.yandex.net:9091,rc1c-rrkmpvc2rg3porhh.db.yandex.net:9091",
              "security_protocol": "sasl_ssl",
              "sasl_mechanism": "scram-sha-512",
              "sasl_username": "sasl_username_test",
              "sasl_password": "new_test_password"
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "connector "cluster-migration" has too small tasks.max setting's value: 0. minimum is 1"

  Scenario: Update connector to have two this_clusters as mirrormaker points follows error
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration",
      "updateMask": {
          "paths": []
      },
      "connector_spec": {
        "connector_config_mirrormaker": {
          "target_cluster": {
            "this_cluster": {}
          }
        }
      }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "cannot create connector with source and target type - this cluster"

  Scenario: Update connector without this_cluster follows error
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration",
      "updateMask": {
          "paths": [
              "connector_spec.tasks_max",
              "connector_spec.connector_config_mirrormaker.topics",
              "connector_spec.connector_config_mirrormaker.replication_factor",
              "connector_spec.connector_config_mirrormaker.source_cluster.alias",
              "connector_spec.connector_config_mirrormaker.source_cluster.external_cluster.bootstrap_servers",
              "connector_spec.connector_config_mirrormaker.source_cluster.external_cluster.security_protocol",
              "connector_spec.connector_config_mirrormaker.target_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.bootstrap_servers",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.security_protocol",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_mechanism",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_username",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_password",
              "connector_spec.properties.offset-syncs.topic.replication.factor",
              "connector_spec.properties.new_property_key"
          ]
      },
      "connector_spec": {
        "tasks_max": 2,
        "connector_config_mirrormaker": {
          "topics": "topic1,topic2",
          "replication_factor": 2,
          "source_cluster": {
            "alias": "source_alias",
            "external_cluster": {
              "bootstrap_servers": "rc1c-tlna06henahos0fj.db.yandex.net:9091,rc1c-rrkmpvc2rg3porhh.db.yandex.net:9091",
              "security_protocol": "plaintext"
            }
          },
          "target_cluster": {
            "alias": "target_alias",
            "external_cluster": {
              "bootstrap_servers": "rc1c-tlna06henahos0fj.db.yandex.net:9091,rc1c-rrkmpvc2rg3porhh.db.yandex.net:9091",
              "security_protocol": "sasl_ssl",
              "sasl_mechanism": "scram-sha-512",
              "sasl_username": "sasl_username_test",
              "sasl_password": "new_test_password"
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "cannot create connector without source or target type - this cluster"

  Scenario: Update connector with too small replication factor follows error
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ConnectorService" with data
    """
    {
      "cluster_id": "cid1",
      "connector_name": "cluster-migration",
      "updateMask": {
          "paths": [
              "connector_spec.tasks_max",
              "connector_spec.connector_config_mirrormaker.topics",
              "connector_spec.connector_config_mirrormaker.replication_factor",
              "connector_spec.connector_config_mirrormaker.source_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.alias",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.bootstrap_servers",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.security_protocol",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_mechanism",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_username",
              "connector_spec.connector_config_mirrormaker.target_cluster.external_cluster.sasl_password",
              "connector_spec.properties.offset-syncs.topic.replication.factor",
              "connector_spec.properties.new_property_key"
          ]
      },
      "connector_spec": {
        "tasks_max": 2,
        "connector_config_mirrormaker": {
          "topics": "topic1,topic2",
          "replication_factor": 0,
          "source_cluster": {
            "alias": "source_alias"
          },
          "target_cluster": {
            "alias": "target_alias",
            "external_cluster": {
              "bootstrap_servers": "rc1c-tlna06henahos0fj.db.yandex.net:9091,rc1c-rrkmpvc2rg3porhh.db.yandex.net:9091",
              "security_protocol": "sasl_ssl",
              "sasl_mechanism": "scram-sha-512",
              "sasl_username": "sasl_username_test",
              "sasl_password": "new_test_password"
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "connector "cluster-migration" has too small replication factor: 0. minimum is 1"
