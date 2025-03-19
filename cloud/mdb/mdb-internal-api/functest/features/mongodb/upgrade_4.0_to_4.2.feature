Feature: Porto MongoDB Cluster version upgrade 4.0 to 4.2

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_MONGODB_ALLOW_DEPRECATED_VERSIONS"]
    """
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_0": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
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

  Scenario: Upgrade 4.0 to 4.2 works
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "4.2"
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
    When we GET "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": false,
        "config": {
            "version": "4.2",
            "featureCompatibilityVersion": "4.0",
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
            "mongodb_4_2": {
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
        }
    }
    """

  Scenario: Upgrade 4.0 to unsupported version fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "4.1"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Version '4.1' is not available, allowed versions are: 3.6, 4.0, 4.2, 4.4, 5.0"
    }
    """

  Scenario: Upgrade 4.0 to 4.2 with bad fcv fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "featureCompatibilityVersion": "3.6"
        }
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "4.2"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Current feature compatibility version 3.6 is invalid for upgrade to version 4.2, expected: 4.0"
    }
    """

  Scenario: Downgrade 4.2 to 4.0 fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "4.2"
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
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "4.0"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Version downgrade detected"
    }
    """

  Scenario: Upgrade 4.0 to 4.2 mixed with another changes fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "4.2",
            "featureCompatibilityVersion": "4.2"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Version change cannot be mixed with other modifications"
    }
    """
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "4.2",
            "mongodbSpec_4_2": {
                "mongod": {
                    "config": {
                        "net": {
                            "maxIncomingConnections": 256
                        }
                    }
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Version change cannot be mixed with other modifications"
    }
    """

