Feature: MongoDB console API

  Background:
    Given default headers

  Scenario: Console stats
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
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
    When we GET "/mdb/mongodb/1.0/console/clusters:stats?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clustersCount": 1
    }
    """

  Scenario: Console stats works when no clusters created
    When we GET "/mdb/mongodb/1.0/console/clusters:stats?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clustersCount": 0
    }
    """

  Scenario: Get console config
    When we GET "/mdb/mongodb/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusterName": {
            "blacklist": [],
            "max": 63,
            "min": 1,
            "regexp": "[a-zA-Z0-9_-]+"
        },
        "dbName": {
            "blacklist": ["admin", "local", "config", "mdb_internal"],
            "max": 63,
            "min": 1,
            "regexp": "[a-zA-Z0-9_-]+"
        },
        "hostCountLimits": {
            "hostCountPerDiskType": [
                {
                    "diskTypeId": "local-nvme",
                    "minHostCount": 3
                }
            ],
            "maxHostCount": 7,
            "minHostCount": 0
        },
        "password": {
            "blacklist": [],
            "max": 128,
            "min": 8,
            "regexp": ".*"
        },
        "userName": {
            "blacklist": ["admin", "monitor", "root"],
            "max": 63,
            "min": 1,
            "regexp": "[a-zA-Z0-9_][a-zA-Z0-9_-]*"
        },
        "versions": ["3.6", "4.0", "4.2", "4.4", "5.0"],
        "availableVersions": [
            { "id": "3.6", "name": "3.6", "deprecated": true, "updatableTo": ["4.0"] },
            { "id": "4.0", "name": "4.0", "deprecated": true, "updatableTo": ["4.2"] },
            { "id": "4.2", "name": "4.2", "deprecated": false, "updatableTo": ["4.4"] },
            { "id": "4.4", "name": "4.4", "deprecated": false, "updatableTo": ["5.0"] },
            { "id": "5.0", "name": "5.0", "deprecated": false, "updatableTo": [] }
        ],
        "availableFCVs": [
            { "id": "3.6", "fcvs": ["3.4", "3.6"] },
            { "id": "4.0", "fcvs": ["3.6", "4.0"] },
            { "id": "4.2", "fcvs": ["4.0", "4.2"] },
            { "id": "4.4", "fcvs": ["4.2", "4.4"] },
            { "id": "4.4-enterprise", "fcvs": ["4.2", "4.4"] },
            { "id": "5.0", "fcvs": ["4.4", "5.0"] },
            { "id": "5.0-enterprise", "fcvs": ["4.4", "5.0"] }
        ],
        "defaultVersion": "5.0"
    }
    """

  Scenario: Get console config and check resourcePresets against schema
    When we GET "/mdb/mongodb/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body matches "mongodb/clusters_config_schema.json" schema

  Scenario: Get console config zones in resourcePresets
    When we GET "/mdb/mongodb/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body at path "$.hostTypes[*].resourcePresets[*].zones[*].zoneId" contains only
    """
    ["myt", "vla", "sas", "iva", "man"]
    """

  Scenario: Get MongoDB sorted resource presets
    When we GET "/mdb/mongodb/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body at path "$.hostTypes[0].resourcePresets[*]" contains fields with order: type;-generation;cpuLimit;cpuFraction;memoryLimit;presetId
    And body at path "$.hostTypes[1].resourcePresets[*]" contains fields with order: type;-generation;cpuLimit;cpuFraction;memoryLimit;presetId
    And body at path "$.hostTypes[2].resourcePresets[*]" contains fields with order: type;-generation;cpuLimit;cpuFraction;memoryLimit;presetId
