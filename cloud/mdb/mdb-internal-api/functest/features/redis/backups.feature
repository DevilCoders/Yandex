Feature: Backup Redis Cluster

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
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
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

  @grpc
  Scenario: Backup creation works
    When we "POST" via REST at "/mdb/redis/1.0/clusters/cid1:backup"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create a backup for Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.BackupClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we "Backup" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create a backup for Redis cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.redis.v1.BackupClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.redis.BackupCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we POST "/mdb/redis/1.0/clusters/cid1:backup"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Conflicting operation worker_task_id2 detected"
    }
    """

  @grpc
  Scenario: Backup list works
    Given s3 response
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
    When we "GET" via REST at "/mdb/redis/1.0/backups?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:41+00:00",
                "folderId": "folder1",
                "id": "cid1:shard2_id:stream_19700101T000040Z",
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:40+00:00",
                "sourceShardNames": ["shard2"]
            },
            {
                "createdAt": "1970-01-01T00:00:36+00:00",
                "folderId": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000035Z",
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:35+00:00",
                "sourceShardNames": ["shard1"]
            },
            {
                "createdAt": "1970-01-01T00:00:31+00:00",
                "folderId": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000030Z",
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:30+00:00",
                "sourceShardNames": ["shard1"]
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.redis.v1.BackupService" with data
    """
    {
        "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "1970-01-01T00:00:41Z",
                "folder_id": "folder1",
                "id": "cid1:shard2_id:stream_19700101T000040Z",
                "source_cluster_id": "cid1",
                "started_at": "1970-01-01T00:00:40Z",
                "source_shard_names": ["shard2"]
            },
            {
                "created_at": "1970-01-01T00:00:36Z",
                "folder_id": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000035Z",
                "source_cluster_id": "cid1",
                "started_at": "1970-01-01T00:00:35Z",
                "source_shard_names": ["shard1"]
            },
            {
                "created_at": "1970-01-01T00:00:31Z",
                "folder_id": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000030Z",
                "source_cluster_id": "cid1",
                "started_at": "1970-01-01T00:00:30Z",
                "source_shard_names": ["shard1"]
            }
        ]
    }
    """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "1970-01-01T00:00:41Z",
                "folder_id": "folder1",
                "id": "cid1:shard2_id:stream_19700101T000040Z",
                "source_cluster_id": "cid1",
                "started_at": "1970-01-01T00:00:40Z",
                "source_shard_names": ["shard2"]
            },
            {
                "created_at": "1970-01-01T00:00:36Z",
                "folder_id": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000035Z",
                "source_cluster_id": "cid1",
                "started_at": "1970-01-01T00:00:35Z",
                "source_shard_names": ["shard1"]
            },
            {
                "created_at": "1970-01-01T00:00:31Z",
                "folder_id": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000030Z",
                "source_cluster_id": "cid1",
                "started_at": "1970-01-01T00:00:30Z",
                "source_shard_names": ["shard1"]
            }
        ]
    }
    """

  Scenario: Backup list with page size works
    Given s3 response
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
    When we GET "/mdb/redis/1.0/backups?folderId=folder1&pageSize=1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:41+00:00",
                "folderId": "folder1",
                "id": "cid1:shard2_id:stream_19700101T000040Z",
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:40+00:00",
                "sourceShardNames": ["shard2"]
            }
        ],
        "nextPageToken": "Y2lkMTpzaGFyZDJfaWQ6c3RyZWFtXzE5NzAwMTAxVDAwMDA0MFo="
    }
    """

  Scenario: Backup list with page token works
    Given s3 response
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
    When we GET "/mdb/redis/1.0/backups?folderId=folder1&pageToken=Y2lkMTpzaGFyZDJfaWQ6c3RyZWFtXzE5NzAwMTAxVDAwMDA0MFo=&pageSize=1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:36+00:00",
                "folderId": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000035Z",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:35+00:00"
            }
        ]
    }
    """

  Scenario: Backup list by cid works
    Given s3 response
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
                "Key": "redis-backup/cid2/shard2_id/basebackups_005/stream_19700101T000040Z_backup_stop_sentinel.json",
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
    When we GET "/mdb/redis/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:36+00:00",
                "folderId": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000035Z",
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:35+00:00",
                "sourceShardNames": ["shard1"]
            },
            {
                "createdAt": "1970-01-01T00:00:31+00:00",
                "folderId": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000030Z",
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:30+00:00",
                "sourceShardNames": ["shard1"]
            }
        ]
    }
    """

  Scenario: Backup list on cluster with no backups works
    Given s3 response
    """
    {}
    """
    When we GET "/mdb/redis/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": []
    }
    """

  @delete @grpc
  Scenario: After cluster delete its backups are shown
    Given s3 response
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
    When we DELETE "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "Delete Redis cluster",
        "id": "worker_task_id2"
    }
    """
    When we GET "/mdb/redis/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:41+00:00",
                "folderId": "folder1",
                "id": "cid1:shard2_id:stream_19700101T000040Z",
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:40+00:00",
                "sourceShardNames": ["shard2"]
            },
            {
                "createdAt": "1970-01-01T00:00:36+00:00",
                "folderId": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000035Z",
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:35+00:00",
                "sourceShardNames": ["shard1"]
            },
            {
                "createdAt": "1970-01-01T00:00:31+00:00",
                "folderId": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000030Z",
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:30+00:00",
                "sourceShardNames": ["shard1"]
            }
        ]
    }
    """
    When we "GET" via REST at "/mdb/redis/1.0/backups/cid1:shard1_id:stream_19700101T000030Z"
    Then we get response with status 200 and body contains
    """
    {
        "createdAt": "1970-01-01T00:00:31+00:00",
        "folderId": "folder1",
        "id": "cid1:shard1_id:stream_19700101T000030Z",
        "sourceClusterId": "cid1",
        "startedAt": "1970-01-01T00:00:30+00:00",
        "sourceShardNames": ["shard1"]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.redis.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:shard1_id:stream_19700101T000030Z"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_at": "1970-01-01T00:00:31Z",
        "folder_id": "folder1",
        "id": "cid1:shard1_id:stream_19700101T000030Z",
        "source_cluster_id": "cid1",
        "started_at": "1970-01-01T00:00:30Z",
        "source_shard_names": ["shard1"]
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1/backups"
    Then we get response with status 403
    # backups for purged cluster not available
    When "worker_task_id2" acquired and finished by worker
    And "worker_task_id3" acquired and finished by worker
    And "worker_task_id4" acquired and finished by worker
    And we GET "/mdb/redis/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": []
    }
    """
    When we "GET" via REST at "/mdb/redis/1.0/backups/cid1:1"
    Then we get response with status 403
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.redis.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:1"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "malformed backup id, awaited X:X:X-like: cid1:1"
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.redis.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:shard1_id:stream_19700101T000030Z"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "cluster id "cid1" not found"
