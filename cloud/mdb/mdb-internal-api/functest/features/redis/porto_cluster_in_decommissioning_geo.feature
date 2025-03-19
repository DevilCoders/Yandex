@decommission @geo
Feature: Create Porto redis Cluster with host in decommissioning geo

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_5"]
    """
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
                "resourcePresetId": "s1.porto.2",
                "diskSize": 42949672960
            }
        },
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
    And "worker_task_id1" acquired and finished by worker
    And "iva-1.db.yandex.net" change fqdn to "ugr-1.db.yandex.net" and geo to "ugr"

  Scenario: Scale up resourcePreset is forbidden
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
           "resources": {
               "resourcePresetId": "s1.porto.3"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "No new resources could be created in zone 'ugr'"
    }
    """

  Scenario: Scale up resourcePreset is allowed if feature flag present
    Given feature flags
    """
    ["MDB_ALLOW_DECOMMISSIONED_ZONE_USE", "MDB_REDIS"]
    """
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
           "resources": {
               "resourcePresetId": "s1.porto.3"
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.redis.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """

  Scenario: Scale down resourcePreset with noeviction works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
           "resources": {
               "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    Then we get response with status 200

  Scenario: Scale down resourcePreset is ok
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_5_0": {
                "maxmemoryPolicy": "ALLKEYS_RANDOM"
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id2"
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
           "resources": {
               "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id3"
    }
    """

  Scenario: Scale up diskSize is ok
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
           "resources": {
               "diskSize": 68719476736
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id2"
    }
    """

  Scenario: Deleting host works for host from dismmissing geo
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["ugr-1.db.yandex.net"]
    }
    """
    Then we get response with status 200

  Scenario: Deleting host works for host from avaliable geo
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    Then we get response with status 200

  Scenario: Creating host in available zone
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
