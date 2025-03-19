Feature: Restore Redis 6 tls cluster from backup

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_6"]
    """
    And s3 response
    """
    {
        "Contents": [
            {
                "Key": "redis-backup/cid1/shard1_id/basebackups_005/stream_19700101T000030Z_backup_stop_sentinel.json",
                "LastModified": 31,
                "Body": {
                    "BackupName": "stream_19700101T000030Z",
                    "StartLocalTime": "1970-01-01T03:00:30.000000+03:00",
                    "FinishLocalTime": "1970-01-01T03:00:31.000000+03:00",
                    "UserData": {
                        "shard_name": "shard1",
                        "backup_id": "none"
                    },
                    "Permanent": false,
                    "BackupSize": 30,
                    "DataSize": 100
                }
            },
            {
                "Key": "redis-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z_backup_stop_sentinel.json",
                "LastModified": 36,
                "Body": {
                    "StartLocalTime": "1970-01-01T03:00:35.000000+03:00",
                    "FinishLocalTime": "1970-01-01T03:00:36.000000+03:00",
                    "UserData": {
                        "shard_name": "shard1",
                        "backup_id": "none"
                    },
                    "Permanent": true,
                    "BackupSize": 40,
                    "DataSize": 120
                }
            },
            {
                "Key": "redis-backup/cid1/shard2_id/basebackups_005/stream_19700101T000040Z_backup_stop_sentinel.json",
                "LastModified": 41,
                "Body": {
                    "StartLocalTime": "1970-01-01T03:00:40.000000+03:00",
                    "FinishLocalTime": "1970-01-01T03:00:41.000000+03:00",
                    "UserData": {
                        "shard_name": "shard2",
                        "backup_id": "none"
                    },
                    "Permanent": false,
                    "BackupSize": 50,
                    "DataSize": 150
                }
            }
        ]
    }
    """
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "tlsEnabled": true
    }
    """
    And "worker_task_id1" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"

  @events @buffers @persistence
  Scenario: Restoring from backup to tls mode ON works
    When we POST "/mdb/redis/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "passw0rd",
                "clientOutputBufferLimitNormal": {
                    "hardLimit": 3000,
                    "hardLimitUnit": "KILOBYTES",
                    "softLimit": 2388608,
                    "softLimitUnit": "BYTES"
                },
                "clientOutputBufferLimitPubsub": {
                    "hardLimit": 1000,
                    "softLimit": 3,
                    "softLimitUnit": "MEGABYTES",
                    "softSeconds": 30
                }
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:shard1_id:stream_19700101T000030Z",
        "tlsEnabled": true,
        "persistenceMode": "OFF"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new Redis cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.RestoreClusterMetadata",
            "backupId": "cid1:shard1_id:stream_19700101T000030Z",
            "clusterId": "cid2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.RestoreCluster" event with
    """
    {
        "details": {
            "backup_id": "cid1:shard1_id:stream_19700101T000030Z",
            "cluster_id": "cid2"
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "tlsEnabled": true,
        "persistenceMode": "OFF"
    }
    """
    When we GET "/api/v1.0/config/iva-2.db.yandex.net"
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
                "data": "2",
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
                        "subcid2": {
                            "hosts": {},
                            "name": "test_restored",
                            "roles": [
                                "redis_cluster"
                            ],
                            "shards": {
                                "shard_id2": {
                                    "hosts": {
                                        "iva-2.db.yandex.net": {"geo": "iva"},
                                        "myt-2.db.yandex.net": {"geo": "myt"},
                                        "sas-2.db.yandex.net": {"geo": "sas"}
                                    },
                                    "name": "shard1"
                                }
                            }
                        }
                    }
                },
                "cluster_hosts": [
                    "iva-2.db.yandex.net",
                    "myt-2.db.yandex.net",
                    "sas-2.db.yandex.net"
                ],
                "cluster_id": "cid2",
                "cluster_name": "test_restored",
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
                "fqdn": "iva-2.db.yandex.net",
                "geo": "iva",
                "region": "ru-central-1",
                "cloud_provider": "yandex",
                "created_at": "**IGNORE**",
                "shard_hosts": [
                    "iva-2.db.yandex.net",
                    "myt-2.db.yandex.net",
                    "sas-2.db.yandex.net"
                ],
                "shard_id": "shard_id2",
                "shard_name": "shard1",
                "space_limit": 10737418240,
                "subcluster_id": "subcid2",
                "subcluster_name": "test_restored",
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
                    "client-output-buffer-limit normal": "3072000 2388608 60",
                    "client-output-buffer-limit pubsub": "1000 3145728 30",
                    "client-output-buffer-limit replica": "429496729 214748364 60",
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
            "s3_bucket": "yandexcloud-dbaas-cid2",
            "versions": {
                "redis": {
                    "edition": "some hidden value",
                    "major_version": "6.0",
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


  @events
  Scenario: Restoring from backup to tls mode OFF works
    When we POST "/mdb/redis/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:shard1_id:stream_19700101T000030Z",
        "tlsEnabled": false
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new Redis cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.RestoreClusterMetadata",
            "backupId": "cid1:shard1_id:stream_19700101T000030Z",
            "clusterId": "cid2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.RestoreCluster" event with
    """
    {
        "details": {
            "backup_id": "cid1:shard1_id:stream_19700101T000030Z",
            "cluster_id": "cid2"
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "tlsEnabled": false,
        "persistenceMode": "ON"
    }
    """

  @events
  Scenario: Restoring from backup without tls mode setting works
    When we POST "/mdb/redis/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:shard1_id:stream_19700101T000030Z"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new Redis cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.RestoreClusterMetadata",
            "backupId": "cid1:shard1_id:stream_19700101T000030Z",
            "clusterId": "cid2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.RestoreCluster" event with
    """
    {
        "details": {
            "backup_id": "cid1:shard1_id:stream_19700101T000030Z",
            "cluster_id": "cid2"
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "tlsEnabled": false,
        "persistenceMode": "ON"
    }
    """
