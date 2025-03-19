Feature: MySQL console API

  Background:
    Given default headers

  Scenario: Console stats
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
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
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 200
    When we GET "/mdb/mysql/1.0/console/clusters:stats?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clustersCount": 1
    }
    """

  Scenario: Console stats works when no clusters created
    When we GET "/mdb/mysql/1.0/console/clusters:stats?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clustersCount": 0
    }
    """

  Scenario: Get console config
    When we GET "/mdb/mysql/1.0/console/clusters:config?folderId=folder1"
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
            "blacklist": ["mysql", "sys", "information_schema", "performance_schema"],
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
            "minHostCount": 1
        },
        "password": {
            "blacklist": [],
            "max": 128,
            "min": 8,
            "regexp": "[^\u0000'\"\b\n\r\t\u001a\\%]*"
        },
        "userName": {
            "blacklist": ["admin", "monitor", "root", "repl", "mysql.session", "mysql.sys"],
            "max": 32,
            "min": 1,
            "regexp": "[a-zA-Z0-9_][a-zA-Z0-9_-]*"
        },
        "versions": ["5.7"],
        "availableVersions": [
            { "id": "5.7", "name": "5.7", "deprecated": false, "updatableTo": [] }
        ],
        "defaultVersion": "5.7"
    }
    """

  Scenario: Get console config with feature flag
    Given feature flags
    """
    ["MDB_MYSQL_8_0", "MDB_MYSQL_5_7_TO_8_0_UPGRADE"]
    """
    When we GET "/mdb/mysql/1.0/console/clusters:config?folderId=folder1"
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
            "blacklist": ["mysql", "sys", "information_schema", "performance_schema"],
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
            "minHostCount": 1
        },
        "password": {
            "blacklist": [],
            "max": 128,
            "min": 8,
            "regexp": "[^\u0000'\"\b\n\r\t\u001a\\%]*"
        },
        "userName": {
            "blacklist": ["admin", "monitor", "root", "repl", "mysql.session", "mysql.sys"],
            "max": 32,
            "min": 1,
            "regexp": "[a-zA-Z0-9_][a-zA-Z0-9_-]*"
        },
        "versions": ["5.7", "8.0"],
        "availableVersions": [
            { "id": "5.7", "name": "5.7", "deprecated": false, "updatableTo": ["8.0"] },
            { "id": "8.0", "name": "8.0", "deprecated": false, "updatableTo": [] }
        ],
        "defaultVersion": "8.0"
    }
    """

  Scenario: Get console config GRPC
    And we shrink valid resources in metadb for flavor "s1.compute.1,s2.compute.1,s2.compute.2,s3.compute.1,s3.compute.2"
    And we shrink valid resources in metadb for disk "network-ssd"
    And we shrink valid resources in metadb for geo "vla"
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.mysql.v1.console.ClusterService" with data
    """
    {
    "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_name": {
            "blacklist": [],
            "max": "63",
            "min": "1",
            "regexp": "^[a-zA-Z0-9_-]+$"
        },
        "db_name": {
            "blacklist": ["mysql", "sys", "information_schema", "performance_schema"],
            "max": "63",
            "min": "1",
            "regexp": "^[a-zA-Z0-9_-]+$"
        },
        "host_count_limits": {
            "host_count_per_disk_type": [],
            "max_host_count": "7",
            "min_host_count": "1"
        },
        "password": {
            "blacklist": [],
            "max": "128",
            "min": "8",
            "regexp": "[^\u0000'\"\b\n\r\t\u001a\\%]*"
        },
        "user_name": {
            "blacklist": ["admin", "monitor", "root", "mdb_admin", "repl", "mysql.session", "mysql.sys"],
            "max": "32",
            "min": "1",
            "regexp": "^[a-zA-Z0-9_][a-zA-Z0-9_-]*$"
        },
        "versions": ["5.7", "8.0"],
        "available_versions": [
            { "id": "5.7", "name": "5.7", "deprecated": false, "updatable_to": ["8.0"] },
            { "id": "8.0", "name": "8.0", "deprecated": false, "updatable_to": [] }
        ],
        "default_version": "8.0",
        "default_resources": {
                "disk_size": "1024",
                "disk_type_id": "test_disk_type_id",
                "generation": "1",
                "generation_name": "Haswell",
                "resource_preset_id": "test_preset_id"
        },
        "resource_presets": [
            {
                "cpu_fraction": "100",
                "cpu_limit": "1",
                "decomissioning": false,
                "generation": "1",
                "generation_name": "Haswell",
                "memory_limit": "4294967296",
                "preset_id": "s1.compute.1",
                "type": "standard",
                "zones": [
                    {
                        "disk_types": [
                            {
                                "disk_size_range": {
                                    "max": "2199023255552",
                                    "min": "10737418240"
                                },
                                "disk_type_id": "network-ssd",
                                "max_hosts": "7",
                                "min_hosts": "1"
                            }
                        ],
                        "zone_id": "vla"
                    }
                ]
            },
            {
                "cpu_fraction": "100",
                "cpu_limit": "1",
                "decomissioning": false,
                "generation": "2",
                "generation_name": "Broadwell",
                "memory_limit": "4294967296",
                "preset_id": "s2.compute.1",
                "type": "standard",
                "zones": [
                    {
                        "disk_types": [
                            {
                                "disk_size_range": {
                                    "max": "2199023255552",
                                    "min": "10737418240"
                                },
                                "disk_type_id": "network-ssd",
                                "max_hosts": "7",
                                "min_hosts": "1"
                            }
                        ],
                        "zone_id": "vla"
                    }
                ]
            },
            {
                "cpu_fraction": "100",
                "cpu_limit": "2",
                "decomissioning": false,
                "generation": "2",
                "generation_name": "Broadwell",
                "memory_limit": "8589934592",
                "preset_id": "s2.compute.2",
                "type": "standard",
                "zones": [
                    {
                        "disk_types": [
                            {
                                "disk_size_range": {
                                    "max": "2199023255552",
                                    "min": "10737418240"
                                },
                                "disk_type_id": "network-ssd",
                                "max_hosts": "7",
                                "min_hosts": "1"
                            }
                        ],
                        "zone_id": "vla"
                    }
                ]
            },
            {
                "cpu_fraction": "100",
                "cpu_limit": "1",
                "decomissioning": false,
                "generation": "3",
                "generation_name": "Cascade Lake",
                "memory_limit": "4294967296",
                "preset_id": "s3.compute.1",
                "type": "standard",
                "zones": [
                    {
                        "disk_types": [
                            {
                                "disk_size_range": {
                                    "max": "2199023255552",
                                    "min": "10737418240"
                                },
                                "disk_type_id": "network-ssd",
                                "max_hosts": "7",
                                "min_hosts": "1"
                            }
                        ],
                        "zone_id": "vla"
                    }
                ]
            },
            {
                "cpu_fraction": "100",
                "cpu_limit": "2",
                "decomissioning": false,
                "generation": "3",
                "generation_name": "Cascade Lake",
                "memory_limit": "8589934592",
                "preset_id": "s3.compute.2",
                "type": "standard",
                "zones": [
                    {
                        "disk_types": [
                            {
                                "disk_size_range": {
                                    "max": "2199023255552",
                                    "min": "10737418240"
                                },
                                "disk_type_id": "network-ssd",
                                "max_hosts": "7",
                                "min_hosts": "1"
                            }
                        ],
                        "zone_id": "vla"
                    }
                ]
            }
        ]
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.mysql.v1.console.ClusterService" with data
    """
    {
      "folder_id": "folder1",
      "host_group_ids": "hg6"
    }
    """
    Then we get gRPC response with body
    # Note: hg6 is dedicated host with 1 Core. So, only one flavor matches: `s3.compute.1`
    """
    {
        "host_count_limits": {
            "host_count_per_disk_type": [],
            "max_host_count": "7",
            "min_host_count": "1"
        },
        "default_resources": {
                "disk_size": "1024",
                "disk_type_id": "test_disk_type_id",
                "generation": "1",
                "generation_name": "Haswell",
                "resource_preset_id": "test_preset_id"
        },
        "resource_presets": [
            {
                "cpu_fraction": "100",
                "cpu_limit": "1",
                "decomissioning": false,
                "generation": "3",
                "generation_name": "Cascade Lake",
                "memory_limit": "4294967296",
                "preset_id": "s3.compute.1",
                "type": "standard",
                "zones": [
                    {
                        "disk_types": [
                            {
                                "disk_size_range": {
                                    "max": "2199023255552",
                                    "min": "10737418240"
                                },
                                "disk_type_id": "network-ssd",
                                "max_hosts": "7",
                                "min_hosts": "1"
                            }
                        ],
                        "zone_id": "vla"
                    }
                ]
            }
        ]
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.mysql.v1.console.ClusterService" with data
    """
    {
    "folder_id": "folder1",
    "host_group_ids": "hg5"
    }
    """
    Then we get gRPC response with body
    """
    {
        "host_count_limits": {
            "host_count_per_disk_type": [],
            "max_host_count": "7",
            "min_host_count": "1"
        },
        "default_resources": {
                "disk_size": "1024",
                "disk_type_id": "test_disk_type_id",
                "generation": "1",
                "generation_name": "Haswell",
                "resource_preset_id": "test_preset_id"
        },
        "resource_presets": [
            {
                "cpu_fraction": "100",
                "cpu_limit": "1",
                "decomissioning": false,
                "generation": "2",
                "generation_name": "Broadwell",
                "memory_limit": "4294967296",
                "preset_id": "s2.compute.1",
                "type": "standard",
                "zones": [
                    {
                        "disk_types": [
                            {
                                "disk_size_range": {
                                    "max": "2199023255552",
                                    "min": "10737418240"
                                },
                                "disk_type_id": "network-ssd",
                                "max_hosts": "7",
                                "min_hosts": "1"
                            }
                        ],
                        "zone_id": "vla"
                    }
                ]
            },
            {
                "cpu_fraction": "100",
                "cpu_limit": "2",
                "decomissioning": false,
                "generation": "2",
                "generation_name": "Broadwell",
                "memory_limit": "8589934592",
                "preset_id": "s2.compute.2",
                "type": "standard",
                "zones": [
                    {
                        "disk_types": [
                            {
                                "disk_size_range": {
                                    "max": "2199023255552",
                                    "min": "10737418240"
                                },
                                "disk_type_id": "network-ssd",
                                "max_hosts": "7",
                                "min_hosts": "1"
                            }
                        ],
                        "zone_id": "vla"
                    }
                ]
            }
        ]
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.mysql.v1.console.ClusterService" with data
    """
    {
      "folder_id": "folder1",
      "host_group_ids": ["hg6", "hg5"]
    }
    """
    Then we get gRPC response with body
    # hg5 is Gen2, hg6 is Gen3. This combo is prohibited: MDB-17089#622f34b1007b7b7c1f63cecb
    """
    {
        "host_count_limits": {
            "host_count_per_disk_type": [],
            "max_host_count": "7",
            "min_host_count": "1"
        },
        "default_resources": {
                "disk_size": "1024",
                "disk_type_id": "test_disk_type_id",
                "generation": "1",
                "generation_name": "Haswell",
                "resource_preset_id": "test_preset_id"
        },
        "resource_presets": [ ]
    }
    """
    And we restore valid resources after shrinking

  Scenario: Get console config and check resourcePresets against schema
    When we GET "/mdb/mysql/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body matches "clusters_config_schema_common.json" schema

  Scenario: Get console config zones in resourcePresets
    When we GET "/mdb/mysql/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body at path "$.resourcePresets[*].zones[*].zoneId" contains only
    """
    ["myt", "vla", "sas", "iva", "man"]
    """

  Scenario: Get MySQL sorted resource presets
    When we GET "/mdb/mysql/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body at path "$.resourcePresets[*]" contains fields with order: type;-generation;cpuLimit;cpuFraction;memoryLimit;presetId
