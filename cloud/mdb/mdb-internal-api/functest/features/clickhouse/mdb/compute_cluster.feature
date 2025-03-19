Feature: Create/Modify Compute ClickHouse Cluster

  Background:
    Given default headers
    Given we use following default resources for "clickhouse_cluster" with role "clickhouse_cluster"
    # language=json
    """
    {
        "resource_preset_id": "s1.compute.1",
        "disk_type_id": "network-ssd",
        "disk_size": 10737418240,
        "generation": 1
    }
    """
    Given we use following default resources for "clickhouse_cluster" with role "zk"
    # language=json
    """
    {
        "resource_preset_id": "s1.compute.1",
        "disk_type_id": "network-ssd",
        "disk_size": 10737418240,
        "generation": 1
    }
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    # language=json
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
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
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1",
        "serviceAccountId": "sa1",
        "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """
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
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
                    "disk_size": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resource_preset_id": "s1.compute.1",
                    "disk_type_id": "network-ssd",
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
            "zone_id": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }],
        "description": "test cluster",
        "network_id": "network1",
        "service_account_id": "sa1",
        "security_group_ids": ["sg_id1", "sg_id2"]
    }
    """
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" containing:
      |sg_id1|
      |sg_id2|
    And worker set "sg_id1,sg_id2" security groups on "cid1"
    And "worker_task_id1" acquired and finished by worker
    And cluster "cid1" has 5 host

  @events
  Scenario: Cluster creation works
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "access": {
                "webSql": false,
                "dataLens": false,
                "metrika": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "version": "21.3",
            "serviceAccountId": "sa1",
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
            "clickhouse": {
                "config": {
                    "defaultConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "effectiveConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "userConfig": {
                        "mergeTree": {}
                    }
                },
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "network-ssd",
                    "resourcePresetId": "s1.compute.1"
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "network-ssd",
                    "resourcePresetId": "s1.compute.1"
                }
            },
            "cloudStorage": {
                "enabled": false
            }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "UNKNOWN",
        "id": "cid1",
        "labels": {},
        "monitoring": [
            {
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm.yandex-team.ru/template/panel/dbaas_clickhouse_metrics/cid=cid1",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid1&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-clickhouse",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/clickhouse_cluster/cid1?section=monitoring",
                "name": "Console"
            }
        ],
        "name": "test",
        "networkId": "network1",
        "status": "RUNNING"
    }
    """
    And for "worker_task_id1" exists "yandex.cloud.events.mdb.clickhouse.CreateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 5.0,
        "memoryUsed": 21474836480,
        "ssdSpaceUsed": 53687091200
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_at": "**IGNORE**",
        "created_by": "user",
        "description": "Create ClickHouse cluster",
        "done": true,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        },
        "modified_at": "**IGNORE**",
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
            "config": {
                "access": {},
                "backup_window_start": {
                     "hours": 22,
                     "minutes": 15,
                     "nanos": 100,
                     "seconds": 30
                },
                "clickhouse": {
                    "config": {
                        "default_config": "**IGNORE**",
                        "user_config": "**IGNORE**",
                        "effective_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_partition_size_to_drop": "53687091200",
                            "max_table_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "uncompressed_cache_size": "8589934592"
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    }
                },
                "cloud_storage": {},
                "embedded_keeper": false,
                "mysql_protocol": false,
                "postgresql_protocol": false,
                "sql_database_management": false,
                "sql_user_management": false,
                "version": "21.3",
                "zookeeper": {
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    }
                }
            },
            "created_at": "**IGNORE**",
            "description": "test cluster",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "id": "cid1",
            "name": "test",
            "network_id": "network1",
            "security_group_ids": ["sg_id1", "sg_id2"],
            "status": "RUNNING",
            "maintenance_window": { "anytime": {} },
            "monitoring": [{
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm/cid=cid1",
                "name": "YASM"
            }, {
                "description": "Solomon charts",
                "link": "https://solomon/cid=cid1&fExtID=folder1",
                "name": "Solomon"
            }, {
                "description": "Console charts",
                "link": "https://console/cid=cid1&fExtID=folder1",
                "name": "Console"
            }],
            "service_account_id": "sa1"
        }
    }
    """

  @grpc_api @events
  Scenario: Cluster creation with default resources works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    # language=json
    """
    {
        "folder_id": "folder1",
        "name": "test-default-resources",
        "environment": "PRESTABLE",
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
            "zone_id": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "myt"
        }],
        "description": "test cluster with default resources",
        "network_id": "network1",
        "service_account_id": "sa1"
    }
    """
    And we GET "/mdb/clickhouse/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    # language=json
    """
    {
        "config": {
            "access": {
                "webSql": false,
                "dataLens": false,
                "metrika": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "version": "21.3",
            "serviceAccountId": "sa1",
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
            "clickhouse": {
                "config": {
                    "defaultConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "effectiveConfig": {
                        "builtinDictionariesReloadInterval": 3600,
                        "compression": [],
                        "dictionaries": [],
                        "graphiteRollup": [],
                        "kafka": {},
                        "kafkaTopics": [],
                        "rabbitmq": {},
                        "keepAliveTimeout": 3,
                        "logLevel": "INFORMATION",
                        "markCacheSize": 5368709120,
                        "maxConcurrentQueries": 500,
                        "maxConnections": 4096,
                        "maxTableSizeToDrop": 53687091200,
                        "maxPartitionSizeToDrop": 53687091200,
                        "timezone": "Europe/Moscow",
                        "mergeTree": {
                            "replicatedDeduplicationWindow": 100,
                            "replicatedDeduplicationWindowSeconds": 604800
                        },
                        "uncompressedCacheSize": 8589934592
                    },
                    "userConfig": {
                        "mergeTree": {}
                    }
                },
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "network-ssd",
                    "resourcePresetId": "s1.compute.1"
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                }
            },
            "cloudStorage": {
                "enabled": false
            }
        },
        "description": "test cluster with default resources",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "UNKNOWN",
        "id": "cid2",
        "labels": {},
        "monitoring": [
            {
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm.yandex-team.ru/template/panel/dbaas_clickhouse_metrics/cid=cid2",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid2&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-clickhouse",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/clickhouse_cluster/cid2?section=monitoring",
                "name": "Console"
            }
        ],
        "name": "test-default-resources",
        "networkId": "network1",
        "status": "CREATING"
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.CreateCluster" event with
    # language=json
    """
    {
        "details": {
            "cluster_id": "cid2"
        }
    }
    """
    Given "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    # language=json
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body ignoring empty
    # language=json
    """
    {
        "created_at": "**IGNORE**",
        "created_by": "user",
        "description": "Create ClickHouse cluster",
        "id": "worker_task_id2",
        "done": true,
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateClusterMetadata",
            "cluster_id": "cid2"
        },
        "modified_at": "**IGNORE**",
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
            "config": {
                "access": {},
                "backup_window_start": {
                     "hours": 22,
                     "minutes": 15,
                     "nanos": 100,
                     "seconds": 30
                },
                "clickhouse": {
                    "config": {
                        "default_config": "**IGNORE**",
                        "user_config": "**IGNORE**",
                        "effective_config": {
                            "merge_tree": {
                                "enable_mixed_granularity_parts": true,
                                "replicated_deduplication_window": "100",
                                "replicated_deduplication_window_seconds": "604800"
                            },
                            "kafka": {},
                            "rabbitmq": {},
                            "builtin_dictionaries_reload_interval": "3600",
                            "keep_alive_timeout": "3",
                            "log_level": "INFORMATION",
                            "mark_cache_size": "5368709120",
                            "max_concurrent_queries": "500",
                            "max_connections": "4096",
                            "max_partition_size_to_drop": "53687091200",
                            "max_table_size_to_drop": "53687091200",
                            "timezone": "Europe/Moscow",
                            "uncompressed_cache_size": "8589934592"
                        }
                    },
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "network-ssd",
                        "resource_preset_id": "s1.compute.1"
                    }
                },
                "cloud_storage": {},
                "embedded_keeper": false,
                "mysql_protocol": false,
                "postgresql_protocol": false,
                "sql_database_management": false,
                "sql_user_management": false,
                "version": "21.3",
                "zookeeper": {
                    "resources": {
                        "disk_size": "10737418240",
                        "disk_type_id": "local-ssd",
                        "resource_preset_id": "s2.porto.1"
                    }
                }
            },
            "created_at": "**IGNORE**",
            "description": "test cluster with default resources",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "id": "cid2",
            "name": "test-default-resources",
            "network_id": "network1",
            "status": "RUNNING",
            "maintenance_window": { "anytime": {} },
            "monitoring": [{
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm/cid=cid2",
                "name": "YASM"
            }, {
                "description": "Solomon charts",
                "link": "https://solomon/cid=cid2&fExtID=folder1",
                "name": "Solomon"
            }, {
                "description": "Console charts",
                "link": "https://console/cid=cid2&fExtID=folder1",
                "name": "Console"
            }],
            "service_account_id": "sa1"
        }
    }
    """

  Scenario: Billing cost estimation works
    When we POST "/mdb/clickhouse/1.0/console/clusters:estimate?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "local-nvme",
                    "diskSize": 107374182400
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
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas",
            "assignPublicIp": true
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "clickhouse_cluster",
                    "disk_size": 107374182400,
                    "disk_type_id": "local-nvme",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "clickhouse_cluster"
                    ],
                    "software_accelerated_network_cores": 0
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "clickhouse_cluster",
                    "disk_size": 107374182400,
                    "disk_type_id": "local-nvme",
                    "online": 1,
                    "public_ip": 1,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "clickhouse_cluster"
                    ],
                    "software_accelerated_network_cores": 0
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "clickhouse_cluster",
                    "disk_size": 10737418240,
                    "disk_type_id": "local-ssd",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s2.porto.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "zk"
                    ],
                    "software_accelerated_network_cores": 0
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "clickhouse_cluster",
                    "disk_size": 10737418240,
                    "disk_type_id": "local-ssd",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s2.porto.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "zk"
                    ],
                    "software_accelerated_network_cores": 0
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "clickhouse_cluster",
                    "disk_size": 10737418240,
                    "disk_type_id": "local-ssd",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s2.porto.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "zk"
                    ],
                    "software_accelerated_network_cores": 0
                }
            }
        ]
    }
    """

  Scenario: Console create host estimate costs
    When we POST "/mdb/clickhouse/1.0/console/hosts:estimate" with data
    """
    {
        "folderId": "folder1",
        "billingHostSpecs": [
            {
                "host": {
                    "type": "CLICKHOUSE",
                    "zoneId": "vla",
                    "subnetId": "subnet-id",
                    "assignPublicIp": true
                },
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskSize": 1,
                    "diskTypeId": "disk-type-id"
                }
            },
            {
                "host": {
                    "type": "ZOOKEEPER",
                    "zoneId": "iva",
                    "subnetId": "subnet-id",
                    "assignPublicIp": true
                },
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskSize": 1,
                    "diskTypeId": "disk-type-id"
                }
            }
        ]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "metrics": [
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "clickhouse_cluster",
                    "disk_size": 1,
                    "disk_type_id": "disk-type-id",
                    "online": 1,
                    "public_ip": 1,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "clickhouse_cluster"
                    ],
                    "software_accelerated_network_cores": 0
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "clickhouse_cluster",
                    "disk_size": 1,
                    "disk_type_id": "disk-type-id",
                    "online": 1,
                    "public_ip": 1,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "zk"
                    ],
                    "software_accelerated_network_cores": 0
                }
            }
        ]
    }
    """

  Scenario: Modify to burstable on multi-host cluster fails
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "b2.compute.3"
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "ClickHouse shard with resource preset 'b2.compute.3' and disk type 'network-ssd' allows at most 1 host"
    }
    """

  Scenario: Resources modify on network-ssd disk works
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.2"
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """

  Scenario: Disk scale up on network-ssd disk works
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "diskSize": 21474836480
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """

  Scenario: Disk scale down on network-ssd disk without feature flag fails
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "diskSize": 21474836480
                }
            }
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "diskSize": 10737418240
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Requested feature is not available"
    }
    """

  Scenario: Modify with wrong disk type id fails
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "diskTypeId": "no-disk-type"
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "diskTypeId 'no-disk-type' is not valid"
    }
    """

  Scenario: Disk scale down on network-ssd disk with feature flag works
    Given feature flags
    """
    ["MDB_NETWORK_DISK_TRUNCATE"]
    """
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "diskSize": 21474836480
                }
            }
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "diskSize": 10737418240
                }
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """

  Scenario: Resources modify on local-nvme disk without feature flag fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "local-nvme",
                    "diskSize": 107374182400
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
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
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "myt"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid2" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.compute.2"
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Requested feature is not available"
    }
    """

  @stop @events
  Scenario: Stop cluster works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:stop"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Stop ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.StopClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Stop" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Stop ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.StopClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    Given for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.StopCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we "GET" via REST at "/mdb/1.0/operations/worker_task_id2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Stop ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.StopClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STOPPING"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "status": "STOPPING"
    }
    """
    Given "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_by": "user",
        "description": "Stop ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.StopClusterMetadata",
            "cluster_id": "cid1"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
            "config": "**IGNORE**",
            "created_at": "**IGNORE**",
            "description": "test cluster",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "id": "cid1",
            "name": "test",
            "network_id": "network1",
            "security_group_ids": ["sg_id1", "sg_id2"],
            "status": "STOPPED",
            "maintenance_window": { "anytime": {} },
            "monitoring": [{
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm/cid=cid1",
                "name": "YASM"
            }, {
                "description": "Solomon charts",
                "link": "https://solomon/cid=cid1&fExtID=folder1",
                "name": "Solomon"
            }, {
                "description": "Console charts",
                "link": "https://console/cid=cid1&fExtID=folder1",
                "name": "Console"
            }],
            "service_account_id": "sa1"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STOPPED"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "status": "STOPPED"
    }
    """

  @stop
  Scenario: Modifying stopped cluster fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:stop"
    And we "Stop" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then "worker_task_id2" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "myt",
            "type": "CLICKHOUSE"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "operation is not allowed in current cluster status"

  @stop
  Scenario: Stop stopped cluster fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:stop"
    And we "Stop" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then "worker_task_id2" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:stop"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """
    When we "Stop" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "operation is not allowed in current cluster status"

  @stop
  Scenario: Delete stopped cluster works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:stop"
    And we "Stop" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then "worker_task_id2" acquired and finished by worker
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.DeleteCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """

  @start @events
  Scenario: Start stopped cluster works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:stop"
    And we "Stop" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then "worker_task_id2" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:start"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.StartClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Start" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Start ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.StartClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    Given for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.StartCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STARTING"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "status": "STARTING"
    }
    """
    When we GET "/mdb/1.0/operations/worker_task_id3"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.StartClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    Given "worker_task_id3" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id3"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_by": "user",
        "description": "Start ClickHouse cluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.StartClusterMetadata",
            "cluster_id": "cid1"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
            "config": "**IGNORE**",
            "created_at": "**IGNORE**",
            "description": "test cluster",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "id": "cid1",
            "name": "test",
            "network_id": "network1",
            "security_group_ids": ["sg_id1", "sg_id2"],
            "status": "RUNNING",
            "maintenance_window": { "anytime": {} },
            "monitoring": [{
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm/cid=cid1",
                "name": "YASM"
            }, {
                "description": "Solomon charts",
                "link": "https://solomon/cid=cid1&fExtID=folder1",
                "name": "Solomon"
            }, {
                "description": "Console charts",
                "link": "https://console/cid=cid1&fExtID=folder1",
                "name": "Console"
            }],
            "service_account_id": "sa1"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "status": "RUNNING"
    }
    """

  @start
  Scenario: Start running cluster fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:start"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """
    When we "Start" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "operation is not allowed in current cluster status"

  @move
  Scenario: Cluster move inside one cloud works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:move" with data
    """
    {
        "destinationFolderId": "folder4"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Move ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.MoveClusterMetadata",
            "clusterId": "cid1",
            "destinationFolderId": "folder4",
            "sourceFolderId": "folder1"
        }
    }
    """
    When we "Move" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "destination_folder_id": "folder4"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Move ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.MoveClusterMetadata",
            "cluster_id": "cid1",
            "destination_folder_id": "folder4",
            "source_folder_id": "folder1"
        }
    }
    """
    Given for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.MoveCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder4"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "folder_id": "folder4"
    }
    """

  @move
  Scenario: Cluster move between different clouds fails
    Given we disallow move cluster between clouds
    When we "Move" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "destination_folder_id": "folder2"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "moving cluster between folders in different clouds is unavailable"

  @security_groups
  Scenario: Removing security groups works
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": []
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" is empty list
    And "worker_task_id2" acquired and finished by worker
    And worker clear security groups for "cid1"
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": null
    }
    """

  @security_groups
  Scenario: Modify security groups works
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": ["sg_id2"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" containing:
      |sg_id2|
    And worker clear security groups for "cid1"
    And worker set "sg_id2" security groups on "cid1"
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": ["sg_id2"]
    }
    """

  @security_groups
  Scenario: Modify security groups with same groups show no changes
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": ["sg_id2", "sg_id1", "sg_id2"]
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """

  @security_groups
  Scenario: Modify security groups with more than limit show error
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": ["sg_id1", "sg_id2", "sg_id3", "sg_id4", "sg_id5", "sg_id6", "sg_id7", "sg_id8", "sg_id9", "sg_id10", "sg_id11"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "too many security groups (10 is the maximum)"
    }
    """

    @grpc_api @assign_public_ip
    Scenario: Modify assign_public_ip works
        When we "UpdateHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            "update_host_specs": [{
              "host_name": "sas-1.df.cloud.yandex.net",
              "assign_public_ip": true
            }]
        }
        """
        Then we get gRPC response with body
        """
        {
            "description": "Update hosts in ClickHouse cluster",
            "done": false,
            "id": "worker_task_id2",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateClusterHostsMetadata",
                "cluster_id": "cid1",
                "host_names": [
                    "sas-1.df.cloud.yandex.net"
                ]
            }
        }
        """
        When we GET "/mdb/1.0/operations/worker_task_id2"
        Then we get response with status 200 and body contains
        """
        {
            "createdBy": "user",
            "description": "Update hosts in ClickHouse cluster",
            "done": false,
            "id": "worker_task_id2",
            "metadata": {
                "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterHostsMetadata",
                "clusterId": "cid1",
                "hostNames": ["sas-1.df.cloud.yandex.net"]
            }
        }
        """
        And we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        And gRPC response body at path "$.hosts[4]" contains
        """
        {
          "name": "sas-1.df.cloud.yandex.net",
          "assign_public_ip": true
        }
        """
