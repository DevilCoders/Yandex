Feature: Create Compute Redis Cluster Validation
  Background:
    Given feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_5"]
    """
    And default headers


  Scenario: Create cluster without tls and with public ip fails
    Given feature flags
    """
    []
    """
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt",
            "assignPublicIp": true
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Public ip for host requires TLS enabled."
    }
    """


  Scenario: Create cluster with empty network id fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": ""
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Unable to find subnet in zone 'myt'"
    }
    """


  Scenario: Create cluster without redisConfig fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "5.0",
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Password is not specified in spec"
    }
    """


  Scenario: Create cluster with local ssd and 2 hosts fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "local-ssd",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "vla"
        }],
        "description": "test cluster",
        "networkId": "network1",
        "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Redis shard with resource preset 's1.compute.1' and disk type 'local-ssd' requires at least 3 hosts"
    }
    """

  Scenario: Create cluster with local ssd and single host in shard fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
      "name": "test",
      "environment": "PRESTABLE",
      "configSpec": {
          "redisConfig_5_0": {
              "password": "p@ssw#$rd!?"
          },
          "resources": {
              "resourcePresetId": "s1.compute.1",
              "diskTypeId": "local-ssd",
              "diskSize": 17179869184
          }
      },
      "hostSpecs": [{
          "zoneId": "myt",
          "shardName": "shard1"
      }, {
          "zoneId": "vla",
          "shardName": "shard2"
      }, {
          "zoneId": "sas",
          "shardName": "shard3"
      }],
      "description": "test cluster",
      "networkId": "network1",
      "sharded": true
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Shard shard1 should contain at least 1 replica."
    }
    """

  Scenario: Create cluster without network id fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Unable to find subnet in zone 'myt'"
    }
    """

  Scenario: Create cluster with nonexistent subnet in network fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
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
        "message": "Unable to find subnet in zone 'man'"
    }
    """

  Scenario: Create cluster with nonuniq subnet in network fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "iva"
        }],
        "description": "test cluster",
        "networkId": "network2"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Multiple subnets in zone 'iva'"
    }
    """

  Scenario: Create cluster with invalid subnet fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt",
            "subnetId": "nosubnet"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Unable to find subnet with id 'nosubnet' in zone 'myt'"
    }
    """

  Scenario: Create cluster with incorrect disk unit fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418245
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
        "code": 9,
        "message": "Disk size must be a multiple of 4194304 bytes"
    }
    """

  @buffers
  Scenario: Create cluster with incorrect memory unit fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd",
                "clientOutputBufferLimitPubsub": {
                    "hardLimit": 36777216,
                    "hardLimitUnit": "tyz",
                    "softLimit":  3388608,
                    "softLimitUnit": "BYTES",
                    "softSeconds": 30
                }
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418245
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
        "message": "The request is invalid.\nconfigSpec.redisConfig_5_0.clientOutputBufferLimitPubsub.hardLimitUnit: Invalid value 'tyz', allowed values: BYTES, GIGABYTES, KILOBYTES, MEGABYTES"
    }
    """

  @buffers
  Scenario: Create cluster with incorrect max bytes fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd",
                "clientOutputBufferLimitPubsub": {
                    "hardLimit": 100000000000,
                    "hardLimitUnit": "BYTES",
                    "softLimit":  3388608,
                    "softLimitUnit": "BYTES",
                    "softSeconds": 30
                }
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Client output buffer limit can't exceed 10Gb"
    }
    """

  @buffers
  Scenario: Create cluster with incorrect max kilobytes fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd",
                "clientOutputBufferLimitNormal": {
                    "hardLimit": 100000000,
                    "hardLimitUnit": "KILOBYTES",
                    "softLimit":  3388608,
                    "softLimitUnit": "BYTES",
                    "softSeconds": 30
                }
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Client output buffer limit can't exceed 10Gb"
    }
    """

  @buffers
  Scenario: Create cluster with incorrect max megabytes fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd",
                "clientOutputBufferLimitPubsub": {
                    "hardLimit": 100,
                    "hardLimitUnit": "BYTES",
                    "softLimit": 100000,
                    "softLimitUnit": "MEGABYTES",
                    "softSeconds": 30
                }
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Client output buffer limit can't exceed 10Gb"
    }
    """

  @buffers
  Scenario: Create cluster with incorrect max gigabytes fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd",
                "clientOutputBufferLimitNormal": {
                    "hardLimit": 10000,
                    "hardLimitUnit": "KILOBYTES",
                    "softLimit": 11,
                    "softLimitUnit": "GIGABYTES",
                    "softSeconds": 30
                }
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Client output buffer limit can't exceed 10Gb"
    }
    """
