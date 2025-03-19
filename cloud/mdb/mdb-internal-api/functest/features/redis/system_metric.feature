Feature: System metrics

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
               ],
               "system": {
                   "cpu": {
                       "timestamp": 1630408992,
                       "used": 0.43
                   },
                   "mem": {
                       "timestamp": 1630408992,
                       "used": 29293992,
                       "total": 219292938
                   },
                   "disk": {
                       "timestamp": 1630409035,
                       "used": 2142349823,
                       "total": 214902848239
                   }
               }
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
               ],
               "system": {
                   "cpu": {
                       "timestamp": 1630409270,
                       "used": 0.43
                   },
                   "mem": {
                       "timestamp": 1630409270,
                       "used": 1023930434,
                       "total": 4392842893523
                   }
               }
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

  @hosts @list @grpc
  Scenario: System metrics works
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
                "shardName": "shard1",
                "system": {
                    "cpu": {
                        "timestamp": 1630408992,
                        "used": 0.43
                    },
                    "memory": {
                        "timestamp": 1630408992,
                        "used": 29293992,
                        "total": 219292938
                    },
                    "disk": {
                        "timestamp": 1630409035,
                        "used": 2142349823,
                        "total": 214902848239
                    }
                }
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
                "shardName": "shard1",
                "system": {
                    "cpu": {
                        "timestamp": 1630409270,
                        "used": 0.43
                    },
                    "memory": {
                        "timestamp": 1630409270,
                        "used": 1023930434,
                        "total": 4392842893523
                    }
                }
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
                "zone_id": "iva",
                "shard_name": "shard1",
                "system": {
                    "cpu": {
                        "timestamp": "1630408992",
                        "used": 0.43
                    },
                    "memory": {
                        "timestamp": "1630408992",
                        "used": "29293992",
                        "total": "219292938"
                    },
                    "disk": {
                        "timestamp": "1630409035",
                        "used": "2142349823",
                        "total": "214902848239"
                    }
                }
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
                "zone_id": "myt",
                "shard_name": "shard1",
                "system": null
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
                "zone_id": "sas",
                "shard_name": "shard1",
                "system": {
                    "cpu": {
                        "timestamp": "1630409270",
                        "used": 0.43
                    },
                    "disk": null,
                    "memory": {
                        "timestamp": "1630409270",
                        "used": "1023930434",
                        "total": "4392842893523"
                    }
                }
            }
        ]
    }
    """
