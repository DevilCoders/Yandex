Feature: Operation for Managed ClickHouse

  Background:
    Given default headers
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
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
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1",
        "serviceAccountId": "sa1",
        "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" containing:
      |sg_id1|
      |sg_id2|
    And worker set "sg_id1,sg_id2" security groups on "cid1"
    And "worker_task_id1" acquired and finished by worker
    And cluster "cid1" has 5 host
    And we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
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
        }],
        "description": "test2 cluster",
        "networkId": "network1",
        "serviceAccountId": "sa1",
        "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And cluster "cid2" has 1 host

  Scenario: Operation listing works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb2"
        }
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_spec": {
            "name": "testdb2"
        }
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.CreateDatabase" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "database_name": "testdb2"
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/formatSchemas" with data
    """
    {
        "formatSchemaName": "test_schema",
        "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
        "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
    }
    """
    Then we get response with status 200
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
    And for "worker_task_id4" exists "yandex.cloud.events.mdb.clickhouse.CreateFormatSchema" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "format_schema_name": "test_schema"
        }
    }
    """
    And "worker_task_id4" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/mlModels" with data
    """
    {
        "mlModelName": "test_model",
        "type": "ML_MODEL_TYPE_CATBOOST",
        "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
    }
    """
    Then we get response with status 200
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
    And for "worker_task_id5" exists "yandex.cloud.events.mdb.clickhouse.CreateMlModel" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "ml_model_name": "test_model"
        }
    }
    """
    And "worker_task_id5" acquired and finished by worker
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1/shards/shard1" with data
    """
    {
        "cluster_id": "cid1",
        "cluster_type": "clickhouse_cluster",
        "shard_name": "shard1",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "diskSize": 21474836480
                }
            }
        }
    }
    """
    Then we get response with status 200
    And for "worker_task_id6" exists event
    And "worker_task_id6" acquired and finished by worker
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/operations"
    Then we get response with status 200 and body equals
    """
    {
        "operations": [{
            "createdBy": "user",
            "description": "Modify ClickHouse shard",
            "done": true,
            "id": "worker_task_id6",
            "metadata": {
                "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterShardMetadata",
                "clusterId": "cid1",
                "shardName": "shard1"
            },
            "createdAt": "**IGNORE**",
            "modifiedAt": "**IGNORE**",
            "response": {
                "@type": "yandex.cloud.mdb.clickhouse.v1.Shard",
                "clusterId": "cid1",
                "config": "**IGNORE**",
                "name": "shard1"
            }
        },{
            "createdBy": "user",
            "description": "Add ML model in ClickHouse cluster",
            "done": true,
            "id": "worker_task_id5",
            "metadata": {
                "@type": "yandex.cloud.mdb.clickhouse.v1.CreateMlModelMetadata",
                "clusterId": "cid1",
                "mlModelName": "test_model"
            },
            "createdAt": "**IGNORE**",
            "modifiedAt": "**IGNORE**",
            "response": {
                "@type": "yandex.cloud.mdb.clickhouse.v1.MlModel",
                "clusterId": "cid1",
                "name": "test_model",
                "type": "ML_MODEL_TYPE_CATBOOST",
                "uri": "https://bucket1.storage.yandexcloud.net/test_model.bin"
            }
        },{
            "createdBy": "user",
            "description": "Add format schema in ClickHouse cluster",
            "done": true,
            "id": "worker_task_id4",
            "metadata": {
                "@type": "yandex.cloud.mdb.clickhouse.v1.CreateFormatSchemaMetadata",
                "clusterId": "cid1",
                "formatSchemaName": "test_schema"
            },
            "createdAt": "**IGNORE**",
            "modifiedAt": "**IGNORE**",
            "response": {
                "@type": "yandex.cloud.mdb.clickhouse.v1.FormatSchema",
                "clusterId": "cid1",
                "name": "test_schema",
                "type": "FORMAT_SCHEMA_TYPE_PROTOBUF",
                "uri": "https://bucket1.storage.yandexcloud.net/test_schema.proto"
            }
        },{
            "createdBy": "user",
            "description": "Add database to ClickHouse cluster",
            "done": true,
            "id": "worker_task_id3",
            "metadata": {
                "@type": "yandex.cloud.mdb.clickhouse.v1.CreateDatabaseMetadata",
                "clusterId": "cid1",
                "databaseName": "testdb2"
            },
            "createdAt": "**IGNORE**",
            "modifiedAt": "**IGNORE**",
            "response": {
                "@type": "yandex.cloud.mdb.clickhouse.v1.Database",
                "clusterId": "cid1",
                "name": "testdb2"
            }
        },{
            "createdBy": "user",
            "description": "Create ClickHouse cluster",
            "done": true,
            "id": "worker_task_id1",
            "metadata": {
                "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterMetadata",
                "clusterId": "cid1"
            },
            "createdAt": "**IGNORE**",
            "modifiedAt": "**IGNORE**",
            "response": "**IGNORE**"
        }]
    }
    """
    When we "ListOperations" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "operations": [{
            "created_by": "user",
            "description": "Modify shard in ClickHouse cluster",
            "done": true,
            "id": "worker_task_id6",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateClusterShardMetadata",
                "cluster_id": "cid1",
                "shard_name": "shard1"
            },
            "created_at": "**IGNORE**",
            "modified_at": "**IGNORE**",
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "message": "OK"
            }
        },{
            "created_by": "user",
            "description": "Add ML model in ClickHouse cluster",
            "done": true,
            "id": "worker_task_id5",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateMlModelMetadata",
                "cluster_id": "cid1",
                "ml_model_name": "test_model"
            },
            "created_at": "**IGNORE**",
            "modified_at": "**IGNORE**",
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "message": "OK"
            }
        },{
            "created_by": "user",
            "description": "Add format schema in ClickHouse cluster",
            "done": true,
            "id": "worker_task_id4",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateFormatSchemaMetadata",
                "cluster_id": "cid1",
                "format_schema_name": "test_schema"
            },
            "created_at": "**IGNORE**",
            "modified_at": "**IGNORE**",
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "message": "OK"
            }
        },{
            "created_by": "user",
            "description": "Add database to ClickHouse cluster",
            "done": true,
            "id": "worker_task_id3",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateDatabaseMetadata",
                "cluster_id": "cid1",
                "database_name": "testdb2"
            },
            "created_at": "**IGNORE**",
            "modified_at": "**IGNORE**",
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "message": "OK"
            }
        },{
            "created_by": "user",
            "description": "Create ClickHouse cluster",
            "done": true,
            "id": "worker_task_id1",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterMetadata",
                "cluster_id": "cid1"
            },
            "created_at": "**IGNORE**",
            "modified_at": "**IGNORE**",
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "message": "OK"
            }
        }]
    }
    """

    Scenario: Operation listing works for multiple clusters
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "operations": [{
            "created_by": "user",
            "description": "Create ClickHouse cluster",
            "done": true,
            "id": "worker_task_id2",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterMetadata",
                "cluster_id": "cid2"
            },
            "created_at": "**IGNORE**",
            "modified_at": "**IGNORE**",
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "message": "OK"
            }
        },{
            "created_by": "user",
            "description": "Create ClickHouse cluster",
            "done": true,
            "id": "worker_task_id1",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterMetadata",
                "cluster_id": "cid1"
            },
            "created_at": "**IGNORE**",
            "modified_at": "**IGNORE**",
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "message": "OK"
            }
        }]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "folder_id": "folder1",
        "page_size": 1
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "operations": [{
            "created_by": "user",
            "description": "Create ClickHouse cluster",
            "done": true,
            "id": "worker_task_id2",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterMetadata",
                "cluster_id": "cid2"
            },
            "created_at": "**IGNORE**",
            "modified_at": "**IGNORE**",
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "message": "OK"
            }
        }],
        "next_page_token": "**IGNORE**"
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data and last page token
    """
    {
        "folder_id": "folder1",
        "page_token": "**IGNORE**"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "operations": [{
            "created_by": "user",
            "description": "Create ClickHouse cluster",
            "done": true,
            "id": "worker_task_id1",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterMetadata",
                "cluster_id": "cid1"
            },
            "created_at": "**IGNORE**",
            "modified_at": "**IGNORE**",
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "message": "OK"
            }
        }]
    }
    """