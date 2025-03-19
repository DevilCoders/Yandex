@grpc_api
Feature: Restore ClickHouse cluster from backup with access params

  Background:
    Given default headers
    And s3 response
    """
    {
        "Contents": [
            {
                "Key": "ch_backup/cid1/0/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:00",
                        "end_time": "1970-01-01 00:00:01",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            },
            {
                "Key": "ch_backup/cid1/1/backup_struct.json",
                "LastModified": 1,
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "state": "created"
                    }
                }
            }
        ]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "access": {
                "web_sql": false,
                "data_lens": false,
                "metrika": false,
                "serverless": true,
                "data_transfer": true,
                "yandex_query": true
            },
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                },
                "config": {
                    "geobase_uri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                    "kafka": {
                        "security_protocol": "SECURITY_PROTOCOL_SSL",
                        "sasl_mechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                        "sasl_username": "kafka_username",
                        "sasl_password": "kafka_pass"
                    },
                    "kafka_topics": [{
                        "name": "kafka_topic",
                        "settings": {
                            "security_protocol": "SECURITY_PROTOCOL_SSL",
                            "sasl_mechanism": "SASL_MECHANISM_GSSAPI",
                            "sasl_username": "topic_username",
                            "sasl_password": "topic_pass"
                        }
                    }],
                    "rabbitmq": {
                        "username": "test_user",
                        "password": "test_password"
                    }
                }
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        },{
            "type": "ZOOKEEPER",
            "zone_id": "iva"
        }],
        "description": "test cluster"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.MlModelService" with data
    """
    {
        "cluster_id": "cid1",
        "ml_model_name": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.FormatSchemaService" with data
    """
    {
        "cluster_id": "cid1",
        "format_schema_name": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id3" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"

  @events
  Scenario: Restoring from backup to original folder works
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
            "access": {
                    "web_sql": true,
                    "data_lens": true,
                    "metrika": true,
                    "serverless": true,
                    "data_transfer": true,
                    "yandex_query": true
            },
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }],
        "backup_id": "cid1:0"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create new ClickHouse cluster from the backup",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.RestoreClusterMetadata",
            "backup_id": "cid1:0",
            "cluster_id": "cid2"
        }
    }
    """
    And for "worker_task_id4" exists "yandex.cloud.events.mdb.clickhouse.RestoreCluster" event with
    """
    {
        "details": {
            "backup_id": "cid1:0",
            "cluster_id": "cid2"
        }
    }
    """
    Given "worker_task_id4" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id4"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_at": "**IGNORE**",
        "created_by": "user",
        "description": "Create new ClickHouse cluster from the backup",
        "done": true,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.RestoreClusterMetadata",
            "backup_id": "cid1:0",
            "cluster_id": "cid2"
        },
        "modified_at": "**IGNORE**",
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
            "id": "cid2",
            "name": "test_restored",
            "description": "Restored from backup(s): 0",
            "folder_id": "folder1",
            "status": "RUNNING",
            "created_at": "**IGNORE**",
            "environment": "PRESTABLE",
            "maintenance_window": "**IGNORE**",
            "monitoring": "**IGNORE**",
            "config": "**IGNORE**"
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2"
    }
    """
    Then we get gRPC response OK
    And gRPC response body at path "$.config.access" contains
    """
    {
      "web_sql": true,
      "data_lens": true,
      "metrika": true,
      "serverless": true,
      "data_transfer": true,
      "yandex_query": true
    }
    """
