Feature: Sharded Porto MongoDB Cluster

  Background:
    Given default headers
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_5_0": {
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
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    When "worker_task_id1" acquired and finished by worker

  Scenario: Change resources of mongocfg with not sharded cluster fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongocfg": {
                    "resources": {
                        "diskSize": 21474836480
                    }
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Sharding must be enabled in order to change mongocfg settings"
    }
    """

  Scenario: Change config of mongos with not sharded cluster fails
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongos": {
                    "config": {
                        "net": {
                            "maxIncomingConnections": 512
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
        "code": 9,
        "message": "Sharding must be enabled in order to change mongos settings"
    }
    """


  Scenario: Enabling sharding with 2 mongocfg hosts fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Only 2 MONGOCFG + MONGOINFRA hosts provided, but at least 3 needed"
    }
    """


  Scenario: Enabling sharding without mongocfg or mongos hosts fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
             {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "iva",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Only 0 MONGOS + MONGOINFRA hosts provided, but at least 2 needed"
    }
    """
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Only 0 MONGOCFG + MONGOINFRA hosts provided, but at least 3 needed"
    }
    """
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": []
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nhostSpecs: Shorter than minimum length 1."
    }
    """

  Scenario Outline: Enabling sharding with unsupported versions fails
    Given feature flags
    """
    ["MDB_MONGODB_ALLOW_DEPRECATED_VERSIONS", "MDB_MONGODB_ENTERPRISE"]
    """
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test_sharding_enable_fails",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_<spec_version_suffix>": {
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
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mongodb/1.0/clusters/cid2:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Sharding requires one of following mongodb versions: 4.0, 4.2, 4.4, 5.0"
    }
    """
    Examples:
      | version            | spec_version_suffix            |
      | 3.6                | 3_6                            |
      | 4.4-enterprise     | 4_4_enterprise                 |
      | 5.0-enterprise     | 5_0_enterprise                 |


  Scenario: Enabling sharding without specifying host type fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Type of MongoDB host is not specified"
    }
    """

  Scenario: Enabling sharding with insufficient hosts fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Only 2 MONGOCFG + MONGOINFRA hosts provided, but at least 3 needed"
    }
    """

  Scenario: Enabling sharding with MONGOD host type fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOD"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "MONGOS, MONGOCFG or MONGOINFRA host type expected"
    }
    """

  Scenario: Enabling sharding second time fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Enable sharding for MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.EnableClusterShardingMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Sharding has been already enabled"
    }
    """

  @decommission @geo
  Scenario: Enabled sharding in decommissioning geo fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "ugr",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "No new resources could be created in zone 'ugr'"
    }
    """

  Scenario: Enabling sharding with insufficient quota fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.5",
                "diskSize": 2199023255552,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.4",
                "diskSize": 2199023255552,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Quota limits exceeded, not enough cpu: 35 cores, memory: 140 GiB, ssd_space: 9.44 TiB",
        "details": [
            {
                "@type": "type.private-api.yandex-cloud.ru/quota.QuotaFailure",
                "cloud_id": "cloud1",
                "violations": [
                    {
                        "metric": {
                            "limit": 24,
                            "name": "mdb.cpu.count",
                            "usage": 3
                        },
                        "required": 59
                    },
                    {
                        "metric": {
                            "limit": 103079215104,
                            "name": "mdb.memory.size",
                            "usage": 12884901888
                        },
                        "required": 253403070464
                    },
                    {
                        "metric": {
                            "limit": 644245094400,
                            "name": "mdb.ssd.size",
                            "usage": 32212254720
                        },
                        "required": 11027328532480
                    }
                ]
            }
        ]
    }
    """


  Scenario: Attempt to exceed shard count limit fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mongodb/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "zoneId": "sas"
            }, {
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id3" acquired and finished by worker

    When we POST "/mdb/mongodb/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard3",
        "hostSpecs": [
            {
                "zoneId": "sas"
            }, {
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id4" acquired and finished by worker

    When we POST "/mdb/mongodb/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard4",
        "hostSpecs": [
            {
                "zoneId": "sas"
            }, {
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Cluster can have up to a maximum of 3 MongoDB shards"
    }
    """

  Scenario: Attempt to exceed shard count limit with enabled MDB_MONGODB_UNLIMITED_SHARD_COUNT succeeds
    Given feature flags
    """
    ["MDB_MONGODB_UNLIMITED_SHARD_COUNT"]
    """
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker

    When we POST "/mdb/mongodb/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "zoneId": "sas"
            }, {
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id3" acquired and finished by worker
    When we POST "/mdb/mongodb/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard3",
        "hostSpecs": [
            {
                "zoneId": "sas"
            }, {
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id4" acquired and finished by worker
    When we POST "/mdb/mongodb/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard4",
        "hostSpecs": [
            {
                "zoneId": "sas"
            }, {
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 200

  Scenario: Enabling sharding with only MongoInfra hosts works
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongoinfra": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOINFRA"
            },
            {
                "zoneId": "sas",
                "type": "MONGOINFRA"
            },
            {
                "zoneId": "vla",
                "type": "MONGOINFRA"
            }
        ]
    }
    """
    Then we get response with status 200

  Scenario: Enabling sharding with mixed configuration fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "mongoinfra": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOINFRA"
            },
            {
                "zoneId": "sas",
                "type": "MONGOINFRA"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Using MongoCFG instances together with MongoInfra is not supported"
    }
    """

  Scenario: Enabling sharding with mixed configuration with feature flag works
    Given feature flags
    """
    ["MDB_MONGODB_INFRA_CFG"]
    """
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "mongoinfra": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOINFRA"
            },
            {
                "zoneId": "sas",
                "type": "MONGOINFRA"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 200

  Scenario: Adding MongoCFG to cluster with MongoInfra fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongoinfra": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOINFRA"
            },
            {
                "zoneId": "sas",
                "type": "MONGOINFRA"
            },
            {
                "zoneId": "vla",
                "type": "MONGOINFRA"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "vla",
            "type": "MONGOCFG"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Using MongoCFG instances together with MongoInfra is not supported"
    }
    """

  Scenario: Adding MongoCFG to cluster with MongoInfra with feature flag works
    Given feature flags
    """
    ["MDB_MONGODB_INFRA_CFG"]
    """
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongoinfra": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOINFRA"
            },
            {
                "zoneId": "sas",
                "type": "MONGOINFRA"
            },
            {
                "zoneId": "vla",
                "type": "MONGOINFRA"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "vla",
            "type": "MONGOCFG"
        }]
    }
    """
    Then we get response with status 200

  Scenario: Adding MongoInfra host to MongoCfg cluster fails
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "vla",
            "type": "MONGOINFRA"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Using MongoCFG instances together with MongoInfra is not supported"
    }
    """

  Scenario: Adding 3 MongoInfra host to MongoCfg cluster with feature flag works
    Given feature flags
    """
    ["MDB_MONGODB_INFRA_CFG"]
    """
    When we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "vla",
            "type": "MONGOINFRA"
        }]
    }
    """
    Then we get response with status 200
    When "worker_task_id3" acquired and finished by worker
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "vla",
            "type": "MONGOINFRA"
        }]
    }
    """
    Then we get response with status 200
    When "worker_task_id4" acquired and finished by worker
    When we POST "/mdb/mongodb/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "vla",
            "type": "MONGOINFRA"
        }]
    }
    """
    Then we get response with status 200
