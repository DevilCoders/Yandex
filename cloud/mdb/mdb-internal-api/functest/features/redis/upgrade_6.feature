Feature: Redis Cluster version upgrade 6.0

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_6"]
    """
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "p@ssw#$rd!?"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 17179869184
            }
        },
        "persistenceMode": "OFF",
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API",
        "securityGroupIds": ["sg_id1", "sg_id3", "sg_id4"]
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

  @restart @persistence
  Scenario: Version change from 6.0 to 6.2 works
    Given feature flags
    """
    ["MDB_REDIS_62"]
    """
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "version": "6.2"
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
    When "worker_task_id2" acquired and finished by worker
    And in worker_queue exists "worker_task_id2" id with args "masters" containing:
      |iva-1.db.yandex.net|
    And we GET "/api/v1.0/config/iva-1.db.yandex.net"
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
                    "databases": 16,
                    "notify-keyspace-events": "",
                    "slowlog-log-slower-than": 10000,
                    "slowlog-max-len": 1000,
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "securityGroupIds": ["sg_id1", "sg_id3", "sg_id4"]
    }
    """
