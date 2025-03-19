Feature: Restore ClickHouse cluster from backup

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
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "networkId": "network1",
        "configSpec": {
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
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                },
                "config": {
                    "geobaseUri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                    "kafka": {
                        "securityProtocol": "SECURITY_PROTOCOL_SSL",
                        "saslMechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                        "saslUsername": "kafka_username",
                        "saslPassword": "kafka_pass"
                    },
                    "kafkaTopics": [
                        {
                            "name": "kafka_topic",
                            "settings": {
                                "securityProtocol": "SECURITY_PROTOCOL_SSL",
                                "saslMechanism": "SASL_MECHANISM_GSSAPI",
                                "saslUsername": "topic_username",
                                "saslPassword": "topic_pass"
                            }
                        }
                    ],
                    "rabbitmq": {
                        "username": "test_user",
                        "password": "test_password"
                    }
                }
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }],
        "description": "test cluster"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "network_id": "network1",
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
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
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
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
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
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "description": "test restored",
        "environment": "PRESTABLE",
        "configSpec": {
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
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new ClickHouse cluster from the backup",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.RestoreClusterMetadata",
            "backupId": "cid1:0",
            "clusterId": "cid2"
        }
    }
    """
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "description": "test restored",
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
            "description": "test restored",
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

  Scenario: Restoring from backup to different folder works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore?folderId=folder2" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new ClickHouse cluster from the backup",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.RestoreClusterMetadata",
            "backupId": "cid1:0",
            "clusterId": "cid2"
        }
    }
    """
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder2",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
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
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "sas"
        }],
        "backup_id": "cid1:0"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id4" acquired and finished by worker
    When we GET "/api/v1.0/config/sas-3.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data" contains
    """
    {
        "backup": {
            "sleep": 7200,
            "start": {
                "hours": 22,
                "minutes": 15,
                "nanos": 100,
                "seconds": 30
            },
            "use_backup_service": false
        },
        "clickhouse": {
            "ch_version":   "21.3.8.76",
            "cluster_name": "cid1",
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "config": {
                "builtin_dictionaries_reload_interval": 3600,
                "compression": [],
                "geobase_uri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                "keep_alive_timeout": 3,
                "log_level": "information",
                "mark_cache_size": 5368709120,
                "max_concurrent_queries": 500,
                "max_connections": 4096,
                "max_partition_size_to_drop": 53687091200,
                "max_table_size_to_drop": 53687091200,
                "merge_tree": {
                    "enable_mixed_granularity_parts": true,
                    "replicated_deduplication_window": 100,
                    "replicated_deduplication_window_seconds": 604800
                },
                "kafka": {
                    "security_protocol": "SSL",
                    "sasl_mechanism": "SCRAM-SHA-512",
                    "sasl_username": "kafka_username",
                    "sasl_password": {
                        "data": "kafka_pass",
                        "encryption_version": 0
                    }
                },
                "kafka_topics": [
                    {
                        "name": "kafka_topic",
                        "settings": {
                            "security_protocol": "SSL",
                            "sasl_mechanism": "GSSAPI",
                            "sasl_username": "topic_username",
                            "sasl_password": {
                                "data": "topic_pass",
                                "encryption_version": 0
                            }
                        }
                    }
                ],
                "rabbitmq": {
                    "username": "test_user",
                    "password": {
                        "data": "test_password",
                        "encryption_version": 0
                    }
                },
                "timezone": "Europe/Moscow",
                "uncompressed_cache_size": 8589934592
            },
            "databases": [
                "testdb"
            ],
            "format_schemas": {
                 "test_schema": {
                       "type": "protobuf",
                       "uri":  "https://bucket1.storage.yandexcloud.net/test_schema.proto"
                 }
            },
            "interserver_credentials": {
                "password": {
                    "data": "dummy",
                    "encryption_version": 0
                },
                "user": "interserver"
            },
            "models": {
                "test_model": {
                    "type": "catboost",
                    "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
                }
            },
            "shards": {
                "shard_id2": {
                    "weight": 100
                }
            },
            "embedded_keeper": false,
            "sql_database_management": false,
            "sql_user_management": false,
            "system_users": {
                "mdb_backup_admin": {
                    "hash": {
                        "data": "b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "dummy",
                        "encryption_version": 0
                    }
                }
            },
            "user_management_v2": true,
            "users": {
                "test": {
                    "databases": {
                        "testdb": {}
                    },
                    "hash": {
                        "data": "10a6e6cc8311a3e2bcc09bf6c199adecd5dd59408c343e926b129c4914f3cb01",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "test_password",
                        "encryption_version": 0
                    },
                    "quotas": [],
                    "settings": {}
                }
            },
            "zk_users": {
                "clickhouse": {
                    "password": {
                        "data": "dummy",
                        "encryption_version": 0
                    }
                }
            }
        },
        "clickhouse default pillar": true,
        "cloud_storage": {
            "enabled": false
        },
        "cluster_private_key": {
            "data": "2",
            "encryption_version": 0
        },
        "default pillar": true,
        "runlist": [
            "components.clickhouse_cluster"
        ],
        "s3_bucket": "yandexcloud-dbaas-cid2",
        "testing_repos": false,
        "unmanaged": {
            "enable_zk_tls": true
        },
        "versions": {}
    }
    """
    And body at path "$.data.dbaas" contains
    """
    {
        "assign_public_ip": false,
        "on_dedicated_host": false,
        "cloud": {
            "cloud_ext_id": "cloud2"
        },
        "cluster": {
            "subclusters": {
                "subcid3": {
                    "hosts": {
                        "man-2.db.yandex.net": {"geo": "man"},
                        "myt-2.db.yandex.net": {"geo": "myt"},
                        "sas-2.db.yandex.net": {"geo": "sas"}
                    },
                    "name": "zookeeper_subcluster",
                    "roles": [
                        "zk"
                    ],
                    "shards": {}
                },
                "subcid4": {
                    "hosts": {},
                    "name": "clickhouse_subcluster",
                    "roles": [
                        "clickhouse_cluster"
                    ],
                    "shards": {
                        "shard_id2": {
                            "hosts": {
                                "myt-3.db.yandex.net": {"geo": "myt"},
                                "sas-3.db.yandex.net": {"geo": "sas"}
                            },
                            "name": "shard1"
                        }
                    }
                }
            }
        },
        "cluster_hosts": [
            "man-2.db.yandex.net",
            "myt-2.db.yandex.net",
            "myt-3.db.yandex.net",
            "sas-2.db.yandex.net",
            "sas-3.db.yandex.net"
        ],
        "cluster_id": "cid2",
        "cluster_name": "test_restored",
        "cluster_type": "clickhouse_cluster",
        "disk_type_id": "local-ssd",
        "flavor": {
            "cpu_fraction": 100,
            "cpu_guarantee": 1,
            "cpu_limit": 1,
            "description": "s1.porto.1",
            "generation": 1,
            "gpu_limit": 0,
            "id": "00000000-0000-0000-0000-000000000000",
            "io_cores_limit": 0,
            "io_limit": 20971520,
            "memory_guarantee": 4294967296,
            "memory_limit": 4294967296,
            "name": "s1.porto.1",
            "network_guarantee": 16777216,
            "network_limit": 16777216,
            "platform_id": "mdb-v1",
            "type": "standard",
            "vtype": "porto"
        },
        "folder": {
            "folder_ext_id": "folder2"
        },
        "fqdn": "sas-3.db.yandex.net",
        "geo": "sas",
        "shard_hosts": [
            "myt-3.db.yandex.net",
            "sas-3.db.yandex.net"
        ],
        "shard_id": "shard_id2",
        "shard_name": "shard1",
        "space_limit": 10737418240,
        "subcluster_id": "subcid4",
        "subcluster_name": "clickhouse_subcluster",
        "vtype": "porto",
        "vtype_id": null
    }
    """

  Scenario: Restoring from backup with less disk size fails
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "diskSize": 21474836480
                }
            }
        }
    }
    """
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Insufficient diskSize, increase it to '21474836480'"
    }
    """
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "insufficient disk_size, increase it to 21474836480"

  Scenario: Restoring from backup of HA cluster to non-HA cluster works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new ClickHouse cluster from the backup",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.RestoreClusterMetadata",
            "backupId": "cid1:0",
            "clusterId": "cid2"
        }
    }
    """
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.4",
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
        }],
        "backup_id": "cid1:0"
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id4" exists "yandex.cloud.events.mdb.clickhouse.RestoreCluster" event with
    """
    {
        "details": {
            "backup_id": "cid1:0",
            "cluster_id": "cid2"
        }
    }
    """

  Scenario: Restoring from backup with nonexistent backup fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:777"
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Backup 'cid1:777' does not exist"
    }
    """
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
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
        "backup_id": "cid1:777"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "backup "cid1:777" does not exist"

  Scenario: Restoring from backup with incorrect disk unit fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418245
                }
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Disk size must be a multiple of 4194304 bytes"
    }
    """
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418245
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "invalid disk size, must be multiple of 4194304 bytes"

  @delete
  Scenario: Restoring from backup belongs to deleted cluster
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id4" exists "yandex.cloud.events.mdb.clickhouse.DeleteCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new ClickHouse cluster from the backup",
        "done": false
    }
    """
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
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
    Then we get gRPC response OK

  @delete
  Scenario: Restoring from backup after delete with less disk size fails
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "diskSize": 21474836480
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "description": "Modify ClickHouse cluster",
        "id": "worker_task_id4"
    }
    """
    When "worker_task_id4" acquired and finished by worker
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id5" exists "yandex.cloud.events.mdb.clickhouse.DeleteCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Insufficient diskSize, increase it to '21474836480'"
    }
    """
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "insufficient disk_size, increase it to 21474836480"

  Scenario: Restoring from backup with a specified version works
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.version" contains only
    """
    ["21.3"]
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 200
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.4",
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
    Then we get gRPC response OK
    When we GET "/mdb/clickhouse/1.0/clusters/cid2"
    Then we get response with status 200
    And body at path "$.config.version" contains only
    """
    ["21.4"]
    """

  @timetravel
  Scenario: Restoring cluster on time before database delete
    . restored cluster should have that deleted database
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "other_database"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id4"
    }
    """
    When "worker_task_id4" acquired and finished by worker
    And we DELETE "/mdb/clickhouse/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "backupId": "cid1:0"
    }
    """
    Then we get response with status 200
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.4",
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
    Then we get gRPC response OK
    When we GET "/mdb/clickhouse/1.0/clusters/cid2/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": [{
            "clusterId": "cid2",
            "name": "testdb"
        }]
    }
    """

  @grpc_api
  Scenario: Shard group restores with single correct shard
    And we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1"]
    }
    """
    And "worker_task_id4" acquired and finished by worker
    When we "AddShard" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_name": "shard2"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id5" acquired and finished by worker
    When we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group2",
        "description": "Test shard group2",
        "shard_names": ["shard2"]
    }
    """
    And "worker_task_id6" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_restored",
        "environment": "PRESTABLE",
        "config_spec": {
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
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "iva"
        }],
        "backup_id": "cid1:0"
    }
    """
    Then we get gRPC response OK
    When "worker_task_id7" acquired and finished by worker
    When we GET via PillarConfig "/v1/config/sas-3.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data" contains
    """
    {
        "backup": {
            "sleep": 7200,
            "start": {
                "hours": 22,
                "minutes": 15,
                "nanos": 100,
                "seconds": 30
            },
            "use_backup_service": false
        },
        "clickhouse": {
            "ch_version":   "21.3.8.76",
            "cluster_name": "cid1",
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "config": {
                "builtin_dictionaries_reload_interval": 3600,
                "compression": [],
                "geobase_uri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                "keep_alive_timeout": 3,
                "log_level": "information",
                "mark_cache_size": 5368709120,
                "max_concurrent_queries": 500,
                "max_connections": 4096,
                "max_partition_size_to_drop": 53687091200,
                "max_table_size_to_drop": 53687091200,
                "merge_tree": {
                    "enable_mixed_granularity_parts": true,
                    "replicated_deduplication_window": 100,
                    "replicated_deduplication_window_seconds": 604800
                },
                "kafka": {
                    "security_protocol": "SSL",
                    "sasl_mechanism": "SCRAM-SHA-512",
                    "sasl_username": "kafka_username",
                    "sasl_password": {
                        "data": "kafka_pass",
                        "encryption_version": 0
                    }
                },
                "kafka_topics": [
                    {
                        "name": "kafka_topic",
                        "settings": {
                            "security_protocol": "SSL",
                            "sasl_mechanism": "GSSAPI",
                            "sasl_username": "topic_username",
                            "sasl_password": {
                                "data": "topic_pass",
                                "encryption_version": 0
                            }
                        }
                    }
                ],
                "rabbitmq": {
                    "username": "test_user",
                    "password": {
                        "data": "test_password",
                        "encryption_version": 0
                    }
                },
                "timezone": "Europe/Moscow",
                "uncompressed_cache_size": 8589934592
            },
            "databases": [
                "testdb"
            ],
            "format_schemas": {
                 "test_schema": {
                       "type": "protobuf",
                       "uri":  "https://bucket1.storage.yandexcloud.net/test_schema.proto"
                 }
            },
            "interserver_credentials": {
                "password": {
                    "data": "dummy",
                    "encryption_version": 0
                },
                "user": "interserver"
            },
            "models": {
                "test_model": {
                    "type": "catboost",
                    "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
                }
            },
            "shard_groups": {
                "test_group": {
                     "description": "Test shard group",
                     "shard_names": ["shard1"]
                }
            },
            "shards": {
                "shard_id3": {
                    "weight": 100
                }
            },
            "embedded_keeper": false,
            "sql_database_management": false,
            "sql_user_management": false,
            "system_users": {
                "mdb_backup_admin": {
                    "hash": {
                        "data": "b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "dummy",
                        "encryption_version": 0
                    }
                }
            },
            "user_management_v2": true,
            "users": {
                "test": {
                    "databases": {
                        "testdb": {}
                    },
                    "hash": {
                        "data": "10a6e6cc8311a3e2bcc09bf6c199adecd5dd59408c343e926b129c4914f3cb01",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "test_password",
                        "encryption_version": 0
                    },
                    "quotas": [],
                    "settings": {}
                }
            },
            "zk_users": {
                "clickhouse": {
                    "password": {
                        "data": "dummy",
                        "encryption_version": 0
                    }
                }
            }
        },
        "clickhouse default pillar": true,
        "cloud_storage": {
            "enabled": false
        },
        "cluster_private_key": {
            "data": "2",
            "encryption_version": 0
        },
        "default pillar": true,
        "runlist": [
            "components.clickhouse_cluster"
        ],
        "s3_bucket": "yandexcloud-dbaas-cid2",
        "testing_repos": false,
        "unmanaged": {
            "enable_zk_tls": true
        },
        "versions": {}
    }
    """
    And body at path "$.data.dbaas" contains
    """
    {
        "assign_public_ip": false,
        "on_dedicated_host": false,
        "cloud": {
            "cloud_ext_id": "cloud1"
        },
        "cluster": {
            "subclusters": {
                "subcid3": {
                    "hosts": {
                        "man-2.db.yandex.net": {"geo": "man"},
                        "vla-2.db.yandex.net": {"geo": "vla"},
                        "iva-2.db.yandex.net": {"geo": "iva"}
                    },
                    "name": "zookeeper_subcluster",
                    "roles": [
                        "zk"
                    ],
                    "shards": {}
                },
                "subcid4": {
                    "hosts": {},
                    "name": "clickhouse_subcluster",
                    "roles": [
                        "clickhouse_cluster"
                    ],
                    "shards": {
                        "shard_id3": {
                            "hosts": {
                                "myt-3.db.yandex.net": {"geo": "myt"},
                                "sas-3.db.yandex.net": {"geo": "sas"}
                            },
                            "name": "shard1"
                        }
                    }
                }
            }
        },
        "cluster_hosts": [
            "iva-2.db.yandex.net",
            "man-2.db.yandex.net",
            "myt-3.db.yandex.net",
            "sas-3.db.yandex.net",
            "vla-2.db.yandex.net"
        ],
        "cluster_id": "cid2",
        "cluster_name": "test_restored",
        "cluster_type": "clickhouse_cluster",
        "disk_type_id": "local-ssd",
        "flavor": {
            "cpu_fraction": 100,
            "cpu_guarantee": 1,
            "cpu_limit": 1,
            "description": "s1.porto.1",
            "generation": 1,
            "gpu_limit": 0,
            "id": "00000000-0000-0000-0000-000000000000",
            "io_cores_limit": 0,
            "io_limit": 20971520,
            "memory_guarantee": 4294967296,
            "memory_limit": 4294967296,
            "name": "s1.porto.1",
            "network_guarantee": 16777216,
            "network_limit": 16777216,
            "platform_id": "mdb-v1",
            "type": "standard",
            "vtype": "porto"
        },
        "folder": {
            "folder_ext_id": "folder1"
        },
        "fqdn": "sas-3.db.yandex.net",
        "geo": "sas",
        "shard_hosts": [
            "myt-3.db.yandex.net",
            "sas-3.db.yandex.net"
        ],
        "shard_id": "shard_id3",
        "shard_name": "shard1",
        "space_limit": 10737418240,
        "subcluster_id": "subcid4",
        "subcluster_name": "clickhouse_subcluster",
        "vtype": "porto",
        "vtype_id": null
    }
    """

  @grpc_api
  Scenario: Restoring with minimum required params works
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "backup_id": "cid1:0"
    }
    """
    Then we get gRPC response with body
    # language=json
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
    # language=json
    """
    {
        "details": {
            "cluster_id": "cid2",
            "backup_id": "cid1:0"
        }
    }
    """
    Given "worker_task_id4" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "cluster_id": "cid2"
    }
    """
    Then we get gRPC response with body ignoring empty
    # language=json
    """
    {
        "created_at": "**IGNORE**",
        "description": "Restored from backup(s): 0",
        "environment": "PRODUCTION",
        "folder_id": "folder1",
        "id": "cid2",
        "name": "test_restored",
        "status": "RUNNING",
        "maintenance_window": { "anytime": {} },
        "monitoring": [{
            "description": "YaSM (Golovan) charts",
            "link": "https://yasm/cid=cid2",
            "name": "YASM"
        }, {
            "description": "Solomon charts",
            "link": "https://solomon/cid=cid2&fExtID=folder1",
            "name": "Solomon"
        }, {
            "description": "Console charts",
            "link": "https://console/cid=cid2&fExtID=folder1",
            "name": "Console"
        }],
        "config": {
            "access": "**IGNORE**",
            "backup_window_start": "**IGNORE**",
            "clickhouse": {
                "config": {
                    "default_config": "**IGNORE**",
                    "user_config": "**IGNORE**",
                    "effective_config": {
                        "geobase_uri": "https://bucket1.storage.yandexcloud.net/geobase.tar.gz",
                        "merge_tree": "**IGNORE**",
                        "kafka": "**IGNORE**",
                        "kafka_topics": [{
                            "name": "kafka_topic",
                            "settings": "**IGNORE**"
                        }],
                        "rabbitmq": {
                            "username": "test_user"
                        },
                        "builtin_dictionaries_reload_interval": "3600",
                        "keep_alive_timeout": "3",
                        "log_level": "INFORMATION",
                        "mark_cache_size": "5368709120",
                        "max_concurrent_queries": "500",
                        "max_connections": "4096",
                        "max_partition_size_to_drop": "53687091200",
                        "max_table_size_to_drop": "53687091200",
                        "timezone": "Europe/Moscow",
                        "uncompressed_cache_size": "8589934592"
                    }
                },
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                }
            },
            "cloud_storage": {},
            "embedded_keeper": false,
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "sql_database_management": false,
            "sql_user_management": false,
            "version": "21.3",
            "zookeeper": "**IGNORE**"
        }
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "cluster_id": "cid2"
    }
    """
    Then we get gRPC response with body ignoring empty
    # language=json
    """
    {
        "hosts": [
            {
                "cluster_id": "cid2",
                "name": "iva-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "type": "ZOOKEEPER",
                "zone_id": "iva"
            },
            {
                "cluster_id": "cid2",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "type": "ZOOKEEPER",
                "zone_id": "man"
            },
            {
                "cluster_id": "cid2",
                "name": "myt-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "shard_name": "shard1",
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            },
            {
                "cluster_id": "cid2",
                "name": "vla-2.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "type": "ZOOKEEPER",
                "zone_id": "vla"
            }
        ]
    }
    """
