Feature: Create Compute Redis 6.2 Sharded Cluster Local Ssd

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
                "diskTypeId": "local-ssd",
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
        },
        {
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
        "cpuUsed": 6.0,
        "memoryUsed": 25769803776,
        "ssdSpaceUsed": 103079215104
    }
    """
    When "worker_task_id1" acquired and finished by worker
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
                "diskTypeId": "local-ssd",
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

  @grpc
  Scenario: Adding host to shard works
    When we "POST" via REST at "/mdb/redis/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "myt",
            "shardName": "shard1",
            "assignPublicIp": true,
            "subnetId": "network1-myt"
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
                "myt-3.df.cloud.yandex.net"
            ]
        }
    }
    """
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "zone_id": "myt",
            "shard_name": "shard1",
            "assign_public_ip": true,
            "subnet_id": "network1-myt"
        }]
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Add hosts to Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.redis.v1.AddClusterHostsMetadata",
            "cluster_id": "cid1",
            "host_names": [
                "myt-3.df.cloud.yandex.net"
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
        "cpuUsed": 7.0,
        "memoryUsed": 30064771072,
        "ssdSpaceUsed": 120259084288
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.AddClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "myt-3.df.cloud.yandex.net"
            ]
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/redis/1.0/clusters/cid1/hosts"
    Then we get response with status 200 and body contains
    """
    {
        "hosts": [
            {
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "myt-1.df.cloud.yandex.net",
                "replicaPriority": null,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.compute.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "subnetId": "network1-myt",
                "zoneId": "myt",
                "shardName": "shard1"
            },
            {
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "myt-2.df.cloud.yandex.net",
                "replicaPriority": null,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.compute.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "subnetId": "network1-myt",
                "zoneId": "myt",
                "shardName": "shard1"
            },
            {
                "assignPublicIp": true,
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "myt-3.df.cloud.yandex.net",
                "replicaPriority": null,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.compute.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "shardName": "shard1",
                "subnetId": "network1-myt",
                "zoneId": "myt"
            },
            {
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "sas-1.df.cloud.yandex.net",
                "replicaPriority": null,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.compute.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "subnetId": "network1-sas",
                "zoneId": "sas",
                "shardName": "shard3"
            },
            {
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "sas-2.df.cloud.yandex.net",
                "replicaPriority": null,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.compute.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "subnetId": "network1-sas",
                "zoneId": "sas",
                "shardName": "shard3"
            },
            {
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-1.df.cloud.yandex.net",
                "replicaPriority": null,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.compute.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "subnetId": "network1-vla",
                "zoneId": "vla",
                "shardName": "shard2"
            },
            {
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-2.df.cloud.yandex.net",
                "replicaPriority": null,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "local-ssd",
                    "resourcePresetId": "s1.compute.1"
                },
                "role": "UNKNOWN",
                "services": [],
                "subnetId": "network1-vla",
                "zoneId": "vla",
                "shardName": "shard2"
            }
        ]
    }
    """
    When we "ListHosts" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 7
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [
            {
                "cluster_id": "cid1",
                "health": "HEALTH_UNKNOWN",
                "name": "myt-1.df.cloud.yandex.net",
                "replica_priority": null,
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "role": "ROLE_UNKNOWN",
                "services": [],
                "subnet_id": "network1-myt",
                "system": null,
                "zone_id": "myt",
                "shard_name": "shard1"
            },
            {
                "cluster_id": "cid1",
                "health": "HEALTH_UNKNOWN",
                "name": "myt-2.df.cloud.yandex.net",
                "replica_priority": null,
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "role": "ROLE_UNKNOWN",
                "services": [],
                "subnet_id": "network1-myt",
                "system": null,
                "zone_id": "myt",
                "shard_name": "shard1"
            },
            {
                "assign_public_ip": true,
                "cluster_id": "cid1",
                "health": "HEALTH_UNKNOWN",
                "name": "myt-3.df.cloud.yandex.net",
                "replica_priority": null,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "role": "ROLE_UNKNOWN",
                "services": [],
                "shard_name": "shard1",
                "subnet_id": "network1-myt",
                "system": null,
                "zone_id": "myt"
            },
            {
                "cluster_id": "cid1",
                "health": "HEALTH_UNKNOWN",
                "name": "sas-1.df.cloud.yandex.net",
                "replica_priority": null,
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "role": "ROLE_UNKNOWN",
                "services": [],
                "subnet_id": "network1-sas",
                "system": null,
                "zone_id": "sas",
                "shard_name": "shard3"
            },
            {
                "cluster_id": "cid1",
                "health": "HEALTH_UNKNOWN",
                "name": "sas-2.df.cloud.yandex.net",
                "replica_priority": null,
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "role": "ROLE_UNKNOWN",
                "services": [],
                "subnet_id": "network1-sas",
                "system": null,
                "zone_id": "sas",
                "shard_name": "shard3"
            },
            {
                "cluster_id": "cid1",
                "health": "HEALTH_UNKNOWN",
                "name": "vla-1.df.cloud.yandex.net",
                "replica_priority": null,
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "role": "ROLE_UNKNOWN",
                "services": [],
                "subnet_id": "network1-vla",
                "system": null,
                "zone_id": "vla",
                "shard_name": "shard2"
            },
            {
                "cluster_id": "cid1",
                "health": "HEALTH_UNKNOWN",
                "name": "vla-2.df.cloud.yandex.net",
                "replica_priority": null,
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.compute.1"
                },
                "role": "ROLE_UNKNOWN",
                "services": [],
                "subnet_id": "network1-vla",
                "system": null,
                "zone_id": "vla",
                "shard_name": "shard2"
            }
        ]
    }
    """

  Scenario: Cluster update resources without feature flag fails
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.compute.2",
                "diskTypeId": "local-ssd",
                "diskSize": 171798691840
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

  Scenario: Cluster update resources with feature flag over quota limit fails
    Given feature flags
    """
    ["MDB_LOCAL_DISK_RESIZE"]
    """
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.compute.2",
                "diskTypeId": "local-ssd",
                "diskSize": 171798691840
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "details": [
            {
                "@type": "type.private-api.yandex-cloud.ru/quota.QuotaFailure",
                "cloud_id": "cloud1",
                "violations": [
                    {
                        "metric": {
                            "limit": 644245094400,
                            "name": "mdb.ssd.size",
                            "usage": 412316860416
                        },
                        "required": 721554505728
                    }
                ]
            }
        ],
        "message": "Quota limits exceeded, not enough ssd_space: 72 GiB"
    }
    """

  Scenario: Cluster localssd update resources with feature flag works
    Given feature flags
    """
    ["MDB_LOCAL_DISK_RESIZE"]
    """
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.compute.2",
                "diskTypeId": "local-ssd",
                "diskSize": 34359738368
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
        "cpuUsed": 12.0,
        "memoryUsed": 51539607552,
        "ssdSpaceUsed": 206158430208
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
