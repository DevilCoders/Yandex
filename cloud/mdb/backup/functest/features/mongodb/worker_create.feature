Feature: MongoDB backup worker creation works

  @create
  Scenario: Backup creation works properly
    When I create "1" mongodb clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
        FROM (VALUES
          ('man01', 'PLANNED', now(), NULL, 'USER'),
          ('auto01', 'PLANNED', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE'),
          ('auto02', 'PLANNED', now() + INTERVAL '1 week', now() + INTERVAL '1 week', 'SCHEDULE'))
            AS t (bid, status, ts, scheduled_date, initiator),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'mongodb_01';
    """
    And I execute mdb-backup-worker with config
    """
    {planned: {enabled: true, queue_producer: {max_tasks: 2}, queue_handler: {parallel: 1}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "3" rows matches
    """
      - backup_id: man01
        backup_status: CREATING
        shipment_id: '2'
      - backup_id: auto01
        backup_status: CREATING
        shipment_id: '1'
      - backup_id: auto02
        backup_status: PLANNED
        shipment_id: null
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
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000030Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:31.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000030Z/stream.br",
                            "Size": 30
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:36.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z/stream.br",
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
                            "backup_id": "man01"
                        },
                        "Permanent": false,
                        "DataSize": 30
                    }
                }
            },
            {
               "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000030Z/stream.br",
                            "Size": 30
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
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z/stream.br",
                            "Size": 35
                        }
                    ]
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000030Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:31.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000030Z/stream.br",
                            "Size": 30
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:36.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z/stream.br",
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
                            "backup_id": "man01"
                        },
                        "Permanent": false,
                        "DataSize": 30
                    }
                }
            },
            {
               "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000030Z/stream.br",
                            "Size": 30
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
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z/stream.br",
                            "Size": 35
                        }
                    ]
                }
            }
        ]
    }
    """
    When I execute mdb-backup-worker with config
    """
    {creating: {enabled: true, queue_producer: {max_tasks: 2}, queue_handler: {parallel: 1}, fraction_delayer: {default_delay: 1s, min_delay: 1s, fractions: null}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "3" rows matches
    """
      - backup_id: man01
        backup_status: DONE
        shipment_id: '2'
      - backup_id: auto01
        backup_status: DONE
        shipment_id: '1'
      - backup_id: auto02
        backup_status: PLANNED
        shipment_id: null
    """


  Scenario: Planned backup delayed due to deploy fails
    Given all deploy shipments creation fails
    When I create "1" mongodb clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
        FROM (VALUES
          ('man01', 'PLANNED', now(), NULL, 'USER'),
          ('auto01', 'PLANNED', now() - INTERVAL '3 hours', now() - INTERVAL '3 hours', 'SCHEDULE'))
            AS t (bid, status, ts, scheduled_date, initiator),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'mongodb_01';
    """
    And I execute mdb-backup-worker with config
    """
    {planned: {enabled: true, queue_producer: {max_tasks: 2}, queue_handler: {parallel: 1}, fraction_delayer: {min_delay: 1s, default_delay: 1s, fractions: null}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_id: man01
        backup_status: PLANNED
        shipment_id: null
      - backup_id: auto01
        backup_status: PLANNED
        shipment_id: null
    """


  Scenario: Creating backup delayed and resumed due to deploy status
    Given all deploy shipments get status is "INPROGRESS"
    When I create "1" mongodb clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, started_at, updated_at, cid, subcid, shard_id, shipment_id)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, shipment_id
        FROM (VALUES
          ('man01', 'CREATING', now(), NULL, 'USER', '1'),
          ('auto01', 'CREATING', now() - INTERVAL '3 hours', now() - INTERVAL '2 hours', 'SCHEDULE', '2'))
            AS t (bid, status, ts, scheduled_date, initiator, shipment_id),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'mongodb_01';
    """
    And I execute mdb-backup-worker with config
    """
    {creating: {enabled: true, queue_producer: {max_tasks: 2}, queue_handler: {parallel: 1}, fraction_delayer: {min_delay: 1s, default_delay: 1s, fractions: null}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_id: man01
        backup_status: CREATING
        shipment_id: '1'
      - backup_id: auto01
        backup_status: CREATING
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
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:36.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z/stream.br",
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
                            "backup_id": "auto01"
                        },
                        "DataSize": 35
                    }
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z/stream.br",
                            "Size": 35
                        }
                    ]
                }
            }
        ]
    }
    """
    When I execute mdb-backup-worker with config
    """
    {creating: {enabled: true, queue_producer: {max_tasks: 1}, queue_handler: {parallel: 1}, fraction_delayer: {min_delay: 1s, default_delay: 1s, fractions: null}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_id: man01
        backup_status: CREATING
        shipment_id: '1'
      - backup_id: auto01
        backup_status: DONE
        shipment_id: '2'
    """
    And s3 responses sequence
    """
    {
        "Responses": [
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z_backup_stop_sentinel.json",
                            "LastModified": "1970-01-01T03:00:36.403547+03:00"
                        },
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z/stream.br",
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
                            "backup_id": "auto01"
                        },
                        "DataSize": 35
                    }
                }
            },
            {
                "ListObjects": {
                    "Contents": [
                        {
                            "Key": "mongodb-backup/cid1/shard1_id/basebackups_005/stream_19700101T000035Z/stream.br",
                            "Size": 35
                        }
                    ]
                }
            }
        ]
    }
    """
    When I execute mdb-backup-worker with config
    """
    {creating: {enabled: true, queue_producer: {max_tasks: 1}, queue_handler: {parallel: 1}, fraction_delayer: {min_delay: 1s, default_delay: 1s, fractions: null}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_id: man01
        backup_status: CREATE-ERROR
        shipment_id: '1'
      - backup_id: auto01
        backup_status: DONE
        shipment_id: '2'
    """
