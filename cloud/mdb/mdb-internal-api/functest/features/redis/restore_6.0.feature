Feature: Restore Redis 6 cluster from backup

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
        "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"

  @events
  Scenario: Restoring to 6.2 works
    Given feature flags
    """
    ["MDB_REDIS_62"]
    """
    When we POST "/mdb/redis/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_2": {
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
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """

  @events
  Scenario: Restoring from backup to original folder works
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

  Scenario: Restoring from backup to different folder works
    When we POST "/mdb/redis/1.0/clusters:restore?folderId=folder2" with data
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
    When we GET "/mdb/redis/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder2"
    }
    """

  Scenario: Restoring from backup with less disk size fails
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
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we POST "/mdb/redis/1.0/clusters:restore?folderId=folder1" with data
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Insufficient diskSize, increase it to '21474836480'"
    }
    """

  Scenario: Restoring from backup with nonexistent backup fails
    When we POST "/mdb/redis/1.0/clusters:restore?folderId=folder1" with data
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
        "backupId": "cid1:777"
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Backup 'cid1:777' does not exist"
    }
    """

  Scenario: Restoring from backup with incorrect disk unit fails
    When we POST "/mdb/redis/1.0/clusters:restore?folderId=folder1" with data
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
                "diskSize": 10737418245
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
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Disk size must be a multiple of 4194304 bytes"
    }
    """

  @delete
  Scenario: Restore from backup belongs to deleted cluster
    When we DELETE "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "Delete Redis cluster",
        "id": "worker_task_id2"
    }
    """
    When all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we POST "/mdb/redis/1.0/clusters:restore?folderId=folder1" with data
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
        "id": "worker_task_id5",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.RestoreClusterMetadata",
            "backupId": "cid1:shard1_id:stream_19700101T000030Z",
            "clusterId": "cid2"
        }
    }
    """

  @timetravel
  Scenario: Restoring onto backup which require less disk_size
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
    And we POST "/mdb/redis/1.0/clusters:restore?folderId=folder1" with data
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
    Then we get response with status 200
