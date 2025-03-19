Feature: Create Porto Redis 6.2 Sharded Cluster

  Background:
    Given default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_2": {
                "password": "p@ssw#$rd!?"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt",
            "shardName": "shard1"
        }, {
            "zoneId": "iva",
            "shardName": "shard2"
        }, {
            "zoneId": "sas",
            "shardName": "shard3"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API",
        "sharded": true
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": true,
        "tlsEnabled": false,
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
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "health": "UNKNOWN",
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
        "status": "CREATING"
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
                                        "myt-1.db.yandex.net": {"geo": "myt"}
                                    },
                                    "name": "shard1"
                                },
                                "shard_id2": {
                                    "hosts": {
                                        "iva-1.db.yandex.net": {"geo": "iva"}
                                    },
                                    "name": "shard2"
                                },
                                "shard_id3": {
                                    "hosts": {
                                        "sas-1.db.yandex.net": {"geo": "sas"}
                                    },
                                    "name": "shard3"
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
                    "iva-1.db.yandex.net"
                ],
                "shard_id": "shard_id2",
                "shard_name": "shard2",
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
                    "cluster-enabled": "yes",
                    "databases": 16,
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
                        },
                        "ACL": {
                            "data": "dummy",
                            "encryption_version": 0
                        }
                    },
                    "cluster_renames": {
                        "ADDSLOTS": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "DELSLOTS": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "FAILOVER": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "FORGET": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "MEET": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "REPLICATE": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "RESET": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "SAVECONFIG": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "SET-CONFIG-EPOCH": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "SETSLOT": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "FLUSHSLOTS": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
                        "BUMPEPOCH": {
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
                       "name": "redis_cluster",
                       "role": "Master",
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
                       "name": "redis_cluster",
                       "role": "Master",
                       "status": "Alive"
                   }
               ]
           },
           {
               "fqdn": "man-1.db.yandex.net",
               "cid": "cid1",
               "status": "Alive",
               "services": [
                   {
                       "name": "redis_cluster",
                       "role": "Replica",
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
                       "name": "redis_cluster",
                       "role": "Master",
                       "status": "Alive"
                   }
               ]
           }
       ]
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "health": "ALIVE",
        "status": "RUNNING",
        "sharded": true
    }
    """

  @events
  Scenario: Adding new shard works
    When we POST "/mdb/redis/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard4",
        "hostSpecs": [
            {
                "type": "REDIS",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add shard to Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.AddClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard4"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.AddClusterShard" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "shard_name": "shard4"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/redis/1.0/operations/worker_task_id2"
    Then we get response with status 200
    And body at path "$.response" contains
    """
    {
        "@type": "yandex.cloud.mdb.redis.v1.Shard",
        "clusterId": "cid1",
        "name": "shard4"
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 4.0,
        "memoryUsed": 17179869184,
        "ssdSpaceUsed": 68719476736
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1/shards"
    Then we get response with status 200 and body contains
    """
    {
        "shards": [
            {
                "clusterId": "cid1",
                "name": "shard1"
            },
            {
                "clusterId": "cid1",
                "name": "shard2"
            },
            {
                "clusterId": "cid1",
                "name": "shard3"
            },
            {
                "clusterId": "cid1",
                "name": "shard4"
            }
        ]
    }
    """

  @events
  Scenario: Deleting shard works
    When we POST "/mdb/redis/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard4",
        "hostSpecs": [
            {
                "type": "REDIS",
                "zoneId": "sas"
            }
        ]
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we DELETE "/mdb/redis/1.0/clusters/cid1/shards/shard1"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete shard from Redis cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.DeleteClusterShardMetadata",
            "clusterId": "cid1",
            "shardName": "shard1"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.redis.DeleteClusterShard" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "shard_name": "shard1"
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1/shards"
    Then we get response with status 200 and body contains
    """
    {
        "shards": [
            {
                "clusterId": "cid1",
                "name": "shard2"
            },
            {
                "clusterId": "cid1",
                "name": "shard3"
            },
            {
                "clusterId": "cid1",
                "name": "shard4"
            }
        ]
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

  @events @grpc
  Scenario: Cluster update resources work with rebalance
    When we "POST" via REST at "/mdb/redis/1.0/clusters/cid1:rebalance"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Rebalance slot distribution in Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.RebalanceClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Rebalance" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Rebalance slot distribution in Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.redis.v1.RebalanceClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.RebalanceCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """

    @grpc
    Scenario Outline: Manual failover on sharded cluster works
      When we "POST" via REST at "/mdb/redis/1.0/clusters/cid1:startFailover" with data
      """
      {
          "hostNames": ["iva-1.db.yandex.net"]
      }
      """
      Then we get response with status 200 and body contains
      """
      {
          "createdBy": "user",
          "description": "<description>",
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
          "host_names": ["iva-1.db.yandex.net"]
      }
      """
      Then we get gRPC response with body
      """
      {
          "created_by": "user",
          "description": "<description>",
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
      Examples:
        | description                            |
        | Start manual failover on Redis cluster |

    @grpc
    Scenario Outline: Failover on sharded cluster fails
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
        | hostnames                                      | message                                                                           |
        | []                                             | To perform failover on sharded redis cluster at least one host must be specified. |
        | ["iva-1.db.yandex.net", "myt-1.db.yandex.net"] | Failover on redis cluster does not support multiple hostnames at this time.       |
        | ["nonexistent.db.yandex.net"]                  | Hostnames you specified are not the part of cluster.                              |
