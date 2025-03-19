Feature: MongoDB perfidag

Background:
    Given default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
                    }
                }
            },
            "performanceDiagnostics": {
                "profilingEnabled": true
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
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
       "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    And health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
               "status": "Alive"
           }
       ],
       "hosts": [
           {
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongod",
                       "role": "Master",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongod",
                       "role": "Replica",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "mongod",
                       "role": "Replica",
                       "status": "Alive"
                   }
               ]
           }
       ]
    }
    """
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create MongoDB cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": false,
        "config": {
            "version": "4.4",
            "featureCompatibilityVersion": "4.4",
            "access": {
                "webSql": false,
                "dataLens": false,
                "dataTransfer": false,
                "serverless": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "backupRetainPeriodDays": 7,
            "performanceDiagnostics": {
                "profilingEnabled": true
            },
            "mongodb_4_4": {
                "mongod": {
                    "config": {
                        "defaultConfig": {
                            "net": {
                                "maxIncomingConnections": 1024
                            },
                            "operationProfiling": {
                                "mode": "SLOW_OP",
                                "slowOpThreshold": 300
                            },
                            "storage": {
                                "journal": {
                                    "commitInterval": 100
                                },
                                "wiredTiger": {
                                    "collectionConfig": {
                                        "blockCompressor": "SNAPPY"
                                    },
                                    "engineConfig": {
                                        "cacheSizeGB": 2.0
                                    }
                                }
                            }
                        },
                        "effectiveConfig": {
                            "net": {
                                "maxIncomingConnections": 1024
                            },
                            "operationProfiling": {
                                "mode": "SLOW_OP",
                                "slowOpThreshold": 300
                            },
                            "storage": {
                                "journal": {
                                    "commitInterval": 100
                                },
                                "wiredTiger": {
                                    "collectionConfig": {
                                        "blockCompressor": "SNAPPY"
                                    },
                                    "engineConfig": {
                                        "cacheSizeGB": 2.0
                                    }
                                }
                            }
                        },
                        "userConfig": {}
                    },
                    "resources": {
                        "diskSize": 10737418240,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
                    }
                }
            }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
        "id": "cid1",
        "labels": {},
        "monitoring": [
            {
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm.yandex-team.ru/template/panel/dbaas_mongodb_metrics/cid=cid1",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid1&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mongodb",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/mongodb_cluster/cid1?section=monitoring",
                "name": "Console"
            }
        ],
        "name": "test",
        "networkId": "",
        "status": "RUNNING"
    }
    """

