Feature: Create Compute Redis Sharded 6.2 Oversized Cluster

  Background:
    Given feature flags
    """
    ["MDB_REDIS_62"]
    """
    And default headers
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
        }, {
            "zoneId": "myt",
            "shardName": "shard4"
        }, {
            "zoneId": "vla",
            "shardName": "shard5"
        }, {
            "zoneId": "sas",
            "shardName": "shard6"
        }, {
            "zoneId": "myt",
            "shardName": "shard7"
        }, {
            "zoneId": "vla",
            "shardName": "shard8"
        }, {
            "zoneId": "sas",
            "shardName": "shard9"
        }, {
            "zoneId": "myt",
            "shardName": "shard10"
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
        "cpuUsed": 10.0,
        "memoryUsed": 42949672960,
        "ssdSpaceUsed": 171798691840
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we GET "/mdb/redis/1.0/clusters/cid1"
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

 Scenario: Adding new shard without patched pillar fails
    When we POST "/mdb/redis/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard11",
        "hostSpecs": [
            {
                "type": "REDIS",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Number of shards must be between 3 and 10."
    }
    """

  Scenario: Adding new shard with patched pillar works
    When we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,redis,max_shards_count}', '11')
        WHERE cid = 'cid1'
    """
    And we POST "/mdb/redis/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard11",
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
            "shardName": "shard11"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.AddClusterShard" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "shard_name": "shard11"
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
        "name": "shard11"
    }
    """
    When we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 11.0,
        "memoryUsed": 47244640256,
        "ssdSpaceUsed": 188978561024
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
                "name": "shard10"
            },
            {
                "clusterId": "cid1",
                "name": "shard11"
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
            },
                        {
                "clusterId": "cid1",
                "name": "shard5"
            },
            {
                "clusterId": "cid1",
                "name": "shard6"
            },
            {
                "clusterId": "cid1",
                "name": "shard7"
            },
            {
                "clusterId": "cid1",
                "name": "shard8"
            },
                        {
                "clusterId": "cid1",
                "name": "shard9"
            }
        ]
    }
    """

  Scenario: Adding new shard with patched pillar over limit fails
    When we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,redis,max_shards_count}', '11')
        WHERE cid = 'cid1'
    """
    And we POST "/mdb/redis/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard11",
        "hostSpecs": [
            {
                "type": "REDIS",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 200
    And we POST "/mdb/redis/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard12",
        "hostSpecs": [
            {
                "type": "REDIS",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Number of shards must be between 3 and 11."
    }
    """

  Scenario: Adding and deleting new host with patched pillar over limit works
    When we run query
    """
    UPDATE dbaas.pillar
        SET value = jsonb_set(value, '{data,redis,max_shards_count}', '11')
        WHERE cid = 'cid1'
    """
    And we POST "/mdb/redis/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard11",
        "hostSpecs": [
            {
                "type": "REDIS",
                "zoneId": "sas"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "myt",
            "shardName": "shard11"
        }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add hosts to Redis cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.AddClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "myt-5.df.cloud.yandex.net"
            ]
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/v1/quota/cloud1"
    Then we get response with status 200 and body contains
    """
    {
        "cloudId": "cloud1",
        "clustersUsed": 1,
        "cpuUsed": 12.0,
        "memoryUsed": 51539607552,
        "ssdSpaceUsed": 206158430208
    }
    """
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-4.df.cloud.yandex.net"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete hosts from Redis cluster",
        "done": false,
        "id": "worker_task_id4",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.DeleteClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "sas-4.df.cloud.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id4" exists "yandex.cloud.events.mdb.redis.DeleteClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "sas-4.df.cloud.yandex.net"
            ]
        }
    }
    """
    When "worker_task_id4" acquired and finished by worker
    And we GET "/mdb/redis/1.0/clusters/cid1/hosts"
    Then we get response with status 200
