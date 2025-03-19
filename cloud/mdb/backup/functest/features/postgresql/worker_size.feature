Feature: PostgreSQL backup worker updates sizes

  Scenario: Backup creation and deletion works properly and updates sizes
    When I create "1" postgresql clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, started_at, updated_at, cid, subcid, shard_id, shipment_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, shipment_id, t.metadata
        FROM (VALUES
          ('auto01', 'CREATING', now() - INTERVAL '3 hours', now() - INTERVAL '2 hours', 'SCHEDULE', '1', '{}'::jsonb)
          )
            AS t (bid, status, ts, scheduled_date, initiator, shipment_id, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
    And all deploy shipments get status is "DONE"
    And s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                        "Contents": [
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000002_backup_stop_sentinel.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000002/stream.br",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000002/metadata.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            }
                        ]
                    }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_lsn": 33554432,
                        "finish_lsn": 50331648,
                        "start_time": "1970-01-01T03:00:30.403547Z",
                        "finish_time": "1970-01-01T03:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 100,
                        "is_permanent": true,
                        "user_data": {
                            "backup_id": "auto01"
                        }
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 33554432,
                        "PgVersion": 100000,
                        "FinishLSN": 50331648,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262569242,
                        "CompressedSize": 100,
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
                                "Key": "wal-e/cid1/1000/wal_005/000000010000000000000002.br",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00",
                                "Size": 1
                            },
                            {
                                "Key": "wal-e/cid1/1000/wal_005/000000010000000000000004.br",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00",
                                "Size": 1
                            }
                        ]
                    }
            }
        ]
    }
    """
    When I execute mdb-backup-worker with config
    """
    {creating: {enabled: true, update_sizes: true, assert_update_sizes_errors: true, queue_producer: {max_tasks: 1}, fraction_delayer: {min_delay: 1s, fractions: {1: 1s}}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "1" rows matches
    """
      - backup_id: auto01
        backup_status: DONE
        shipment_id: '1'
    """
    And I execute query
    """
    SELECT backup_id, data_size, journal_size
    FROM dbaas.backups_history
    """
    Then it returns "1" rows matches
    """
      - backup_id: auto01
        data_size: 100
        journal_size: 1
    """
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, started_at, updated_at, cid, subcid, shard_id, shipment_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, shipment_id, t.metadata
        FROM (VALUES
          ('auto02', 'CREATING', now() - INTERVAL '3 hours', now() - INTERVAL '2 hours', 'SCHEDULE', '2', '{}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, shipment_id, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
    And all deploy shipments get status is "DONE"
    And s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                        "Contents": [
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000005_backup_stop_sentinel.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000005/stream.br",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            },
                            {
                                "Key": "wal-e/cid1/1000/basebackups_005/base_000000010000000000000005/metadata.json",
                                "LastModified": "1970-01-01T03:00:31.403547+03:00"
                            }
                        ]
                    }
            },
            {
                "GetObject": {
                    "Body": {
                        "start_lsn": 83886080,
                        "finish_lsn": 117440512,
                        "start_time": "1970-01-01T03:00:30.403547Z",
                        "finish_time": "1970-01-01T03:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 200,
                        "is_permanent": true,
                        "user_data": {
                            "backup_id": "auto02"
                        }
                    }
                }
            },
            {
                "GetObject": {
                    "Body": {
                        "LSN": 83886080,
                        "PgVersion": 100000,
                        "FinishLSN": 117440512,
                        "SystemIdentifier": 6952858960630391265,
                        "UncompressedSize": 262569242,
                        "CompressedSize": 200,
                        "UserData": {
                            "backup_id": "auto02"
                        }
                    }
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "wal-e/cid1/1000/wal_005/000000010000000000000002.br",
                            "LastModified": "1970-01-01T03:00:31.403547+03:00",
                            "Size": 1
                        },
                        {
                            "Key": "wal-e/cid1/1000/wal_005/000000010000000000000004.br",
                            "LastModified": "1970-01-01T03:00:31.403547+03:00",
                            "Size": 1
                        },
                        {
                            "Key": "wal-e/cid1/1000/wal_005/000000010000000000000006.br",
                            "LastModified": "1970-01-01T03:00:31.403547+03:00",
                            "Size": 1
                        },
                        {
                            "Key": "wal-e/cid1/1000/wal_005/000000010000000000000009.br",
                            "LastModified": "1970-01-01T03:00:31.403547+03:00",
                            "Size": 1
                        }
                    ]
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "wal-e/cid1/1000/wal_005/000000010000000000000002.br",
                            "LastModified": "1970-01-01T03:00:31.403547+03:00",
                            "Size": 1
                        },
                        {
                            "Key": "wal-e/cid1/1000/wal_005/000000010000000000000004.br",
                            "LastModified": "1970-01-01T03:00:31.403547+03:00",
                            "Size": 1
                        },
                        {
                            "Key": "wal-e/cid1/1000/wal_005/000000010000000000000006.br",
                            "LastModified": "1970-01-01T03:00:31.403547+03:00",
                            "Size": 1
                        },
                        {
                            "Key": "wal-e/cid1/1000/wal_005/000000010000000000000009.br",
                            "LastModified": "1970-01-01T03:00:31.403547+03:00",
                            "Size": 1
                        }
                    ]
                }
            }
        ]
    }
    """
    When I execute mdb-backup-worker with config
    """
    {creating: {enabled: true, update_sizes: true, assert_update_sizes_errors: true, queue_producer: {max_tasks: 1}, fraction_delayer: {min_delay: 1s, fractions: {1: 1s}}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_id: auto01
        backup_status: DONE
        shipment_id: '1'
      - backup_id: auto02
        backup_status: DONE
        shipment_id: '2'
    """
    And I execute query
    """
    SELECT backup_id, data_size, journal_size
    FROM dbaas.backups_history
    """
    Then it returns "3" rows matches
    """
      - backup_id: auto01
        data_size: 100
        journal_size: 1
      - backup_id: auto02
        data_size: 200
        journal_size: 1
      - backup_id: auto01
        data_size: 100
        journal_size: 2
    """
    And I successfully execute query
    """
    UPDATE dbaas.backups
    SET status = 'DELETING'
    WHERE backup_id = 'auto01'
    """
    Given s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                    "Contents": []
                }
            }
        ]
    }
    """
    When I execute mdb-backup-worker with config
    """
    {deleting: {enabled: true, update_sizes: true, assert_update_sizes_errors: true, queue_producer: {max_tasks: 1}, fraction_delayer: {min_delay: 1s, fractions: {1: 1s}}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_id: auto01
        backup_status: DELETED
        shipment_id: '1'
      - backup_id: auto02
        backup_status: DONE
        shipment_id: '2'
    """
    And I execute query
    """
    SELECT backup_id, data_size, journal_size
    FROM dbaas.backups_history
    """
    Then it returns "4" rows matches
    """
      - backup_id: auto01
        data_size: 100
        journal_size: 1
      - backup_id: auto02
        data_size: 200
        journal_size: 1
      - backup_id: auto01
        data_size: 100
        journal_size: 2
      - backup_id: auto01
        data_size: 0
        journal_size: 0
    """