Scenario: Disable perf_diag works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "performanceDiagnostics": {
                "profilingEnabled": false
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.mongodb.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": false,
        "config": {
            "version": "4.4",
            "featureCompatibilityVersion": "4.4",
            "access": {
                "webSql": false,
                "dataLens": false,
                "dataTransfer": false,
                "serverless": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "backupRetainPeriodDays": 7,
            "performanceDiagnostics": {
                "profilingEnabled": false
            },
            "mongodb_4_4": {
                "mongod": {
                    "config": {
                        "defaultConfig": {
                            "net": {
                                "maxIncomingConnections": 1024
                            },
                            "operationProfiling": {
                                "mode": "SLOW_OP",
                                "slowOpThreshold": 300
                            },
                            "storage": {
                                "journal": {
                                    "commitInterval": 100
                                },
                                "wiredTiger": {
                                    "collectionConfig": {
                                        "blockCompressor": "SNAPPY"
                                    },
                                    "engineConfig": {
                                        "cacheSizeGB": 2.0
                                    }
                                }
                            }
                        },
                        "effectiveConfig": {
                            "net": {
                                "maxIncomingConnections": 1024
                            },
                            "operationProfiling": {
                                "mode": "SLOW_OP",
                                "slowOpThreshold": 300
                            },
                            "storage": {
                                "journal": {
                                    "commitInterval": 100
                                },
                                "wiredTiger": {
                                    "collectionConfig": {
                                        "blockCompressor": "SNAPPY"
                                    },
                                    "engineConfig": {
                                        "cacheSizeGB": 2.0
                                    }
                                }
                            }
                        },
                        "userConfig": {}
                    },
                    "resources": {
                        "diskSize": 10737418240,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
                    }
                }
            }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
        "id": "cid1",
        "labels": {},
        "monitoring": [
            {
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm.yandex-team.ru/template/panel/dbaas_mongodb_metrics/cid=cid1",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid1&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-mongodb",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/mongodb_cluster/cid1?section=monitoring",
                "name": "Console"
            }
        ],
        "name": "test",
        "networkId": "",
        "status": "RUNNING"
    }
    """

Scenario: GetProfilerStats
    When we "GetProfilerStats" via gRPC at "yandex.cloud.priv.mdb.mongodb.v1.PerformanceDiagnosticsService" with data
    """
    {
      "cluster_id": "cid1",
      "from_time": "1970-01-01T00:00:01Z",
      "to_time": "1970-01-02T00:00:01Z",
      "aggregate_by": "DURATION",
      "aggregation_function": "SUM",
      "page_size": 100
    }
    """
    Then we get gRPC response with body
    """
    {
        "data": [
            {
                "time": "1970-01-01T00:00:01Z",
                "dimensions": {
                    "form": "{a:1, b:1}"
                },
                "value": "10"
            }
        ],
        "next_page_token": "0"
    }
    """

Scenario: GetProfilerRecsAtTime
    When we "GetProfilerRecsAtTime" via gRPC at "yandex.cloud.priv.mdb.mongodb.v1.PerformanceDiagnosticsService" with data
    """
    {
      "cluster_id": "cid1",
      "from_time": "1970-01-01T00:00:01Z",
      "to_time": "1970-01-02T00:00:01Z",
      "request_form": "{a:1, b:1}",
      "page_size": 100
    }
    """
    Then we get gRPC response with body
    """
    {
        "data": [
            {
                "time": "1970-01-01T00:00:01Z",
                "raw": "{some_request: 'raw'}",
                "request_form": "{a:1, b:1}",
                "hostname": "man-0.db.yanex.net",
                "user": "user1",
                "ns": "db.collention",
                "operation": "query",
                "duration": "512",
                "plan_summary": "COLLSCAN",
                "response_length": "128",
                "keys_examined": "102400",
                "documents_examined": "32",
                "documents_returned": "32"
            }
        ],
        "next_page_token": "0"
    }
    """
 
Scenario: GetProfilerTopFormsByStat
    When we "GetProfilerTopFormsByStat" via gRPC at "yandex.cloud.priv.mdb.mongodb.v1.PerformanceDiagnosticsService" with data
    """
    {
      "cluster_id": "cid1",
      "from_time": "1970-01-01T00:00:01Z",
      "to_time": "1970-01-02T00:00:01Z",
      "aggregate_by": "DURATION",
      "aggregation_function": "SUM",
      "page_size": 100
    }
    """
    Then we get gRPC response with body
    """
    {
        "data": [
            {
                "request_form": "{a:1, b:1}",
                "plan_summary": "COLLSCAN",
                "queries_count": "128",
                "total_queries_duration": "102400",
                "avg_query_duration": "800",
                "total_response_length": "65536",
                "avg_response_length": "512",
                "total_keys_examined": "10000000",
                "total_documents_examined": "456",
                "total_documents_returned": "300",
                "keys_examined_per_document_returned": "33333",
                "documents_examined_per_document_returned": "2"
            }
        ],
        "next_page_token": "0"
    }
    """

Scenario: GetPossibleIndexes
    When we "GetPossibleIndexes" via gRPC at "yandex.cloud.priv.mdb.mongodb.v1.PerformanceDiagnosticsService" with data
    """
    {
      "cluster_id": "cid1",
      "from_time": "1970-01-01T00:00:01Z",
      "to_time": "1970-01-02T00:00:01Z",
      "page_size": 100
    }
    """
    Then we get gRPC response with body
    """
    {
        "data": [
            {
                "database":     "db",
                "collection":   "collection",
                "index":        "{a:1, b:1}",
                "request_count": "128"
            }
        ],
        "next_page_token": "0"
    }
    """
