Feature: Check that grpc api won't mess cluster pillar

  Background:
    Given default headers

  @grpc
  Scenario: Create sharded cluster with all pillar values set
    Given feature flags
    """
    ["MDB_REDIS_62"]
    """
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "access": {
                "dataLens": true,
                "dataTransfer": true,
                "serverless": true,
                "webSql": true
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "redisConfig_6_2": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "network-ssd",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt",
            "subnetId": "network1-myt",
            "assignPublicIp": true,
            "shardName": "shard1"
        },
        {
            "zoneId": "vla",
            "subnetId": "network1-vla",
            "assignPublicIp": true,
            "shardName": "shard2"
        },
        {
            "zoneId": "sas",
            "subnetId": "network1-sas",
            "assignPublicIp": true,
            "shardName": "shard3"
        }],
        "description": "test cluster",
        "networkId": "network1",
        "securityGroupIds": ["sg_id1", "sg_id3", "sg_id4"],
        "sharded": true,
        "tlsEnabled": true
    }
    """
    Then we get response with status 200
    When "worker_task_id1" acquired and finished by worker
    And we "POST" via REST at "/mdb/redis/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "vla",
            "subnetId": "network1-vla",
            "assignPublicIp": true,
            "shardName": "shard3"
        }]
    }
    """
    Then we get response with status 200
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "zone_id": "vla",
            "subnet_id": "network1-vla",
            "assign_public_ip": true,
            "shard_name": "shard3"
        }]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we GET "/api/v1.0/config/vla-2.df.cloud.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
           "access": {
               "data_lens": true,
               "data_transfer": true,
               "serverless": true,
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
                "assign_public_ip": true,
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
                                        "myt-1.df.cloud.yandex.net": {"geo": "myt"}
                                    },
                                    "name": "shard1"
                                },
                                "shard_id2": {
                                    "hosts": {
                                        "vla-1.df.cloud.yandex.net": {"geo": "vla"}
                                    },
                                    "name": "shard2"
                                },
                                "shard_id3": {
                                    "hosts": {
                                        "sas-1.df.cloud.yandex.net": {"geo": "sas"},
                                        "vla-2.df.cloud.yandex.net": {"geo": "vla"}
                                    },
                                    "name": "shard3"
                                }
                            }
                        }
                    }
                },
                "cluster_hosts": [
                    "myt-1.df.cloud.yandex.net",
                    "sas-1.df.cloud.yandex.net",
                    "vla-1.df.cloud.yandex.net",
                    "vla-2.df.cloud.yandex.net"
                ],
                "cluster_id": "cid1",
                "cluster_name": "test",
                "cluster_type": "redis_cluster",
                "disk_type_id": "network-ssd",
                "flavor": {
                    "cpu_guarantee": 1.0,
                    "cpu_limit": 1.0,
                    "cpu_fraction": 100,
                    "gpu_limit": 0,
                    "description": "s1.compute.1",
                    "id": "00000000-0000-0000-0000-000000000001",
                    "io_limit": 20971520,
                    "io_cores_limit": 0,
                    "memory_guarantee": 4294967296,
                    "memory_limit": 4294967296,
                    "name": "s1.compute.1",
                    "network_guarantee": 16777216,
                    "network_limit": 16777216,
                    "type": "standard",
                    "generation": 1,
                    "platform_id": "mdb-v1",
                    "vtype": "compute"
                },
                "folder": {
                    "folder_ext_id": "folder1"
                },
                "fqdn": "vla-2.df.cloud.yandex.net",
                "geo": "vla",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "sas-1.df.cloud.yandex.net",
                    "vla-2.df.cloud.yandex.net"
                ],
                "shard_id": "shard_id3",
                "shard_name": "shard3",
                "space_limit": 17179869184,
                "subcluster_id": "subcid1",
                "subcluster_name": "test",
                "vtype": "compute",
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
                            "data": "passw0rd",
                            "encryption_version": 0
                        },
                    "masterauth": {
                            "data": "passw0rd",
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
                    "enabled": true
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

  @grpc
  Scenario: Create non-sharded cluster with all pillar values set
    Given feature flags
    """
    ["MDB_REDIS_62"]
    """
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "access": {
                "dataLens": true,
                "dataTransfer": true,
                "serverless": true,
                "webSql": true
            },
            "backupWindowStart": {
                "hours": 22,
                "minutes": 15,
                "seconds": 30,
                "nanos": 100
            },
            "redisConfig_6_2": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "network-ssd",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt",
            "subnetId": "network1-myt",
            "assignPublicIp": true,
            "replicaPriority": 42
        }],
        "description": "test cluster",
        "networkId": "network1",
        "securityGroupIds": ["sg_id1", "sg_id3", "sg_id4"],
        "tlsEnabled": true
    }
    """
    Then we get response with status 200
    When "worker_task_id1" acquired and finished by worker
    And we "POST" via REST at "/mdb/redis/1.0/clusters/cid1/hosts:batchCreate" with data
    """
    {
        "hostSpecs": [{
            "zoneId": "vla",
            "subnetId": "network1-vla",
            "assignPublicIp": true,
            "replicaPriority": 43
        }]
    }
    """
    Then we get response with status 200
    When we "AddHosts" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "host_specs": [{
            "zone_id": "vla",
            "subnet_id": "network1-vla",
            "assign_public_ip": true,
            "replica_priority": 43
        }]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id2" acquired and finished by worker
    And we GET "/api/v1.0/config/vla-1.df.cloud.yandex.net"
    Then we get response with status 200 and body contains
    """
    {
        "data": {
           "access": {
               "data_lens": true,
               "data_transfer": true,
               "serverless": true,
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
                "assign_public_ip": true,
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
                                        "myt-1.df.cloud.yandex.net": {"geo": "myt"},
                                        "vla-1.df.cloud.yandex.net": {"geo": "vla"}
                                    },
                                    "name": "shard1"
                                }
                            }
                        }
                    }
                },
                "cluster_hosts": [
                    "myt-1.df.cloud.yandex.net",
                    "vla-1.df.cloud.yandex.net"
                ],
                "cluster_id": "cid1",
                "cluster_name": "test",
                "cluster_type": "redis_cluster",
                "disk_type_id": "network-ssd",
                "flavor": {
                    "cpu_guarantee": 1.0,
                    "cpu_limit": 1.0,
                    "cpu_fraction": 100,
                    "gpu_limit": 0,
                    "description": "s1.compute.1",
                    "id": "00000000-0000-0000-0000-000000000001",
                    "io_limit": 20971520,
                    "io_cores_limit": 0,
                    "memory_guarantee": 4294967296,
                    "memory_limit": 4294967296,
                    "name": "s1.compute.1",
                    "network_guarantee": 16777216,
                    "network_limit": 16777216,
                    "type": "standard",
                    "generation": 1,
                    "platform_id": "mdb-v1",
                    "vtype": "compute"
                },
                "folder": {
                    "folder_ext_id": "folder1"
                },
                "fqdn": "vla-1.df.cloud.yandex.net",
                "geo": "vla",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "myt-1.df.cloud.yandex.net",
                    "vla-1.df.cloud.yandex.net"
                ],
                "shard_id": "shard_id1",
                "shard_name": "shard1",
                "space_limit": 17179869184,
                "subcluster_id": "subcid1",
                "subcluster_name": "test",
                "vtype": "compute",
                "vtype_id": null
            },
            "default pillar": true,
            "redis": {
                "config": {
                    "maxmemory": 3221225472,
                    "maxmemory-policy": "noeviction",
                    "repl-backlog-size": 429496729,
                    "replica-priority": 43,
                    "client-output-buffer-limit normal": "16777216 8388608 60",
                    "client-output-buffer-limit replica": "429496729 214748364 60",
                    "client-output-buffer-limit pubsub": "16777216 8388608 60",
                    "timeout": 0,
                    "requirepass": {
                            "data": "passw0rd",
                            "encryption_version": 0
                        },
                    "masterauth": {
                            "data": "passw0rd",
                            "encryption_version": 0
                        },
                    "cluster-enabled": "no",
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
                    "sentinel_direct_renames": {
                        "ACL": {
                            "data": "dummy",
                            "encryption_version": 0
                        }
                    },
                    "sentinel_renames": {
                        "CONFIG-SET": {
                            "data": "dummy",
                            "encryption_version": 0
                        },
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
                    "enabled": true
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
