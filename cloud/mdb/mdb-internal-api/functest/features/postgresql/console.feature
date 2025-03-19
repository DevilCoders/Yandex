Feature: PostgreSQL console API

  Background:
    Given default headers

  Scenario: Console stats
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
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
    When we GET "/mdb/postgresql/1.0/console/clusters:stats?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clustersCount": 1
    }
    """

  Scenario: Console stats works when no clusters created
    When we GET "/mdb/postgresql/1.0/console/clusters:stats?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clustersCount": 0
    }
    """

  Scenario: Get console config
    When we GET "/mdb/postgresql/1.0/console/clusters:config?folderId=folder1"
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
            "blacklist": ["template0", "template1", "postgres"],
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
            "maxHostCount": 32,
            "minHostCount": 1
        },
        "password": {
            "blacklist": [],
            "max": 128,
            "min": 8,
            "regexp": ".*"
        },
        "userName": {
            "blacklist": ["admin", "repl", "monitor", "postgres", "public", "none", "mdb_admin", "mdb_replication", "mdb_monitor"],
            "max": 63,
            "min": 1,
            "regexp": "(?!pg_)[a-zA-Z0-9_][a-zA-Z0-9_-]*"
        },
        "versions": ["10", "11", "12", "13", "14"],
        "availableVersions": [
                { "id": "10",    "name": "10",    "deprecated": true, "updatableTo": ["11"], "versionedConfigKey": "postgresqlConfig_10" },
                { "id": "11",    "name": "11",    "deprecated": false, "updatableTo": ["12"], "versionedConfigKey": "postgresqlConfig_11" },
                { "id": "12",    "name": "12",    "deprecated": false, "updatableTo": ["13"]    , "versionedConfigKey": "postgresqlConfig_12" },
                { "id": "13",    "name": "13",    "deprecated": false, "updatableTo": ["14"]    , "versionedConfigKey": "postgresqlConfig_13" },
                { "id": "14",    "name": "14",    "deprecated": false, "updatableTo": []    , "versionedConfigKey": "postgresqlConfig_14" }
          ],
        "defaultVersion": "14"
    }
    """

    Given feature flags
    """
    ["MDB_POSTGRESQL_11"]
    """
    When we GET "/mdb/postgresql/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "versions": ["10", "11", "12", "13", "14"],
        "availableVersions": [
                { "id": "10",    "name": "10",    "deprecated": true, "updatableTo": ["11"], "versionedConfigKey": "postgresqlConfig_10" },
                { "id": "11",    "name": "11",    "deprecated": false, "updatableTo": ["12"], "versionedConfigKey": "postgresqlConfig_11" },
                { "id": "12",    "name": "12",    "deprecated": false, "updatableTo": ["13"]    , "versionedConfigKey": "postgresqlConfig_12" },
                { "id": "13",    "name": "13",    "deprecated": false, "updatableTo": ["14"]    , "versionedConfigKey": "postgresqlConfig_13" },
                { "id": "14",    "name": "14",    "deprecated": false, "updatableTo": []    , "versionedConfigKey": "postgresqlConfig_14" }
          ]
    }
    """

  Scenario: Get console config with all feature flags
    Given feature flags
    """
    ["MDB_POSTGRESQL_10_1C", "MDB_POSTGRESQL_11", "MDB_POSTGRESQL_11_1C", "MDB_POSTGRESQL_12", "MDB_POSTGRESQL_12_1C", "MDB_POSTGRESQL_13", "MDB_POSTGRESQL_13_1C", "MDB_POSTGRESQL_14", "MDB_POSTGRESQL_14_1C"]
    """
    When we GET "/mdb/postgresql/1.0/console/clusters:config?folderId=folder1"
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
            "blacklist": ["template0", "template1", "postgres"],
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
            "maxHostCount": 32,
            "minHostCount": 1
        },
        "password": {
            "blacklist": [],
            "max": 128,
            "min": 8,
            "regexp": ".*"
        },
        "userName": {
            "blacklist": ["admin", "repl", "monitor", "postgres", "public", "none", "mdb_admin", "mdb_replication","mdb_monitor"],
            "max": 63,
            "min": 1,
            "regexp": "(?!pg_)[a-zA-Z0-9_][a-zA-Z0-9_-]*"
        },
        "versions": ["10", "10-1c", "11", "11-1c", "12", "12-1c", "13", "13-1c", "14", "14-1c" ],
        "availableVersions": [
            { "id": "10",    "name": "10",    "deprecated": true, "updatableTo": ["11"], "versionedConfigKey": "postgresqlConfig_10" },
            { "id": "10-1c", "name": "10-1c", "deprecated": true, "updatableTo": ["11-1c"]    , "versionedConfigKey": "postgresqlConfig_10_1c" },
            { "id": "11",    "name": "11",    "deprecated": false, "updatableTo": ["12"], "versionedConfigKey": "postgresqlConfig_11" },
            { "id": "11-1c", "name": "11-1c", "deprecated": false, "updatableTo": []    , "versionedConfigKey": "postgresqlConfig_11_1c" },
            { "id": "12",    "name": "12",    "deprecated": false, "updatableTo": ["13"]    , "versionedConfigKey": "postgresqlConfig_12" },
            { "id": "12-1c", "name": "12-1c", "deprecated": false, "updatableTo": []    , "versionedConfigKey": "postgresqlConfig_12_1c" },
            { "id": "13",    "name": "13",    "deprecated": false, "updatableTo": ["14"]    , "versionedConfigKey": "postgresqlConfig_13" },
            { "id": "13-1c", "name": "13-1c", "deprecated": false, "updatableTo": []    , "versionedConfigKey": "postgresqlConfig_13_1c" },
            { "id": "14",    "name": "14",    "deprecated": false, "updatableTo": []    , "versionedConfigKey": "postgresqlConfig_14" },
            { "id": "14-1c", "name": "14-1c", "deprecated": false, "updatableTo": []    , "versionedConfigKey": "postgresqlConfig_14_1c" }
        ],
        "defaultVersion": "14"
    }
    """


  Scenario: Get console config and check resourcePresets against schema
    When we GET "/mdb/postgresql/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body matches "clusters_config_schema_common.json" schema

  Scenario: Get console config zones in resourcePresets
    When we GET "/mdb/postgresql/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body at path "$.resourcePresets[*].zones[*].zoneId" contains only
    """
    ["myt", "vla", "sas", "iva", "man"]
    """

  Scenario: Get PostgreSQL sorted resource presets
    When we GET "/mdb/postgresql/1.0/console/clusters:config?folderId=folder1"
    Then we get response with status 200
    And body at path "$.resourcePresets[*]" contains fields with order: type;-generation;cpuLimit;cpuFraction;memoryLimit;presetId

  Scenario: User list for grants works
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{ "zoneId": "myt" }]
    }
    """
    Then we get response with status 200
    When we GET "/mdb/postgresql/1.0/console/clusters/cid1/users/unknown:for_grants"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "User 'unknown' does not exist"
    }
    """
    When we GET "/mdb/postgresql/1.0/console/clusters/cid1/users/test:for_grants"
    Then we get response with status 200 and body equals
    """
    ["mdb_admin", "mdb_monitor", "mdb_replication"]
    """
    When we GET "/mdb/postgresql/1.0/console/clusters/cid1/users:for_grants"
    Then we get response with status 200 and body equals
    """
    ["test", "mdb_admin", "mdb_monitor", "mdb_replication"]
    """

  Scenario: List of available extensions works
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{ "zoneId": "myt" }]
    }
    """
    Then we get response with status 200
    When we GET "/mdb/postgresql/1.0/console/clusters/cid1:available_extensions"
    Then we get response with status 200 and body equals
    """
    ["pgvector", "pg_trgm", "uuid-ossp", "bloom", "dblink", "postgres_fdw", "oracle_fdw", "orafce", "tablefunc", "pg_repack", "autoinc", "pgrowlocks", "hstore", "cube", "citext", "earthdistance", "btree_gist", "xml2", "isn", "fuzzystrmatch", "ltree", "btree_gin", "intarray", "moddatetime", "lo", "pgcrypto", "dict_xsyn", "seg", "unaccent", "dict_int", "postgis", "postgis_topology", "address_standardizer", "address_standardizer_data_us", "postgis_tiger_geocoder", "pgrouting", "jsquery", "smlar", "pg_stat_statements", "pg_stat_kcache", "pg_partman", "pg_tm_aux", "amcheck", "pg_hint_plan", "pgstattuple", "pg_buffercache", "timescaledb", "rum", "plv8", "hypopg", "pg_qualstats", "pg_cron"]
    """

  Scenario: Get console config GRPC
    Given feature flags
    """
    ["MDB_POSTGRESQL_10_1C", "MDB_POSTGRESQL_11", "MDB_POSTGRESQL_11_1C", "MDB_POSTGRESQL_12", "MDB_POSTGRESQL_12_1C", "MDB_POSTGRESQL_13", "MDB_POSTGRESQL_13_1C", "MDB_POSTGRESQL_14", "MDB_POSTGRESQL_14_1C"]
    """
    And we shrink valid resources in metadb for flavor "s1.compute.1,s2.compute.1,s2.compute.2,s3.compute.1,s3.compute.2"
    And we shrink valid resources in metadb for disk "network-ssd"
    And we shrink valid resources in metadb for geo "vla"
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.postgresql.v1.console.ClusterService" with data
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
            "blacklist": ["template0", "template1", "postgres"],
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
            "regexp": ".*"
        },
        "user_name": {
            "blacklist": ["admin", "repl", "monitor", "postgres", "public", "none", "mdb_admin", "mdb_replication"],
            "max": "63",
            "min": "1",
            "regexp": "^(?!pg_)[a-zA-Z0-9_][a-zA-Z0-9_-]*$"
        },
        "versions": ["10", "10-1c", "11", "11-1c", "12", "12-1c", "13", "13-1c", "14", "14-1c" ],
        "available_versions": [
            { "id": "10",    "name": "10",    "deprecated": false, "updatable_to": ["11"], "versioned_config_key": "postgresqlConfig_10" },
            { "id": "10-1c", "name": "10-1c", "deprecated": false, "updatable_to": []    , "versioned_config_key": "postgresqlConfig_10_1c" },
            { "id": "11",    "name": "11",    "deprecated": false, "updatable_to": ["12"], "versioned_config_key": "postgresqlConfig_11" },
            { "id": "11-1c", "name": "11-1c", "deprecated": false, "updatable_to": []    , "versioned_config_key": "postgresqlConfig_11_1c" },
            { "id": "12",    "name": "12",    "deprecated": false, "updatable_to": ["13"], "versioned_config_key": "postgresqlConfig_12" },
            { "id": "12-1c", "name": "12-1c", "deprecated": false, "updatable_to": []    , "versioned_config_key": "postgresqlConfig_12_1c" },
            { "id": "13",    "name": "13",    "deprecated": false, "updatable_to": []    , "versioned_config_key": "postgresqlConfig_13" },
            { "id": "13-1c", "name": "13-1c", "deprecated": false, "updatable_to": []    , "versioned_config_key": "postgresqlConfig_13_1c" },
            { "id": "14",    "name": "14",    "deprecated": false, "updatable_to": []    , "versioned_config_key": "postgresqlConfig_14" },
            { "id": "14-1c", "name": "14-1c", "deprecated": false, "updatable_to": []    , "versioned_config_key": "postgresqlConfig_14_1c" }
        ],
        "default_version": "13",
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
                                "max_hosts": "32",
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
                                "max_hosts": "32",
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
                                "max_hosts": "32",
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
                                "max_hosts": "32",
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
                                "max_hosts": "32",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.postgresql.v1.console.ClusterService" with data
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
                                "max_hosts": "32",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.postgresql.v1.console.ClusterService" with data
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
                                "max_hosts": "32",
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
                                "max_hosts": "32",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.postgresql.v1.console.ClusterService" with data
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
