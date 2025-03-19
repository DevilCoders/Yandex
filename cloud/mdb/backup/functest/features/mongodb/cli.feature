Feature: MongoDB backup import works

  Background: Empty databases
    When I create "1" mongodb clusters
    And I execute mdb-backup-cli for cluster "mongodb_01" to "enable backup-service"

  @import
  Scenario: Backup import works
    And I enable sharding on "mongodb_01" mongodb cluster
    And I add shard "rs02" to "mongodb_01" cluster
    And I add shard "rs03" to "mongodb_01" cluster
    And s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                    "CommonPrefixes": [
                        {
                            "Prefix": "mongodb-backup/cid1/shard_id1"
                        },
                        {
                            "Prefix": "mongodb-backup/cid1/shard_id2"
                        },
                        {
                            "Prefix": "mongodb-backup/cid1/subcid2"
                        }
                    ]
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000030Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:31.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000030Z/stream.br",
                            "Size": 30
                        }
                    ]
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "StartLocalTime": "1970-01-01T03:00:30.403547+03:00",
                        "FinishLocalTime": "1970-01-01T03:00:31.403547+03:00",
                        "MongoMeta": {
                            "Before": {
                                "LastMajTS": {
                                    "TS": 30,
                                    "Inc": 1
                                }
                            },
                            "After": {
                                "LastMajTS": {
                                    "TS": 31,
                                    "Inc": 6
                                }
                            }
                        },
                        "UserData": {
                            "backup_id": "man01",
                            "shard_name": "rs01"
                        },
                        "Permanent": true,
                        "DataSize": 30
                    }
                }
            },
            {
               "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000030Z/stream.br",
                            "Size": 30
                        }
                    ]
                }
            },
            {
               "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id2/basebackups_005/stream_19700101T000035Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:36.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard_id2/basebackups_005/stream_19700101T000045Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:56.403547+03:00"
                        }
                    ]
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "StartLocalTime": "1970-01-01T03:00:35.403547+03:00",
                        "FinishLocalTime": "1970-01-01T03:00:36.403547+03:00",
                        "MongoMeta": {
                            "Before": {
                                "LastMajTS": {
                                    "TS": 35,
                                    "Inc": 10
                                }
                            },
                            "After": {
                                "LastMajTS": {
                                    "TS": 36,
                                    "Inc": 60
                                }
                            }
                        },
                        "UserData": {
                            "backup_id": "none",
                            "shard_name": "rs02"
                        },
                        "Permanent": false,
                        "DataSize": 35
                    }
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id2/basebackups_005/stream_19700101T000035Z/stream.br",
                            "Size": 35
                        }
                    ]
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "StartLocalTime": "1970-01-01T03:00:45.403547+03:00",
                        "FinishLocalTime": "1970-01-01T03:00:56.403547+03:00",
                        "MongoMeta": {
                            "Before": {
                                "LastMajTS": {
                                    "TS": 45,
                                    "Inc": 10
                                }
                            },
                            "After": {
                                "LastMajTS": {
                                    "TS": 56,
                                    "Inc": 60
                                }
                            }
                        },
                        "UserData": {
                            "backup_id": "none",
                            "shard_name": "rs02"
                        },
                        "Permanent": true
                    }
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id2/basebackups_005/stream_19700101T000045Z/stream.br",
                            "Size": 45
                        }
                    ]
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/subcid2/basebackups_005/stream_19700101T000055Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:55.403547+03:00"
                        }
                    ]
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "StartLocalTime": "1970-01-01T03:00:55.403547+03:00",
                        "FinishLocalTime": "1970-01-01T03:00:58.403547+03:00",
                        "MongoMeta": {
                            "Before": {
                                "LastMajTS": {
                                    "TS": 55,
                                    "Inc": 10
                                }
                            },
                            "After": {
                                "LastMajTS": {
                                    "TS": 58,
                                    "Inc": 60
                                }
                            }
                        },
                        "UserData": {
                            "backup_id": "none",
                            "shard_name": "mongocfg_subcluster"
                        },
                        "Permanent": false,
                        "DataSize": 55
                    }
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id2/basebackups_005/stream_19700101T000055Z/stream.br",
                            "Size": 55
                        }
                    ]
                }
            }
        ]
    }
    """
    When I execute mdb-backup-cli for cluster "mongodb_01" to "import s3 backups" and got
    """
    {"CompletedCreation": 0, "ExistsInMetadb": 0, "ExistsInStorage": 4, "ImportedIntoMetadb": 4, "SkippedDueToDryRun": 0, "SkippedDueToExistence": 0, "SkippedDueToUniqSchedDate": 0, "SkippedNoIncrementBase": 0}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status, b.initiator, b.method, b.subcid, b.shard_id, b.created_at, b.delayed_until, b.started_at, b.finished_at, b.updated_at
    FROM dbaas.backups b
    """
    Then it returns "4" rows matches
    """
      - backup_id:     backup_id1
        status:        DONE
        initiator:     USER
        method:        FULL
        subcid:        subcid1
        shard_id:      shard_id1
        created_at:    1970-01-01T00:00:30.403547+00:00
        delayed_until: 1970-01-01T00:00:30.403547+00:00
        started_at:    1970-01-01T00:00:30.403547+00:00
        finished_at:   1970-01-01T00:00:31.403547+00:00
        updated_at:    1970-01-01T00:00:31.403547+00:00
      - backup_id:     backup_id2
        status:        DONE
        initiator:     SCHEDULE
        method:        FULL
        subcid:        subcid1
        shard_id:      shard_id2
        created_at:    1970-01-01T00:00:35.403547+00:00
        delayed_until: 1970-01-01T00:00:35.403547+00:00
        started_at:    1970-01-01T00:00:35.403547+00:00
        finished_at:   1970-01-01T00:00:36.403547+00:00
        updated_at:    1970-01-01T00:00:36.403547+00:00
      - backup_id:     backup_id3
        status:        DONE
        initiator:     USER
        method:        FULL
        subcid:        subcid1
        shard_id:      shard_id2
        created_at:    1970-01-01T00:00:45.403547+00:00
        delayed_until: 1970-01-01T00:00:45.403547+00:00
        started_at:    1970-01-01T00:00:45.403547+00:00
        finished_at:   1970-01-01T00:00:56.403547+00:00
        updated_at:    1970-01-01T00:00:56.403547+00:00
      - backup_id:     backup_id4
        status:        DONE
        initiator:     SCHEDULE
        method:        FULL
        subcid:        subcid2
        shard_id:
        created_at:    1970-01-01T00:00:55.403547+00:00
        delayed_until: 1970-01-01T00:00:55.403547+00:00
        started_at:    1970-01-01T00:00:55.403547+00:00
        finished_at:   1970-01-01T00:00:58.403547+00:00
        updated_at:    1970-01-01T00:00:58.403547+00:00
    """
    And I list backups for "mongodb_01" cluster and got
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:56+00:00",
                "folderId": "folder1",
                "id": "cid1:backup_id3",
                "size": 45,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["rs02"],
                "startedAt": "1970-01-01T00:00:45+00:00",
                "type": "MANUAL"
            },
            {
                "createdAt": "1970-01-01T00:00:36+00:00",
                "folderId": "folder1",
                "id": "cid1:backup_id2",
                "size": 35,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["rs02"],
                "startedAt": "1970-01-01T00:00:35+00:00",
                "type": "AUTOMATED"
            },
            {
                "createdAt": "1970-01-01T00:00:31+00:00",
                "folderId": "folder1",
                "id": "cid1:backup_id1",
                "size": 30,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["rs01"],
                "startedAt": "1970-01-01T00:00:30+00:00",
                "type": "MANUAL"
            }
        ]
    }
    """

  @secondrun
  Scenario: Second run does not handle imported backups
    Given s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                    "CommonPrefixes": [
                        {
                            "Prefix": "mongodb-backup/cid1/shard_id1"
                        }
                    ]
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000035Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:36.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000035Z/stream.br",
                            "Size": 35
                        }
                    ]
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "StartLocalTime": "1970-01-01T03:00:35.403547+03:00",
                        "FinishLocalTime": "1970-01-01T03:00:36.403547+03:00",
                        "MongoMeta": {
                            "Before": {
                                "LastMajTS": {
                                    "TS": 35,
                                    "Inc": 10
                                }
                            },
                            "After": {
                                "LastMajTS": {
                                    "TS": 36,
                                    "Inc": 60
                                }
                            }
                        },
                        "UserData": {
                            "backup_id": "none",
                            "shard_name": "rs01"
                        },
                        "Permanent": false
                    }
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000035Z/stream.br",
                            "Size": 35
                        }
                    ]
                }
            }
        ]
    }
    """
    When I execute mdb-backup-cli for cluster "mongodb_01" to "import s3 backups" and got
    """
    {"CompletedCreation": 0, "ExistsInMetadb": 0, "ExistsInStorage": 1, "ImportedIntoMetadb": 1, "SkippedDueToDryRun": 0, "SkippedDueToExistence": 0, "SkippedDueToUniqSchedDate": 0, "SkippedNoIncrementBase": 0}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status, b.initiator, b.method, b.subcid, b.shard_id, b.created_at, b.delayed_until, b.started_at, b.finished_at, b.updated_at
    FROM dbaas.backups b
    """
    Then it returns "1" rows matches
    """
      - backup_id:     backup_id1
        status:        DONE
        initiator:     SCHEDULE
        method:        FULL
        subcid:        subcid1
        shard_id:      shard_id1
        created_at:    1970-01-01T00:00:35.403547+00:00
        delayed_until: 1970-01-01T00:00:35.403547+00:00
        started_at:    1970-01-01T00:00:35.403547+00:00
        finished_at:   1970-01-01T00:00:36.403547+00:00
        updated_at:    1970-01-01T00:00:36.403547+00:00
    """
    And s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                    "CommonPrefixes": [
                        {
                            "Prefix": "mongodb-backup/cid1/shard_id1"
                        }
                    ]
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000035Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:36.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000035Z/stream.br",
                            "Size": 35
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000045Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:56.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000045Z/stream.br",
                            "Size": 45
                        }
                    ]
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "MongoMeta": {
                            "Before": {
                                "LastMajTS": {
                                    "TS": 35,
                                    "Inc": 10
                                }
                            },
                            "After": {
                                "LastMajTS": {
                                    "TS": 36,
                                    "Inc": 60
                                }
                            }
                        },
                        "UserData": {
                            "backup_id": "auto01"
                        }
                    }
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000035Z/stream.br",
                            "Size": 35
                        }
                    ]
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "StartLocalTime": "1970-01-01T03:00:45.403547+03:00",
                        "FinishLocalTime": "1970-01-01T03:00:56.403547+03:00",
                        "MongoMeta": {
                            "Before": {
                                "LastMajTS": {
                                    "TS": 45,
                                    "Inc": 10
                                }
                            },
                            "After": {
                                "LastMajTS": {
                                    "TS": 56,
                                    "Inc": 60
                                }
                            }
                        },
                        "UserData": {
                            "backup_id": "none",
                            "shard_name": "rs01"
                        },
                        "Permanent": true
                    }
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000045Z/stream.br",
                            "Size": 45
                        }
                    ]
                }
            }
        ]
    }
    """
    When I execute mdb-backup-cli for cluster "mongodb_01" to "import s3 backups" and got
    """
    {"CompletedCreation": 0, "ExistsInMetadb": 1, "ExistsInStorage": 2, "ImportedIntoMetadb": 1, "SkippedDueToDryRun": 0, "SkippedDueToExistence": 1, "SkippedDueToUniqSchedDate": 0, "SkippedNoIncrementBase": 0}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status, b.initiator, b.method, b.subcid, b.shard_id, b.created_at, b.delayed_until, b.started_at, b.finished_at, b.updated_at
    FROM dbaas.backups b
    """
    Then it returns "2" rows matches
    """
      - backup_id:     backup_id1
        status:        DONE
        initiator:     SCHEDULE
        method:        FULL
        subcid:        subcid1
        shard_id:      shard_id1
        created_at:    1970-01-01T00:00:35.403547+00:00
        delayed_until: 1970-01-01T00:00:35.403547+00:00
        started_at:    1970-01-01T00:00:35.403547+00:00
        finished_at:   1970-01-01T00:00:36.403547+00:00
        updated_at:    1970-01-01T00:00:36.403547+00:00
      - backup_id:     backup_id2
        status:        DONE
        initiator:     USER
        method:        FULL
        subcid:        subcid1
        shard_id:      shard_id1
        created_at:    1970-01-01T00:00:45.403547+00:00
        delayed_until: 1970-01-01T00:00:45.403547+00:00
        started_at:    1970-01-01T00:00:45.403547+00:00
        finished_at:   1970-01-01T00:00:56.403547+00:00
        updated_at:    1970-01-01T00:00:56.403547+00:00
    """
    And I list backups for "mongodb_01" cluster and got
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:56+00:00",
                "folderId": "folder1",
                "id": "cid1:backup_id2",
                "size": 45,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["rs01"],
                "startedAt": "1970-01-01T00:00:45+00:00",
                "type": "MANUAL"
            },
            {
                "createdAt": "1970-01-01T00:00:36+00:00",
                "folderId": "folder1",
                "id": "cid1:backup_id1",
                "size": 35,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["rs01"],
                "startedAt": "1970-01-01T00:00:35+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """

  @skipdups
  Scenario: Skip dups flag import oldest backup and completes successfully
    Given s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                    "CommonPrefixes": [
                        {
                            "Prefix": "mongodb-backup/cid1/shard_id1"
                        }
                    ]
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000035Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:36.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000045Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:56.403547+03:00"
                        }
                    ]
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "StartLocalTime": "1970-01-01T03:00:35.403547+03:00",
                        "FinishLocalTime": "1970-01-01T03:00:36.403547+03:00",
                        "MongoMeta": {
                            "Before": {
                                "LastMajTS": {
                                    "TS": 35,
                                    "Inc": 10
                                }
                            },
                            "After": {
                                "LastMajTS": {
                                    "TS": 36,
                                    "Inc": 60
                                }
                            }
                        },
                        "UserData": {
                            "backup_id": "backup_id10000",
                            "shard_name": "rs01"
                        },
                        "Permanent": false,
                        "DataSize": 35
                    }
                }
            },
            {
               "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000035Z/stream.br",
                            "Size": 35
                        }
                    ]
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "StartLocalTime": "1970-01-01T03:00:45.403547+03:00",
                        "FinishLocalTime": "1970-01-01T03:00:56.403547+03:00",
                        "MongoMeta": {
                            "Before": {
                                "LastMajTS": {
                                    "TS": 45,
                                    "Inc": 10
                                }
                            },
                            "After": {
                                "LastMajTS": {
                                    "TS": 56,
                                    "Inc": 60
                                }
                            }
                        },
                        "UserData": {
                            "backup_id": "none",
                            "shard_name": "rs01"
                        },
                        "Permanent": false,
                        "DataSize": 45
                    }
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000045Z/stream.br",
                            "Size": 45
                        }
                    ]
                }
            }
        ]
    }
    """
    When I execute mdb-backup-cli for cluster "mongodb_01" to "import s3 backups ignoring sched date dups" and got
    """
    {"CompletedCreation": 0, "ExistsInMetadb": 0, "ExistsInStorage": 2, "ImportedIntoMetadb": 1, "SkippedDueToDryRun": 0, "SkippedDueToExistence": 0, "SkippedDueToUniqSchedDate": 1, "SkippedNoIncrementBase": 0}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status, b.initiator, b.method, b.subcid, b.shard_id, b.created_at, b.delayed_until, b.started_at, b.finished_at, b.updated_at
    FROM dbaas.backups b
    """
    Then it returns "1" rows matches
    """
      - backup_id:     backup_id1
        status:        DONE
        initiator:     SCHEDULE
        method:        FULL
        subcid:        subcid1
        shard_id:      shard_id1
        created_at:    1970-01-01T00:00:35.403547+00:00
        delayed_until: 1970-01-01T00:00:35.403547+00:00
        started_at:    1970-01-01T00:00:35.403547+00:00
        finished_at:   1970-01-01T00:00:36.403547+00:00
        updated_at:    1970-01-01T00:00:36.403547+00:00
    """
    And I list backups for "mongodb_01" cluster and got
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:36+00:00",
                "folderId": "folder1",
                "id": "cid1:backup_id1",
                "size": 35,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["rs01"],
                "startedAt": "1970-01-01T00:00:35+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """

  @dryrun
  Scenario: Dry run does not import backups
    And s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                    "CommonPrefixes": [
                        {
                            "Prefix": "mongodb-backup/cid1/shard_id1"
                        }
                    ]
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000035Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:36.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000035Z/stream.br",
                            "Size": 35
                        }
                    ]
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "MongoMeta": {
                            "Before": {
                                "LastMajTS": {
                                    "TS": 35,
                                    "Inc": 10
                                }
                            },
                            "After": {
                                "LastMajTS": {
                                    "TS": 36,
                                    "Inc": 60
                                }
                            }
                        },
                        "UserData": {
                            "backup_id": "none",
                            "shard_name": "rs01"
                        },
                        "DataSize": 35
                    }
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard_id1/basebackups_005/stream_19700101T000035Z/stream.br",
                            "Size": 35
                        }
                    ]
                }
            }
        ]
    }
    """
    When I execute mdb-backup-cli for cluster "mongodb_01" to "import s3 backups with dry run" and got
    """
    {"CompletedCreation": 0, "ExistsInMetadb": 0, "ExistsInStorage": 1, "ImportedIntoMetadb": 0, "SkippedDueToDryRun": 1, "SkippedDueToExistence": 0, "SkippedDueToUniqSchedDate": 0, "SkippedNoIncrementBase": 0}
    """
    Then in metadb there are "0" backups


  @backup_service_usage
  Scenario: CLI disables and enables backup service usage for cluster
    When I execute mdb-backup-cli for cluster "mongodb_01" to "disable backup-service"
    Then backup service is "disabled" for "mongodb_01" cluster
    When I execute mdb-backup-cli for cluster "mongodb_01" to "enable backup-service"
    Then backup service is "enabled" for "mongodb_01" cluster

  @roller
  Scenario: CLI rolls metadata on cluster hosts
    Given all deploy shipments get status is "DONE"
    When I execute mdb-backup-cli for cluster "mongodb_01" to "roll metadata"
