Feature: Create Porto Redis 6.0 Cluster Validation
  Background:
    Given default headers
    And feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_6"]
    """

  Scenario: Create cluster with bad notifyKeyspaceEvents fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "p@ssw#$rd!?",
                "notifyKeyspaceEvents": "B"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt",
            "replicaPriority": 50
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
       "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.redisConfig_6_0.notifyKeyspaceEvents: Notify keyspace events 'B' should be a subset of KEg$lshzxeAtm"
    }
    """

  Scenario: Create cluster with no hosts fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.5",
                "diskSize": 1073741824000
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

  Scenario: Create cluster with no password fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.5",
                "diskSize": 1073741824000
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.redisConfig_6_0.password: Missing data for required field."
    }
    """

  Scenario: Create standalone cluster with specified shard name fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "12345678"
            },
            "resources": {
                "resourcePresetId": "s1.porto.5",
                "diskSize": 1073741824000
            }
        },
        "hostSpecs": [{
            "zoneId": "myt",
            "shardName": "shard"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nCan't define shards in standalone mode."
    }
    """

  Scenario: Create cluster with nonexistent flavor fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "invalid",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "resourcePreset 'invalid' does not exist"
    }
    """

  Scenario: Create cluster with invalid password fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "inv*lid passw?rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.5",
                "diskSize": 1073741824000
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.redisConfig_6_0.password: Password 'inv*lid passw?rd' does not conform to naming rules"
    }
    """

  Scenario: Create cluster with no disk size fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.5"
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.resources.diskSize: Missing data for required field."
    }
    """

  Scenario: Create cluster with insufficient disk size fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.5",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Disk size must be at least two times larger than memory size (137438953472 for the current resource preset)"
    }
    """

  Scenario: Create cluster with insufficient quota fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.5",
                "diskSize": 1073741824000
            }
        },
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Quota limits exceeded, not enough cpu: 24 cores, memory: 96 GiB, ssd_space: 2.34 TiB",
        "details": [
            {
                "@type": "type.private-api.yandex-cloud.ru/quota.QuotaFailure",
                "cloud_id": "cloud1",
                "violations": [
                    {
                        "metric": {
                            "limit": 24,
                            "name": "mdb.cpu.count",
                            "usage": 0
                        },
                        "required": 48
                    },
                    {
                        "metric": {
                            "limit": 103079215104,
                            "name": "mdb.memory.size",
                            "usage": 0
                        },
                        "required": 206158430208
                    },
                    {
                        "metric": {
                            "limit": 644245094400,
                            "name": "mdb.ssd.size",
                            "usage": 0
                        },
                        "required": 3221225472000
                    }
                ]
            }
        ]
    }
    """

  Scenario: Create cluster with read-only token fails
    Given default headers with "ro-token" token
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240
            }
        },
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
    Then we get response with status 403 and body contains
    """
    {
        "code": 7,
        "message": "You do not have permission to access the requested object or object does not exist"
    }
    """

  Scenario: Create cluster with host in unknown zone fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "nodc"
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nhostSpecs.0.zoneId: Invalid value, valid value is one of ['iva', 'man', 'myt', 'sas', 'vla']"
    }
    """

  Scenario: Create cluster without feature flag fails and shows only available versions
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "6",
            "redisConfig_6_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "man"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Version '6' is not available, allowed versions are: 5.0, 6.0"
    }
    """

  Scenario: Create cluster with incorrect version fails
    Given feature flags
    """
    ["MDB_REDIS_62"]
    """
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "6",
            "redisConfig_6_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "man"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Version '6' is not available, allowed versions are: 5.0, 6.0, 6.2"
    }
    """

  @decommission @geo
  Scenario: Create cluster in dismissing geo
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "p@ssw#$rd!?"
            },
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 34359738368
            }
        },
        "hostSpecs": [{
            "zoneId": "ugr"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "No new resources could be created in zone 'ugr'"
    }
    """

  Scenario: Create sharded cluster with incorrect number of shards fails
    Given feature flags
    """
    ["MDB_REDIS_SHARDING"]
    """
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "sharded": true,
        "configSpec": {
            "redisConfig_6_0": {
                "password": "p@ssw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.5",
                "diskSize": 137438953472
            }
        },
        "hostSpecs": [{
            "zoneId": "myt",
            "shardName": "shard1"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Number of shards must be between 3 and 10."
    }
    """

  Scenario: Create sharded cluster without shard names fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "sharded": true,
        "configSpec": {
            "redisConfig_6_0": {
                "password": "p@ssw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.5",
                "diskSize": 137438953472
            }
        },
        "hostSpecs": [
            {
                "zoneId": "myt",
                "shardName": "shard1"
            },{
                "zoneId": "iva",
                "shardName": "shard2"
            },{
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nShard name is not specified for host in sas."
    }
    """
