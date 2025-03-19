Feature: Create/Modify Compute Redis Cluster Local Ssd

  Background:
    Given feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_5"]
    """
    And default headers
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "local-ssd",
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
        "networkId": "network1",
        "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """
    And "worker_task_id1" acquired and finished by worker
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" containing:
      |sg_id1|
      |sg_id2|

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

  @grpc
  Scenario: Adding and deleting host works
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "myt"
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
    And "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["myt-2.df.cloud.yandex.net"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete hosts from Redis cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.DeleteClusterHostsMetadata",
            "clusterId": "cid1",
            "hostNames": [
                "myt-2.df.cloud.yandex.net"
            ]
        }
    }
    """
    And for "worker_task_id3" exists "yandex.cloud.events.mdb.redis.DeleteClusterHosts" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "host_names": [
                "myt-2.df.cloud.yandex.net"
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
                "health": "UNKNOWN",
                "name": "myt-1.df.cloud.yandex.net",
                "replicaPriority": 100,
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
                "name": "sas-1.df.cloud.yandex.net",
                "replicaPriority": 100,
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
                "shardName": "shard1"
            },
            {
                "clusterId": "cid1",
                "health": "UNKNOWN",
                "name": "vla-1.df.cloud.yandex.net",
                "replicaPriority": 100,
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
                "health": "HEALTH_UNKNOWN",
                "name": "myt-1.df.cloud.yandex.net",
                "replica_priority": "100",
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
                "name": "sas-1.df.cloud.yandex.net",
                "replica_priority": "100",
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
                "shard_name": "shard1"
            },
            {
                "cluster_id": "cid1",
                "health": "HEALTH_UNKNOWN",
                "name": "vla-1.df.cloud.yandex.net",
                "replica_priority": "100",
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
                "shard_name": "shard1"
            }
        ]
    }
    """
    And "worker_task_id3" acquired and finished by worker

  Scenario: Deleting host fails in 3-node cluster
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["myt-1.df.cloud.yandex.net"]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Redis shard with resource preset 's1.compute.1' and disk type 'local-ssd' requires at least 3 hosts"
    }
    """

  Scenario: Billing cost estimation works
    When we POST "/mdb/redis/1.0/console/clusters:estimate?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "local-ssd",
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
                    "disk_type_id": "local-ssd",
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
                    "disk_type_id": "local-ssd",
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
                    "disk_type_id": "local-ssd",
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
                    "diskTypeId": "local-ssd",
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
                    "disk_type_id": "local-ssd",
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

  Scenario: Resources modify on local-ssd disk without feature flag fails
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.compute.2"
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

  Scenario: Disk scale up on local-ssd disk without feature flag fails
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 34359738368
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

  Scenario: Disk scale up on local-ssd disk with feature flag works
    Given feature flags
    """
    ["MDB_LOCAL_DISK_RESIZE"]
    """
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
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

  Scenario: Disk scale down on local-ssd disk without feature flag fails
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 8589934592
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

  Scenario: Disk scale down on local-ssd disk with feature flag works
    Given feature flags
    """
    ["MDB_LOCAL_DISK_RESIZE"]
    """
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 10737418240
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
