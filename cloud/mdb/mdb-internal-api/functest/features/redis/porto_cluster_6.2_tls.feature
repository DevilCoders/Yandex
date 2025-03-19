Feature: Create/Modify Porto Redis 6.2 tls Cluster

  Background:
    Given default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_2": {
                "password": "p@ssw#$rd!?",
                "databases": 15,
                "clientOutputBufferLimitNormal": {
                    "hardLimit": 46777216,
                    "hardLimitUnit": "BYTES",
                    "softLimit": 4388608,
                    "softLimitUnit": "BYTES",
                    "softSeconds": 40
                },
                "clientOutputBufferLimitPubsub": {
                    "hardLimit": 56777216,
                    "hardLimitUnit": "BYTES",
                    "softLimit": 5388608,
                    "softLimitUnit": "BYTES",
                    "softSeconds": 50
                }
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
        "networkId": "IN-PORTO-NO-NETWORK-API",
        "tlsEnabled": true,
        "persistenceMode": "ON"
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
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "redis",
                       "role": "Master",
                       "status": "Alive"
                   },
                   {
                       "name": "sentinel",
                       "role": "Unknown",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "redis",
                       "role": "Replica",
                       "status": "Alive"
                   },
                   {
                       "name": "sentinel",
                       "role": "Unknown",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "redis",
                       "role": "Replica",
                       "status": "Alive"
                   },
                   {
                       "name": "sentinel",
                       "role": "Unknown",
                       "status": "Alive"
                   }
               ]
           }
       ]
    }
    """
    And feature flags
    """
    ["MDB_REDIS_62"]
    """
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create Redis cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker

  @events @persistence
  Scenario: Cluster creation works
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "6.2",
            "access": {
                "webSql": false,
                "dataLens": false,
                "dataTransfer": false,
                "serverless": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "redisConfig_6_2": {
                "defaultConfig": {
                    "maxmemoryPolicy": "NOEVICTION",
                    "timeout": 0,
                    "databases": 16,
                    "notifyKeyspaceEvents": "",
                    "slowlogLogSlowerThan": 10000,
                    "slowlogMaxLen": 1000,
                    "clientOutputBufferLimitNormal": {
                        "hardLimitUnit": "BYTES",
                        "softLimitUnit": "BYTES"
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimitUnit": "BYTES",
                        "softLimitUnit": "BYTES"
                    }
                },
                "effectiveConfig": {
                    "maxmemoryPolicy": "NOEVICTION",
                    "timeout": 0,
                    "databases": 15,
                    "notifyKeyspaceEvents": "",
                    "slowlogLogSlowerThan": 10000,
                    "slowlogMaxLen": 1000,
                    "clientOutputBufferLimitNormal": {
                        "hardLimit": 46777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 4388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 40
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 56777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 5388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 50
                    }
                },
                "userConfig": {
                    "databases": 15,
                    "clientOutputBufferLimitNormal": {
                        "hardLimit": 46777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 4388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 40
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 56777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 5388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 50
                    }
                }
            },
            "resources": {
                "diskSize": 17179869184,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
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
                "description": "Solomon charts",
                "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid1&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-redis",
                "name": "Solomon"
            },
            {
                "description": "Console charts",
                "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/redis_cluster/cid1?section=monitoring",
                "name": "Console"
            }
        ],
        "name": "test",
        "networkId": "",
        "status": "RUNNING",
        "sharded": false,
        "tlsEnabled": true
    }
    """
    And for "worker_task_id1" exists "yandex.cloud.events.mdb.redis.CreateCluster" event with
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
        "cpuUsed": 3.0,
        "memoryUsed": 12884901888,
        "ssdSpaceUsed": 51539607552
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "backup": {
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                }
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "dbaas": {
                "assign_public_ip": false,
                "on_dedicated_host": false,
                "cloud": {
                    "cloud_ext_id": "cloud1"
                },
                "cluster": {
                    "subclusters": {
                        "subcid1": {
                            "hosts": {},
                            "name": "test",
                            "roles": [
                                "redis_cluster"
                            ],
                            "shards": {
                                "shard_id1": {
                                    "hosts": {
                                        "iva-1.db.yandex.net": {"geo": "iva"},
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
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "cluster_id": "cid1",
                "cluster_name": "test",
                "cluster_type": "redis_cluster",
                "disk_type_id": "local-ssd",
                "flavor": {
                    "cpu_guarantee": 1.0,
                    "cpu_limit": 1.0,
                    "cpu_fraction": 100,
                    "gpu_limit": 0,
                    "description": "s1.porto.1",
                    "id": "00000000-0000-0000-0000-000000000000",
                    "io_limit": 20971520,
                    "io_cores_limit": 0,
                    "memory_guarantee": 4294967296,
                    "memory_limit": 4294967296,
                    "name": "s1.porto.1",
                    "network_guarantee": 16777216,
                    "network_limit": 16777216,
                    "type": "standard",
                    "generation": 1,
                    "platform_id": "mdb-v1",
                    "vtype": "porto"
                },
                "folder": {
                    "folder_ext_id": "folder1"
                },
                "fqdn": "iva-1.db.yandex.net",
                "geo": "iva",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "shard_id": "shard_id1",
                "shard_name": "shard1",
                "space_limit": 17179869184,
                "subcluster_id": "subcid1",
                "subcluster_name": "test",
                "vtype": "porto",
                "vtype_id": null
            },
            "default pillar": true,
            "redis": {
                "config": {
                    "maxmemory": 3221225472,
                    "maxmemory-policy": "noeviction",
                    "repl-backlog-size": 429496729,
                    "replica-priority": 100,
                    "client-output-buffer-limit normal": "46777216 4388608 40",
                    "client-output-buffer-limit replica": "429496729 214748364 60",
                    "client-output-buffer-limit pubsub": "56777216 5388608 50",
                    "timeout": 0,
                    "requirepass": {
                            "data": "p@ssw#$rd!?",
                            "encryption_version": 0
                        },
                    "masterauth": {
                            "data": "p@ssw#$rd!?",
                            "encryption_version": 0
                        },
                    "cluster-enabled": "no",
                    "databases": 15,
                    "notify-keyspace-events": "",
                    "slowlog-log-slower-than": 10000,
                    "slowlog-max-len": 1000,
                    "appendonly": "yes",
                    "save": ""
                },
                "secrets": {
                    "renames": {
                        "ACL": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "BGREWRITEAOF": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "BGSAVE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "CONFIG": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "DEBUG": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "LASTSAVE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MIGRATE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MODULE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MONITOR": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MOVE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "OBJECT": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "REPLICAOF": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "SAVE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "SHUTDOWN": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "SLAVEOF": {
                            "data": "dummy",
                            "encryption_version": 0
                        }
                    },
                    "sentinel_direct_renames": {
                        "ACL": {
                            "data": "dummy",
                            "encryption_version": 0
                        }
                    },
                    "sentinel_renames": {
                        "CONFIG-SET": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "FAILOVER": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "RESET": {
                            "data": "dummy",
                            "encryption_version": 0
                        }
                    }
                },
                "tls": {
                    "enabled": true
                },
                "zk_hosts": [
                    "localhost"
                ]
            },
            "redis default pillar": true,
            "runlist": [
                "components.redis_cluster"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
            "versions": {
                "redis": {
                    "edition": "some hidden value",
                    "major_version": "6.2",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                }
            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """

  @grpc
  Scenario: Cluster update resources work with rebalance
    When we "Rebalance" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "sharding must be enabled"

  @persistence
  Scenario: Persistence only same configuration gives no changes detected
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "persistenceMode": "ON"
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """

  @persistence
  Scenario: Persistence only configuration change works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "persistenceMode": "OFF"
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body partially contains
    """
    {
        "data": {
            "redis": {
                "config": {
                    "appendonly": "no",
                    "save": ""
                }
            }
        }
    }
    """

  @persistence
  Scenario: Persistence turning on with patched pillar (rdb on, aof off) gives no changes detected
    When we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,redis,config,save}', to_jsonb(CAST('900 1' AS text)))
        WHERE cid = 'cid1'
    """
    And we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,redis,config,appendonly}', to_jsonb(CAST('no' AS text)))
        WHERE cid = 'cid1'
    """
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "persistenceMode": "ON"
    }
    """
    Then we get response with status 400 and body contains
    """
    {
        "code": 3,
        "message": "No changes detected"
    }
    """

  @persistence
  Scenario: Persistence turning off with patched pillar (rdb on, aof on) works
    When we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,redis,config,save}', to_jsonb(CAST('900 1' AS text)))
        WHERE cid = 'cid1'
    """
    And we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,redis,config,appendonly}', to_jsonb(CAST('yes' AS text)))
        WHERE cid = 'cid1'
    """
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "persistenceMode": "OFF"
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body partially contains
    """
    {
        "data": {
            "redis": {
                "config": {
                    "appendonly": "no",
                    "save": ""
                }
            }
        }
    }
    """

  @persistence
  Scenario: Persistence turning off with patched pillar (rdb on, aof off) works
    When we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,redis,config,save}', to_jsonb(CAST('900 1' AS text)))
        WHERE cid = 'cid1'
    """
    And we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,redis,config,appendonly}', to_jsonb(CAST('no' AS text)))
        WHERE cid = 'cid1'
    """
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "persistenceMode": "OFF"
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body partially contains
    """
    {
        "data": {
            "redis": {
                "config": {
                    "appendonly": "no",
                    "save": ""
                }
            }
        }
    }
    """

  @persistence
  Scenario: Persistence turning on with patched pillar (rdb off, aof off) works
    When we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,redis,config,save}', to_jsonb(CAST('' AS text)))
        WHERE cid = 'cid1'
    """
    And we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,redis,config,appendonly}', to_jsonb(CAST('no' AS text)))
        WHERE cid = 'cid1'
    """
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "persistenceMode": "ON"
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body partially contains
    """
    {
        "data": {
            "redis": {
                "config": {
                    "appendonly": "yes",
                    "save": ""
                }
            }
        }
    }
    """

  @buffers
  Scenario: Memory limits change validation works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_6_2": {
                "clientOutputBufferLimitNormal": {
                    "hardLimit": 26777216,
                    "hardLimitUnit": "tyz",
                    "softLimit":  2388608,
                    "softLimitUnit": "BYTES",
                    "softSeconds": 20
                }
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.redisConfig_6_2.clientOutputBufferLimitNormal.hardLimitUnit: Invalid value 'tyz', allowed values: BYTES, GIGABYTES, KILOBYTES, MEGABYTES"
    }
    """

  @buffers @persistence
  Scenario: Additional configuration change works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_6_2": {
                "maxmemoryPolicy": "VOLATILE_LRU",
                "timeout": 10,
                "slowlogLogSlowerThan": 30000,
                "slowlogMaxLen": 4000,
                "notifyKeyspaceEvents": "Elgtdm",
                "clientOutputBufferLimitNormal": {
                    "hardLimit": 26777216,
                    "hardLimitUnit": "BYTES",
                    "softLimit":  2388608,
                    "softLimitUnit": "BYTES",
                    "softSeconds": 20
                },
                "clientOutputBufferLimitPubsub": {
                    "hardLimit": 36777216,
                    "hardLimitUnit": "BYTES",
                    "softLimit":  3388608,
                    "softLimitUnit": "BYTES",
                    "softSeconds": 30
                }
            }
        },
        "persistenceMode": "OFF"
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "backup": {
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                }
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "dbaas": {
                "assign_public_ip": false,
                "on_dedicated_host": false,
                "cloud": {
                    "cloud_ext_id": "cloud1"
                },
                "cluster": {
                    "subclusters": {
                        "subcid1": {
                            "hosts": {},
                            "name": "test",
                            "roles": [
                                "redis_cluster"
                            ],
                            "shards": {
                                "shard_id1": {
                                    "hosts": {
                                        "iva-1.db.yandex.net": {"geo": "iva"},
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
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "cluster_id": "cid1",
                "cluster_name": "test",
                "cluster_type": "redis_cluster",
                "disk_type_id": "local-ssd",
                "flavor": {
                    "cpu_guarantee": 1.0,
                    "cpu_limit": 1.0,
                    "cpu_fraction": 100,
                    "gpu_limit": 0,
                    "description": "s1.porto.1",
                    "id": "00000000-0000-0000-0000-000000000000",
                    "io_limit": 20971520,
                    "io_cores_limit": 0,
                    "memory_guarantee": 4294967296,
                    "memory_limit": 4294967296,
                    "name": "s1.porto.1",
                    "network_guarantee": 16777216,
                    "network_limit": 16777216,
                    "type": "standard",
                    "generation": 1,
                    "platform_id": "mdb-v1",
                    "vtype": "porto"
                },
                "folder": {
                    "folder_ext_id": "folder1"
                },
                "fqdn": "iva-1.db.yandex.net",
                "geo": "iva",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "shard_id": "shard_id1",
                "shard_name": "shard1",
                "space_limit": 17179869184,
                "subcluster_id": "subcid1",
                "subcluster_name": "test",
                "vtype": "porto",
                "vtype_id": null
            },
            "default pillar": true,
            "redis": {
                "config": {
                    "maxmemory": 3221225472,
                    "maxmemory-policy": "volatile-lru",
                    "repl-backlog-size": 429496729,
                    "replica-priority": 100,
                    "client-output-buffer-limit normal": "26777216 2388608 20",
                    "client-output-buffer-limit pubsub": "36777216 3388608 30",
                    "client-output-buffer-limit replica": "429496729 214748364 60",
                    "timeout": 10,
                    "requirepass": {
                            "data": "p@ssw#$rd!?",
                            "encryption_version": 0
                        },
                    "masterauth": {
                            "data": "p@ssw#$rd!?",
                            "encryption_version": 0
                        },
                    "cluster-enabled": "no",
                    "databases": 15,
                    "notify-keyspace-events": "Elgtdm",
                    "slowlog-log-slower-than": 30000,
                    "slowlog-max-len": 4000,
                    "appendonly": "no",
                    "save": ""
                },
                "secrets": {
                    "renames": {
                        "ACL": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "BGREWRITEAOF": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "BGSAVE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "CONFIG": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "DEBUG": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "LASTSAVE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MIGRATE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MODULE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MONITOR": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MOVE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "OBJECT": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "REPLICAOF": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "SAVE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "SHUTDOWN": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "SLAVEOF": {
                            "data": "dummy",
                            "encryption_version": 0
                        }
                    },
                    "sentinel_direct_renames": {
                        "ACL": {
                            "data": "dummy",
                            "encryption_version": 0
                        }
                    },
                    "sentinel_renames": {
                        "CONFIG-SET": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "FAILOVER": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "RESET": {
                            "data": "dummy",
                            "encryption_version": 0
                        }
                    }
                },
                "tls": {
                    "enabled": true
                },
                "zk_hosts": [
                    "localhost"
                ]
            },
            "redis default pillar": true,
            "runlist": [
                "components.redis_cluster"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
            "versions": {
                "redis": {
                    "edition": "some hidden value",
                    "major_version": "6.2",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                }
            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """

  @slowlog
  Scenario: Slowlog max len change works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_6_2": {
                "slowlogMaxLen": 0
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body partially contains
    """
    {
        "data": {
            "redis": {
                "config": {
                    "slowlog-max-len": 0
                }
            }
        }
    }
    """

  @buffers
  Scenario: Only soft or hard buffer limit change works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_6_2": {
                "clientOutputBufferLimitNormal": {
                    "hardLimitUnit": "BYTES",
                    "softLimit":  2388607,
                    "softLimitUnit": "BYTES"
                },
                "clientOutputBufferLimitPubsub": {
                    "hardLimit": 36777217,
                    "hardLimitUnit": "BYTES",
                    "softLimitUnit": "BYTES"
                }
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.UpdateCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "backup": {
                "sleep": 7200,
                "start": {
                    "hours": 22,
                    "minutes": 15,
                    "seconds": 30,
                    "nanos": 100
                }
            },
            "cluster_private_key": {
                "data": "1",
                "encryption_version": 0
            },
            "dbaas": {
                "assign_public_ip": false,
                "on_dedicated_host": false,
                "cloud": {
                    "cloud_ext_id": "cloud1"
                },
                "cluster": {
                    "subclusters": {
                        "subcid1": {
                            "hosts": {},
                            "name": "test",
                            "roles": [
                                "redis_cluster"
                            ],
                            "shards": {
                                "shard_id1": {
                                    "hosts": {
                                        "iva-1.db.yandex.net": {"geo": "iva"},
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
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "cluster_id": "cid1",
                "cluster_name": "test",
                "cluster_type": "redis_cluster",
                "disk_type_id": "local-ssd",
                "flavor": {
                    "cpu_guarantee": 1.0,
                    "cpu_limit": 1.0,
                    "cpu_fraction": 100,
                    "gpu_limit": 0,
                    "description": "s1.porto.1",
                    "id": "00000000-0000-0000-0000-000000000000",
                    "io_limit": 20971520,
                    "io_cores_limit": 0,
                    "memory_guarantee": 4294967296,
                    "memory_limit": 4294967296,
                    "name": "s1.porto.1",
                    "network_guarantee": 16777216,
                    "network_limit": 16777216,
                    "type": "standard",
                    "generation": 1,
                    "platform_id": "mdb-v1",
                    "vtype": "porto"
                },
                "folder": {
                    "folder_ext_id": "folder1"
                },
                "fqdn": "iva-1.db.yandex.net",
                "geo": "iva",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "iva-1.db.yandex.net",
                    "myt-1.db.yandex.net",
                    "sas-1.db.yandex.net"
                ],
                "shard_id": "shard_id1",
                "shard_name": "shard1",
                "space_limit": 17179869184,
                "subcluster_id": "subcid1",
                "subcluster_name": "test",
                "vtype": "porto",
                "vtype_id": null
            },
            "default pillar": true,
            "redis": {
                "config": {
                    "maxmemory": 3221225472,
                    "maxmemory-policy": "noeviction",
                    "repl-backlog-size": 429496729,
                    "replica-priority": 100,
                    "client-output-buffer-limit normal": "46777216 2388607 40",
                    "client-output-buffer-limit pubsub": "36777217 5388608 50",
                    "client-output-buffer-limit replica": "429496729 214748364 60",
                    "timeout": 0,
                    "requirepass": {
                            "data": "p@ssw#$rd!?",
                            "encryption_version": 0
                        },
                    "masterauth": {
                            "data": "p@ssw#$rd!?",
                            "encryption_version": 0
                        },
                    "cluster-enabled": "no",
                    "databases": 15,
                    "notify-keyspace-events": "",
                    "slowlog-log-slower-than": 10000,
                    "slowlog-max-len": 1000,
                    "appendonly": "yes",
                    "save": ""
                },
                "secrets": {
                    "renames": {
                        "ACL": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "BGREWRITEAOF": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "BGSAVE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "CONFIG": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "DEBUG": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "LASTSAVE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MIGRATE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MODULE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MONITOR": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MOVE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "OBJECT": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "REPLICAOF": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "SAVE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "SHUTDOWN": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "SLAVEOF": {
                            "data": "dummy",
                            "encryption_version": 0
                        }
                    },
                    "sentinel_direct_renames": {
                        "ACL": {
                            "data": "dummy",
                            "encryption_version": 0
                        }
                    },
                    "sentinel_renames": {
                        "CONFIG-SET": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "FAILOVER": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "RESET": {
                            "data": "dummy",
                            "encryption_version": 0
                        }
                    }
                },
                "tls": {
                    "enabled": true
                },
                "zk_hosts": [
                    "localhost"
                ]
            },
            "redis default pillar": true,
            "runlist": [
                "components.redis_cluster"
            ],
            "s3_bucket": "yandexcloud-dbaas-cid1",
            "versions": {
                "redis": {
                    "edition": "some hidden value",
                    "major_version": "6.2",
                    "minor_version": "some hidden value",
                    "package_version": "some hidden value"
                }
            }
        },
        "yandex": {
            "environment": "qa"
        }
    }
    """

  @events @grpc
  Scenario: Manual failover works
    When we "POST" via REST at "/mdb/redis/1.0/clusters/cid1:startFailover"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start manual failover on Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.StartClusterFailoverMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "StartFailover" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Start manual failover on Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.redis.v1.StartClusterFailoverMetadata",
            "cluster_id": "cid1",
            "host_names": ["iva-1.db.yandex.net"]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.StartClusterFailover" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    Then "worker_task_id2" acquired and finished by worker

  @events @grpc
  Scenario: Manual failover with specified host works
    When we "POST" via REST at "/mdb/redis/1.0/clusters/cid1:startFailover" with data
    """
    {
        "hostNames": [
            "iva-1.db.yandex.net"
        ]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start manual failover on Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.StartClusterFailoverMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "StartFailover" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": [
            "iva-1.db.yandex.net"
        ]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Start manual failover on Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.redis.v1.StartClusterFailoverMetadata",
            "cluster_id": "cid1",
            "host_names": ["iva-1.db.yandex.net"]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.StartClusterFailover" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    Then "worker_task_id2" acquired and finished by worker

  @grpc
  Scenario Outline: Failover on sentinel cluster fails
    When we "POST" via REST at "/mdb/redis/1.0/clusters/cid1:startFailover" with data
    """
    {
        "hostNames": <hostnames>
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "<message>"
    }
    """
    When we "StartFailover" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_names": <hostnames>
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "<message>"
    Examples:
      | hostnames                                      | message                                                                     |
      | ["iva-1.db.yandex.net", "myt-1.db.yandex.net"] | Failover on redis cluster does not support multiple hostnames at this time. |
      | ["nonexistent.db.yandex.net"]                  | Hostnames you specified are not the part of cluster.                        |

  @grpc
  Scenario Outline: Failover on sentinel cluster fails due to broken health check
    Given health response
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
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "redis",
                       "status": "Alive"
                   },
                   {
                       "name": "sentinel",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "redis",
                       "status": "Alive"
                   },
                   {
                       "name": "sentinel",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "redis",
                       "status": "Alive"
                   },
                   {
                       "name": "sentinel",
                       "status": "Alive"
                   }
               ]
           }
       ]
    }
    """
    When we "POST" via REST at "/mdb/redis/1.0/clusters/cid1:startFailover"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "<message>"
    }
    """
    When we "StartFailover" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "<message>"
    Examples:
      | message                                                                      |
      | Unable to find masters for cluster. Failover is not safe. Aborting failover. |
