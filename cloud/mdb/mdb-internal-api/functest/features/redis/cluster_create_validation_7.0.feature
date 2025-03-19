Feature: Create Porto Redis 7.0 Cluster Validation
  Background:
    Given default headers
    And feature flags
    """
    ["MDB_REDIS_70"]
    """

  Scenario: Create cluster with bad notifyKeyspaceEvents fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_7_0": {
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
        "message": "The request is invalid.\nconfigSpec.redisConfig_7_0.notifyKeyspaceEvents: Notify keyspace events 'B' should be a subset of KEg$lshzxeAtmnd"
    }
    """

