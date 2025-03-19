Feature: Create/Modify Porto Redis Cluster

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_5"]
    """
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "p@ssw#$rd!?",
                "databases": 15
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
        "networkId": "IN-PORTO-NO-NETWORK-API"
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

  @events
  Scenario: Cluster creation works
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "5.0",
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
            "redisConfig_5_0": {
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
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    }
                },
                "userConfig": {
                    "clientOutputBufferLimitNormal": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "databases": 15
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
        "tlsEnabled": false
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
                    "client-output-buffer-limit normal": "16777216 8388608 60",
                    "client-output-buffer-limit replica": "429496729 214748364 60",
                    "client-output-buffer-limit pubsub": "16777216 8388608 60",
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
                    "sentinel_renames": {
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
                    "enabled": false
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
                    "major_version": "5.0",
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
               "fqdn": "iva-1.db.yandex.net",
               "cid": "cid1",
               "status": "<status>",
               "services": [
                   {
                       "name": "redis",
                       "role": "Master",
                       "status": "<redis-iva>"
                   },
                   {
                       "name": "sentinel",
                       "role": "Unknown",
                       "status": "<sentinel-iva>"
                   }
               ]
           },
           {
               "fqdn": "myt-1.db.yandex.net",
               "cid": "cid1",
               "status": "<status>",
               "services": [
                   {
                       "name": "redis",
                       "role": "Replica",
                       "status": "<redis-myt>"
                   },
                   {
                       "name": "sentinel",
                       "role": "Unknown",
                       "status": "<sentinel-myt>"
                   }
               ]
           },
           {
               "fqdn": "sas-1.db.yandex.net",
               "cid": "cid1",
               "status": "<status>",
               "services": [
                   {
                       "name": "redis",
                       "role": "Replica",
                       "status": "<redis-sas>"
                   },
                   {
                       "name": "sentinel",
                       "role": "Unknown",
                       "status": "<sentinel-sas>"
                   }
               ]
           }
       ]
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "health": "<health>"
    }
    """
    Examples:
      | sentinel-iva | sentinel-myt | sentinel-sas | redis-iva | redis-myt | redis-sas | status   | health   |
      | Alive        | Dead         | Alive        | Alive     | Alive     | Alive     | Degraded | DEGRADED |
      | Alive        | Alive        | Alive        | Dead      | Alive     | Alive     | Degraded | DEGRADED |
      | Alive        | Alive        | Alive        | Dead      | Dead      | Dead      | Dead     | DEAD     |
      | Dead         | Dead         | Dead         | Alive     | Alive     | Alive     | Degraded | DEGRADED |
      | Dead         | Dead         | Dead         | Dead      | Dead      | Dead      | Dead     | DEAD     |

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
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_5_0": {
                "maxmemoryPolicy": "VOLATILE_LRU",
                "timeout": 10
            }
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_5_0": {
                "timeout": 100
            }
        }
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_5_0": {
                "timeout": 10
            }
        }
    }
    """
    And "worker_task_id4" acquired and finished by worker
    And we run query
    """
    UPDATE dbaas.worker_queue
    SET
        start_ts = null,
        end_ts = null,
        result = null,
        create_ts = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    And we GET "/mdb/redis/1.0/operations?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "operations": [
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify Redis cluster",
                "done": false,
                "id": "worker_task_id4",
                "metadata": {
                    "@type": "yandex.cloud.mdb.redis.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify Redis cluster",
                "done": false,
                "id": "worker_task_id3",
                "metadata": {
                    "@type": "yandex.cloud.mdb.redis.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify Redis cluster",
                "done": false,
                "id": "worker_task_id2",
                "metadata": {
                    "@type": "yandex.cloud.mdb.redis.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            }
        ]
    }
    """

  Scenario: Cluster list works
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "another test cluster"
    }
    """
    And we run query
    """
    UPDATE dbaas.clusters
    SET created_at = TIMESTAMP WITH TIME ZONE '2000-01-01 00:00:00+00'
    """
    And we GET "/mdb/redis/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusters": [
            {
                "config": {
                    "version": "5.0",
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
                    "redisConfig_5_0": {
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
                                "hardLimit": 16777216,
                                "hardLimitUnit": "BYTES",
                                "softLimit": 8388608,
                                "softLimitUnit": "BYTES",
                                "softSeconds": 60
                            },
                            "clientOutputBufferLimitPubsub": {
                                "hardLimit": 16777216,
                                "hardLimitUnit": "BYTES",
                                "softLimit": 8388608,
                                "softLimitUnit": "BYTES",
                                "softSeconds": 60
                            }
                        },
                        "userConfig": {
                            "clientOutputBufferLimitNormal": {
                                "hardLimit": 16777216,
                                "hardLimitUnit": "BYTES",
                                "softLimit": 8388608,
                                "softLimitUnit": "BYTES",
                                "softSeconds": 60
                            },
                            "clientOutputBufferLimitPubsub": {
                                "hardLimit": 16777216,
                                "hardLimitUnit": "BYTES",
                                "softLimit": 8388608,
                                "softLimitUnit": "BYTES",
                                "softSeconds": 60
                            },
                            "databases": 15
                        }
                    },
                    "resources": {
                        "diskSize": 17179869184,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
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
                "persistenceMode": "ON",
                "plannedOperation": null,
                "securityGroupIds": [],
                "hostGroupIds": [],
                "maintenanceWindow": {
                    "anytime": {}
                },
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
                "tlsEnabled": false
            },
            {
                "config": {
                    "version": "5.0",
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
                    "redisConfig_5_0": {
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
                            "databases": 16,
                            "notifyKeyspaceEvents": "",
                            "slowlogLogSlowerThan": 10000,
                            "slowlogMaxLen": 1000,
                            "clientOutputBufferLimitNormal": {
                                "hardLimit": 16777216,
                                "hardLimitUnit": "BYTES",
                                "softLimit": 8388608,
                                "softLimitUnit": "BYTES",
                                "softSeconds": 60
                            },
                            "clientOutputBufferLimitPubsub": {
                                "hardLimit": 16777216,
                                "hardLimitUnit": "BYTES",
                                "softLimit": 8388608,
                                "softLimitUnit": "BYTES",
                                "softSeconds": 60
                            }
                        },
                        "userConfig": {
                            "clientOutputBufferLimitNormal": {
                                "hardLimit": 16777216,
                                "hardLimitUnit": "BYTES",
                                "softLimit": 8388608,
                                "softLimitUnit": "BYTES",
                                "softSeconds": 60
                            },
                            "clientOutputBufferLimitPubsub": {
                                "hardLimit": 16777216,
                                "hardLimitUnit": "BYTES",
                                "softLimit": 8388608,
                                "softLimitUnit": "BYTES",
                                "softSeconds": 60
                            }
                        }
                    },
                    "resources": {
                        "diskSize": 17179869184,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
                    }
                },
                "createdAt": "2000-01-01T00:00:00+00:00",
                "deletionProtection": false,
                "description": "another test cluster",
                "environment": "PRESTABLE",
                "folderId": "folder1",
                "health": "UNKNOWN",
                "id": "cid2",
                "labels": {},
                "persistenceMode": "ON",
                "plannedOperation": null,
                "securityGroupIds": [],
                "hostGroupIds": [],
                "maintenanceWindow": {
                    "anytime": {}
                },
                "monitoring": [
                    {
                        "description": "Solomon charts",
                        "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid2&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-redis",
                        "name": "Solomon"
                    },
                    {
                        "description": "Console charts",
                        "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/redis_cluster/cid2?section=monitoring",
                        "name": "Console"
                    }
                ],
                "name": "test2",
                "networkId": "",
                "status": "CREATING",
                "sharded": false,
                "tlsEnabled": false
            }
        ]
    }
    """
    When we GET "/mdb/redis/1.0/clusters" with params
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
                "config": {
                    "version": "5.0",
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
                    "redisConfig_5_0": {
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
                            "databases": 16,
                            "notifyKeyspaceEvents": "",
                            "slowlogLogSlowerThan": 10000,
                            "slowlogMaxLen": 1000,
                            "clientOutputBufferLimitNormal": {
                                "hardLimit": 16777216,
                                "hardLimitUnit": "BYTES",
                                "softLimit": 8388608,
                                "softLimitUnit": "BYTES",
                                "softSeconds": 60
                            },
                            "clientOutputBufferLimitPubsub": {
                                "hardLimit": 16777216,
                                "hardLimitUnit": "BYTES",
                                "softLimit": 8388608,
                                "softLimitUnit": "BYTES",
                                "softSeconds": 60
                            }
                        },
                        "userConfig": {
                            "clientOutputBufferLimitNormal": {
                                "hardLimit": 16777216,
                                "hardLimitUnit": "BYTES",
                                "softLimit": 8388608,
                                "softLimitUnit": "BYTES",
                                "softSeconds": 60
                            },
                            "clientOutputBufferLimitPubsub": {
                                "hardLimit": 16777216,
                                "hardLimitUnit": "BYTES",
                                "softLimit": 8388608,
                                "softLimitUnit": "BYTES",
                                "softSeconds": 60
                            }
                        }
                    },
                    "resources": {
                        "diskSize": 17179869184,
                        "diskTypeId": "local-ssd",
                        "resourcePresetId": "s1.porto.1"
                    }
                },
                "createdAt": "2000-01-01T00:00:00+00:00",
                "deletionProtection": false,
                "description": "another test cluster",
                "environment": "PRESTABLE",
                "folderId": "folder1",
                "health": "UNKNOWN",
                "id": "cid2",
                "labels": {},
                "persistenceMode": "ON",
                "plannedOperation": null,
                "securityGroupIds": [],
                "hostGroupIds": [],
                "maintenanceWindow": {
                    "anytime": {}
                },
                "monitoring": [
                    {
                        "description": "Solomon charts",
                        "link": "https://solomon.yandex-team.ru/?cluster=mdb_cid2&project=internal-mdb&service=mdb&dashboard=internal-mdb-cluster-redis",
                        "name": "Solomon"
                    },
                    {
                        "description": "Console charts",
                        "link": "https://console-preprod.cloud.yandex.ru/folders/folder1/mdb/cluster/redis_cluster/cid2?section=monitoring",
                        "name": "Console"
                    }
                ],
                "name": "test2",
                "networkId": "",
                "status": "CREATING",
                "sharded": false,
                "tlsEnabled": false
            }
        ]
    }
    """

  Scenario Outline: Cluster list with invalid filter fails
    When we GET "/mdb/redis/1.0/clusters" with params
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

  @grpc
  Scenario: Host list works
    When we GET "/mdb/redis/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "replicaPriority": 100,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva",
                "shardName": "shard1"
            },
            {
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "replicaPriority": 50,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt",
                "shardName": "shard1"
            },
            {
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "replicaPriority": 100,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas",
                "shardName": "shard1"
            }
        ]
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 3
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [
            {
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "replica_priority": "100",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "MASTER",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnet_id": "",
                "system": null,
                "zone_id": "iva",
                "shard_name": "shard1"
            },
            {
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "replica_priority": "50",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnet_id": "",
                "system": null,
                "zone_id": "myt",
                "shard_name": "shard1"
            },
            {
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "replica_priority": "100",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnet_id": "",
                "system": null,
                "zone_id": "sas",
                "shard_name": "shard1"
            }
        ]
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 1
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [
            {
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "replica_priority": "100",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "MASTER",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnet_id": "",
                "system": null,
                "zone_id": "iva",
                "shard_name": "shard1"
            }
        ]
    }
    """

  @events @grpc
  Scenario: Modifying host works
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "iva-1.db.yandex.net",
            "replicaPriority": 300
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify hosts in Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.UpdateClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "iva-1.db.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.UpdateClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "iva-1.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "replicaPriority": 300,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva",
                "shardName": "shard1"
            },
            {
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "replicaPriority": 50,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt",
                "shardName": "shard1"
            },
            {
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "replicaPriority": 100,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas",
                "shardName": "shard1"
            }
        ]
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 3
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [
            {
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "replica_priority": "300",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "MASTER",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnet_id": "",
                "system": null,
                "zone_id": "iva",
                "shard_name": "shard1"
            },
            {
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "replica_priority": "50",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnet_id": "",
                "system": null,
                "zone_id": "myt",
                "shard_name": "shard1"
            },
            {
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "replica_priority": "100",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnet_id": "",
                "system": null,
                "zone_id": "sas",
                "shard_name": "shard1"
            }
        ]
    }
    """

  @events @grpc
  Scenario: Adding host works
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "man-1.db.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.AddClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "man-1.db.yandex.net"
            ]
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "replicaPriority": 100,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva",
                "shardName": "shard1"
            },
            {
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "man-1.db.yandex.net",
                "replicaPriority": 100,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "subnetId": "",
                "zoneId": "man",
                "shardName": "shard1"
            },
            {
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "replicaPriority": 50,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt",
                "shardName": "shard1"
            },
            {
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "replicaPriority": 100,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnetId": "",
                "zoneId": "sas",
                "shardName": "shard1"
            }
        ]
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 4
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [
            {
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "replica_priority": "100",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "MASTER",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnet_id": "",
                "system": null,
                "zone_id": "iva",
                "shard_name": "shard1"
            },
            {
                "cluster_id": "cid1",
                "health": "HEALTH_UNKNOWN",
                "name": "man-1.db.yandex.net",
                "replica_priority": "100",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "ROLE_UNKNOWN",
                "services": [],
                "subnet_id": "",
                "system": null,
                "zone_id": "man",
                "shard_name": "shard1"
            },
            {
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "replica_priority": "50",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnet_id": "",
                "system": null,
                "zone_id": "myt",
                "shard_name": "shard1"
            },
            {
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "sas-1.db.yandex.net",
                "replica_priority": "100",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnet_id": "",
                "system": null,
                "zone_id": "sas",
                "shard_name": "shard1"
            }
        ]
    }
    """

  Scenario Outline: Adding host with invalid params fails
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchCreate" with data
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
      | hosts                                  | status | error code | message                                                                                                               |
      | []                                     | 422    | 3          | No hosts to add are specified                                                                                         |
      | [{"zoneId": "man"}, {"zoneId": "man"}] | 501    | 12         | Adding multiple hosts at once is not supported yet                                                                    |
      | [{"zoneId": "nodc"}]                   | 422    | 3          | The request is invalid.\nhostSpecs.0.zoneId: Invalid value, valid value is one of ['iva', 'man', 'myt', 'sas', 'vla'] |

  @events @grpc
  Scenario: Deleting host works
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete hosts from Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.DeleteClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-1.db.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.DeleteClusterHosts" event with
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
    When we GET "/mdb/redis/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "replicaPriority": 100,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "MASTER",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnetId": "",
                "zoneId": "iva",
                "shardName": "shard1"
            },
            {
                "clusterId": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "replicaPriority": 50,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnetId": "",
                "zoneId": "myt",
                "shardName": "shard1"
            }
        ]
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 2
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [
            {
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "iva-1.db.yandex.net",
                "replica_priority": "100",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "MASTER",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnet_id": "",
                "system": null,
                "zone_id": "iva",
                "shard_name": "shard1"
            },
            {
                "cluster_id": "cid1",
                "health": "ALIVE",
                "name": "myt-1.db.yandex.net",
                "replica_priority": "50",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                },
                "role": "REPLICA",
                "services": [
                    {
                        "health": "ALIVE",
                        "type": "REDIS"
                    },
                    {
                        "health": "ALIVE",
                        "type": "ARBITER"
                    }
                ],
                "subnet_id": "",
                "system": null,
                "zone_id": "myt",
                "shard_name": "shard1"
            }
        ]
    }
    """

  Scenario Outline: Deleting host with invalid params fails
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
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
      | host list                                      | status | error code | message                                              |
      | []                                             | 422    | 3          | No hosts to delete are specified                     |
      | ["sas-1.db.yandex.net", "myt-1.db.yandex.net"] | 501    | 12         | Deleting multiple hosts at once is not supported yet |
      | ["nohost"]                                     | 404    | 5          | Host 'nohost' does not exist                         |

  Scenario: Deleting last host fails
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.db.yandex.net"]
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["iva-1.db.yandex.net"]
    }
    """
    And "worker_task_id3" acquired and finished by worker
    And we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["myt-1.db.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Redis shard with resource preset 's1.porto.1' and disk type 'local-ssd' requires at least 1 host"
    }
    """

  @events
  Scenario: Label set works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "labels": {
            "acid": "yes"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify Redis cluster",
        "done": true,
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "labels": {
            "acid": "yes"
        }
    }
    """

  @events
  Scenario: Description set works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "description": "my cool description"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify Redis cluster",
        "done": true,
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "my cool description"
    }
    """

  Scenario: Change disk size to invalid value fails
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 1
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Disk size must be at least two times larger than memory size (8589934592 for the current resource preset)"
    }
    """

  Scenario: Change disk size to same value fails
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 17179869184
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

  @events
  Scenario: Change disk size to valid value works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 21474836480
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
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 3.0,
        "memoryUsed": 12884901888,
        "ssdSpaceUsed": 64424509440
    }
    """

  Scenario: Password change with other parameters fails
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_5_0": {
                "databases": 10,
                "password": "my_secret_password"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Password change cannot be mixed with other modifications"
    }
    """

  Scenario: Password change to short value fails
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_5_0": {
                "password": "short"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The request is invalid.\nconfigSpec.redisConfig_5_0.password: Password must be between 8 and 128 characters long"
    }
    """

  @events
  Scenario: Password change works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_5_0": {
                "password": "my_secret_password"
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

  @events
  Scenario: Backup window start option change works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "backupWindowStart": {
                "hours": 23,
                "minutes": 10,
                "seconds": 0,
                "nanos": 0
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "5.0",
            "access": {
                "webSql": false,
                "dataLens": false,
                "dataTransfer": false,
                "serverless": false
            },
            "backupWindowStart": {
                "hours": 23,
                "minutes": 10,
                "seconds": 0,
                "nanos": 0
            },
            "redisConfig_5_0": {
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
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    }
                },
                "userConfig": {
                    "clientOutputBufferLimitNormal": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "databases": 15
                }
            },
            "resources": {
                "diskSize": 17179869184,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  @events
  Scenario: Additional configuration change works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_5_0": {
                "maxmemoryPolicy": "VOLATILE_LRU",
                "timeout": 10,
                "slowlogLogSlowerThan": 30000,
                "slowlogMaxLen": 4000,
                "notifyKeyspaceEvents": "Elg",
                "databases": 10
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "5.0",
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
            "redisConfig_5_0": {
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
                    "maxmemoryPolicy": "VOLATILE_LRU",
                    "timeout": 10,
                    "databases": 10,
                    "notifyKeyspaceEvents": "",
                    "slowlogLogSlowerThan": 30000,
                    "slowlogMaxLen": 4000,
                    "clientOutputBufferLimitNormal": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "notifyKeyspaceEvents": "Elg"
                },
                "userConfig": {
                    "maxmemoryPolicy": "VOLATILE_LRU",
                    "timeout": 10,
                    "slowlogLogSlowerThan": 30000,
                    "slowlogMaxLen": 4000,
                    "notifyKeyspaceEvents": "Elg",
                    "databases": 10,
                    "clientOutputBufferLimitNormal": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    }
                }
            },
            "resources": {
                "diskSize": 17179869184,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  Scenario: Eviction configuration change to same value fails
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_5_0": {
                "maxmemoryPolicy": "VOLATILE_LRU",
                "timeout": 10
            }
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_5_0": {
                "timeout": 10
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

  @restart
  Scenario: Databases configuration change to same value fails
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_5_0": {
                "databases": 15
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

  @events
  Scenario: Scaling cluster up works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.porto.2"
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
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 6.0,
        "memoryUsed": 25769803776,
        "ssdSpaceUsed": 51539607552
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "5.0",
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
            "redisConfig_5_0": {
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
                        "hardLimit": 33554432,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 16777216,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 33554432,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 16777216,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    }
                },
                "userConfig": {
                    "clientOutputBufferLimitNormal": {
                        "hardLimit": 33554432,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 16777216,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 33554432,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 16777216,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "databases": 15
                }
            },
            "resources": {
                "diskSize": 17179869184,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.2"
            }
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
                    "cpu_guarantee": 2.0,
                    "cpu_limit": 2.0,
                    "cpu_fraction": 100,
                    "gpu_limit": 0,
                    "description": "s1.porto.2",
                    "id": "00000000-0000-0000-0000-000000000002",
                    "io_limit": 41943040,
                    "io_cores_limit": 0,
                    "memory_guarantee": 8589934592,
                    "memory_limit": 8589934592,
                    "name": "s1.porto.2",
                    "network_guarantee": 33554432,
                    "network_limit": 33554432,
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
                    "maxmemory": 6442450944,
                    "maxmemory-policy": "noeviction",
                    "repl-backlog-size": 858993459,
                    "replica-priority": 100,
                    "client-output-buffer-limit normal": "33554432 16777216 60",
                    "client-output-buffer-limit replica": "858993459 429496729 60",
                    "client-output-buffer-limit pubsub": "33554432 16777216 60",
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
                    "sentinel_renames": {
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
                    "enabled": false
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
                    "major_version": "5.0",
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

  @events
  Scenario: Allow webSql access
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "access": {
                "webSql": true
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "5.0",
            "access": {
                "webSql": true,
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
            "redisConfig_5_0": {
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
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    }
                },
                "userConfig": {
                    "clientOutputBufferLimitNormal": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "databases": 15
                }
            },
            "resources": {
                "diskSize": 17179869184,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
    When we GET "/api/v1.0/config/iva-1.db.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
            "access": {
                "web_sql": true
            },
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
                    "client-output-buffer-limit normal": "16777216 8388608 60",
                    "client-output-buffer-limit replica": "429496729 214748364 60",
                    "client-output-buffer-limit pubsub": "16777216 8388608 60",
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
                    "sentinel_renames": {
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
                    "enabled": false
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
                    "major_version": "5.0",
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

  Scenario: Scaling cluster down with noeviction works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.porto.2"
            }
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
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


  Scenario: Scaling cluster down works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_5_0": {
                "maxmemoryPolicy": "ALLKEYS_RANDOM"
            },
            "resources": {
                "resourcePresetId": "s1.porto.2"
            }
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
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
        "createdBy": "user",
        "description": "Modify Redis cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
          "@type": "yandex.cloud.mdb.redis.v1.UpdateClusterMetadata",
          "clusterId": "cid1"
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "5.0",
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
            "redisConfig_5_0": {
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
                    "maxmemoryPolicy": "ALLKEYS_RANDOM",
                    "timeout": 0,
                    "databases": 15,
                    "notifyKeyspaceEvents": "",
                    "slowlogLogSlowerThan": 10000,
                    "slowlogMaxLen": 1000,
                    "clientOutputBufferLimitNormal": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    }
                },
                "userConfig": {
                    "clientOutputBufferLimitNormal": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "clientOutputBufferLimitPubsub": {
                        "hardLimit": 16777216,
                        "hardLimitUnit": "BYTES",
                        "softLimit": 8388608,
                        "softLimitUnit": "BYTES",
                        "softSeconds": 60
                    },
                    "databases": 15,
                    "maxmemoryPolicy": "ALLKEYS_RANDOM"
                }
            },
            "resources": {
                "diskSize": 17179869184,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  @events
  Scenario: Cluster name change works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "name": "changed"
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "name": "changed"
    }
    """

  Scenario: Cluster name change to same value fails
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
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

  Scenario: Cluster name change with duplicate value fails
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "another test cluster"
    }
    """
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
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

  @grpc
  Scenario: Cluster move between different clouds fails [gRPC]
    Given we disallow move cluster between clouds
    When we "Move" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "destination_folder_id": "folder2"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "moving cluster between folders in different clouds is unavailable"

  @events @grpc
  Scenario: Cluster move works
    When we "POST" via REST at "/mdb/redis/1.0/clusters/cid1:move" with data
    """
    {
        "destinationFolderId": "folder2"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Move Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.MoveClusterMetadata",
            "clusterId": "cid1",
            "destinationFolderId": "folder2",
            "sourceFolderId": "folder1"
        }
    }
    """
    Given we allow move cluster between clouds
    When we "Move" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
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
        "description": "Move Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.redis.v1.MoveClusterMetadata",
            "cluster_id": "cid1",
            "destination_folder_id": "folder2",
            "source_folder_id": "folder1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.MoveCluster" event with
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
        "description": "Move Redis cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.MoveClusterMetadata",
            "clusterId": "cid1",
            "destinationFolderId": "folder2",
            "sourceFolderId": "folder1"
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder2"
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
        "cpuUsed": 3.0,
        "memoryUsed": 12884901888,
        "ssdSpaceUsed": 51539607552
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.MoveCluster" event with
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
        "description": "Move Redis cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.MoveClusterMetadata",
            "clusterId": "cid1",
            "destinationFolderId": "folder2",
            "sourceFolderId": "folder1"
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder2"
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
        "cpuUsed": 3.0,
        "memoryUsed": 12884901888,
        "ssdSpaceUsed": 51539607552
    }
    """

  @delete @events
  Scenario: Cluster removal works
    When we DELETE "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.DeleteClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.DeleteCluster" event with
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
    When we GET "/mdb/redis/1.0/clusters?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "clusters": []
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 403

  @delete
  Scenario: After cluster delete cluster.name can be reused
    When we DELETE "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200

  @delete
  Scenario: Cluster with running operations can not be deleted
    When we run query
    """
    UPDATE dbaas.worker_queue
       SET result = NULL,
           end_ts = NULL
    """
    And we DELETE "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Cluster 'cid1' has active tasks"
    }
    """

  @delete
  Scenario: Cluster with failed operations can be deleted
    When we run query
    """
    UPDATE dbaas.worker_queue
       SET result = false
    """
    And we DELETE "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200

  @delete @operations
  Scenario: After cluster delete cluster operations are shown
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "description": "changed"
    }
    """
    Then we get response with status 200
    When we DELETE "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200
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
    When we GET "/mdb/redis/1.0/operations?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "operations": [
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Delete Redis cluster",
                "done": false,
                "id": "worker_task_id3",
                "metadata": {
                    "@type": "yandex.cloud.mdb.redis.v1.DeleteClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00"
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Modify Redis cluster",
                "done": true,
                "id": "worker_task_id2",
                "metadata": {
                    "@type": "yandex.cloud.mdb.redis.v1.UpdateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00",
                "response": {}
            },
            {
                "createdAt": "2000-01-01T00:00:00+00:00",
                "createdBy": "user",
                "description": "Create Redis cluster",
                "done": true,
                "id": "worker_task_id1",
                "metadata": {
                    "@type": "yandex.cloud.mdb.redis.v1.CreateClusterMetadata",
                    "clusterId": "cid1"
                },
                "modifiedAt": "2000-01-01T00:00:00+00:00",
                "response": {}
            }
        ]
    }
    """

  @decommission @geo
  Scenario: Create cluster host in decommissioning geo
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "ugr"
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

  @stop
  Scenario: Stop cluster not implemented
    When we POST "/mdb/redis/1.0/clusters/cid1:stop"
    Then we get response with status 501 and body contains
    """
    {
        "code": 12,
        "message": "Stop for redis_cluster not implemented in that installation"
    }
    """

  @status
  Scenario: Cluster status
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "man"
        }]
    }
    """
    Then we get response with status 200
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "UPDATING"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """
