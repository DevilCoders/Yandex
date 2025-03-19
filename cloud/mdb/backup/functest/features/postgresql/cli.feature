Feature: PostgreSQL backup import works

  Background: Empty databases
    When I create "2" postgresql clusters
    And I execute mdb-backup-cli for cluster "postgresql_01" to "enable backup-service"
    And I execute mdb-backup-cli for cluster "postgresql_02" to "enable backup-service"

  @import
  Scenario: Backup import works
    Given databases timezone set to "UTC"
    Given s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                        "CommonPrefixes": [
                            {
                                "Prefix": "wal-e/cid1/1000"
                            }
                        ]
                }
            },
            {
                "ListObjects": {
                        "Contents": [
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E_backup_stop_sentinel.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/stream.br",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/metadata.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            }
                        ]
                    }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-01T00:00:30.403547Z",
                        "finish_time": "1970-01-01T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1337,
                        "is_permanent": true,
                        "user_data": {
                            "backup_id": "man01"
                        }
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 234881240,
                        "PgVersion": 100000,
                        "FinishLSN": 234881512,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262569242,
                        "CompressedSize": 1337,
                        "Spec": null
                    }
                }
            }
        ]
    }
    """
    When I execute mdb-backup-cli for cluster "postgresql_01" to "import s3 backups" and got
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
        initiator:     USER
        method:        FULL
        shard_id:      NULL
        subcid:        subcid1
        created_at:    1970-01-01T00:00:30.403547+00:00
        delayed_until: 1970-01-01T00:00:30.403547+00:00
        finished_at:   1970-01-01T00:00:31.403547+00:00
        started_at:    1970-01-01T00:00:30.403547+00:00
        updated_at:    1970-01-01T00:00:31.403547+00:00
    """
    And I list backups for "postgresql_01" cluster and got
    """
    {
        "backups": [
            {
                "createdAt":        "1970-01-01T00:00:31.403547+00:00",
                "folderId":         "folder1",
                "id":               "cid1:backup_id1",
                "size":             1337,
                "sourceClusterId":  "cid1",
                "startedAt":        "1970-01-01T00:00:30.403547+00:00",
                "type":             "MANUAL",
                "sourceShardNames": null
            }
        ]
    }
    """

  @import_old_majors
  Scenario: Backup import works for old major versions
    Given databases timezone set to "UTC"
    Given s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                        "CommonPrefixes": [
                            {
                                "Prefix": "wal-e/cid1/900"
                            },
                            {
                                "Prefix": "wal-e/cid1/1000"
                            }
                        ]
                }
            },
            {
                "ListObjects": {
                        "Contents": [
                            {
                                "Key": "wal-e/cid1/900/basebackups_005/base_00000001000000000000000D_backup_stop_sentinel.json",
                                "LastModified": "1970-01-01T03:00:30.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/900/basebackups_005/base_00000001000000000000000D/stream.br",
                                "LastModified": "1970-01-01T03:00:30.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/900/basebackups_005/base_00000001000000000000000D/metadata.json",
                                "LastModified": "1970-01-01T03:00:30.403547+03:00"
                            }
                        ]
                    }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-01T00:00:29.403547Z",
                        "finish_time": "1970-01-01T00:00:30.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 90000,
                        "compressed_size": 1337,
                        "is_permanent": true,
                        "user_data": {
                            "backup_id": "oldmajor01"
                        }
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 134881240,
                        "PgVersion": 90000,
                        "FinishLSN": 134881512,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262569242,
                        "CompressedSize": 1337,
                        "Spec": null
                    }
                }
            },
            {
                "ListObjects": {
                        "Contents": [
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E_backup_stop_sentinel.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/stream.br",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/metadata.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            }
                        ]
                    }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-01T00:00:30.403547Z",
                        "finish_time": "1970-01-01T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1337,
                        "is_permanent": true,
                        "user_data": {
                            "backup_id": "man01"
                        }
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 234881240,
                        "PgVersion": 100000,
                        "FinishLSN": 234881512,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262569242,
                        "CompressedSize": 1337,
                        "Spec": null
                    }
                }
            }
        ]
    }
    """
    When I execute mdb-backup-cli for cluster "postgresql_01" to "import s3 backups" and got
    """
    {"CompletedCreation": 0, "ExistsInMetadb": 0, "ExistsInStorage": 2, "ImportedIntoMetadb": 2, "SkippedDueToDryRun": 0, "SkippedDueToExistence": 0, "SkippedDueToUniqSchedDate": 0, "SkippedNoIncrementBase": 0}
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
        initiator:     USER
        method:        FULL
        shard_id:      NULL
        subcid:        subcid1
        created_at:    1970-01-01T00:00:29.403547+00:00
        delayed_until: 1970-01-01T00:00:29.403547+00:00
        finished_at:   1970-01-01T00:00:30.403547+00:00
        started_at:    1970-01-01T00:00:29.403547+00:00
        updated_at:    1970-01-01T00:00:30.403547+00:00
      - backup_id:     backup_id2
        status:        DONE
        initiator:     USER
        method:        FULL
        shard_id:      NULL
        subcid:        subcid1
        created_at:    1970-01-01T00:00:30.403547+00:00
        delayed_until: 1970-01-01T00:00:30.403547+00:00
        finished_at:   1970-01-01T00:00:31.403547+00:00
        started_at:    1970-01-01T00:00:30.403547+00:00
        updated_at:    1970-01-01T00:00:31.403547+00:00
    """
    And I list backups for "postgresql_01" cluster and got
    """
    {
        "backups": [
            {
                "createdAt":        "1970-01-01T00:00:31.403547+00:00",
                "folderId":         "folder1",
                "id":               "cid1:backup_id2",
                "size":             1337,
                "sourceClusterId":  "cid1",
                "startedAt":        "1970-01-01T00:00:30.403547+00:00",
                "type":             "MANUAL",
                "sourceShardNames": null
            },
            {
                "createdAt":        "1970-01-01T00:00:30.403547+00:00",
                "folderId":         "folder1",
                "id":               "cid1:backup_id1",
                "size":             1337,
                "sourceClusterId":  "cid1",
                "startedAt":        "1970-01-01T00:00:29.403547+00:00",
                "type":             "MANUAL",
                "sourceShardNames": null
            }
        ]
    }
    """

  @batch_import
  Scenario: Batch backup import works
    Given databases timezone set to "UTC"
    Given s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                        "CommonPrefixes": [
                            {
                                "Prefix": "wal-e/cid1/1000"
                            }
                        ]
                }
            },
            {
                "ListObjects": {
                        "Contents": [
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E_backup_stop_sentinel.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/stream.br",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/metadata.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            }
                        ]
                    }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-01T00:00:30.403547Z",
                        "finish_time": "1970-01-01T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1337,
                        "is_permanent": true,
                        "user_data": {
                            "backup_id": "man01"
                        }
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 234881240,
                        "PgVersion": 100000,
                        "FinishLSN": 234881512,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262569242,
                        "CompressedSize": 1337,
                        "Spec": null
                    }
                }
            },
            {
                "ListObjects": {
                        "CommonPrefixes": [
                            {
                                "Prefix": "wal-e/cid1/1000"
                            }
                        ]
                }
            },
            {
                "ListObjects": {
                        "Contents": [
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E_backup_stop_sentinel.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/stream.br",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/metadata.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            }
                        ]
                    }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-01T00:00:30.403547Z",
                        "finish_time": "1970-01-01T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1337,
                        "is_permanent": true,
                        "user_data": {
                            "backup_id": "man02"
                        }
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 234881240,
                        "PgVersion": 100000,
                        "FinishLSN": 234881512,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262569242,
                        "CompressedSize": 1337,
                        "Spec": null
                    }
                }
            }
        ]
    }
    """
    When I execute mdb-backup-cli to import backups for ctype "postgresql_cluster" with batch size 100 and interval "24h" and got
    """
    {"CompletedCreation": 0, "ExistsInMetadb": 0, "ExistsInStorage": 2, "ImportedIntoMetadb": 2, "SkippedDueToDryRun": 0, "SkippedDueToExistence": 0, "SkippedDueToUniqSchedDate": 0, "SkippedNoIncrementBase": 0}
    """
    And I execute query
    """
    SELECT b.cid, b.status, b.initiator, b.method, b.subcid, b.shard_id, b.created_at, b.delayed_until, b.started_at, b.finished_at, b.updated_at
    FROM dbaas.backups b
    """
    Then it returns "2" rows matches
    """
      - cid:           cid1
        status:        DONE
        initiator:     USER
        method:        FULL
        shard_id:      NULL
        subcid:        subcid1
        created_at:    1970-01-01T00:00:30.403547+00:00
        delayed_until: 1970-01-01T00:00:30.403547+00:00
        finished_at:   1970-01-01T00:00:31.403547+00:00
        started_at:    1970-01-01T00:00:30.403547+00:00
        updated_at:    1970-01-01T00:00:31.403547+00:00
      - cid:           cid2
        status:        DONE
        initiator:     USER
        method:        FULL
        shard_id:      NULL
        subcid:        subcid2
        created_at:    1970-01-01T00:00:30.403547+00:00
        delayed_until: 1970-01-01T00:00:30.403547+00:00
        finished_at:   1970-01-01T00:00:31.403547+00:00
        started_at:    1970-01-01T00:00:30.403547+00:00
        updated_at:    1970-01-01T00:00:31.403547+00:00
    """

  @import_incremental
    Scenario: Incremental backup import works
    Given databases timezone set to "UTC"
    Given s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                        "CommonPrefixes": [
                            {
                                "Prefix": "wal-e/cid1/1000"
                            }
                        ]
                }
            },
            {
                "ListObjects": {
                        "Contents": [
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E_backup_stop_sentinel.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/stream.br",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/metadata.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000017_D_00000001000000000000000E_backup_stop_sentinel.json",
                                "LastModified": "1970-01-02T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000017_D_00000001000000000000000E/stream.br",
                                "LastModified": "1970-01-02T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000017_D_00000001000000000000000E/metadata.json",
                                "LastModified": "1970-01-02T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000054_D_000000010000000000000052_backup_stop_sentinel.json",
                                "LastModified": "1970-01-04T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000054_D_000000010000000000000052/stream.br",
                                "LastModified": "1970-01-04T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000054_D_000000010000000000000052/metadata.json",
                                "LastModified": "1970-01-04T03:00:31.403547+03:00"
                            },
                                                        {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000067_D_000000010000000000000017_backup_stop_sentinel.json",
                                "LastModified": "1970-01-05T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000067_D_000000010000000000000017/stream.br",
                                "LastModified": "1970-01-05T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000067_D_000000010000000000000017/metadata.json",
                                "LastModified": "1970-01-05T03:00:31.403547+03:00"
                            }
                        ]
                    }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-01T00:00:30.403547Z",
                        "finish_time": "1970-01-01T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1337
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 234881240,
                        "PgVersion": 100000,
                        "FinishLSN": 234881512,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262569242,
                        "CompressedSize": 1337,
                        "Spec": null
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-02T00:00:30.403547Z",
                        "finish_time": "1970-01-02T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1338
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 385876008,
                        "DeltaFromLSN": 234881240,
                        "DeltaFrom": "base_00000001000000000000000E",
                        "DeltaFullName": "base_00000001000000000000000E",
                        "DeltaCount": 1,
                        "PgVersion": 100000,
                        "FinishLSN": 385876224,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262872348,
                        "CompressedSize": 1338,
                        "Spec": null
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-04T00:00:30.403547Z",
                        "finish_time": "1970-01-04T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1487
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 1409286184,
                        "DeltaFromLSN": 1375731752,
                        "DeltaFrom": "base_000000010000000000000052",
                        "DeltaFullName": "base_000000010000000000000052",
                        "DeltaCount": 1,
                        "PgVersion": 100000,
                        "FinishLSN": 1409286456,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 362872348,
                        "CompressedSize": 1487,
                        "Spec": null
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-05T00:00:30.403547Z",
                        "finish_time": "1970-01-05T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1987
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 1728053464,
                        "DeltaFromLSN": 385876008,
                        "DeltaFrom": "base_000000010000000000000017_D_00000001000000000000000E",
                        "DeltaFullName": "base_00000001000000000000000E",
                        "DeltaCount": 2,
                        "PgVersion": 100000,
                        "FinishLSN": 1728053736,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 362872348,
                        "CompressedSize": 1987,
                        "Spec": null
                    }
                }
            }
        ]
    }
    """
    When I execute mdb-backup-cli for cluster "postgresql_01" to "import s3 backups" and got
    """
    {"CompletedCreation": 0, "ExistsInMetadb": 0, "ExistsInStorage": 4, "ImportedIntoMetadb": 3, "SkippedDueToDryRun": 0, "SkippedDueToExistence": 0, "SkippedDueToUniqSchedDate": 0, "SkippedNoIncrementBase": 1}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status, b.initiator, b.method, b.subcid, b.shard_id, b.created_at, b.delayed_until, b.started_at, b.finished_at, b.updated_at
    FROM dbaas.backups b
    """
    Then it returns "3" rows matches
    """
      - backup_id:     backup_id1
        status:        DONE
        initiator:     SCHEDULE
        method:        FULL
        subcid:        subcid1
        shard_id:      NULL
        created_at:    1970-01-01T00:00:30.403547+00:00
        delayed_until: 1970-01-01T00:00:30.403547+00:00
        started_at:    1970-01-01T00:00:30.403547+00:00
        finished_at:   1970-01-01T00:00:31.403547+00:00
        updated_at:    1970-01-01T00:00:31.403547+00:00
      - backup_id:     backup_id2
        status:        DONE
        initiator:     SCHEDULE
        method:        INCREMENTAL
        subcid:        subcid1
        shard_id:      NULL
        created_at:    1970-01-02T00:00:30.403547+00:00
        delayed_until: 1970-01-02T00:00:30.403547+00:00
        started_at:    1970-01-02T00:00:30.403547+00:00
        finished_at:   1970-01-02T00:00:31.403547+00:00
        updated_at:    1970-01-02T00:00:31.403547+00:00
      - backup_id:     backup_id4
        status:        DONE
        initiator:     SCHEDULE
        method:        INCREMENTAL
        subcid:        subcid1
        shard_id:      NULL
        created_at:    1970-01-05T00:00:30.403547+00:00
        delayed_until: 1970-01-05T00:00:30.403547+00:00
        started_at:    1970-01-05T00:00:30.403547+00:00
        finished_at:   1970-01-05T00:00:31.403547+00:00
        updated_at:    1970-01-05T00:00:31.403547+00:00
    """
    And I list backups for "postgresql_01" cluster and got
    """
    {
        "backups": [
            {
                "createdAt":        "1970-01-05T00:00:31.403547+00:00",
                "folderId":         "folder1",
                "id":               "cid1:backup_id4",
                "size":             1987,
                "sourceClusterId":  "cid1",
                "startedAt":        "1970-01-05T00:00:30.403547+00:00",
                "type":             "AUTOMATED",
                "sourceShardNames": null
            },
            {
                "createdAt":        "1970-01-02T00:00:31.403547+00:00",
                "folderId":         "folder1",
                "id":               "cid1:backup_id2",
                "size":             1338,
                "sourceClusterId":  "cid1",
                "startedAt":        "1970-01-02T00:00:30.403547+00:00",
                "type":             "AUTOMATED",
                "sourceShardNames": null
            },
            {
                "createdAt":        "1970-01-01T00:00:31.403547+00:00",
                "folderId":         "folder1",
                "id":               "cid1:backup_id1",
                "size":             1337,
                "sourceClusterId":  "cid1",
                "startedAt":        "1970-01-01T00:00:30.403547+00:00",
                "type":             "AUTOMATED",
                "sourceShardNames": null
            }
        ]
    }
    """
    And I execute query
    """
    SELECT b.backup_id as bid, b.metadata->'id' as meta_bid, b.metadata->'meta'->'user_data'->'backup_id' as userdata_bid
    FROM dbaas.backups b
    """
    Then it returns "3" rows matches
    """
        - bid:          backup_id1
          meta_bid:     backup_id1
          userdata_bid: backup_id1
        - bid:          backup_id2
          meta_bid:     backup_id2
          userdata_bid: backup_id2
        - bid:          backup_id4
          meta_bid:     backup_id4
          userdata_bid: backup_id4
    """

  @secondrun
  Scenario: Second run does not handle imported backups
    Given databases timezone set to "UTC"
    Given s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                        "CommonPrefixes": [
                            {
                                "Prefix": "wal-e/cid1/1000"
                            }
                        ]
                }
            },
            {
                "ListObjects": {
                        "Contents": [
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E_backup_stop_sentinel.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/stream.br",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/metadata.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            }
                        ]
                    }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-01T00:00:30.403547Z",
                        "finish_time": "1970-01-01T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1337,
                        "is_permanent": true,
                        "user_data": {
                            "backup_id": "man01"
                        }
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 234881240,
                        "PgVersion": 100000,
                        "FinishLSN": 234881512,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262569242,
                        "CompressedSize": 1337,
                        "Spec": null
                    }
                }
            }
        ]
    }
    """
    When I execute mdb-backup-cli for cluster "postgresql_01" to "import s3 backups" and got
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
        initiator:     USER
        method:        FULL
        subcid:        subcid1
        shard_id:      NULL
        created_at:    1970-01-01T00:00:30.403547+00:00
        delayed_until: 1970-01-01T00:00:30.403547+00:00
        started_at:    1970-01-01T00:00:30.403547+00:00
        finished_at:   1970-01-01T00:00:31.403547+00:00
        updated_at:    1970-01-01T00:00:31.403547+00:00
    """
    And s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                        "CommonPrefixes": [
                            {
                                "Prefix": "wal-e/cid1/1000"
                            }
                        ]
                }
            },
            {
                "ListObjects": {
                        "Contents": [
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E_backup_stop_sentinel.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/stream.br",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/metadata.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000017_backup_stop_sentinel.json",
                                "LastModified": "1970-01-02T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000017/stream.br",
                                "LastModified": "1970-01-02T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000017/metadata.json",
                                "LastModified": "1970-01-02T03:00:31.403547+03:00"
                            }
                        ]
                    }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-01T00:00:30.403547Z",
                        "finish_time": "1970-01-01T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1337,
                        "is_permanent": true,
                        "user_data": {
                            "backup_id": "man01"
                        }
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 234881240,
                        "PgVersion": 100000,
                        "FinishLSN": 234881512,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262569242,
                        "CompressedSize": 1337,
                        "Spec": null
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-02T00:00:30.403547Z",
                        "finish_time": "1970-01-02T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1338,
                        "user_data": {
                            "backup_id": "auto01"
                        }
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 385876008,
                        "PgVersion": 100000,
                        "FinishLSN": 385876224,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262872348,
                        "CompressedSize": 1337,
                        "Spec": null
                    }
                }
            }
        ]
    }
    """
    When I execute mdb-backup-cli for cluster "postgresql_01" to "import s3 backups" and got
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
        initiator:     USER
        method:        FULL
        subcid:        subcid1
        shard_id:      NULL
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
        shard_id:      NULL
        created_at:    1970-01-02T00:00:30.403547+00:00
        delayed_until: 1970-01-02T00:00:30.403547+00:00
        started_at:    1970-01-02T00:00:30.403547+00:00
        finished_at:   1970-01-02T00:00:31.403547+00:00
        updated_at:    1970-01-02T00:00:31.403547+00:00
    """
    And I list backups for "postgresql_01" cluster and got
    """
    {
        "backups": [
            {
                "createdAt":        "1970-01-02T00:00:31.403547+00:00",
                "folderId":         "folder1",
                "id":               "cid1:backup_id2",
                "size":             1338,
                "sourceClusterId":  "cid1",
                "sourceShardNames": null,
                "startedAt":        "1970-01-02T00:00:30.403547+00:00",
                "type":             "AUTOMATED"
            },
            {
                "createdAt":        "1970-01-01T00:00:31.403547+00:00",
                "folderId":         "folder1",
                "id":               "cid1:backup_id1",
                "size":             1337,
                "sourceClusterId":  "cid1",
                "sourceShardNames": null,
                "startedAt":        "1970-01-01T00:00:30.403547+00:00",
                "type":             "MANUAL"
            }
        ]
    }
    """

  @complete_creating
  Scenario: Failed backups completion works
    Given databases timezone set to "UTC"
    Given s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                        "CommonPrefixes": [
                            {
                                "Prefix": "wal-e/cid1/1000"
                            }
                        ]
                }
            },
            {
                "ListObjects": {
                        "Contents": [
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E_backup_stop_sentinel.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/stream.br",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_00000001000000000000000E/metadata.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000017_backup_stop_sentinel.json",
                                "LastModified": "1970-01-02T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000017/stream.br",
                                "LastModified": "1970-01-02T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000017/metadata.json",
                                "LastModified": "1970-01-02T03:00:31.403547+03:00"
                            }
                        ]
                    }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-01T00:00:30.403547Z",
                        "finish_time": "1970-01-01T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1337,
                        "is_permanent": true,
                        "user_data": {
                            "backup_id": "man01"
                        }
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 234881240,
                        "PgVersion": 100000,
                        "FinishLSN": 234881512,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262569242,
                        "CompressedSize": 1337,
                        "Spec": null
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_time": "1970-01-02T00:00:30.403547Z",
                        "finish_time": "1970-01-02T00:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1338,
                        "user_data": {
                            "backup_id": "failed01"
                        }
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 385876008,
                        "PgVersion": 100000,
                        "FinishLSN": 385876224,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262872348,
                        "CompressedSize": 1337,
                        "Spec": null
                    }
                }
            }
        ]
    }
    """
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, started_at, cid, subcid)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.started_ts, c.cid, sc.subcid
        FROM (VALUES
          ('failed01', 'CREATE-ERROR',  date'1970-01-02', timestamp'1970-01-02 00:00:30.403547', timestamp'1970-01-02 00:00:30.999991', 'SCHEDULE'))
            AS t (bid, status, scheduled_date, ts, started_ts, initiator),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid)
        WHERE c.name = 'postgresql_01';
    """
    And I execute mdb-backup-cli for cluster "postgresql_01" to "import s3 backups and complete failed" and got
    """
    {"CompletedCreation": 1, "ExistsInMetadb": 1, "ExistsInStorage": 2, "ImportedIntoMetadb": 1, "SkippedDueToDryRun": 0, "SkippedDueToExistence": 0, "SkippedDueToUniqSchedDate": 0, "SkippedNoIncrementBase": 0}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status, b.initiator, b.method, b.subcid, b.shard_id, b.created_at, b.delayed_until, b.started_at, b.finished_at
    FROM dbaas.backups b
    """
    Then it returns "2" rows matches
    """
      - backup_id:     backup_id1
        status:        DONE
        initiator:     USER
        method:        FULL
        subcid:        subcid1
        shard_id:      NULL
        created_at:    1970-01-01T00:00:30.403547+00:00
        delayed_until: 1970-01-01T00:00:30.403547+00:00
        started_at:    1970-01-01T00:00:30.403547+00:00
        finished_at:   1970-01-01T00:00:31.403547+00:00
      - backup_id:     failed01
        status:        DONE
        initiator:     SCHEDULE
        method:        FULL
        subcid:        subcid1
        shard_id:      NULL
        created_at:    1970-01-02T00:00:30.403547+00:00
        delayed_until: 1970-01-02T00:00:30.403547+00:00
        started_at:    1970-01-02T00:00:30.999991+00:00
        finished_at:   1970-01-02T00:00:31.403547+00:00
    """
    And I list backups for "postgresql_01" cluster and got
    """
    {
        "backups": [
            {
                "createdAt":        "1970-01-02T00:00:31.403547+00:00",
                "folderId":         "folder1",
                "id":               "cid1:failed01",
                "size":             1338,
                "sourceClusterId":  "cid1",
                "sourceShardNames": null,
                "startedAt":        "1970-01-02T00:00:30.999991+00:00",
                "type":             "AUTOMATED"
            },
            {
                "createdAt":        "1970-01-01T00:00:31.403547+00:00",
                "folderId":         "folder1",
                "id":               "cid1:backup_id1",
                "size":             1337,
                "sourceClusterId":  "cid1",
                "sourceShardNames": null,
                "startedAt":        "1970-01-01T00:00:30.403547+00:00",
                "type":             "MANUAL"
            }
        ]
    }
    """

  @backup_service_usage
  Scenario: CLI disables and enables backup service usage for cluster
    When I execute mdb-backup-cli for cluster "postgresql_01" to "disable backup-service"
    Then backup service is "disabled" for "postgresql_01" cluster
    When I execute mdb-backup-cli for cluster "postgresql_01" to "enable backup-service"
    Then backup service is "enabled" for "postgresql_01" cluster

  @roller
  Scenario: CLI rolls metadata on cluster hosts
    Given all deploy shipments get status is "DONE"
    When I execute mdb-backup-cli for cluster "postgresql_01" to "roll metadata"
