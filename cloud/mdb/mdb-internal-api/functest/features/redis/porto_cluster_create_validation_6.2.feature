Feature: Create Porto Redis 6.2 Cluster Validation
  Background:
    Given default headers


  Scenario: Create sharded cluster with empty password fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_2": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.5",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt",
            "shardName": "shard1"
        }, {
            "zoneId": "iva",
            "shardName": "shard2"
        }, {
            "zoneId": "sas",
            "shardName": "shard3"
        }],
        "sharded": true
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.redisConfig_6_2.password: Missing data for required field."
    }
    """


  Scenario: Create single cluster with empty password fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_2": {
                "password": ""
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
        "message": "The request is invalid.\nconfigSpec.redisConfig_6_2.password: Password must be between 8 and 128 characters long"
    }
    """


  Scenario: Create cluster without feature flag fails and shows only available versions
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "6.2",
            "redisConfig_6_2": {
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
        "message": "Version '6.2' is not available, allowed versions are: 5.0, 6.0"
    }
    """
