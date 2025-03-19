Feature: Management of ClickHouse external dictionaries

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
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
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
    And "worker_task_id1" acquired and finished by worker

  @events
  Scenario: Updating external dictionaries works
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "dictionaries": [
                        {
                            "name": "test_dict",
                            "httpSource": {
                                "url": "test_url",
                                "format": "TSV"
                            },
                            "structure": {
                                "id": {
                                    "name": "id"
                                },
                                "attributes": [{
                                    "name": "text",
                                    "type": "String"
                                }]
                            },
                            "layout": {
                                "type": "FLAT"
                            },
                            "fixedLifetime": 300
                        }
                    ]
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "httpSource": {
                    "url": "test_url",
                    "format": "TSV"
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }]
                },
                "layout": {
                    "type": "FLAT"
                },
                "fixedLifetime": 300
            }
        ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response OK
    And gRPC response body at path "$.config.clickhouse.config.effective_config" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "http_source": {
                    "url": "test_url",
                    "format": "TSV"
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String"
                    }]
                },
                "layout": {
                    "type": "FLAT"
                },
                "fixed_lifetime": "300"
            }
        ]
    }
    """

  Scenario: Updating external dictionaries with duplicate entries fails
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "dictionaries": [
                        {
                            "name": "test_dict",
                            "httpSource": {
                                "url": "test_url",
                                "format": "TSV"
                            },
                            "structure": {
                                "id": {
                                    "name": "id"
                                },
                                "attributes": [{
                                    "name": "text",
                                    "type": "String"
                                }]
                            },
                            "layout": {
                                "type": "FLAT"
                            },
                            "fixedLifetime": 300
                        },
                        {
                            "name": "test_dict",
                            "httpSource": {
                                "url": "test_url",
                                "format": "TSV"
                            },
                            "structure": {
                                "id": {
                                    "name": "id"
                                },
                                "attributes": [{
                                    "name": "text",
                                    "type": "String"
                                }]
                            },
                            "layout": {
                                "type": "FLAT"
                            },
                            "fixedLifetime": 300
                        }
                    ]
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Cluster cannot have multiple external dictionaries with the same name ('test_dict')"
    }
    """

  @events
  Scenario: Adding external dictionary with HTTP source works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:createExternalDictionary" with data
    """
    {
        "externalDictionary": {
            "name": "test_dict",
            "httpSource": {
                "url": "test_url",
                "format": "TSV"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixedLifetime": 300
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we "CreateExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary": {
            "name": "test_dict",
            "http_source": {
                "url": "test_url",
                "format": "TSV"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": 300
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.CreateClusterExternalDictionary" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "httpSource": {
                    "url": "test_url",
                    "format": "TSV"
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }]
                },
                "layout": {
                    "type": "FLAT"
                },
                "fixedLifetime": 300
            }
        ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And gRPC response body at path "$.response" contains
    """
    {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
        "id": "cid1"
    }
    """
    And gRPC response body at path "$.response.config.clickhouse.config.effective_config" contains
    """
    {
        "dictionaries": [{
            "name": "test_dict",
            "http_source": {
                "url": "test_url",
                "format": "TSV"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": "300"
        }]
    }
    """

  Scenario: Adding external dictionary with MySQL source works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:createExternalDictionary" with data
    """
    {
        "externalDictionary": {
            "name": "test_dict",
            "mysqlSource": {
                "db": "test_db",
                "table": "test_table",
                "replicas": [{
                    "host": "test_host"
                }]
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String",
                    "nullValue": ""
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixedLifetime": 300
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we "CreateExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary": {
            "name": "test_dict",
            "mysql_source": {
                "db": "test_db",
                "table": "test_table",
                "replicas": [{
                    "host": "test_host"
                }]
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String",
                    "null_value": ""
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": 300
        }
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "dictionaries": [{
            "name": "test_dict",
            "mysqlSource": {
                "db": "test_db",
                "table": "test_table",
                "replicas": [{
                    "host": "test_host",
                    "priority": 100
                }]
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String",
                    "nullValue": "",
                    "hierarchical": false,
                    "injective": false,
                    "nullValue": ""
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixedLifetime": 300
        }]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And gRPC response body at path "$.response" contains
    """
    {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
        "id": "cid1"
    }
    """
    And gRPC response body at path "$.response.config.clickhouse.config.effective_config" contains
    """
    {
        "dictionaries": [{
            "name": "test_dict",
            "mysql_source": {
                "db": "test_db",
                "table": "test_table",
                "replicas": [{
                    "host": "test_host",
                    "priority": "100"
                }]
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": "300"
        }]
    }
    """

  Scenario: Adding external dictionary with ClickHouse source works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:createExternalDictionary" with data
    """
    {
        "externalDictionary": {
            "name": "test_dict",
            "clickhouseSource": {
                "host": "test_host",
                "db": "test_db",
                "table": "test_table"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String",
                    "nullValue": ""
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixedLifetime": 300
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "CreateExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary": {
            "name": "test_dict",
            "clickhouse_source": {
                "host": "test_host",
                "db": "test_db",
                "table": "test_table"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String",
                    "null_value": ""
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": 300
        }
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.clickhouse.v1.Cluster",
        "id": "cid1"
    }
    """
    And body at path "$.response.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "clickhouseSource": {
                    "host": "test_host",
                    "db": "test_db",
                    "table": "test_table",
                    "port": 9000
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }]
                },
                "layout": {
                    "type": "FLAT"
                },
                "fixedLifetime": 300
            }
        ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And gRPC response body at path "$.response" contains
    """
    {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
        "id": "cid1"
    }
    """
    And gRPC response body at path "$.response.config.clickhouse.config.effective_config" contains
    """
    {
        "dictionaries": [{
            "name": "test_dict",
            "clickhouse_source": {
                "host": "test_host",
                "db": "test_db",
                "table": "test_table",
                "port": "9000"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": "300"
        }]
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse.config" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "clickhouse_source": {
                    "host": "test_host",
                    "db": "test_db",
                    "table": "test_table",
                    "port": 9000
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "null_value": "",
                        "hierarchical": false,
                        "injective": false
                    }]
                },
                "layout": {
                    "type": "flat"
                },
                "fixed_lifetime": 300
            }
        ]
    }
    """

  Scenario: Adding external dictionary with MongoDB source works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:createExternalDictionary" with data
    """
    {
        "externalDictionary": {
            "name": "test_dict",
            "mongodbSource": {
                "host": "test_host",
                "db": "test_db",
                "collection": "test_collection"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixedLifetime": 300
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we "CreateExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary": {
            "name": "test_dict",
            "mongodb_source": {
                "host": "test_host",
                "db": "test_db",
                "collection": "test_collection"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": 300
        }
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.clickhouse.v1.Cluster",
        "id": "cid1"
    }
    """
    And body at path "$.response.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "mongodbSource": {
                    "host": "test_host",
                    "db": "test_db",
                    "collection": "test_collection",
                    "port": 27017
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }]
                },
                "layout": {
                    "type": "FLAT"
                },
                "fixedLifetime": 300
            }
        ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And gRPC response body at path "$.response" contains
    """
    {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
        "id": "cid1"
    }
    """
    And gRPC response body at path "$.response.config.clickhouse.config.effective_config" contains
    """
    {
        "dictionaries": [{
            "name": "test_dict",
            "mongodb_source": {
                "host": "test_host",
                "db": "test_db",
                "collection": "test_collection",
                "port": "27017"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": "300"
        }]
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse.config" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "mongodb_source": {
                    "host": "test_host",
                    "db": "test_db",
                    "collection": "test_collection",
                    "port": 27017
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "hierarchical": false,
                        "injective": false,
                        "null_value": ""
                    }]
                },
                "layout": {
                    "type": "flat"
                },
                "fixed_lifetime": 300
            }
        ]
    }
    """

  Scenario: Adding external dictionary with PostgreSQL source works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:createExternalDictionary" with data
    """
    {
        "externalDictionary": {
            "name": "test_dict",
            "postgresqlSource": {
                "db": "test_db",
                "table": "test_table",
                "hosts": ["host1", "host2", "host3"],
                "user": "test_user",
                "password": "test_password"
            },
            "structure": {
                "id": {
                     "name": "id"
                 },
                 "attributes": [{
                     "name": "text",
                     "type": "String"
                 }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixedLifetime": 300
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we "CreateExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary": {
            "name": "test_dict",
            "postgresql_source": {
                "db": "test_db",
                "table": "test_table",
                "hosts": ["host1", "host2", "host3"],
                "user": "test_user",
                "password": "test_password"
            },
            "structure": {
                "id": {
                     "name": "id"
                 },
                 "attributes": [{
                     "name": "text",
                     "type": "String"
                 }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": 300
        }
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.clickhouse.v1.Cluster",
        "id": "cid1"
    }
    """
    And body at path "$.response.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "postgresqlSource": {
                    "db": "test_db",
                    "table": "test_table",
                    "hosts": ["host1", "host2", "host3"],
                    "user": "test_user",
                    "port": 5432
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }]
                },
                "layout": {
                    "type": "FLAT"
                },
                "fixedLifetime": 300
            }
        ]
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse.config" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "postgresql_source": {
                    "db": "test_db",
                    "table": "test_table",
                    "hosts": ["host1", "host2", "host3"],
                    "user": "test_user",
                    "password": {
                        "data": "test_password",
                        "encryption_version": 0
                    },
                    "port": 5432
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "hierarchical": false,
                        "injective": false,
                        "null_value": ""
                    }]
                },
                "layout": {
                    "type": "flat"
                },
                "fixed_lifetime": 300
            }
        ]
    }
    """

  Scenario: Adding external dictionary with YT source containing query parameter works
    When we POST "/mdb/clickhouse/1.0/clusters/cid1:createExternalDictionary" with data
    """
    {
        "externalDictionary": {
            "name": "test_dict",
            "ytSource": {
                "clusters": ["hahn"],
                "table": "//home/TestTable",
                "keys": ["PageID"],
                "query": "PageID, TargetType",
                "user": "test_user",
                "token": "test_token"
            },
            "structure": {
                "id": {
                    "name": "PageID"
                },
                "attributes": [{
                    "name": "TargetType",
                    "type": "Int32"
                }]
            },
            "layout": {
                "type": "HASHED"
            },
            "fixedLifetime": 300
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.clickhouse.v1.Cluster",
        "id": "cid1"
    }
    """
    And body at path "$.response.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "ytSource": {
                    "clusters": ["hahn"],
                    "table": "//home/TestTable",
                    "keys": ["PageID"],
                    "query": "PageID, TargetType",
                    "user": "test_user"
                },
                "structure": {
                    "id": {
                        "name": "PageID"
                    },
                    "attributes": [{
                        "name": "TargetType",
                        "type": "Int32",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }]
                },
                "layout": {
                    "type": "HASHED"
                },
                "fixedLifetime": 300
            }
        ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And gRPC response body at path "$.response" contains
    """
    {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
        "id": "cid1"
    }
    """
    And gRPC response body at path "$.response.config.clickhouse.config.effective_config" contains
    """
    {
        "dictionaries": [{
            "name": "test_dict",
            "yt_source": {
                "clusters": ["hahn"],
                "table": "//home/TestTable",
                "keys": ["PageID"],
                "query": "PageID, TargetType",
                "user": "test_user"
            },
            "structure": {
                "id": {
                    "name": "PageID"
                },
                "attributes": [{
                    "name": "TargetType",
                    "type": "Int32"
                }]
            },
            "layout": {
                "type": "HASHED"
            },
            "fixed_lifetime": "300"
        }]
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse.config" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "yt_source": {
                    "clusters": ["hahn"],
                    "table": "//home/TestTable",
                    "keys": ["PageID"],
                    "query": "PageID, TargetType",
                    "user": "test_user",
                    "token": {
                        "data": "test_token",
                        "encryption_version": 0
                    }
                },
                "structure": {
                    "id": {
                        "name": "PageID"
                    },
                    "attributes": [{
                        "name": "TargetType",
                        "type": "Int32",
                        "hierarchical": false,
                        "injective": false,
                        "null_value": ""
                    }]
                },
                "layout": {
                    "type": "hashed"
                },
                "fixed_lifetime": 300
            }
        ]
    }
    """

  Scenario: Adding external dictionary with YT source containing field parameters works
    When we POST "/mdb/clickhouse/1.0/clusters/cid1:createExternalDictionary" with data
    """
    {
        "externalDictionary": {
            "name": "test_dict",
            "ytSource": {
                "clusters": ["hahn"],
                "table": "//home/TestTable",
                "keys": ["PageID"],
                "fields": ["TargetType"],
                "dateFields": ["EventDate"],
                "datetimeFields": ["EventDatetime"],
                "user": "test_user",
                "token": "test_token",
                "clusterSelection": "RANDOM",
                "forceReadTable": true,
                "useQueryForCache": true,
                "rangeExpansionLimit": 1000,
                "inputRowLimit": 1000,
                "ytSocketTimeout": 100000,
                "ytConnectionTimeout": 200000,
                "ytLookupTimeout": 30000,
                "ytSelectTimeout": 40000,
                "outputRowLimit": 1024,
                "ytRetryCount": 10
            },
            "structure": {
                "id": {
                    "name": "PageID"
                },
                "attributes": [{
                    "name": "TargetType",
                    "type": "Int32"
                }, {
                    "name": "EventDate",
                    "type": "Date"
                },{
                    "name": "EventDatetime",
                    "type": "DateTime"
                }]
            },
            "layout": {
                "type": "HASHED"
            },
            "fixedLifetime": 300
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.clickhouse.v1.Cluster",
        "id": "cid1"
    }
    """
    And body at path "$.response.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "ytSource": {
                    "clusters": ["hahn"],
                    "table": "//home/TestTable",
                    "keys": ["PageID"],
                    "fields": ["TargetType"],
                    "dateFields": ["EventDate"],
                    "datetimeFields": ["EventDatetime"],
                    "user": "test_user",
                    "clusterSelection": "RANDOM",
                    "forceReadTable": true,
                    "useQueryForCache": true,
                    "rangeExpansionLimit": 1000,
                    "inputRowLimit": 1000,
                    "ytSocketTimeout": 100000,
                    "ytConnectionTimeout": 200000,
                    "ytLookupTimeout": 30000,
                    "ytSelectTimeout": 40000,
                    "outputRowLimit": 1024,
                    "ytRetryCount": 10
                },
                "structure": {
                    "id": {
                        "name": "PageID"
                    },
                    "attributes": [{
                        "name": "TargetType",
                        "type": "Int32",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }, {
                        "name": "EventDate",
                        "type": "Date",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    },{
                        "name": "EventDatetime",
                        "type": "DateTime",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }]
                },
                "layout": {
                    "type": "HASHED"
                },
                "fixedLifetime": 300
            }
        ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And gRPC response body at path "$.response" contains
    """
    {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
        "id": "cid1"
    }
    """
    And gRPC response body at path "$.response.config.clickhouse.config.effective_config" contains
    """
    {
        "dictionaries": [{
            "name": "test_dict",
            "yt_source": {
                "clusters": ["hahn"],
                "table": "//home/TestTable",
                "keys": ["PageID"],
                "fields": ["TargetType"],
                "date_fields": ["EventDate"],
                "datetime_fields": ["EventDatetime"],
                "user": "test_user",
                "cluster_selection": "RANDOM",
                "use_query_for_cache": true,
                "force_read_table": true,
                "range_expansion_limit": "1000",
                "input_row_limit": "1000",
                "output_row_limit": "1024",
                "yt_socket_timeout": "100000",
                "yt_connection_timeout": "200000",
                "yt_lookup_timeout": "30000",
                "yt_select_timeout": "40000",
                "yt_retry_count": "10"
            },
            "structure": {
                "id": {
                    "name": "PageID"
                },
                "attributes": [{
                    "name": "TargetType",
                    "type": "Int32"
                }, {
                    "name": "EventDate",
                    "type": "Date"
                },{
                    "name": "EventDatetime",
                    "type": "DateTime"
                }]
            },
            "layout": {
                "type": "HASHED"
            },
            "fixed_lifetime": "300"
        }]
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse.config" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict",
                "yt_source": {
                    "clusters": ["hahn"],
                    "table": "//home/TestTable",
                    "keys": ["PageID"],
                    "fields": ["TargetType"],
                    "date_fields": ["EventDate"],
                    "datetime_fields": ["EventDatetime"],
                    "user": "test_user",
                    "token": {
                        "data": "test_token",
                        "encryption_version": 0
                    },
                    "cluster_selection": "Random",
                    "use_query_for_cache": true,
                    "force_read_table": true,
                    "range_expansion_limit": 1000,
                    "input_row_limit": 1000,
                    "yt_socket_timeout_msec": 100000,
                    "yt_connection_timeout_msec": 200000,
                    "yt_lookup_timeout_msec": 30000,
                    "yt_select_timeout_msec": 40000,
                    "output_row_limit": 1024,
                    "yt_retry_count": 10
                },
                "structure": {
                    "id": {
                        "name": "PageID"
                    },
                    "attributes": [{
                        "name": "TargetType",
                        "type": "Int32",
                        "hierarchical": false,
                        "injective": false,
                        "null_value": ""
                    }, {
                        "name": "EventDate",
                        "type": "Date",
                        "hierarchical": false,
                        "injective": false,
                        "null_value": ""
                    },{
                        "name": "EventDatetime",
                        "type": "DateTime",
                        "hierarchical": false,
                        "injective": false,
                        "null_value": ""
                    }]
                },
                "layout": {
                    "type": "hashed"
                },
                "fixed_lifetime": 300
            }
        ]
    }
    """

  Scenario: Adding external dictionaries using different API works
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "dictionaries": [
                        {
                            "name": "test_dict1",
                            "httpSource": {
                                "url": "test_url1",
                                "format": "TSV"
                            },
                            "structure": {
                                "id": {
                                    "name": "id"
                                },
                                "attributes": [{
                                    "name": "text",
                                    "type": "String"
                                }]
                            },
                            "layout": {
                                "type": "FLAT"
                            },
                            "fixedLifetime": 300
                        }
                    ]
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict1",
                "httpSource": {
                    "url": "test_url1",
                    "format": "TSV"
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }]
                },
                "layout": {
                    "type": "FLAT"
                },
                "fixedLifetime": 300
            }
        ]
    }
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:createExternalDictionary" with data
    """
    {
        "externalDictionary": {
            "name": "test_dict2",
            "httpSource": {
                "url": "test_url2",
                "format": "TSV"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixedLifetime": 300
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add external dictionary in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterExternalDictionaryMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we "CreateExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary": {
            "name": "test_dict2",
            "http_source": {
                "url": "test_url2",
                "format": "TSV"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": 300
        }
    }
    """
    Then we get gRPC response OK
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id3"
    Then we get response with status 200
    And body at path "$.response.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict1",
                "httpSource": {
                    "url": "test_url1",
                    "format": "TSV"
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }]
                },
                "layout": {
                    "type": "FLAT"
                },
                "fixedLifetime": 300
            },
            {
                "name": "test_dict2",
                "httpSource": {
                    "url": "test_url2",
                    "format": "TSV"
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }]
                },
                "layout": {
                    "type": "FLAT"
                },
                "fixedLifetime": 300
            }
        ]
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/shards/shard1"
    Then we get response with status 200
    And body at path "$.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "dictionaries": [
            {
                "name": "test_dict1",
                "httpSource": {
                    "url": "test_url1",
                    "format": "TSV"
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }]
                },
                "layout": {
                    "type": "FLAT"
                },
                "fixedLifetime": 300
            },
            {
                "name": "test_dict2",
                "httpSource": {
                    "url": "test_url2",
                    "format": "TSV"
                },
                "structure": {
                    "id": {
                        "name": "id"
                    },
                    "attributes": [{
                        "name": "text",
                        "type": "String",
                        "hierarchical": false,
                        "injective": false,
                        "nullValue": ""
                    }]
                },
                "layout": {
                    "type": "FLAT"
                },
                "fixedLifetime": 300
            }
        ]
    }
    """

  Scenario: Adding duplicate external dictionary fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:createExternalDictionary" with data
    """
    {
        "externalDictionary": {
            "name": "test_dict",
            "httpSource": {
                "url": "test_url",
                "format": "TSV"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixedLifetime": 300
        }
    }
    """
    Then we get response with status 200
    When we "CreateExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary": {
            "name": "test_dict",
            "http_source": {
                "url": "test_url",
                "format": "TSV"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": 300
        }
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:createExternalDictionary" with data
    """
    {
        "externalDictionary": {
            "name": "test_dict",
            "httpSource": {
                "url": "test_url",
                "format": "TSV"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixedLifetime": 300
        }
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "External dictionary 'test_dict' already exists"
    }
    """
    When we "CreateExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary": {
            "name": "test_dict",
            "http_source": {
                "url": "test_url",
                "format": "TSV"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": 300
        }
    }
    """
    Then we get gRPC response error with code ALREADY_EXISTS and message "dictionary "test_dict" already exists"

  @events
  Scenario: Deleting external dictionary works
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "dictionaries": [
                        {
                            "name": "test_dict",
                            "httpSource": {
                                "url": "test_url",
                                "format": "TSV"
                            },
                            "structure": {
                                "id": {
                                    "name": "id"
                                },
                                "attributes": [{
                                    "name": "text",
                                    "type": "String"
                                }]
                            },
                            "layout": {
                                "type": "FLAT"
                            },
                            "fixedLifetime": 300
                        },
                        {
                            "name": "test_dict_2",
                            "httpSource": {
                                "url": "test_url",
                                "format": "TSV"
                            },
                            "structure": {
                                "id": {
                                    "name": "id"
                                },
                                "attributes": [{
                                    "name": "text",
                                    "type": "String"
                                }]
                            },
                            "layout": {
                                "type": "FLAT"
                            },
                            "fixedLifetime": 300
                        }
                    ]
                }
            }
        }
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:deleteExternalDictionary" with data
    """
    {
        "externalDictionaryName": "test_dict_2"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete external dictionary in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.DeleteClusterExternalDictionaryMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we "DeleteExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary_name": "test_dict_2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete external dictionary in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.DeleteClusterExternalDictionaryMetadata",
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
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.DeleteClusterExternalDictionary" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/operations/worker_task_id3"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.clickhouse.v1.Cluster",
        "id": "cid1",
        "config": {
            "version": "21.3",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "metrika": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "clickhouse": {
                "config": {
                    "defaultConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "effectiveConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [
                            {
                                "name": "test_dict",
                                "httpSource": {
                                    "url": "test_url",
                                    "format": "TSV"
                                },
                                "structure": {
                                    "id": {
                                        "name": "id"
                                    },
                                    "attributes": [{
                                        "name": "text",
                                        "type": "String",
                                        "hierarchical": false,
                                        "injective": false,
                                        "nullValue": ""
                                    }]
                                },
                                "layout": {
                                    "type": "FLAT"
                                },
                                "fixedLifetime": 300
                            }
                        ],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "userConfig": {
                        "mergeTree": {},
                        "dictionaries": [
                            {
                                "name": "test_dict",
                                "httpSource": {
                                    "url": "test_url",
                                    "format": "TSV"
                                },
                                "structure": {
                                    "id": {
                                        "name": "id"
                                    },
                                    "attributes": [{
                                        "name": "text",
                                        "type": "String",
                                        "hierarchical": false,
                                        "injective": false,
                                        "nullValue": ""
                                    }]
                                },
                                "layout": {
                                    "type": "FLAT"
                                },
                                "fixedLifetime": 300
                            }
                        ]
                    }
                },
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                }
            },
            "cloudStorage": {
                "enabled": false
            }
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id3"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_at": "**IGNORE**",
        "created_by": "user",
        "description": "Delete external dictionary in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.DeleteClusterExternalDictionaryMetadata",
            "cluster_id": "cid1"
        },
        "modified_at": "**IGNORE**",
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
            "id": "cid1",
            "name": "test",
            "description": "test cluster",
            "folder_id": "folder1",
            "status": "RUNNING",
            "created_at": "**IGNORE**",
            "environment": "PRESTABLE",
            "maintenance_window": "**IGNORE**",
            "monitoring": "**IGNORE**",
            "config": {
                "version": "**IGNORE**",
                "zookeeper": "**IGNORE**",
                "access": {},
                "backup_window_start": "**IGNORE**",
                "cloud_storage": {},
                "embedded_keeper": false,
                "mysql_protocol": false,
                "postgresql_protocol": false,
                "sql_database_management": false,
                "sql_user_management": false,
                "clickhouse": {
                    "config": {
                        "default_config": "**IGNORE**",
                        "user_config": {
                            "merge_tree": "**IGNORE**",
                            "dictionaries": [{
                                "name": "test_dict",
                                "http_source": {
                                    "url": "test_url",
                                    "format": "TSV"
                                },
                                "structure": {
                                    "id": {
                                        "name": "id"
                                    },
                                    "attributes": [{
                                        "name": "text",
                                        "type": "String"
                                    }]
                                },
                                "layout": {
                                    "type": "FLAT"
                                },
                                "fixed_lifetime": "300"
                            }],
                            "kafka": {},
                            "rabbitmq": {}
                        },
                        "effective_config": "**IGNORE**"
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    }
                }
            }
        }
    }
    """

  Scenario: Deleting nonexistent external dictionary fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:deleteExternalDictionary" with data
    """
    {
        "externalDictionaryName": "nodict"
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "External dictionary 'nodict' does not exist"
    }
    """
    When we "DeleteExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary_name": "nodict"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "dictionary "nodict" not found"

  @grpc_api
  Scenario: Update nonexistend external dictionary fails
    When we "UpdateExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary": {
            "name": "nodict",
            "http_source": {
                "url": "test_url",
                "format": "TSV"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": 300
        }
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "dictionary "nodict" not found"

  @grpc_api
  Scenario: Update external dictionary works
    When we "CreateExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary": {
            "name": "test_dict",
            "http_source": {
                "url": "test_url",
                "format": "TSV"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": 300
        }
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    When we "UpdateExternalDictionary" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "external_dictionary": {
            "name": "test_dict",
            "http_source": {
                "url": "test_url2",
                "format": "TSV"
            },
            "structure": {
                "id": {
                    "name": "id"
                },
                "attributes": [{
                    "name": "text2",
                    "type": "String"
                }]
            },
            "layout": {
                "type": "FLAT"
            },
            "fixed_lifetime": 400
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Update external dictionary in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateClusterExternalDictionaryMetadata",
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
    When "worker_task_id3" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "config": {
            "version": "21.3",
            "access": {},
            "backup_window_start": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "clickhouse": {
                "config": {
                    "default_config": "**IGNORE**",
                    "user_config": "**IGNORE**",
                    "effective_config": {
                        "builtin_dictionaries_reload_interval": "3600",
                        "dictionaries": [
                            {
                                "name": "test_dict",
                                "http_source": {
                                    "url": "test_url2",
                                    "format": "TSV"
                                },
                                "structure": {
                                    "id": {
                                        "name": "id"
                                    },
                                    "attributes": [{
                                        "name": "text2",
                                        "type": "String"
                                    }]
                                },
                                "layout": {
                                    "type": "FLAT"
                                },
                                "fixed_lifetime": "400"
                            }
                        ],
                        "keep_alive_timeout": "3",
                        "log_level": "INFORMATION",
                        "mark_cache_size": "5368709120",
                        "max_concurrent_queries": "500",
                        "max_connections": "4096",
                        "max_partition_size_to_drop": "53687091200",
                        "max_table_size_to_drop": "53687091200",
                        "merge_tree": {
                            "enable_mixed_granularity_parts": true,
                            "replicated_deduplication_window": "100",
                            "replicated_deduplication_window_seconds": "604800"
                        },
                        "kafka": {},
                        "rabbitmq": {},
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
            "zookeeper": {
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                }
            },
            "cloud_storage": {},
            "embedded_keeper": false,
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "sql_database_management": false,
            "sql_user_management": false
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id3"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_at": "**IGNORE**",
        "created_by": "user",
        "description": "Update external dictionary in ClickHouse cluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateClusterExternalDictionaryMetadata",
            "cluster_id": "cid1"
        },
        "modified_at": "**IGNORE**",
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
            "id": "cid1",
            "name": "test",
            "description": "test cluster",
            "folder_id": "folder1",
            "status": "RUNNING",
            "created_at": "**IGNORE**",
            "environment": "PRESTABLE",
            "maintenance_window": "**IGNORE**",
            "monitoring": "**IGNORE**",
            "config": {
                "version": "**IGNORE**",
                "zookeeper": "**IGNORE**",
                "access": {},
                "backup_window_start": "**IGNORE**",
                "cloud_storage": {},
                "embedded_keeper": false,
                "mysql_protocol": false,
                "postgresql_protocol": false,
                "sql_database_management": false,
                "sql_user_management": false,
                "clickhouse": {
                    "config": {
                        "default_config": "**IGNORE**",
                        "user_config": {
                            "merge_tree": "**IGNORE**",
                            "dictionaries": [{
                                "name": "test_dict",
                                "http_source": {
                                    "url": "test_url2",
                                    "format": "TSV"
                                },
                                "structure": {
                                    "id": {
                                        "name": "id"
                                    },
                                    "attributes": [{
                                        "name": "text2",
                                        "type": "String"
                                    }]
                                },
                                "layout": {
                                    "type": "FLAT"
                                },
                                "fixed_lifetime": "400"
                            }],
                            "kafka": {},
                            "rabbitmq": {}
                        },
                        "effective_config": "**IGNORE**"
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s1.porto.1"
                    }
                }
            }
        }
    }
    """
