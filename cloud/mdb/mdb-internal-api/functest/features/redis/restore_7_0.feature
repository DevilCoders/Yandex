Feature: Restore Redis 7 cluster from backup

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_REDIS_70"]
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
            "redisConfig_7_0": {
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
  Scenario: Restoring to 7.0 works
    When we POST "/mdb/redis/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_7_0": {
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
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        }
    }
    """
