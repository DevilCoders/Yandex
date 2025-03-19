Feature: Create Compute Redis Sharded Cluster

  Background:
    Given feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_5"]
    """
    And default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "p@ssw#$rd!?"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt",
            "shardName": "shard1"
        }, {
            "zoneId": "vla",
            "shardName": "shard2"
        }, {
            "zoneId": "sas",
            "shardName": "shard3"
        }],
        "description": "test cluster",
        "networkId": "network1",
        "sharded": true
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": true,
        "tlsEnabled": false,
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
                "diskTypeId": "network-ssd",
                "resourcePresetId": "s1.compute.1"
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
        "networkId": "network1",
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
    When "worker_task_id1" acquired and finished by worker
    And we GET "/mdb/redis/1.0/clusters/cid1"
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
                "diskTypeId": "network-ssd",
                "resourcePresetId": "s1.compute.1"
            }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folderId": "folder1",
        "id": "cid1",
        "labels": {},
        "name": "test",
        "networkId": "network1",
        "status": "RUNNING",
        "sharded": true
    }
    """

  Scenario: Modifying host in shard fails
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchUpdate" with data
    """
    {
        "updateHostSpecs": [{
            "hostName": "sas-1.df.cloud.yandex.net",
            "replicaPriority": 100
        }]
    }
    """
    Then we get response with status 501 and body contains
    """
    {
        "code": 12,
        "message": "Modifying replica priority in hosts of sharded clusters is not supported"
    }
    """

  Scenario: Adding host to shard works
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "myt",
            "shardName": "shard1"
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
                "myt-2.df.cloud.yandex.net"
            ]
        }
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

  Scenario: Deleting last host from shard fails
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.df.cloud.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Redis shard with resource preset 's1.compute.1' and disk type 'network-ssd' requires at least 1 host"
    }
    """

  Scenario: Getting shard works
    When we GET "/mdb/redis/1.0/clusters/cid1/shards/shard1"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "shard1"
    }
    """

  Scenario: Adding new shard with already used name fails
    When we POST "/mdb/redis/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard1",
        "hostSpecs": [
            {
                "type": "REDIS",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Shard 'shard1' already exists"
    }
    """

  Scenario: Adding new shard with a name that differs only by case from the existing fails
    When we POST "/mdb/redis/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "Shard1",
        "hostSpecs": [
            {
                "type": "REDIS",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 409 and body contains
    """
    {
        "code": 6,
        "message": "Cannot create shard 'Shard1': shard with name 'shard1' already exists"
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

  Scenario: Deleting shard from cluster with 3 shards fails
    When we DELETE "/mdb/redis/1.0/clusters/cid1/shards/shard1"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "There must be at least 3 shards in a cluster"
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

  Scenario: Cluster update resources work
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.compute.2",
                "diskSize": 171798691840
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
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cpuUsed": 6.0,
        "memoryUsed": 25769803776,
        "ssdSpaceUsed": 515396075520
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "sharded": true,
        "config": {
            "access": {
                "dataLens": false,
                "webSql": false,
                "dataTransfer": false,
                "serverless": false
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "nanos": 100,
                "seconds": 30
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
                    }
                }
            },
            "resources": {
                "diskSize": 171798691840,
                "diskTypeId": "network-ssd",
                "resourcePresetId": "s1.compute.2"
            },
            "version": "5.0"
        }
    }
    """

  @events
  Scenario: Cluster update resources work with rebalance
    When we POST "/mdb/redis/1.0/clusters/cid1:rebalance"
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
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.RebalanceCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
