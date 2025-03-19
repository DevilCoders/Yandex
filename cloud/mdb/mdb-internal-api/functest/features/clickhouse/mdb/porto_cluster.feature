Feature: Create/Modify Porto ClickHouse Cluster

  Background:
    Given default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "cloudStorage": {
                "enabled": false
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
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
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    And "create_grpc" data
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
    And health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
               "status": "Alive"
           }
       ],
       "hosts": [
           {
               "fqdn": "man-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "vla-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "clickhouse",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "clickhouse",
                       "status": "Alive",
                       "role": "Unknown"
                   }
               ]
           }
       ]
    }
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create ClickHouse cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with "create_grpc" data
    Then we get gRPC response OK
    When "worker_task_id1" acquired and finished by worker

  @clusters @create @events
  Scenario: Cluster creation works
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.3",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
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
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
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
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
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
        "networkId": "",
        "status": "RUNNING"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folder_id": "folder1",
        "health": "ALIVE",
        "id": "cid1",
        "monitoring": [
            {
                "description": "YaSM (Golovan) charts",
                "link": "https://yasm/cid=cid1",
                "name": "YASM"
            },
            {
                "description": "Solomon charts",
                "link": "https://solomon/cid=cid1&fExtID=folder1",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console/cid=cid1&fExtID=folder1",
                "name": "Console"
            }
        ],
        "name": "test",
        "status": "RUNNING",
        "config": "**IGNORE**"
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
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data" contains
    """
    {
        "backup": {
            "sleep": 7200,
            "start": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "use_backup_service": false
        },
        "unmanaged": {
            "enable_zk_tls": true
        },
        "testing_repos": false,
        "cloud_storage": {
            "enabled": false
        },
        "clickhouse": {
            "ch_version": "21.3.8.76",
            "mysql_protocol": false,
            "postgresql_protocol": false,
            "embedded_keeper": false,
            "sql_user_management": false,
            "sql_database_management": false,
            "system_users": {
                "mdb_backup_admin": {
                    "hash": {
                        "data": "b5a2c96250612366ea272ffac6d9744aaf4b45aacd96aa7cfcb931ee3b558259",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "dummy",
                        "encryption_version": 0
                    }
                }
            },
            "user_management_v2": true,
            "config": {
                "builtin_dictionaries_reload_interval": 3600,
                "compression": [],
                "keep_alive_timeout": 3,
                "log_level": "information",
                "mark_cache_size": 5368709120,
                "max_concurrent_queries": 500,
                "max_connections": 4096,
                "max_table_size_to_drop": 53687091200,
                "max_partition_size_to_drop": 53687091200,
                "timezone": "Europe/Moscow",
                "merge_tree": {
                    "enable_mixed_granularity_parts": true,
                    "replicated_deduplication_window": 100,
                    "replicated_deduplication_window_seconds": 604800
                },
                "uncompressed_cache_size": 8589934592
            },
            "databases": [
                "testdb"
            ],
            "interserver_credentials": {
                "password": {
                    "data": "dummy",
                    "encryption_version": 0
                },
                "user": "interserver"
            },
            "shards": {
                "shard_id1": {
                    "weight": 100
                }
            },
            "users": {
                "test": {
                    "databases": {
                        "testdb": {}
                    },
                    "hash": {
                        "data": "10a6e6cc8311a3e2bcc09bf6c199adecd5dd59408c343e926b129c4914f3cb01",
                        "encryption_version": 0
                    },
                    "password": {
                        "data": "test_password",
                        "encryption_version": 0
                    },
                    "settings": {},
                    "quotas": []
                }
            },
            "zk_users": {
                "clickhouse": {
                    "password": {
                        "data": "dummy",
                        "encryption_version": 0
                    }
                }
            },
            "models": {},
            "format_schemas": {}
        },
        "clickhouse default pillar": true,
        "cluster_private_key": {
            "data": "1",
            "encryption_version": 0
        },
        "default pillar": true,
        "runlist": [
            "components.clickhouse_cluster"
        ],
        "s3_bucket": "yandexcloud-dbaas-cid1",
        "versions": {}
    }
    """
    And body at path "$.data.dbaas" contains
    """
    {
        "assign_public_ip": false,
        "on_dedicated_host": false,
        "cloud": {
            "cloud_ext_id": "cloud1"
        },
        "cluster": {
            "subclusters": {
                "subcid1": {
                    "hosts": {
                        "iva-1.db.yandex.net": {"geo": "iva"},
                        "man-1.db.yandex.net": {"geo": "man"},
                        "vla-1.db.yandex.net": {"geo": "vla"}
                    },
                    "name": "zookeeper_subcluster",
                    "roles": [
                        "zk"
                    ],
                    "shards": {}
                },
                "subcid2": {
                    "hosts": {},
                    "name": "clickhouse_subcluster",
                    "roles": [
                        "clickhouse_cluster"
                    ],
                    "shards": {
                        "shard_id1": {
                            "hosts": {
                                "myt-1.db.yandex.net": {"geo": "myt"},
                                "sas-1.db.yandex.net": {"geo": "sas"}
                            },
                            "name": "shard1"
                        }
                    }
                }
            }
        },
        "cluster_hosts": [
            "iva-1.db.yandex.net",
            "man-1.db.yandex.net",
            "myt-1.db.yandex.net",
            "sas-1.db.yandex.net",
            "vla-1.db.yandex.net"
        ],
        "cluster_id": "cid1",
        "cluster_name": "test",
        "cluster_type": "clickhouse_cluster",
        "disk_type_id": "local-ssd",
        "flavor": {
            "cpu_guarantee": 1.0,
            "cpu_limit": 1.0,
            "cpu_fraction": 100,
            "gpu_limit": 0,
            "description": "s2.porto.1",
            "id": "00000000-0000-0000-0000-000000000028",
            "io_limit": 20971520,
            "io_cores_limit": 0,
            "memory_guarantee": 4294967296,
            "memory_limit": 4294967296,
            "name": "s2.porto.1",
            "network_guarantee": 16777216,
            "network_limit": 16777216,
            "type": "standard",
            "generation": 2,
            "platform_id": "mdb-v1",
            "vtype": "porto"
        },
        "folder": {
            "folder_ext_id": "folder1"
        },
        "fqdn": "myt-1.db.yandex.net",
        "geo": "myt",
        "shard_hosts": [
            "myt-1.db.yandex.net",
            "sas-1.db.yandex.net"
        ],
        "shard_id": "shard_id1",
        "shard_name": "shard1",
        "space_limit": 10737418240,
        "subcluster_id": "subcid2",
        "subcluster_name": "clickhouse_subcluster",
        "vtype": "porto",
        "vtype_id": null
    }
    """

  @clusters @create
  Scenario: Cluster creation deprecated version fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.1",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
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
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Can't create cluster, version '21.1' is deprecated"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.1",
            "clickhouse": {
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
    Then we get gRPC response error with code INVALID_ARGUMENT and message "can't create cluster, version "21.1" is deprecated"

  @clusters @get
  Scenario Outline: Cluster health deduction works
    Given health response
    """
    {
       "clusters": [
           {
               "cid": "cid1",
               "status": "<status>"
           }
       ],
       "hosts": [
           {
               "fqdn": "man-1.db.yandex.net",
               "cid": "cid1",
               "status": "<zk-man>",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "<zk-man>"
                   }
               ]
           },
           {
               "fqdn": "vla-1.db.yandex.net",
               "cid": "cid1",
               "status": "<zk-vla>",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "<zk-vla>"
                   }
               ]
           },
           {
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "<zk-iva>",
               "services": [
                   {
                       "name": "zookeeper",
                       "status": "<zk-iva>"
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "<ch-myt>",
               "services": [
                   {
                       "name": "clickhouse",
                       "status": "<ch-myt>"
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "<ch-sas>",
               "services": [
                   {
                       "name": "clickhouse",
                       "status": "<ch-sas>"
                   }
               ]
           }
       ]
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "health": "<health>"
    }
    """
    Examples:
      | zk-iva  | zk-man  | zk-vla  | ch-myt  | ch-sas  | status   | health   |
      | Alive   | Alive   | Alive   | Alive   | Alive   | Alive    | ALIVE    |
      | Alive   | Dead    | Alive   | Alive   | Alive   | Degraded | DEGRADED |
      | Alive   | Alive   | Alive   | Dead    | Alive   | Degraded | DEGRADED |
      | Alive   | Alive   | Alive   | Dead    | Dead    | Dead     | DEAD     |
      | Dead    | Dead    | Dead    | Dead    | Dead    | Dead     | DEAD     |
      | Unknown | Unknown | Unknown | Unknown | Unknown | Unknown  | UNKNOWN  |

  Scenario: Config for unmanaged cluster doesn't work
    When we GET "/api/v1.0/config_unmanaged/iva-1.db.yandex.net"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Unknown cluster for host iva-1.db.yandex.net"
    }
    """

  Scenario: Folder operations list works
    When we run query
    """
    DELETE FROM dbaas.worker_queue
    """
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "logLevel": "DEBUG"
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
                "config": {
                    "logLevel": "ERROR"
                }
            }
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "logLevel": "DEBUG"
                }
            }
        }
    }
    """
    And we run query
    """
    UPDATE dbaas.worker_queue
    SET create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00',
        start_ts = null,
        end_ts = null,
        task_args = '{}',
        worker_id = null,
        result = null
    """
    And we "GET" via REST at "/mdb/clickhouse/1.0/operations?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "operations": [
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify ClickHouse cluster",
                "done": false,
                "id": "worker_task_id4",
                "metadata": {
                    "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify ClickHouse cluster",
                "done": false,
                "id": "worker_task_id3",
                "metadata": {
                    "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify ClickHouse cluster",
                "done": false,
                "id": "worker_task_id2",
                "metadata": {
                    "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            }
        ]
    }
    """
    When we "ListOperations" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "operations": [{
            "created_at": "2000-01-01T00:00:00Z",
            "created_by": "user",
            "description": "Modify ClickHouse cluster",
            "done": false,
            "id": "worker_task_id4",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateClusterMetadata",
                "cluster_id": "cid1"
            },
            "modified_at": "2000-01-01T00:00:00Z",
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "code": 0,
                "details": [],
                "message": "OK"
            }
        }, {
            "created_at": "2000-01-01T00:00:00Z",
            "created_by": "user",
            "description": "Modify ClickHouse cluster",
            "done": false,
            "id": "worker_task_id3",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateClusterMetadata",
                "cluster_id": "cid1"
            },
            "modified_at": "2000-01-01T00:00:00Z",
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "code": 0,
                "details": [],
                "message": "OK"
            }
        }, {
            "created_at": "2000-01-01T00:00:00Z",
            "created_by": "user",
            "description": "Modify ClickHouse cluster",
            "done": false,
            "id": "worker_task_id2",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateClusterMetadata",
                "cluster_id": "cid1"
            },
            "modified_at": "2000-01-01T00:00:00Z",
            "response": {
                "@type": "type.googleapis.com/google.rpc.Status",
                "code": 0,
                "details": [],
                "message": "OK"
            }
        }]
    }
    """

  @clusters @list
  Scenario: Cluster list works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
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
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "description": "test cluster"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test2",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.3",
            "clickhouse": {
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
        }],
        "description": "test cluster"
    }
    """
    Then we get gRPC response OK
    Given we run query
    """
    UPDATE dbaas.clusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we GET "/mdb/clickhouse/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusters": [
            {
                "name": "test",
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
                    "serviceAccountId": null,
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
                            "diskTypeId": "local-ssd",
                            "resourcePresetId": "s2.porto.1"
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
                "createdAt": "2000-01-01T00:00:00+00:00",
                "deletionProtection": false,
                "description": "test cluster",
                "environment": "PRESTABLE",
                "folderId": "folder1",
                "health": "ALIVE",
                "id": "cid1",
                "labels": {},
                "plannedOperation": null,
                "securityGroupIds": [],
                "serviceAccountId": null,
                "hostGroupIds": [],
                "maintenanceWindow": {
                    "anytime": {}
                },
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
                "networkId": "",
                "status": "RUNNING"
            },
            {
                "name": "test2",
                "config": {
                    "version": "21.3",
                    "serviceAccountId": null,
                    "embeddedKeeper": false,
                    "mysqlProtocol": false,
                    "postgresqlProtocol": false,
                    "sqlUserManagement": false,
                    "sqlDatabaseManagement": false,
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
                            "diskTypeId": "local-ssd",
                            "resourcePresetId": "s2.porto.1"
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
                "createdAt": "2000-01-01T00:00:00+00:00",
                "deletionProtection": false,
                "description": "test cluster",
                "environment": "PRESTABLE",
                "folderId": "folder1",
                "health": "UNKNOWN",
                "id": "cid2",
                "labels": {},
                "plannedOperation": null,
                "securityGroupIds": [],
                "serviceAccountId": null,
                "hostGroupIds": [],
                "maintenanceWindow": {
                    "anytime": {}
                },
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
                "networkId": "",
                "status": "CREATING"
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "page_size": 1
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "clusters": [{
            "name": "test",
            "created_at": "2000-01-01T00:00:00Z",
            "description": "test cluster",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "id": "cid1",
            "status": "RUNNING",
            "health": "ALIVE",
            "maintenance_window": { "anytime": {} },
            "config": "**IGNORE**"
        }],
        "next_page_token": "eyJMYXN0Q2x1c3Rlck5hbWUiOiJ0ZXN0In0="
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "page_size": 1,
        "page_token": "eyJMYXN0Q2x1c3Rlck5hbWUiOiJ0ZXN0In0="
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "clusters": [{
            "name": "test2",
            "created_at": "2000-01-01T00:00:00Z",
            "description": "test cluster",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "id": "cid2",
            "status": "CREATING",
            "maintenance_window": { "anytime": {} },
            "config": "**IGNORE**"
        }],
        "next_page_token": "eyJMYXN0Q2x1c3Rlck5hbWUiOiJ0ZXN0MiJ9"
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters" with params
    """
    {
        "folderId": "folder1",
        "filter": "name = \"test2\""
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "clusters": [
            {
                "name": "test2",
                "config": {
                    "version": "21.3",
                    "serviceAccountId": null,
                    "embeddedKeeper": false,
                    "mysqlProtocol": false,
                    "postgresqlProtocol": false,
                    "sqlUserManagement": false,
                    "sqlDatabaseManagement": false,
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
                            "diskTypeId": "local-ssd",
                            "resourcePresetId": "s2.porto.1"
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
                "createdAt": "2000-01-01T00:00:00+00:00",
                "deletionProtection": false,
                "description": "test cluster",
                "environment": "PRESTABLE",
                "folderId": "folder1",
                "health": "UNKNOWN",
                "id": "cid2",
                "labels": {},
                "plannedOperation": null,
                "securityGroupIds": [],
                "serviceAccountId": null,
                "hostGroupIds": [],
                "maintenanceWindow": {
                    "anytime": {}
                },
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
                "networkId": "",
                "status": "CREATING"
            }
        ]
    }
    """

  @clusters @list
  Scenario Outline: Cluster list with invalid filter fails
    When we GET "/mdb/clickhouse/1.0/clusters" with params
    """
    {
        "folderId": "folder1",
        "filter": "<filter>"
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | filter          | status | error code | message                                                                                            |
      | name = 1        | 422    | 3          | Filter 'name = 1' has wrong 'name' attribute type. Expected a string.                              |
      | my = 1          | 422    | 3          | Filter by 'my' ('my = 1') is not supported.                                                        |
      | name =          | 422    | 3          | The request is invalid.\nfilter: Filter syntax error (missing value) at or near 6.\nname =\n     ^ |
      | name < \"test\" | 501    | 12         | Operator '<' not implemented.                                                                      |

  @hosts @list
  Scenario: Host list works
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shardName": "shard1",
                "subnetId": "",
                "type": "CLICKHOUSE",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shardName": "shard1",
                "subnetId": "",
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "vla"
            }
        ]
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
        {
        "hosts": [
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shard_name": "",
                "subnet_id": "",
                "type": "ZOOKEEPER",
                "zone_id": "iva",
                "system": null
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shard_name": "",
                "subnet_id": "",
                "type": "ZOOKEEPER",
                "zone_id": "man",
                "system": null
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shard_name": "shard1",
                "subnet_id": "",
                "type": "CLICKHOUSE",
                "zone_id": "myt",
                "system": null
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shard_name": "shard1",
                "subnet_id": "",
                "type": "CLICKHOUSE",
                "zone_id": "sas",
                "system": null
            },
            {
                "assign_public_ip": false,
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "disk_size": "10737418240",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shard_name": "",
                "subnet_id": "",
                "type": "ZOOKEEPER",
                "zone_id": "vla",
                "system": null
            }
        ],
        "next_page_token": ""
    }
    """

  @hosts @list
  Scenario: Host listing pagination works
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 1
    }
    """
    Then we get gRPC response with body ignoring empty
    """
        {
        "hosts": [{
            "cluster_id": "cid1",
            "health": "ALIVE",
            "name": "iva-1.db.yandex.net",
            "resources": {
                "disk_size": "10737418240",
                "disk_type_id": "local-ssd",
                "resource_preset_id": "s2.porto.1"
            },
            "services": [
                {
                    "health": "ALIVE",
                    "type": "ZOOKEEPER"
                }
            ],
            "type": "ZOOKEEPER",
            "zone_id": "iva"
        }],
        "next_page_token": "eyJPZmZzZXQiOjF9"
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 4,
        "page_token": "eyJPZmZzZXQiOjF9"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
        {
        "hosts": [{
            "cluster_id": "cid1",
            "health": "ALIVE",
            "name": "man-1.db.yandex.net",
            "resources": {
                "disk_size": "10737418240",
                "disk_type_id": "local-ssd",
                "resource_preset_id": "s2.porto.1"
            },
            "services": [
                {
                    "health": "ALIVE",
                    "type": "ZOOKEEPER"
                }
            ],
            "type": "ZOOKEEPER",
            "zone_id": "man"
        },
        {
            "cluster_id": "cid1",
            "health": "ALIVE",
            "name": "myt-1.db.yandex.net",
            "resources": {
                "disk_size": "10737418240",
                "disk_type_id": "local-ssd",
                "resource_preset_id": "s2.porto.1"
            },
            "services": [
                {
                    "health": "ALIVE",
                    "type": "CLICKHOUSE"
                }
            ],
            "shard_name": "shard1",
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        },
        {
            "cluster_id": "cid1",
            "health": "ALIVE",
            "name": "sas-1.db.yandex.net",
            "resources": {
                "disk_size": "10737418240",
                "disk_type_id": "local-ssd",
                "resource_preset_id": "s2.porto.1"
            },
            "services": [
                {
                    "health": "ALIVE",
                    "type": "CLICKHOUSE"
                }
            ],
            "shard_name": "shard1",
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        },
        {
            "cluster_id": "cid1",
            "health": "ALIVE",
            "name": "vla-1.db.yandex.net",
            "resources": {
                "disk_size": "10737418240",
                "disk_type_id": "local-ssd",
                "resource_preset_id": "s2.porto.1"
            },
            "services": [
                {
                    "health": "ALIVE",
                    "type": "ZOOKEEPER"
                }
            ],
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }],
        "next_page_token": "eyJPZmZzZXQiOjV9"
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 1,
        "page_token": "eyJPZmZzZXQiOjV9"
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [],
        "next_page_token": ""
    }
    """

  @hosts @create @events
  Scenario: Adding host works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }],
        "copySchema": true
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "man-2.db.yandex.net"
            ]
        }
    }
    """
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "man"
        }],
        "copy_schema": true
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add hosts to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.AddClusterHostsMetadata",
            "cluster_id": "cid1",
            "host_names": ["man-2.db.yandex.net"]
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.AddClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "man-2.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [],
                "shardName": "shard1",
                "subnetId": "",
                "type": "CLICKHOUSE",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shardName": "shard1",
                "subnetId": "",
                "type": "CLICKHOUSE",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shardName": "shard1",
                "subnetId": "",
                "type": "CLICKHOUSE",
                "zoneId": "sas"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "vla"
            }
        ]
    }
    """

  @hosts @create
  Scenario Outline: Adding host with invalid params fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": <hosts>
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | hosts                                                                              | status | error code | message                                                                                                               |
      | []                                                                                 | 422    | 3          | No hosts to add are specified                                                                                         |
      | [{"zoneId": "man", "type": "CLICKHOUSE"}, {"zoneId": "man", "type": "CLICKHOUSE"}] | 501    | 12         | Adding multiple hosts at once is not supported yet                                                                    |
      | [{"zoneId": "nodc", "type": "CLICKHOUSE"}]                                         | 422    | 3          | The request is invalid.\nhostSpecs.0.zoneId: Invalid value, valid value is one of ['iva', 'man', 'myt', 'sas', 'vla'] |
      | [{"zoneId": "man", "type": "NOTYPE"}]                                              | 422    | 3          | The request is invalid.\nhostSpecs.0.type: Invalid value 'NOTYPE', allowed values: CLICKHOUSE, ZOOKEEPER              |

  @hosts @create
  Scenario Outline: Adding host with invalid params fails via gRPC
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": <hosts>
    }
    """
    Then we get gRPC response error with code <code> and message "<message>"
    Examples:
      | hosts                                                                                | code             | message                                                                                 |
      | []                                                                                   | INVALID_ARGUMENT | no hosts to add are specified                                                           |
      | [{"zone_id": "man", "type": "CLICKHOUSE"}, {"zone_id": "man", "type": "CLICKHOUSE"}] | UNIMPLEMENTED    | adding multiple hosts at once is not supported yet                                      |
      | [{"zone_id": "nodc", "type": "CLICKHOUSE"}]                                          | INVALID_ARGUMENT | invalid zone nodc, valid are: iva, man, myt, sas, vla |
      | [{"zone_id": "man", "type": "TYPE_UNSPECIFIED"}]                                     | INVALID_ARGUMENT | unknown host role                                                                       |

  @hosts @delete @events
  Scenario: Deleting host works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }]
    }
    """
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "man"
        }]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete hosts from ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.DeleteClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    When we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["sas-1.db.yandex.net"]
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.clickhouse.DeleteClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "iva"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "man-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "man-2.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [],
                "shardName": "shard1",
                "subnetId": "",
                "type": "CLICKHOUSE",
                "zoneId": "man"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "CLICKHOUSE"
                    }
                ],
                "shardName": "shard1",
                "subnetId": "",
                "type": "CLICKHOUSE",
                "zoneId": "myt"
            },
            {
                "assignPublicIp": false,
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "vla-1.db.yandex.net",
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                },
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "ZOOKEEPER"
                    }
                ],
                "shardName": null,
                "subnetId": "",
                "type": "ZOOKEEPER",
                "zoneId": "vla"
            }
        ]
    }
    """

  @hosts @delete
  Scenario Outline: Deleting host with invalid params fails
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": <host list>
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | host list                                      | status | error code | message                                                              |
      | []                                             | 422    | 3          | No hosts to delete are specified                                     |
      | ["sas-1.db.yandex.net", "myt-1.db.yandex.net"] | 501    | 12         | Deleting multiple hosts at once is not supported yet                 |
      | ["nohost"]                                     | 404    | 5          | Host 'nohost' does not exist                                         |
      | ["sas-1.db.yandex.net"]                        | 422    | 9          | Cluster cannot have less than 2 ClickHouse hosts in HA configuration |

  @hosts @delete
  Scenario Outline: Deleting host with invalid params fails gRPC
    When we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": <host list>
    }
    """
    Then we get gRPC response error with code <code> and message "<message>"
    Examples:
      | host list                                      | code                | message                                                              |
      | []                                             | FAILED_PRECONDITION | no hosts to delete are specified                                     |
      | ["sas-1.db.yandex.net", "myt-1.db.yandex.net"] | UNIMPLEMENTED       | deleting multiple hosts at once is not supported yet                 |
      | ["nohost"]                                     | NOT_FOUND           | host "nohost" does not exist                                         |
      | ["sas-1.db.yandex.net"]                        | FAILED_PRECONDITION | cluster cannot have less than 2 ClickHouse hosts in HA configuration |

  @hosts @delete
  Scenario: Deleting host with exclusive constraint violation fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "man"
        }]
    }
    """
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "man"
        }]
    }
    """
    And we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Conflicting operation worker_task_id2 detected"
    }
    """
    When we "DeleteHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": ["sas-1.db.yandex.net"]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "conflicting operation "worker_task_id2" detected"

  @clusters @update @events
  Scenario: Label set works
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "labels": {
            "olap": "yes"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Update ClickHouse cluster metadata",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "labels": {
            "olap": "yes"
        },
        "update_mask": {
            "paths": [
                "labels"
            ]
        }
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "labels": {
            "olap": "yes"
        }
    }
    """

  @clusters @update @events
  Scenario: Description set works
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "description": "my cool description"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "description": "my cool description",
        "update_mask": {
            "paths": [
                "description"
            ]
        }
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "my cool description"
    }
    """

  @clusters @update
  Scenario: Change disk size to invalid value fails
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "diskSize": 1
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Invalid disk_size, must be between or equal 10737418240 and 2199023255552"
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "disk_size": 1
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.clickhouse.resources.disk_size"
            ]
        }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "invalid disk size, must be inside [10737418240,2199023255553) range"

  @clusters @update
  Scenario: Change disk size to same value fails
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
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
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "disk_size": 10737418240
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.clickhouse.resources.disk_size"
            ]
        }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "no changes detected"

  @clusters @update @malformed-request
  Scenario: Modify with malformed json
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
           "resources": {
               "resourcePresetId": "s2.porto.3"
            }
        }
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """

  @clusters @update @events
  Scenario: Change disk size to valid value works
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "disk_size": 21474836480
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.clickhouse.resources.disk_size"
            ]
        }
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.UpdateCluster" event with
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
        "ssdSpaceUsed": 75161927680
    }
    """

  @clusters @update
  Scenario: Scaling up ClickHouse and ZooKeeper with insufficient quota fails
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.4"
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s2.porto.3"
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Quota limits exceeded, not enough cpu: 4 cores, memory: 16 GiB",
        "details": [
            {
                "@type": "type.private-api.yandex-cloud.ru/quota.QuotaFailure",
                "cloud_id": "cloud1",
                "violations": [
                    {
                        "metric": {
                            "limit": 24,
                            "name": "mdb.cpu.count",
                            "usage": 19
                        },
                        "required": 28
                    },
                    {
                        "metric": {
                            "limit": 103079215104,
                            "name": "mdb.memory.size",
                            "usage": 81604378624
                        },
                        "required": 120259084288
                    }
                ]
            }
        ]
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.4"
                }
            },
            "zookeeper": {
                "resources": {
                    "resource_preset_id": "s2.porto.3"
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.clickhouse.resources.resource_preset_id",
                "config_spec.zookeeper.resources.resource_preset_id"
            ]
        }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "Quota limits exceeded, not enough "cpu: 4, memory: 16 GiB". Please contact support to request extra resource quota."

  @clusters @update
  Scenario: Scaling up ClickHouse and ZooKeeper disks with insufficient quota fails
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "diskSize": 211527139328
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 193273528320
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Quota limits exceeded, not enough ssd_space: 334 GiB",
        "details": [
            {
                "@type": "type.private-api.yandex-cloud.ru/quota.QuotaFailure",
                "cloud_id": "cloud1",
                "violations": [
                    {
                        "metric": {
                            "limit": 644245094400,
                            "name": "mdb.ssd.size",
                            "usage": 455266533376
                        },
                        "required": 1002874863616
                    }
                ]
            }
        ]
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "disk_size": 211527139328
                }
            },
            "zookeeper": {
                "resources": {
                    "disk_size": 193273528320
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.clickhouse.resources.disk_size",
                "config_spec.zookeeper.resources.disk_size"
            ]
        }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "Quota limits exceeded, not enough "ssd space: 334 GiB". Please contact support to request extra resource quota."

  @clusters @update
  Scenario: Scaling cluster up works
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.2"
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.2"
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.clickhouse.resources.resource_preset_id"
            ]
        }
    }
    """
    Then we get gRPC response OK
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 7.0,
        "memoryUsed": 30064771072,
        "ssdSpaceUsed": 53687091200
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.3",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
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
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.2"
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
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
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
        "networkId": "",
        "status": "RUNNING"
    }
    """

  @clusters @update
  Scenario: Scaling cluster up works (with redundant parameters)
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.2",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
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
        "id": "worker_task_id2",
        "metadata": {
          "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.2",
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
        "update_mask": {
            "paths": [
                "config_spec.clickhouse.resources.resource_preset_id",
                "config_spec.clickhouse.resources.disk_type_id",
                "config_spec.clickhouse.resources.disk_size",
                "config_spec.zookeeper.resources.resource_preset_id",
                "config_spec.zookeeper.resources.disk_type_id",
                "config_spec.zookeeper.resources.disk_size"
            ]
        }
    }
    """
    Then we get gRPC response OK
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 7.0,
        "memoryUsed": 30064771072,
        "ssdSpaceUsed": 53687091200
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.3",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
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
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.2"
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
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
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
        "networkId": "",
        "status": "RUNNING"
    }
    """

  @clusters @update
  Scenario: Scaling cluster down works
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.2"
                }
            }
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.2"
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.clickhouse.resources.resource_preset_id"
            ]
        }
    }
    """
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker
    And we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1"
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1"
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.clickhouse.resources.resource_preset_id"
            ]
        }
    }
    """
    Then we get gRPC response OK
    And in worker_queue exists "worker_task_id3" id with args "reverse_order" set to "true"
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/v1/quota/cloud1"
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
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.3",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
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
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
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
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
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
        "networkId": "",
        "status": "RUNNING"
    }
    """

  @clusters @update
  Scenario: Scaling cluster down works (with redundant parameters)
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.2"
                }
            }
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.2"
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.clickhouse.resources.resource_preset_id"
            ]
        }
    }
    """
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker
    And we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
                    "diskTypeId": "local-ssd",
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
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
        "update_mask": {
            "paths": [
                "config_spec.clickhouse.resources.resource_preset_id",
                "config_spec.clickhouse.resources.disk_type_id",
                "config_spec.clickhouse.resources.disk_size",
                "config_spec.zookeeper.resources.resource_preset_id",
                "config_spec.zookeeper.resources.disk_type_id",
                "config_spec.zookeeper.resources.disk_size"
            ]
        }
    }
    """
    Then we get gRPC response OK
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/v1/quota/cloud1"
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
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.3",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
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
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
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
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
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
        "networkId": "",
        "status": "RUNNING"
    }
    """

  @clusters @update
  Scenario: Scaling ZooKeeper subcluster up works
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s2.porto.2"
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "zookeeper": {
                "resources": {
                    "resource_preset_id": "s2.porto.2"
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.zookeeper.resources.resource_preset_id"
            ]
        }
    }
    """
    Then we get gRPC response OK
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 8.0,
        "memoryUsed": 34359738368,
        "ssdSpaceUsed": 53687091200
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.3",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
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
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
                }
            },
            "zookeeper": {
                "resources": {
                    "diskSize": 10737418240,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.2"
                }
            },
            "cloudStorage": {
                "enabled": false
            }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
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
        "networkId": "",
        "status": "RUNNING"
    }
    """

  @clusters @update
  Scenario: Scaling ZooKeeper subcluster down works
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s2.porto.2"
                }
            }
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "zookeeper": {
                "resources": {
                    "resource_preset_id": "s2.porto.2"
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.zookeeper.resources.resource_preset_id"
            ]
        }
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s2.porto.1"
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "config_spec": {
            "zookeeper": {
                "resources": {
                    "resource_preset_id": "s2.porto.1"
                }
            }
        },
        "update_mask": {
            "paths": [
                "config_spec.zookeeper.resources.resource_preset_id"
            ]
        }
    }
    """
    Then we get gRPC response OK
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
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.3",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
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
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
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
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
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
        "networkId": "",
        "status": "RUNNING"
    }
    """

  @clusters @update
  Scenario: Upgrading ClickHouse version works
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "21.4"
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
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.4",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
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
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
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
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
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
        "networkId": "",
        "status": "RUNNING"
    }
    """

  @clusters @update
  Scenario: Upgrading ClickHouse to invalid version fails
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "19.30"
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Version '19.30' is not available, allowed versions are: 21.1, 21.2, 21.3, 21.4, 21.7, 21.8, 21.11, 22.1, 22.3, 22.5"
    }
    """

  @clusters @update
  Scenario: Upgrading ClickHouse with scaling up ClickHouse subcluster fails
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "21.4",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.2"
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Version update cannot be mixed with update of host resources"
    }
    """

  @clusters @update
  Scenario: Upgrading ClickHouse with scaling up ZooKeeper subcluster fails
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "21.4",
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s2.porto.2"
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Version update cannot be mixed with update of host resources"
    }
    """

  @clusters @update @events
  Scenario: Backup window start option change works
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "backupWindowStart": {
                "hours": 23,
                "minutes": 10
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "21.3",
            "serviceAccountId": null,
            "embeddedKeeper": false,
            "mysqlProtocol": false,
            "mysqlProtocol": false,
            "postgresqlProtocol": false,
            "sqlUserManagement": false,
            "sqlDatabaseManagement": false,
            "access": {
                "webSql": false,
                "dataLens": false,
                "metrika": false,
                "serverless": false,
                "dataTransfer": false
            },
            "backupWindowStart": {
                "hours": 23,
                "minutes": 10,
                "seconds": 0,
                "nanos": 0
            },
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
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s2.porto.1"
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
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "ALIVE",
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
        "networkId": "",
        "status": "RUNNING"
    }
    """

  @clusters @update
  Scenario: Adding graphite rollup settings works
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "graphiteRollup": [
                        {
                            "name": "test_graphite_rollup",
                            "patterns": [
                                {
                                    "function": "any",
                                    "retention": [
                                        {
                                            "age": 60,
                                            "precision": 1
                                        }
                                    ]
                                }
                            ]
                        }
                    ]
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
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "graphiteRollup": [
            {
                "name": "test_graphite_rollup",
                "patterns": [
                    {
                        "function": "any",
                        "retention": [
                            {
                                "age": 60,
                                "precision": 1
                            }
                        ]
                    }
                ]
            }
        ]
    }
    """

  @clusters @update
  Scenario: Adding duplicated graphite rollup settings fails
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "graphiteRollup": [
                        {
                            "name": "test_graphite_rollup",
                            "patterns": [
                                {
                                    "function": "any",
                                    "retention": [
                                        {
                                            "age": 60,
                                            "precision": 1
                                        }
                                    ]
                                }
                            ]
                        },
                        {
                            "name": "test_graphite_rollup",
                            "patterns": [
                                {
                                    "function": "any",
                                    "retention": [
                                        {
                                            "age": 60,
                                            "precision": 1
                                        }
                                    ]
                                }
                            ]
                        }
                    ]
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Cluster cannot have multiple Graphite rollup settings with the same name ('test_graphite_rollup')"
    }
    """

  @clusters @update
  Scenario: Adding duplicated kafka topic settings fails
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "kafkaTopics": [
                        {
                            "name": "test_topic",
                            "settings": {}
                        },
                        {
                            "name": "test_topic",
                            "settings": {}
                        }
                    ]
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Configuration cannot have multiple Kafka topics with the same name ('test_topic')"
    }
    """

  @clusters @update @events
  Scenario: Allow access for DataLens
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "access": {
                "dataLens": true
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.access" contains
    """
    {
        "webSql": false,
        "dataLens": true,
        "metrika": false,
        "serverless": false,
        "dataTransfer": false
    }
    """

  @clusters @update @events
  Scenario: Allow access for Metrika
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "access": {
                "metrika": true
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.access" contains
    """
    {
        "webSql": false,
        "dataLens": false,
        "metrika": true,
        "serverless": false,
        "dataTransfer": false
    }
    """

  @clusters @update @events
  Scenario: Cluster name change works
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "name": "changed"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Update ClickHouse cluster metadata",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "changed",
        "update_mask": {
            "paths": [
                "name"
            ]
        }
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "name": "changed"
    }
    """

  @clusters @update
  Scenario: Cluster name change to same value fails
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "name": "test"
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "test",
        "update_mask": {
            "paths": [
                "name"
            ]
        }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "no changes detected"

  @clusters @update
  Scenario: Cluster name change with duplicate value fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
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
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }],
        "description": "test cluster"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test2",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
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
        }],
        "description": "test cluster"
    }
    """
    Then we get gRPC response OK
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "name": "test2"
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Cluster 'test2' already exists"
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "test2",
        "update_mask": {
            "paths": [
                "name"
            ]
        }
    }
    """
    Then we get gRPC response error with code ALREADY_EXISTS and message "cluster "test2" already exists"

  @clusters @update
  Scenario: Change cluster settings works
    When we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "clickhouse": {
                "config": {
                    "logLevel": "TRACE",
                    "maxConnections": 30,
                    "maxConcurrentQueries": 100,
                    "keepAliveTimeout": 5,
                    "uncompressedCacheSize": 83886080,
                    "markCacheSize": 10737418240,
                    "maxTableSizeToDrop": 214748364800,
                    "maxPartitionSizeToDrop": 214748364800,
                    "timezone": "UTC",
                    "queryLogRetentionSize": 1000000000,
                    "queryLogRetentionTime": 864000000,
                    "queryThreadLogEnabled": true,
                    "queryThreadLogRetentionSize": 1000000001,
                    "queryThreadLogRetentionTime": 864001000,
                    "partLogRetentionSize": 1000000002,
                    "partLogRetentionTime": 864002000,
                    "metricLogEnabled": true,
                    "metricLogRetentionSize": 1000000003,
                    "metricLogRetentionTime": 864003000,
                    "traceLogEnabled": true,
                    "traceLogRetentionSize": 1000000004,
                    "traceLogRetentionTime": 864004000,
                    "textLogEnabled": true,
                    "textLogRetentionSize": 1000000005,
                    "textLogRetentionTime": 864005000,
                    "textLogLevel": "INFORMATION",
                    "mergeTree": {
                        "replicatedDeduplicationWindow": 200,
                        "replicatedDeduplicationWindowSeconds": 36000,
                        "partsToDelayInsert": 300,
                        "partsToThrowInsert": 600,
                        "maxReplicatedMergesInQueue": 6,
                        "numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge": 5,
                        "maxBytesToMergeAtMinSpaceInPool": 1000000,
                        "maxBytesToMergeAtMaxSpaceInPool": 1000001
                    },
                    "backgroundPoolSize": 32,
                    "backgroundSchedulePoolSize": 32,
                    "kafka": {
                        "securityProtocol": "SECURITY_PROTOCOL_SASL_PLAINTEXT",
                        "saslMechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                        "saslUsername": "test_user",
                        "saslPassword": "test_password"
                    },
                    "kafkaTopics": [
                        {
                            "name": "test_topic",
                            "settings": {
                                "securityProtocol": "SECURITY_PROTOCOL_SASL_PLAINTEXT",
                                "saslMechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                                "saslUsername": "test_user2",
                                "saslPassword": "test_password2"
                            }
                        }
                    ],
                    "rabbitmq": {
                        "username": "rabbitmq_user",
                        "password": "rabbitmq_password"
                    }
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And body at path "$.config.clickhouse.config.effectiveConfig" contains
    """
    {
        "logLevel": "TRACE",
        "maxConnections": 30,
        "maxConcurrentQueries": 100,
        "keepAliveTimeout": 5,
        "uncompressedCacheSize": 83886080,
        "markCacheSize": 10737418240,
        "maxTableSizeToDrop": 214748364800,
        "maxPartitionSizeToDrop": 214748364800,
        "timezone": "UTC",
        "queryLogRetentionSize": 1000000000,
        "queryLogRetentionTime": 864000000,
        "queryThreadLogEnabled": true,
        "queryThreadLogRetentionSize": 1000000001,
        "queryThreadLogRetentionTime": 864001000,
        "partLogRetentionSize": 1000000002,
        "partLogRetentionTime": 864002000,
        "metricLogEnabled": true,
        "metricLogRetentionSize": 1000000003,
        "metricLogRetentionTime": 864003000,
        "traceLogEnabled": true,
        "traceLogRetentionSize": 1000000004,
        "traceLogRetentionTime": 864004000,
        "textLogEnabled": true,
        "textLogRetentionSize": 1000000005,
        "textLogRetentionTime": 864005000,
        "textLogLevel": "INFORMATION",
        "mergeTree": {
            "replicatedDeduplicationWindow": 200,
            "replicatedDeduplicationWindowSeconds": 36000,
            "partsToDelayInsert": 300,
            "partsToThrowInsert": 600,
            "maxReplicatedMergesInQueue": 6,
            "numberOfFreeEntriesInPoolToLowerMaxSizeOfMerge": 5,
            "maxBytesToMergeAtMinSpaceInPool": 1000000,
            "maxBytesToMergeAtMaxSpaceInPool": 1000001
        },
        "backgroundPoolSize": 32,
        "backgroundSchedulePoolSize": 32,
        "kafka": {
            "securityProtocol": "SECURITY_PROTOCOL_SASL_PLAINTEXT",
            "saslMechanism": "SASL_MECHANISM_SCRAM_SHA_512",
            "saslUsername": "test_user"
        },
        "kafkaTopics": [
            {
                "name": "test_topic",
                "settings": {
                    "securityProtocol": "SECURITY_PROTOCOL_SASL_PLAINTEXT",
                    "saslMechanism": "SASL_MECHANISM_SCRAM_SHA_512",
                    "saslUsername": "test_user2"
                }
            }
        ],
        "rabbitmq": {
            "username": "rabbitmq_user"
        }
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse.config" contains
    """
    {
        "log_level": "trace",
        "max_connections": 30,
        "max_concurrent_queries": 100,
        "keep_alive_timeout": 5,
        "mark_cache_size": 10737418240,
        "uncompressed_cache_size": 83886080,
        "max_table_size_to_drop": 214748364800,
        "max_partition_size_to_drop": 214748364800,
        "timezone": "UTC",
        "query_log_retention_size": 1000000000,
        "query_log_retention_time": 864000,
        "query_thread_log_enabled": true,
        "query_thread_log_retention_size": 1000000001,
        "query_thread_log_retention_time": 864001,
        "part_log_retention_size": 1000000002,
        "part_log_retention_time": 864002,
        "metric_log_enabled": true,
        "metric_log_retention_size": 1000000003,
        "metric_log_retention_time": 864003,
        "trace_log_enabled": true,
        "trace_log_retention_size": 1000000004,
        "trace_log_retention_time": 864004,
        "text_log_enabled": true,
        "text_log_retention_size": 1000000005,
        "text_log_retention_time": 864005,
        "text_log_level": "information",
        "merge_tree": {
            "enable_mixed_granularity_parts": true,
            "replicated_deduplication_window": 200,
            "replicated_deduplication_window_seconds": 36000,
            "parts_to_delay_insert": 300,
            "parts_to_throw_insert": 600,
            "max_replicated_merges_in_queue": 6,
            "number_of_free_entries_in_pool_to_lower_max_size_of_merge": 5,
            "max_bytes_to_merge_at_min_space_in_pool": 1000000,
            "max_bytes_to_merge_at_max_space_in_pool": 1000001
        },
        "background_pool_size": 32,
        "background_schedule_pool_size": 32,
        "kafka": {
            "security_protocol": "SASL_PLAINTEXT",
            "sasl_mechanism": "SCRAM-SHA-512",
            "sasl_username": "test_user",
            "sasl_password": {
                "data": "test_password",
                "encryption_version": 0
            }
        },
        "kafka_topics": [
            {
                "name": "test_topic",
                "settings": {
                    "security_protocol": "SASL_PLAINTEXT",
                    "sasl_mechanism": "SCRAM-SHA-512",
                    "sasl_username": "test_user2",
                    "sasl_password": {
                        "data": "test_password2",
                        "encryption_version": 0
                    }
                }
            }
        ],
        "rabbitmq": {
            "username": "rabbitmq_user",
            "password": {
                "data": "rabbitmq_password",
                "encryption_version": 0
            }
        }
    }
    """

  @clusters @add-zookeeper
  Scenario: Adding second ZooKeeper subcluster fails
    When we POST "/mdb/clickhouse/1.0/clusters/cid1:addZookeeper" with data
    """
    {
        "hostSpecs": [{
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "ZooKeeper has been already configured for this cluster"
    }
    """

  @clusters @move @events
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

  @clusters @move @events
  Scenario: Cluster move between different clouds works
    Given we allow move cluster between clouds
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:move" with data
    """
    {
        "destinationFolderId": "folder2"
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
            "destinationFolderId": "folder2",
            "sourceFolderId": "folder1"
        }
    }
    """
    When we "Move" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "destination_folder_id": "folder2"
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
            "destination_folder_id": "folder2",
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
    When we GET "/mdb/1.0/operations/worker_task_id3"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Move ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.MoveClusterMetadata",
            "clusterId": "cid1",
            "destinationFolderId": "folder2",
            "sourceFolderId": "folder1"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder2"
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
        "folder_id": "folder2"
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 0,
        "cpuUsed": 0.0,
        "memoryUsed": 0,
        "ssdSpaceUsed": 0
    }
    """
    When we GET "/mdb/v1/quota/cloud2"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud2",
        "clustersUsed": 1,
        "cpuUsed": 5.0,
        "memoryUsed": 21474836480,
        "ssdSpaceUsed": 53687091200
    }
    """
    Given "worker_task_id2" acquired and finished by worker
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
        "created_at": "**IGNORE**",
        "created_by": "user",
        "description": "Move ClickHouse cluster",
        "done": true,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.MoveClusterMetadata",
            "cluster_id": "cid1",
            "source_folder_id": "folder1",
            "destination_folder_id": "folder2"
        },
        "modified_at": "**IGNORE**",
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Cluster",
            "id": "cid1",
            "name": "test",
            "description": "test cluster",
            "folder_id": "folder2",
            "health": "ALIVE",
            "status": "RUNNING",
            "created_at": "**IGNORE**",
            "environment": "PRESTABLE",
            "maintenance_window": "**IGNORE**",
            "monitoring": "**IGNORE**",
            "config": "**IGNORE**"
        }
    }
    """

  @clusters @delete @events
  Scenario: Cluster removal works
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.DeleteClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.DeleteClusterMetadata",
            "cluster_id": "cid1"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.DeleteCluster" event with
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
        "clustersUsed": 0,
        "cpuUsed": 0.0,
        "memoryUsed": 0,
        "ssdSpaceUsed": 0
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusters": []
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 403

  @clusters @delete @create
  Scenario: After cluster delete cluster.name can be reused
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.DeleteCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with "create_grpc" data
    Then we get gRPC response OK

  @clusters @delete
  Scenario: Cluster with running operations can not be deleted
    When we run query
    """
    UPDATE dbaas.worker_queue
       SET result = NULL,
           end_ts = NULL
    """
    And we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Cluster 'cid1' has active tasks"
    }
    """
    And we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "conflicting operation "worker_task_id1" detected"

  @clusters @delete
  Scenario: Cluster with failed operations can be deleted
    When we run query
    """
    UPDATE dbaas.worker_queue
       SET result = false
    """
    And we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response OK
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.DeleteCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """

  @clusters @delete @operations
  Scenario: After cluster delete cluster operations are shown
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "description": "changed"
    }
    """
    Then we get response with status 200
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "description": "changed",
        "update_mask": {
            "paths": [
                "description"
            ]
        }
    }
    """
    Then we get gRPC response OK
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200
    And we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
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
    When we run query
    """
    UPDATE dbaas.worker_queue
       SET start_ts = CASE WHEN start_ts ISNULL THEN null
                      ELSE TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
                      END,
             end_ts = CASE WHEN end_ts ISNULL THEN null
                      ELSE TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
                      END,
          create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    When we GET "/mdb/clickhouse/1.0/operations?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "operations": [
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Delete ClickHouse cluster",
                "done": false,
                "id": "worker_task_id3",
                "metadata": {
                    "@type": "yandex.cloud.mdb.clickhouse.v1.DeleteClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify ClickHouse cluster",
                "done": true,
                "id": "worker_task_id2",
                "metadata": {
                    "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00",
                "response": {}
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Create ClickHouse cluster",
                "done": true,
                "id": "worker_task_id1",
                "metadata": {
                    "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00",
                "response": {}
            }
        ]
    }
    """

  @hosts @create @decommission @geo
  Scenario: Create cluster host in decommissioning geo
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "ugr",
            "type": "CLICKHOUSE"
        }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "No new resources could be created in zone 'ugr'"
    }
    """
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "ugr"
        }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "no new resources can be created in zone "ugr""

  @clusters @stop
  Scenario: Stop cluster not implemented
    When we POST "/mdb/clickhouse/1.0/clusters/cid1:stop"
    Then we get response with status 501 and body contains
    """
    {
        "code": 12,
        "message": "Stop for clickhouse_cluster not implemented in that installation"
    }
    """

  @clusters @get @status
  Scenario: Cluster status
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man",
            "type": "CLICKHOUSE"
        }]
    }
    """
    Then we get response with status 200
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "man"
        }]
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "UPDATING"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """

  Scenario: Operation with burstable flavor
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test_burstable",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "b2.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resourcePresetId": "s2.porto.1",
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
        "description": "test burstable cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test_burstable",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "b2.porto.1",
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
        "description": "test burstable cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker
    Then in worker_queue exists "worker_task_id2" id and data contains
    """
    {
        "timeout": "06:00:00"
    }
    """
