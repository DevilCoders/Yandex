Feature: ClickHouse console API

    Background:
        Given default headers

    Scenario: Console stats
        When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
        # language=json
        """
        {
            "name": "test",
            "environment": "PRESTABLE",
            "configSpec": {
                "clickhouse": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
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
                "type": "CLICKHOUSE",
                "zoneId": "myt"
            }],
            "description": "test cluster"
        }
        """
        Then we get response with status 200
        When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        # language=json
        """
        {
            "folder_id": "folder1",
            "name": "test",
            "environment": "PRESTABLE",
            "config_spec": {
                "clickhouse": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                }
            },
            "database_specs": [{
                "name": "testdb"
            }],
            "user_specs": [{
                "name": "test",
                "password": "test_password"
            }],
            "host_specs": [{
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            }],
            "description": "test cluster"
        }
        """
        Then "worker_task_id1" acquired and finished by worker
        When we "GET" via REST at "/mdb/clickhouse/1.0/console/clusters:stats?folderId=folder1"
        Then we get response with status 200 and body contains
        # language=json
        """
        {
            "clustersCount": 1
        }
        """
        When we "GetClustersStats" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.console.ClusterService" with data
        # language=json
        """
        {
            "folder_id": "folder1"
        }
        """
        Then we get gRPC response with body
        # language=json
        """
        {
            "clusters_count": "1"
        }
        """

    Scenario: Console stats works when no clusters created
        When we "GET" via REST at "/mdb/clickhouse/1.0/console/clusters:stats?folderId=folder1"
        Then we get response with status 200 and body contains
        # language=json
        """
        {
            "clustersCount": 0
        }
        """
        When we "GetClustersStats" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.console.ClusterService" with data
        # language=json
        """
        {
            "folder_id": "folder1"
        }
        """
        Then we get gRPC response with body
        # language=json
        """
        {
            "clusters_count": "0"
        }
        """

    Scenario: Get console config
        When we "GET" via REST at "/mdb/clickhouse/1.0/console/clusters:config?folderId=folder1"
        Then we get response with status 200 and body contains
        # language=json
        """
        {
            "clusterName": {
                "blacklist": [],
                "max": 63,
                "min": 1,
                "regexp": "[a-zA-Z0-9_-]+"
            },
            "dbName": {
                "blacklist": [],
                "max": 63,
                "min": 1,
                "regexp": "[a-zA-Z0-9_-]+"
            },
            "password": {
                "blacklist": [],
                "max": 128,
                "min": 8,
                "regexp": ".*"
            },
            "userName": {
                "blacklist": [],
                "max": 63,
                "min": 1,
                "regexp": "[a-zA-Z_][a-zA-Z0-9_-]*"
            },
            "mlModelName": {
                "blacklist": [],
                "max": 63,
                "min": 1,
                "regexp": "[a-zA-Z_-][a-zA-Z0-9_-]*"
            },
            "mlModelUri": {
                "blacklist": [],
                "max": 512,
                "min": 1,
                "regexp": "https://(?:[a-zA-Z0-9-]+\\.)?storage\\.yandexcloud\\.net/\\S+"
            },
            "formatSchemaName": {
                "blacklist": [],
                "max": 63,
                "min": 1,
                "regexp": "[a-zA-Z_-][a-zA-Z0-9_-]*"
            },
            "formatSchemaUri": {
                "blacklist": [],
                "max": 512,
                "min": 1,
                "regexp": "https://(?:[a-zA-Z0-9-]+\\.)?storage\\.yandexcloud\\.net/\\S+"
            },
            "hostCountLimits": {
                "hostCountPerDiskType": [
                    {
                        "diskTypeId": "local-nvme",
                        "minHostCount": 2
                    }
                ],
                "maxHostCount": 7,
                "minHostCount": 1
            },
            "versions": [
                "21.1",
                "21.2",
                "21.3",
                "21.4",
                "21.7",
                "21.8",
                "21.11",
                "22.1",
                "22.3",
                "22.5"
            ],
            "availableVersions": [
                {"id": "21.1",  "name": "21.1",     "updatableTo": ["22.5", "22.3", "22.1", "21.11", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": true},
                {"id": "21.2",  "name": "21.2",     "updatableTo": ["22.5", "22.3", "22.1", "21.11", "21.8", "21.7", "21.4", "21.3"        ], "deprecated": false},
                {"id": "21.3",  "name": "21.3 LTS", "updatableTo": ["22.5", "22.3", "22.1", "21.11", "21.8", "21.7", "21.4",         "21.2"], "deprecated": false},
                {"id": "21.4",  "name": "21.4",     "updatableTo": ["22.5", "22.3", "22.1", "21.11", "21.8", "21.7",         "21.3", "21.2"], "deprecated": false},
                {"id": "21.7",  "name": "21.7",     "updatableTo": ["22.5", "22.3", "22.1", "21.11", "21.8",         "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.8",  "name": "21.8 LTS", "updatableTo": ["22.5", "22.3", "22.1", "21.11",         "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.11", "name": "21.11",    "updatableTo": ["22.5", "22.3", "22.1",          "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "22.1",  "name": "22.1",     "updatableTo": ["22.5", "22.3",         "21.11", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "22.3",  "name": "22.3 LTS", "updatableTo": ["22.5",         "22.1", "21.11", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "22.5",  "name": "22.5",     "updatableTo": [        "22.3", "22.1", "21.11", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false}
            ],
            "defaultVersion": "21.3"
        }
        """
        When we "GetClustersConfig" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.console.ClusterService"
        Then we get gRPC response with body
        # language=json
        """
        {
            "cluster_name": {
                "regexp": "^[a-zA-Z0-9_-]+$",
                "min": "1",
                "max": "63",
                "blacklist": []
            },
            "db_name": {
                "regexp": "^[a-zA-Z0-9_-]+$",
                "min": "1",
                "max": "63",
                "blacklist": []
            },
            "user_name": {
                "regexp": "^[a-zA-Z0-9_][a-zA-Z0-9_-]*$",
                "min": "1",
                "max": "32",
                "blacklist": []
            },
            "password": {
                "regexp": ".*",
                "min": "8",
                "max": "128",
                "blacklist": []
            },
            "ml_model_name": {
                "regexp": "^[a-zA-Z_-][a-zA-Z0-9_-]*$",
                "min": "1",
                "max": "63",
                "blacklist": []
            },
            "ml_model_uri": {
                "regexp": "^https://(?:[a-zA-Z0-9-]+\\.)?storage\\.yandexcloud\\.net/\\S+$",
                "min": "1",
                "max": "512",
                "blacklist": []
            },
            "format_schema_name": {
                "regexp": "^[a-zA-Z_-][a-zA-Z0-9_-]*$",
                "min": "1",
                "max": "63",
                "blacklist": []
            },
            "format_schema_uri": {
                "regexp": "^https://(?:[a-zA-Z0-9-]+\\.)?storage\\.yandexcloud\\.net/\\S+$",
                "min": "1",
                "max": "512",
                "blacklist": []
            },
            "host_count_limits": {
                "min_host_count": "1",
                "max_host_count": "7",
                "host_count_per_disk_type": [{
                    "disk_type_id": "local-nvme",
                    "min_host_count": "2"
                }]
            },
            "host_types": [{
                "type": "CLICKHOUSE",
                "resource_presets": "**IGNORE**",
                "default_resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "generation": "1",
                    "generation_name": "Haswell",
                    "resource_preset_id": "s1.compute.1"
                }
            },{
                "type": "ZOOKEEPER",
                "resource_presets": "**IGNORE**",
                "default_resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "network-ssd",
                    "generation": "1",
                    "generation_name": "Haswell",
                    "resource_preset_id": "s1.compute.1"
                }
            }],
            "versions": [
                "22.5",
                "22.3",
                "22.1",
                "21.11",
                "21.10",
                "21.9",
                "21.8",
                "21.7",
                "21.4",
                "21.3",
                "21.2",
                "21.1"
            ],
            "available_versions": [
                {"id": "22.5",  "name": "22.5",     "updatable_to": [        "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "22.3",  "name": "22.3 LTS", "updatable_to": ["22.5",         "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "22.1",  "name": "22.1",     "updatable_to": ["22.5", "22.3",         "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.11", "name": "21.11",    "updatable_to": ["22.5", "22.3", "22.1",          "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.10", "name": "21.10",    "updatable_to": ["22.5", "22.3", "22.1", "21.11",          "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.9",  "name": "21.9",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10",         "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.8",  "name": "21.8 LTS", "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9",         "21.7", "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.7",  "name": "21.7",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8",         "21.4", "21.3", "21.2"], "deprecated": false},
                {"id": "21.4",  "name": "21.4",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7",         "21.3", "21.2"], "deprecated": false},
                {"id": "21.3",  "name": "21.3 LTS", "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4",         "21.2"], "deprecated": false},
                {"id": "21.2",  "name": "21.2",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3"        ], "deprecated": false},
                {"id": "21.1",  "name": "21.1",     "updatable_to": ["22.5", "22.3", "22.1", "21.11", "21.10", "21.9", "21.8", "21.7", "21.4", "21.3", "21.2"], "deprecated": true}
            ],
            "default_version": "21.3"
        }
        """
        And gRPC response body at path "$.host_types[1].resource_presets[0]" equals to
        # language=json
        """
        {
            "preset_id": "a1.aws.1",
            "type": "aws-standard",
            "cpu_fraction": "100",
            "cpu_limit": "1",
            "decomissioning": false,
            "generation": "1",
            "generation_name": "Haswell",
            "memory_limit": "4294967296",
            "zones": [{
                "zone_id": "iva",
                "disk_types": [{
                    "disk_type_id": "local-ssd",
                    "disk_sizes": { "sizes": [
                        "34359738368",
                        "68719476736",
                        "137438953472",
                        "274877906944",
                        "549755813888",
                        "1099511627776",
                        "2199023255552"
                    ]},
                    "min_hosts": "3",
                    "max_hosts": "5"
                }]
            }, {
                "zone_id": "man",
                "disk_types": [{
                    "disk_type_id": "local-ssd",
                    "disk_sizes": { "sizes": [
                        "34359738368",
                        "68719476736",
                        "137438953472",
                        "274877906944",
                        "549755813888",
                        "1099511627776",
                        "2199023255552"
                    ]},
                    "min_hosts": "3",
                    "max_hosts": "5"
                }]
            }, {
                "zone_id": "myt",
                "disk_types": [{
                    "disk_type_id": "local-ssd",
                    "disk_sizes": { "sizes": [
                        "34359738368",
                        "68719476736",
                        "137438953472",
                        "274877906944",
                        "549755813888",
                        "1099511627776",
                        "2199023255552"
                    ]},
                    "min_hosts": "3",
                    "max_hosts": "5"
                }]
            }, {
                "zone_id": "sas",
                "disk_types": [{
                    "disk_type_id": "local-ssd",
                    "disk_sizes": { "sizes": [
                        "34359738368",
                        "68719476736",
                        "137438953472",
                        "274877906944",
                        "549755813888",
                        "1099511627776",
                        "2199023255552"
                    ]},
                    "min_hosts": "3",
                    "max_hosts": "5"
                }]
            }, {
                "zone_id": "vla",
                "disk_types": [{
                    "disk_type_id": "local-ssd",
                    "disk_sizes": { "sizes": [
                        "34359738368",
                        "68719476736",
                        "137438953472",
                        "274877906944",
                        "549755813888",
                        "1099511627776",
                        "2199023255552"
                    ]},
                    "min_hosts": "3",
                    "max_hosts": "5"
                }]
            }]
        }
        """

    Scenario: Get console config and check resourcePresets against schema
        When we "GET" via REST at "/mdb/clickhouse/1.0/console/clusters:config?folderId=folder1"
        Then we get response with status 200
        When we "GetClustersConfig" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.console.ClusterService"
        Then we get gRPC response OK
        Given body matches "clickhouse/clusters_config_schema.json" schema

    Scenario: Get console config zones in resourcePresets
        When we "GET" via REST at "/mdb/clickhouse/1.0/console/clusters:config?folderId=folder1"
        Then we get response with status 200
        When we "GetClustersConfig" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.console.ClusterService"
        Then we get gRPC response OK
        Given body at path "$.hostTypes[*].resourcePresets[*].zones[*].zoneId" contains only
        # language=json
        """
        ["myt", "vla", "sas", "iva", "man"]
        """

    Scenario: Get ClickHouse sorted resource presets
        When we "GET" via REST at "/mdb/clickhouse/1.0/console/clusters:config?folderId=folder1"
        Then we get response with status 200
        When we "GetClustersConfig" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.console.ClusterService"
        Then we get gRPC response OK
        Given body at path "$.hostTypes[0].resourcePresets[*]" contains fields with order: type;-generation;cpuLimit;cpuFraction;memoryLimit;presetId
        And body at path "$.hostTypes[1].resourcePresets[*]" contains fields with order: type;-generation;cpuLimit;cpuFraction;memoryLimit;presetId

    @grpc_api
    Scenario: Get Create estimate billing
        When we "EstimateCreate" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.console.ClusterService" with data
        # language=json
        """
        {
            "folder_id": "folder1",
            "name": "test",
            "environment": "PRESTABLE",
            "config_spec": {
                "version": "21.3",
                "clickhouse": {
                    "resources": {
                        "resource_preset_id": "s2.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                },
                "zookeeper": {
                    "resources": {
                        "resource_preset_id": "s2.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                }
            },
            "database_specs": [{
                "name": "testdb"
            }],
            "user_specs": [{
                "name": "test",
                "password": "test_password"
            }],
            "host_specs": [{
                "type": "CLICKHOUSE",
                "zone_id": "myt"
            }, {
                "type": "CLICKHOUSE",
                "zone_id": "sas"
            }, {
                "type": "ZOOKEEPER",
                "zone_id": "man"
            }, {
                "type": "ZOOKEEPER",
                "zone_id": "vla"
            }, {
                "type": "ZOOKEEPER",
                "zone_id": "iva"
            }],
            "description": "test cluster",
            "network_id": "IN-PORTO-NO-NETWORK-API"
        }
        """
        Then we get gRPC response with body ignoring empty
        # language=json
        """
        {
            "metrics": [{
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "clickhouse_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "resource_preset_id":"s2.porto.1",
                    "roles": ["clickhouse_cluster"]
                }
            }, {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "clickhouse_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "resource_preset_id":"s2.porto.1",
                    "roles": ["clickhouse_cluster"]
                }
            }, {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "clickhouse_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "resource_preset_id":"s2.porto.1",
                    "roles": ["zk"]
                }
            }, {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "clickhouse_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "resource_preset_id":"s2.porto.1",
                    "roles": ["zk"]
                }
            }, {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "clickhouse_cluster",
                    "core_fraction": "100",
                    "cores": "1",
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "memory": "4294967296",
                    "online": "1",
                    "platform_id": "mdb-v1",
                    "resource_preset_id":"s2.porto.1",
                    "roles": ["zk"]
                }
            }]
        }
        """

    Scenario: Get console config for create cluster operation
        When we GET "/mdb/clickhouse/1.0/console/clusters:clusterCreateConfig?folderId=folder1"
        Then we get response with status 200
        And body at path "$.ClickhouseConfigSchemaV1.properties" contains
        # language=json
        """
        {
            "compression": {
                "items": {
                    "$ref": "#/Compression"
                },
                "type": "array"
            },
            "graphiteRollup": {
                "items": {
                    "$ref": "#/GraphiteRollup"
                },
                "type": "array"
            },
            "keepAliveTimeout": {
                "format": "int32",
                "minimum": 0,
                "type": "integer"
            },
            "logLevel": {
                "description": {
                    "DEBUG": "debug",
                    "ERROR": "error",
                    "INFORMATION": "information",
                    "TRACE": "trace",
                    "WARNING": "warning"
                },
                "enum": [
                    "TRACE",
                    "DEBUG",
                    "INFORMATION",
                    "WARNING",
                    "ERROR"
                ],
                "type": "string"
            },
            "markCacheSize": {
                "format": "int32",
                "minimum": 1,
                "type": "integer"
            },
            "maxConcurrentQueries": {
                "format": "int32",
                "minimum": 60,
                "type": "integer"
            },
            "maxConnections": {
                "format": "int32",
                "minimum": 10,
                "type": "integer"
            },
            "maxPartitionSizeToDrop": {
                "format": "int32",
                "minimum": 0,
                "type": "integer"
            },
            "maxTableSizeToDrop": {
                "format": "int32",
                "minimum": 0,
                "type": "integer"
            },
            "mergeTree": {
                "$ref": "#/MergeTree"
            },
            "uncompressedCacheSize": {
                "format": "int32",
                "minimum": 0,
                "type": "integer"
            }
        }
        """

    Scenario: Get console config for create dictionary operation
        When we GET "/mdb/clickhouse/1.0/console/clusters:clusterCreateDictionaryConfig?folderId=folder1"
        Then we get response with status 200 and body contains
        # language=json
        """
        {
            "ClusterCreateDictionary": {
                "properties": {
                    "externalDictionary": {
                        "$ref": "#/ClickhouseDictionarySchemaV1"
                    }
                },
                "required": [
                    "externalDictionary"
                ],
                "type": "object"
            },
            "ClickhouseDictionarySchemaV1": {
                "properties": {
                    "name": {
                        "pattern": "^[a-zA-Z0-9_-]+$",
                        "type": "string"
                    },
                    "structure": {
                        "$ref": "#/Structure"
                    },
                    "layout": {
                        "$ref": "#/Layout"
                    },
                    "fixedLifetime": {
                        "format": "int32",
                        "type": "integer"
                    },
                    "lifetimeRange": {
                        "$ref": "#/UIntRange"
                    },
                    "clickhouseSource": {
                        "$ref": "#/ClickhouseSource"
                    },
                    "httpSource": {
                        "$ref": "#/HttpSource"
                    },
                    "mongodbSource": {
                        "$ref": "#/MongodbSource"
                    },
                    "mysqlSource": {
                       "$ref": "#/MysqlSource"
                    },
                    "postgresqlSource": {
                        "$ref": "#/PostgresqlSource"
                    }
                },
                "required": [
                    "layout",
                    "name",
                    "structure"
                ],
                "type": "object"
            }
        }
        """

  @grpc_api
  Scenario: Get JSON schema for ClickHouse config
      When we "GetClickhouseConfigSchema" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.console.ClusterService"
      Then we get gRPC response with JSON Schema
      # language=json
      """
      {
          "ClickhouseConfig": {
              "type": "object",
              "properties": {
                  "mergeTree":                   { "$ref": "#/MergeTree" },
                  "compression":                 { "type": "array", "items": { "$ref": "#/Compression" } },
                  "graphiteRollup":              { "$ref": "#/GraphiteRollup" },
                  "kafka":                       { "$ref": "#/Kafka" },
                  "kafkaTopics":                 { "type": "array", "items": { "$ref": "#/KafkaTopic" } },
                  "rabbitmq":                    { "$ref": "#/Rabbitmq" },
                  "logLevel":                    { "$ref": "#/LogLevel" },
                  "maxConnections":              { "$ref": "#/google.protobuf.Int64Value" },
                  "maxConcurrentQueries":        { "$ref": "#/google.protobuf.Int64Value" },
                  "keepAliveTimeout":            { "$ref": "#/google.protobuf.Int64Value" },
                  "uncompressedCacheSize":       { "$ref": "#/google.protobuf.Int64Value" },
                  "markCacheSize":               { "$ref": "#/google.protobuf.Int64Value" },
                  "maxTableSizeToDrop":          { "$ref": "#/google.protobuf.Int64Value" },
                  "maxPartitionSizeToDrop":      { "$ref": "#/google.protobuf.Int64Value" },
                  "timezone":                    { "type": "string" },
                  "geobaseUri":                  { "type": "string" },
                  "queryLogRetentionSize":       { "$ref": "#/google.protobuf.Int64Value" },
                  "queryLogRetentionTime":       { "$ref": "#/google.protobuf.Int64Value" },
                  "queryThreadLogEnabled":       { "$ref": "#/google.protobuf.BoolValue" },
                  "queryThreadLogRetentionSize": { "$ref": "#/google.protobuf.Int64Value" },
                  "queryThreadLogRetentionTime": { "$ref": "#/google.protobuf.Int64Value" },
                  "partLogRetentionSize":        { "$ref": "#/google.protobuf.Int64Value" },
                  "partLogRetentionTime":        { "$ref": "#/google.protobuf.Int64Value" },
                  "metricLogEnabled":            { "$ref": "#/google.protobuf.BoolValue" },
                  "metricLogRetentionSize":      { "$ref": "#/google.protobuf.Int64Value" },
                  "metricLogRetentionTime":      { "$ref": "#/google.protobuf.Int64Value" },
                  "traceLogEnabled":             { "$ref": "#/google.protobuf.BoolValue" },
                  "traceLogRetentionSize":       { "$ref": "#/google.protobuf.Int64Value" },
                  "traceLogRetentionTime":       { "$ref": "#/google.protobuf.Int64Value" },
                  "textLogEnabled":              { "$ref": "#/google.protobuf.BoolValue" },
                  "textLogRetentionSize":        { "$ref": "#/google.protobuf.Int64Value" },
                  "textLogRetentionTime":        { "$ref": "#/google.protobuf.Int64Value" },
                  "textLogLevel":                { "$ref": "#/LogLevel" },
                  "backgroundPoolSize":          { "$ref": "#/google.protobuf.Int64Value" },
                  "backgroundSchedulePoolSize":  { "$ref": "#/google.protobuf.Int64Value" }
              }
          },
          "MergeTree": {
              "type": "object",
              "properties": {
                  "replicatedDeduplicationWindow":                  { "$ref": "#/google.protobuf.Int64Value" },
                  "replicatedDeduplicationWindowSeconds":           { "$ref": "#/google.protobuf.Int64Value" },
                  "partsToDelayInsert":                             { "$ref": "#/google.protobuf.Int64Value" },
                  "partsToThrowInsert":                             { "$ref": "#/google.protobuf.Int64Value" },
                  "maxReplicatedMergesInQueue":                     { "$ref": "#/google.protobuf.Int64Value" },
                  "numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge": { "$ref": "#/google.protobuf.Int64Value" },
                  "maxBytesToMergeAtMinSpaceInPool":                { "$ref": "#/google.protobuf.Int64Value" },
                  "maxBytesToMergeAtMaxSpaceInPool":                { "$ref": "#/google.protobuf.Int64Value" }
              }
          },
          "Compression": {
              "type": "object",
              "properties": {
                  "method": {
                      "type": "string",
                      "enum": [
                          "METHOD_UNSPECIFIED",
                          "LZ4",
                          "ZSTD"
                      ],
                      "description": {
                          "METHOD_UNSPECIFIED": null,
                          "LZ4": "lz4",
                          "ZSTD": "zstd"
                      }
                  },
                  "minPartSize": {
                      "type": "integer"
                  },
                  "minPartSizeRatio": {
                      "type": "number"
                  }
              }
          },
          "GraphiteRollup": {
              "type": "object",
              "properties": {
                  "name": {
                      "type": "string"
                  },
                  "patterns": {
                      "type": "array",
                      "items": {
                          "type": "object",
                          "properties": {
                              "regexp": {
                                 "type": "string"
                              },
                              "function": {
                                 "type": "string"
                              },
                              "retention": {
                                  "type": "array",
                                  "items": {
                                      "type": "object",
                                      "properties": {
                                          "age": {
                                              "type": "integer"
                                          },
                                          "precision": {
                                              "type": "integer"
                                          }
                                      }
                                  }
                              }
                          }
                      }
                  }
              }
          },
          "Kafka": {
              "type": "object",
              "properties": {
                  "security_protocol": {
                      "type": "string",
                      "enum": [
                          "SECURITY_PROTOCOL_UNSPECIFIED",
                          "SECURITY_PROTOCOL_PLAINTEXT",
                          "SECURITY_PROTOCOL_SSL",
                          "SECURITY_PROTOCOL_SASL_PLAINTEXT",
                          "SECURITY_PROTOCOL_SASL_SSL"
                      ],
                      "description": {
                          "SECURITY_PROTOCOL_UNSPECIFIED": null,
                          "SECURITY_PROTOCOL_PLAINTEXT": "PLAINTEXT",
                          "SECURITY_PROTOCOL_SSL": "SSL",
                          "SECURITY_PROTOCOL_SASL_PLAINTEXT": "SASL_PLAINTEXT",
                          "SECURITY_PROTOCOL_SASL_SSL": "SASL_SSL"
                      }
                  },
                  "sasl_mechanism": {
                      "type": "string",
                      "enum": [
                          "SASL_MECHANISM_UNSPECIFIED",
                          "SASL_MECHANISM_GSSAPI",
                          "SASL_MECHANISM_PLAIN",
                          "SASL_MECHANISM_SCRAM_SHA_256",
                          "SASL_MECHANISM_SCRAM_SHA_512"
                      ],
                      "description": {
                          "SASL_MECHANISM_UNSPECIFIED": null,
                          "SASL_MECHANISM_GSSAPI": "GSSAPI",
                          "SASL_MECHANISM_PLAIN": "PLAIN",
                          "SASL_MECHANISM_SCRAM_SHA_256": "SCRAM_SHA_256",
                          "SASL_MECHANISM_SCRAM_SHA_512": "SCRAM_SHA_512"
                      }
                  },
                  "sasl_username": {
                      "type": "string"
                  },
                  "sasl_password": {
                      "type": "string"
                  }
              }
          },
          "KafkaTopic": {
              "type": "object",
              "properties": {
                  "name": {
                      "type": "string"
                  },
                  "settings": {
                      "$ref": "#/Kafka"
                  }
              }
          },
          "Rabbitmq": {
              "type": "object",
              "properties": {
                  "username": {
                      "type": "string"
                  },
                  "password": {
                      "type": "string"
                  }
              }
          },
          "LogLevel": {
              "type": "string",
              "enum": [
                  "LOG_LEVEL_UNSPECIFIED",
                  "TRACE",
                  "DEBUG",
                  "INFORMATION",
                  "WARNING",
                  "ERROR"
              ],
              "description": {
                  "LOG_LEVEL_UNSPECIFIED": null,
                  "TRACE": "trace",
                  "DEBUG": "debug",
                  "INFORMATION": "information",
                  "WARNING": "warning",
                  "ERROR": "error"
              }
          },
          "google.protobuf.Int64Value": {
              "type": "object",
              "properties": {
                  "value": {
                      "type": "integer"
                  }
              }
          },
          "google.protobuf.BoolValue": {
              "type": "object",
              "properties": {
                  "value": {
                      "type": "integer"
                  }
              }
          }
      }
      """

  @grpc_api
  Scenario: Get JSON schema for user settings
      When we "GetUserSettingsSchema" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.console.ClusterService"
      Then we get gRPC response with JSON Schema
      # language=json
      """
      {
          "UserSettings": {
              "type": "object",
              "properties": {
                  "readonly":                                     { "$ref": "#/google.protobuf.Int64Value" },
                  "allowDdl":                                     { "$ref": "#/google.protobuf.BoolValue" },

                  "connectTimeout":                               { "$ref": "#/google.protobuf.Int64Value" },
                  "receiveTimeout":                               { "$ref": "#/google.protobuf.Int64Value" },
                  "sendTimeout":                                  { "$ref": "#/google.protobuf.Int64Value" },

                  "insertQuorum":                                 { "$ref": "#/google.protobuf.Int64Value" },
                  "insertQuorumTimeout":                          { "$ref": "#/google.protobuf.Int64Value" },
                  "selectSequentialConsistency":                  { "$ref": "#/google.protobuf.BoolValue" },
                  "replicationAlterPartitionsSync":               { "$ref": "#/google.protobuf.Int64Value" },

                  "maxReplicaDelayForDistributedQueries":         { "$ref": "#/google.protobuf.Int64Value" },
                  "fallbackToStaleReplicasForDistributedQueries": { "$ref": "#/google.protobuf.BoolValue" },
                  "distributedProductMode":                       { "$ref": "#/DistributedProductMode" },
                  "distributedAggregationMemoryEfficient":        { "$ref": "#/google.protobuf.BoolValue" },
                  "distributedDdlTaskTimeout":                    { "$ref": "#/google.protobuf.Int64Value" },
                  "skipUnavailableShards":                        { "$ref": "#/google.protobuf.BoolValue" },

                  "compileExpressions":                           { "$ref": "#/google.protobuf.BoolValue" },
                  "minCountToCompileExpression":                  { "$ref": "#/google.protobuf.Int64Value" },

                  "maxBlockSize":                                 { "$ref": "#/google.protobuf.Int64Value" },
                  "minInsertBlockSizeRows":                       { "$ref": "#/google.protobuf.Int64Value" },
                  "minInsertBlockSizeBytes":                      { "$ref": "#/google.protobuf.Int64Value" },
                  "maxInsertBlockSize":                           { "$ref": "#/google.protobuf.Int64Value" },
                  "minBytesToUseDirectIo":                        { "$ref": "#/google.protobuf.Int64Value" },
                  "useUncompressedCache":                         { "$ref": "#/google.protobuf.BoolValue" },
                  "mergeTreeMaxRowsToUseCache":                   { "$ref": "#/google.protobuf.Int64Value" },
                  "mergeTreeMaxBytesToUseCache":                  { "$ref": "#/google.protobuf.Int64Value" },
                  "mergeTreeMinRowsForConcurrentRead":            { "$ref": "#/google.protobuf.Int64Value" },
                  "mergeTreeMinBytesForConcurrentRead":           { "$ref": "#/google.protobuf.Int64Value" },
                  "maxBytesBeforeExternalGroupBy":                { "$ref": "#/google.protobuf.Int64Value" },
                  "maxBytesBeforeExternalSort":                   { "$ref": "#/google.protobuf.Int64Value" },
                  "groupByTwoLevelThreshold":                     { "$ref": "#/google.protobuf.Int64Value" },
                  "groupByTwoLevelThresholdBytes":                { "$ref": "#/google.protobuf.Int64Value" },

                  "priority":                                     { "$ref": "#/google.protobuf.Int64Value" },
                  "maxThreads":                                   { "$ref": "#/google.protobuf.Int64Value" },
                  "maxMemoryUsage":                               { "$ref": "#/google.protobuf.Int64Value" },
                  "maxMemoryUsageForUser":                        { "$ref": "#/google.protobuf.Int64Value" },
                  "maxNetworkBandwidth":                          { "$ref": "#/google.protobuf.Int64Value" },
                  "maxNetworkBandwidthForUser":                   { "$ref": "#/google.protobuf.Int64Value" },

                  "forceIndexByDate":                             { "$ref": "#/google.protobuf.BoolValue" },
                  "forcePrimaryKey":                              { "$ref": "#/google.protobuf.BoolValue" },
                  "maxRowsToRead":                                { "$ref": "#/google.protobuf.Int64Value" },
                  "maxBytesToRead":                               { "$ref": "#/google.protobuf.Int64Value" },
                  "readOverflowMode":                             { "$ref": "#/OverflowMode" },
                  "maxRowsToGroupBy":                             { "$ref": "#/google.protobuf.Int64Value" },
                  "groupByOverflowMode":                          { "$ref": "#/GroupByOverflowMode" },
                  "maxRowsToSort":                                { "$ref": "#/google.protobuf.Int64Value" },
                  "maxBytesToSort":                               { "$ref": "#/google.protobuf.Int64Value" },
                  "sortOverflowMode":                             { "$ref": "#/OverflowMode" },
                  "maxResultRows":                                { "$ref": "#/google.protobuf.Int64Value" },
                  "maxResultBytes":                               { "$ref": "#/google.protobuf.Int64Value" },
                  "resultOverflowMode":                           { "$ref": "#/OverflowMode" },
                  "maxRowsInDistinct":                            { "$ref": "#/google.protobuf.Int64Value" },
                  "maxBytesInDistinct":                           { "$ref": "#/google.protobuf.Int64Value" },
                  "distinctOverflowMode":                         { "$ref": "#/OverflowMode" },
                  "maxRowsToTransfer":                            { "$ref": "#/google.protobuf.Int64Value" },
                  "maxBytesToTransfer":                           { "$ref": "#/google.protobuf.Int64Value" },
                  "transferOverflowMode":                         { "$ref": "#/OverflowMode" },
                  "maxExecutionTime":                             { "$ref": "#/google.protobuf.Int64Value" },
                  "timeoutOverflowMode":                          { "$ref": "#/OverflowMode" },
                  "maxRowsInSet":                                 { "$ref": "#/google.protobuf.Int64Value" },
                  "maxBytesInSet":                                { "$ref": "#/google.protobuf.Int64Value" },
                  "setOverflowMode":                              { "$ref": "#/OverflowMode" },
                  "maxRowsInJoin":                                { "$ref": "#/google.protobuf.Int64Value" },
                  "maxBytesInJoin":                               { "$ref": "#/google.protobuf.Int64Value" },
                  "joinOverflowMode":                             { "$ref": "#/OverflowMode" },
                  "maxColumnsToRead":                             { "$ref": "#/google.protobuf.Int64Value" },
                  "maxTemporaryColumns":                          { "$ref": "#/google.protobuf.Int64Value" },
                  "maxTemporaryNonConstColumns":                  { "$ref": "#/google.protobuf.Int64Value" },
                  "maxQuerySize":                                 { "$ref": "#/google.protobuf.Int64Value" },
                  "maxAstDepth":                                  { "$ref": "#/google.protobuf.Int64Value" },
                  "maxAstElements":                               { "$ref": "#/google.protobuf.Int64Value" },
                  "maxExpandedAstElements":                       { "$ref": "#/google.protobuf.Int64Value" },
                  "minExecutionSpeed":                            { "$ref": "#/google.protobuf.Int64Value" },
                  "minExecutionSpeedBytes":                       { "$ref": "#/google.protobuf.Int64Value" },

                  "inputFormatValuesInterpretExpressions":        { "$ref": "#/google.protobuf.BoolValue" },
                  "inputFormatDefaultsForOmittedFields":          { "$ref": "#/google.protobuf.BoolValue" },
                  "outputFormatJsonQuote_64bitIntegers":          { "$ref": "#/google.protobuf.BoolValue" },
                  "outputFormatJsonQuoteDenormals":               { "$ref": "#/google.protobuf.BoolValue" },
                  "lowCardinalityAllowInNativeFormat":            { "$ref": "#/google.protobuf.BoolValue" },
                  "emptyResultForAggregationByEmptySet":          { "$ref": "#/google.protobuf.BoolValue" },

                  "httpConnectionTimeout":                        { "$ref": "#/google.protobuf.Int64Value" },
                  "httpReceiveTimeout":                           { "$ref": "#/google.protobuf.Int64Value" },
                  "httpSendTimeout":                              { "$ref": "#/google.protobuf.Int64Value" },
                  "enableHttpCompression":                        { "$ref": "#/google.protobuf.BoolValue" },
                  "sendProgressInHttpHeaders":                    { "$ref": "#/google.protobuf.BoolValue" },
                  "httpHeadersProgressInterval":                  { "$ref": "#/google.protobuf.Int64Value" },
                  "addHttpCorsHeader":                            { "$ref": "#/google.protobuf.BoolValue" },

                  "quotaMode":                                    { "$ref": "#/QuotaMode" },

                  "countDistinctImplementation":                  { "$ref": "#/CountDistinctImplementation" },
                  "joinedSubqueryRequiresAlias":                  { "$ref": "#/google.protobuf.BoolValue" },
                  "joinUseNulls":                                 { "$ref": "#/google.protobuf.BoolValue" },
                  "transformNullIn":                              { "$ref": "#/google.protobuf.BoolValue" }
              }
          },
          "UserQuota": {
              "type": "object",
              "properties": {
                  "intervalDuration":                             { "$ref": "#/google.protobuf.Int64Value" },
                  "queries":                                      { "$ref": "#/google.protobuf.Int64Value" },
                  "errors":                                       { "$ref": "#/google.protobuf.Int64Value" },
                  "resultRows":                                   { "$ref": "#/google.protobuf.Int64Value" },
                  "readRows":                                     { "$ref": "#/google.protobuf.Int64Value" },
                  "executionTime":                                { "$ref": "#/google.protobuf.Int64Value" }
              }
          },
          "OverflowMode": {
              "type": "string",
              "enum": [
                  "OVERFLOW_MODE_UNSPECIFIED",
                  "OVERFLOW_MODE_THROW",
                  "OVERFLOW_MODE_BREAK"
              ],
              "description": {
                  "OVERFLOW_MODE_UNSPECIFIED": null,
                  "OVERFLOW_MODE_THROW": "throw",
                  "OVERFLOW_MODE_BREAK": "break"
              }
          },
          "GroupByOverflowMode": {
              "type": "string",
              "enum": [
                  "GROUP_BY_OVERFLOW_MODE_UNSPECIFIED",
                  "GROUP_BY_OVERFLOW_MODE_THROW",
                  "GROUP_BY_OVERFLOW_MODE_BREAK",
                  "GROUP_BY_OVERFLOW_MODE_ANY"
              ],
              "description": {
                  "GROUP_BY_OVERFLOW_MODE_UNSPECIFIED": null,
                  "GROUP_BY_OVERFLOW_MODE_THROW": "throw",
                  "GROUP_BY_OVERFLOW_MODE_BREAK": "break",
                  "GROUP_BY_OVERFLOW_MODE_ANY": "any"
              }
          },
          "DistributedProductMode": {
              "type": "string",
              "enum": [
                  "DISTRIBUTED_PRODUCT_MODE_UNSPECIFIED",
                  "DISTRIBUTED_PRODUCT_MODE_DENY",
                  "DISTRIBUTED_PRODUCT_MODE_LOCAL",
                  "DISTRIBUTED_PRODUCT_MODE_GLOBAL",
                  "DISTRIBUTED_PRODUCT_MODE_ALLOW"
              ],
              "description": {
                  "DISTRIBUTED_PRODUCT_MODE_UNSPECIFIED": null,
                  "DISTRIBUTED_PRODUCT_MODE_DENY": "deny",
                  "DISTRIBUTED_PRODUCT_MODE_LOCAL": "local",
                  "DISTRIBUTED_PRODUCT_MODE_GLOBAL": "global",
                  "DISTRIBUTED_PRODUCT_MODE_ALLOW": "allow"
              }
          },
          "QuotaMode": {
              "type": "string",
              "enum": [
                  "QUOTA_MODE_UNSPECIFIED",
                  "QUOTA_MODE_DEFAULT",
                  "QUOTA_MODE_KEYED",
                  "QUOTA_MODE_KEYED_BY_IP"
              ],
              "description": {
                  "QUOTA_MODE_UNSPECIFIED": null,
                  "QUOTA_MODE_DEFAULT": "default",
                  "QUOTA_MODE_KEYED": "keyed",
                  "QUOTA_MODE_KEYED_BY_IP": "keyed_by_ip"
              }
          },
          "CountDistinctImplementation": {
              "type": "string",
              "enum": [
                  "COUNT_DISTINCT_IMPLEMENTATION_UNSPECIFIED",
                  "COUNT_DISTINCT_IMPLEMENTATION_UNIQ",
                  "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_COMBINED",
                  "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_COMBINED_64",
                  "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12",
                  "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_EXACT"
              ],
              "description": {
                  "COUNT_DISTINCT_IMPLEMENTATION_UNSPECIFIED": null,
                  "COUNT_DISTINCT_IMPLEMENTATION_UNIQ": "uniq",
                  "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_COMBINED": "uniqCombined",
                  "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_COMBINED_64": "uniqCombined64",
                  "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_HLL_12": "uniqHLL12",
                  "COUNT_DISTINCT_IMPLEMENTATION_UNIQ_EXACT": "uniqExact"
              }
          },
          "google.protobuf.Int64Value": {
              "type": "object",
              "properties": {
                  "value": {
                      "type": "integer"
                  }
              }
          },
          "google.protobuf.BoolValue": {
              "type": "object",
              "properties": {
                  "value": {
                      "type": "integer"
                  }
              }
          }
      }
      """
