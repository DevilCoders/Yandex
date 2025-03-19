Feature: Create/Modify Compute Redis Cluster

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
                "diskTypeId": "network-ssd",
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
        "securityGroupIds": ["sg_id1", "sg_id3", "sg_id4"]
    }
    """
    And "worker_task_id1" acquired and finished by worker
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" containing:
      |sg_id1|
      |sg_id3|
      |sg_id4|
    And worker set "sg_id1,sg_id3,sg_id4" security groups on "cid1"
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": ["sg_id1", "sg_id3", "sg_id4"]
    }
    """

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
            "redisConfig_5_0": {
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
            "assignPublicIp": true,
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
                    "public_ip": 1,
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

  Scenario: Modify to burstable on multi-host cluster fails
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "b2.compute.3"
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Redis shard with resource preset 'b2.compute.3' and disk type 'network-ssd' allows at most 1 host"
    }
    """

  Scenario: Resources modify on network-ssd disk works
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

  Scenario: Disk scale up on network-ssd disk works
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

  Scenario: Disk scale down on network-ssd disk without feature flag fails
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
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 10737418240
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

  Scenario: Disk scale down on network-ssd disk with feature flag works
    Given feature flags
    """
    ["MDB_NETWORK_DISK_TRUNCATE"]
    """
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
    And "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/redis/1.0/clusters/cid1" with data
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
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """

  @stop @events
  Scenario: Stop cluster works
    When we POST "/mdb/redis/1.0/clusters/cid1:stop"
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

  @stop
  Scenario: Modifying stopped cluster fails
    When we POST "/mdb/redis/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "myt"
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

  @stop
  Scenario: Metadata-only modifying on stopped cluster works
    When we POST "/mdb/redis/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
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
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
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

  @stop
  Scenario: Stop stopped cluster fails
    When we POST "/mdb/redis/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/redis/1.0/clusters/cid1:stop"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  @stop
  Scenario: Delete stopped cluster works
    When we POST "/mdb/redis/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we DELETE "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200

  @start @events
  Scenario: Start stopped cluster works
    When we POST "/mdb/redis/1.0/clusters/cid1:stop"
    And "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/redis/1.0/clusters/cid1:start"
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

  @start
  Scenario: Start running cluster fails
    When we POST "/mdb/redis/1.0/clusters/cid1:start"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Operation is not allowed in current cluster status"
    }
    """

  @security_groups
  Scenario: Removing security groups works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": []
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" is empty list
    And "worker_task_id2" acquired and finished by worker
    And worker clear security groups for "cid1"
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": null
    }
    """

  @security_groups
  Scenario: Modify security groups works
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" containing:
      |sg_id1|
      |sg_id2|
    And worker clear security groups for "cid1"
    And worker set "sg_id1,sg_id2" security groups on "cid1"
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": ["sg_id1", "sg_id2"]
    }
    """

  @security_groups
  Scenario: Modify security groups with same groups show no changes
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
       "securityGroupIds": ["sg_id1", "sg_id3", "sg_id4", "sg_id1"]
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
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
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

  @events @grpc
  Scenario: Deleting all but one hosts works
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["sas-1.df.cloud.yandex.net"]
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
                "sas-1.df.cloud.yandex.net"
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
                "sas-1.df.cloud.yandex.net"
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
                    "diskTypeId": "network-ssd",
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
                "name": "vla-1.df.cloud.yandex.net",
                "replicaPriority": 100,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "network-ssd",
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
        "page_size": 2
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
                    "disk_type_id": "network-ssd",
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
                "name": "vla-1.df.cloud.yandex.net",
                "replica_priority": "100",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "network-ssd",
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
    And "worker_task_id2" acquired and finished by worker
    When we POST "/mdb/redis/1.0/clusters/cid1/hosts:batchDelete" with data
    """
    {
        "hostNames": ["myt-1.df.cloud.yandex.net"]
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
                "myt-1.df.cloud.yandex.net"
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
                "myt-1.df.cloud.yandex.net"
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
                "name": "vla-1.df.cloud.yandex.net",
                "replicaPriority": 100,
                "assignPublicIp": false,
                "resources": {
                    "diskSize": 17179869184,
                    "diskTypeId": "network-ssd",
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
        "page_size": 2
    }
    """
    Then we get gRPC response with body
    """
    {
        "hosts": [
            {
                "cluster_id": "cid1",
                "health": "HEALTH_UNKNOWN",
                "name": "vla-1.df.cloud.yandex.net",
                "replica_priority": "100",
                "assign_public_ip": false,
                "resources": {
                    "disk_size": "17179869184",
                    "disk_type_id": "network-ssd",
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
