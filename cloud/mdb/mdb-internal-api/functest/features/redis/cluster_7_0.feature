Feature: Create/Modify Compute Redis 7.0 Cluster

  Background:
    Given feature flags
    """
    ["MDB_REDIS_70"]
    """
    And default headers
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_7_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "vla"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "network1"
    }
    """
    And "worker_task_id1" acquired and finished by worker

  @events
  Scenario: Cluster creation works
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "config": {
            "version": "7.0",
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
            "redisConfig_7_0": {
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
        "sharded": false
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

  Scenario: Billing cost estimation works
    When we POST "/mdb/redis/1.0/console/clusters:estimate?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_7_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskSize": 107374182400
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "vla"
        }, {
            "zoneId": "sas"
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
                    "cluster_type": "redis_cluster",
                    "disk_size": 107374182400,
                    "disk_type_id": "network-ssd",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "redis_cluster"
                    ],
                    "software_accelerated_network_cores": 0
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "redis_cluster",
                    "disk_size": 107374182400,
                    "disk_type_id": "network-ssd",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "redis_cluster"
                    ],
                    "software_accelerated_network_cores": 0
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "redis_cluster",
                    "disk_size": 107374182400,
                    "disk_type_id": "network-ssd",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "redis_cluster"
                    ],
                    "software_accelerated_network_cores": 0
                }
            }
        ]
    }
    """

  Scenario: Console create host estimate costs
    When we POST "/mdb/redis/1.0/console/hosts:estimate" with data
    """
    {
        "folderId": "folder1",
        "billingHostSpecs": [
            {
                "host": {
                    "zoneId": "vla",
                    "subnetId": "subnet-id",
                    "assignPublicIp": false
                },
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 1
                }
            },
            {
                "host": {
                    "zoneId": "iva",
                    "subnetId": "subnet-id",
                    "assignPublicIp": false
                },
                "resources": {
                    "resourcePresetId": "s1.compute.1",
                    "diskTypeId": "network-ssd",
                    "diskSize": 1
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
                    "cluster_type": "redis_cluster",
                    "disk_size": 1,
                    "disk_type_id": "network-ssd",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "redis_cluster"
                    ],
                    "software_accelerated_network_cores": 0
                }
            },
            {
                "folder_id": "folder1",
                "schema": "mdb.db.generic.v1",
                "tags": {
                    "cluster_type": "redis_cluster",
                    "disk_size": 1,
                    "disk_type_id": "network-ssd",
                    "online": 1,
                    "public_ip": 0,
                    "resource_preset_id": "s1.compute.1",
                    "platform_id": "mdb-v1",
                    "cores": 1,
                    "core_fraction": 100,
                    "memory": 4294967296,
                    "on_dedicated_host": 0,
                    "roles": [
                        "redis_cluster"
                    ],
                    "software_accelerated_network_cores": 0
                }
            }
        ]
    }
    """

  @stop @events @grpc
  Scenario: Stop cluster works
    When we "POST" via REST at "/mdb/redis/1.0/clusters/cid1:stop"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Stop Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.StopClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Stop" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Stop Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.redis.v1.StopClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.StopCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we GET "/mdb/1.0/operations/worker_task_id2"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Stop Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.StopClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STOPPING"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "STOPPED"
    }
    """

  @start @events @grpc
  Scenario: Start stopped cluster works
    When we POST "/mdb/redis/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we "POST" via REST at "/mdb/redis/1.0/clusters/cid1:start"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start Redis cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.StartClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Start" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Start Redis cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.redis.v1.StartClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.redis.StartCluster" event with
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
        "status": "STARTING"
    }
    """
    When we GET "/mdb/1.0/operations/worker_task_id3"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Start Redis cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.StartClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "status": "RUNNING"
    }
    """

  @events
  Scenario: Additional configuration change works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "redisConfig_7_0": {
                "maxmemoryPolicy": "VOLATILE_LRU",
                "timeout": 10,
                "slowlogLogSlowerThan": 30000,
                "slowlogMaxLen": 4000,
                "notifyKeyspaceEvents": "nd",
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
            "version": "7.0",
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
            "redisConfig_7_0": {
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
                    "notifyKeyspaceEvents": "nd"
                },
                "userConfig": {
                    "maxmemoryPolicy": "VOLATILE_LRU",
                    "timeout": 10,
                    "slowlogLogSlowerThan": 30000,
                    "slowlogMaxLen": 4000,
                    "notifyKeyspaceEvents": "nd",
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
                "diskTypeId": "network-ssd",
                "resourcePresetId": "s1.compute.1"
            }
        }
    }
    """

