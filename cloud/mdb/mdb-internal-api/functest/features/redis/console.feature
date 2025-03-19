Feature: Redis console API

  Background:
    Given feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_5", "MDB_REDIS_ALLOW_DEPRECATED_6"]
    """
    And default headers

  Scenario: Console stats
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
    Then we get response with status 200
    When we GET "/mdb/redis/1.0/console/clusters:stats?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clustersCount": 1
    }
    """

  Scenario: Console stats works when no clusters created
    When we GET "/mdb/redis/1.0/console/clusters:stats?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clustersCount": 0
    }
    """

  Scenario: Get console config
        right now don't check
        hostTypes.*, cause it looks
        ugly, we should decide how to check it
    When we GET "/mdb/redis/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusterName": {
            "blacklist": [],
            "max": 63,
            "min": 1,
            "regexp": "[a-zA-Z0-9_-]+"
        },
        "hostCountLimits": {
            "maxHostCount": 7,
            "minHostCount": 1
        },
        "password": {
            "blacklist": [],
            "max": 128,
            "min": 8,
            "regexp": "[a-zA-Z0-9@=+?*.,!&#$^<>_-]*"
        },
        "versions": [
            "5.0",
            "6.0"
        ],
        "tlsSupportedVersions": ["6.0"],
        "persistenceModes": ["ON", "OFF"],
        "availableVersions": [
            { "id": "5.0", "name": "5.0", "deprecated": true, "updatableTo": ["6.0"] },
            { "id": "6.0", "name": "6.0", "deprecated": true, "updatableTo": [] }
        ],
        "defaultVersion": "5.0"
    }
    """

  Scenario: Get console config with feature flag
        right now don't check
        hostTypes.*, cause it looks
        ugly, we should decide how to check it
    Given feature flags
    """
    ["MDB_REDIS_62", "MDB_REDIS_70"]
    """
    When we GET "/mdb/redis/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusterName": {
            "blacklist": [],
            "max": 63,
            "min": 1,
            "regexp": "[a-zA-Z0-9_-]+"
        },
        "hostCountLimits": {
            "maxHostCount": 7,
            "minHostCount": 1
        },
        "password": {
            "blacklist": [],
            "max": 128,
            "min": 8,
            "regexp": "[a-zA-Z0-9@=+?*.,!&#$^<>_-]*"
        },
        "versions": [
            "5.0",
            "6.0",
            "6.2",
            "7.0"
        ],
        "tlsSupportedVersions": ["6.0", "6.2", "7.0"],
        "persistenceModes": ["ON", "OFF"],
        "availableVersions": [
            { "id": "5.0", "name": "5.0", "deprecated": true, "updatableTo": ["6.0"] },
            { "id": "6.0", "name": "6.0", "deprecated": true, "updatableTo": ["6.2"] },
            { "id": "6.2", "name": "6.2", "deprecated": false, "updatableTo": [] },
            { "id": "7.0", "name": "7.0", "deprecated": false, "updatableTo": [] }
        ],
        "defaultVersion": "5.0"
    }
    """

  Scenario: Get console config and check resourcePresets against schema
    When we GET "/mdb/redis/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body matches "clusters_config_schema_common.json" schema

  Scenario: Get console config zones in resourcePresets
    When we GET "/mdb/redis/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body at path "$.resourcePresets[*].zones[*].zoneId" contains only
    """
    ["myt", "vla", "sas", "iva", "man"]
    """

  Scenario: Get Redis sorted resource presets
    When we GET "/mdb/redis/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body at path "$.resourcePresets[*]" contains fields with order: type;-generation;cpuLimit;cpuFraction;memoryLimit;presetId
