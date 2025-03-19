@search
Feature: Redis search API

  Scenario: Verify search render
    Given default headers
    And feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_5"]
    """
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "testRedis",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "p@ssw#$rd!?"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }]
    }
    """
    Then we get response with status 200
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "timestamp": "<TIMESTAMP>",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testRedis",
        "service": "managed-redis",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testRedis",
            "description": "",
            "labels": {},
            "hosts": ["myt-1.db.yandex.net"]
        }
    }
    """
