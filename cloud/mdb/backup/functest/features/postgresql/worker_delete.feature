Feature: PostgreSQL backup worker deletion works

  Scenario: Backup deletion works properly
    When I create "2" postgresql clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
          ('man01', 'OBSOLETE', now() + INTERVAL '1 week', NULL, 'USER', '{"name": "base_0000000800010C2B000000AF", "compressed_size": 10, "meta": {"start_lsn": 100, "finish_lsn": 150}}'::jsonb),
          ('auto01', 'OBSOLETE', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE', '{"name": "base_0000000800010C2B0000000F", "compressed_size": 5, "meta": {"start_lsn": 10, "finish_lsn": 50}}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
          ('auto02', 'OBSOLETE', now() - INTERVAL '1 week', now() - INTERVAL '1 week', 'SCHEDULE', '{"name": "base_0000000800010C2B0000001F", "compressed_size": 5, "meta": {"start_lsn": 5, "finish_lsn": 10}}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_02';
    """
    And I execute mdb-backup-worker with config
    """
    {obsolete: {enabled: true, queue_producer: {max_tasks: 2}, queue_handler: {parallel: 1}, fraction_delayer: {min_delay: 1s, default_delay: 1s, fractions: null}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "3" rows matches
    """
      - backup_id: auto01
        backup_status: DELETING
        shipment_id: '1'
      - backup_id: auto02
        backup_status: DELETING
        shipment_id: '2'
      - backup_id: man01
        backup_status: OBSOLETE
        shipment_id: NULL
    """
    Given all deploy shipments get status is "DONE"
    And s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                    "Contents": []
                }
            },
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
    {deleting: {enabled: true, queue_producer: {max_tasks: 2}, queue_handler: {parallel: 1}, fraction_delayer: {min_delay: 1s, default_delay: 1s, fractions: null}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "3" rows matches
    """
      - backup_id: auto01
        backup_status: DELETED
        shipment_id: '1'
      - backup_id: auto02
        backup_status: DELETED
        shipment_id: '2'
      - backup_id: man01
        backup_status: OBSOLETE
        shipment_id: NULL
    """


  Scenario: Obsolete backup delayed due to deploy fails
    Given all deploy shipments creation fails
    When I create "1" postgresql clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
          ('man01', 'OBSOLETE', now(), NULL, 'USER', '{"name": "bname1", "root_path": "root"}'::jsonb),
          ('auto01', 'OBSOLETE', now() - INTERVAL '3 hours', now() - INTERVAL '3 hours', 'SCHEDULE', '{"name": "bname2", "root_path": "root"}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
    And I execute mdb-backup-worker with config
    """
    {obsolete: {enabled: true, queue_producer: {max_tasks: 2}, queue_handler: {parallel: 1}, fraction_delayer: {min_delay: 1s, default_delay: 1s, fractions: null}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_id: man01
        backup_status: OBSOLETE
        shipment_id: null
      - backup_id: auto01
        backup_status: OBSOLETE
        shipment_id: null
    """

  Scenario: Deleting backup delayed and resumed due to deploy status
    Given all deploy shipments get status is "INPROGRESS"
    When I create "1" postgresql clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, started_at, updated_at, cid, subcid, shard_id, shipment_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, shipment_id, t.metadata
        FROM (VALUES
          ('man01', 'DELETING', now(), NULL, 'USER', '1', '{"name": "base_0000000800010C2B000000AF", "compressed_size": 10, "meta": {"start_lsn": 100, "finish_lsn": 150}}'::jsonb),
          ('auto01', 'DELETING', now() - INTERVAL '2 hours', now() - INTERVAL '2 hours', 'SCHEDULE', '2', '{"name": "base_0000000800010C2B0000001F", "compressed_size": 5, "meta": {"start_lsn": 5, "finish_lsn": 10}}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, shipment_id, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
    And I execute mdb-backup-worker with config
    """
    {deleting: {enabled: true, queue_producer: {max_tasks: 2}, queue_handler: {parallel: 1}, fraction_delayer: {min_delay: 1s, default_delay: 1s, fractions: null}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_id: man01
        backup_status: DELETING
        shipment_id: '1'
      - backup_id: auto01
        backup_status: DELETING
        shipment_id: '2'
    """
    Given all deploy shipments get status is "DONE"
    And s3 responses sequence
    """
    {
        "Responses": [
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
                        "start_time": "1970-01-01T03:00:30.403547Z",
                        "finish_time": "1970-01-01T03:00:31.403547Z",
                        "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                        "pg_version": 100000,
                        "compressed_size": 1,
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
                        "CompressedSize": 1,
                        "Spec": null,
                        "UserData": {
                            "backup_id": "man01"
                        }
                    }
                }
            }
        ]
    }
    """
    When I execute mdb-backup-worker with config
    """
    {deleting: {enabled: true, queue_producer: {max_tasks: 1}, queue_handler: {parallel: 1}, fraction_delayer: {min_delay: 1s, default_delay: 1s, fractions: null}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_id: man01
        backup_status: DELETING
        shipment_id: '1'
      - backup_id: auto01
        backup_status: DELETED
        shipment_id: '2'
    """
    When I execute mdb-backup-worker with config
    """
    {deleting: {enabled: true, queue_producer: {max_tasks: 1}, queue_handler: {parallel: 1}, fraction_delayer: {min_delay: 1s, default_delay: 1s, fractions: null}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_id: man01
        backup_status: DELETE-ERROR
        shipment_id: '1'
      - backup_id: auto01
        backup_status: DELETED
        shipment_id: '2'
    """
