@restore-hints
Feature: Restore Redis cluster from backup with hints

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_5"]
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
            }
        ]
    }
    """

  Scenario: Restore hints for Redis 5.0
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
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{ "zoneId": "myt" }]
    }
    """
    Then we get response with status 200
    When all "cid1" revs committed before "1970-01-01T00:00:40+00:00"
    And we GET "/mdb/redis/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:31+00:00",
                "folderId": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000030Z",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:30+00:00"
            }
        ]
    }
    """
    When we GET "/mdb/redis/1.0/console/restore-hints/cid1:shard1_id:stream_19700101T000030Z"
    Then we get response with status 200 and body equals
    """
    {
        "environment": "PRESTABLE",
        "networkId": "",
        "resources": {
            "diskSize": 10737418240,
            "resourcePresetId": "s1.porto.1"
        },
        "version": "5.0"
    }
    """

    Scenario: Restore hints for Redis 6.0
    Given feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_6"]
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
        "hostSpecs": [{ "zoneId": "myt" }]
    }
    """
    Then we get response with status 200
    When all "cid1" revs committed before "1970-01-01T00:00:40+00:00"
    And we GET "/mdb/redis/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:31+00:00",
                "folderId": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000030Z",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:30+00:00"
            }
        ]
    }
    """
    When we GET "/mdb/redis/1.0/console/restore-hints/cid1:shard1_id:stream_19700101T000030Z"
    Then we get response with status 200 and body equals
    """
    {
        "environment": "PRESTABLE",
        "networkId": "",
        "resources": {
            "diskSize": 10737418240,
            "resourcePresetId": "s1.porto.1"
        },
        "version": "6.0"
    }
    """


    Scenario: Restore hints for Redis 6.2
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
            "redisConfig_6_2": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 10737418240
            }
        },
        "hostSpecs": [{ "zoneId": "myt" }]
    }
    """
    Then we get response with status 200
    When all "cid1" revs committed before "1970-01-01T00:00:40+00:00"
    And we GET "/mdb/redis/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:31+00:00",
                "folderId": "folder1",
                "id": "cid1:shard1_id:stream_19700101T000030Z",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:30+00:00"
            }
        ]
    }
    """
    When we GET "/mdb/redis/1.0/console/restore-hints/cid1:shard1_id:stream_19700101T000030Z"
    Then we get response with status 200 and body equals
    """
    {
        "environment": "PRESTABLE",
        "networkId": "",
        "resources": {
            "diskSize": 10737418240,
            "resourcePresetId": "s1.porto.1"
        },
        "version": "6.2"
    }
    """
