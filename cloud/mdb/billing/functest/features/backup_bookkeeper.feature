Feature: Backup billing bookkeeper works

  Scenario: Backup bookkeeper works properly
    When I create "1" postgresql clusters
    And I successfully execute query in metadb
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
          ('man01', 'DONE', now() - INTERVAL '3 weeks', NULL, 'USER', '{"compressed_size": 100}'::jsonb),
          ('auto01', 'DONE', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE', '{"compressed_size": 200}'::jsonb),
          ('auto02', 'CREATE-ERROR', now() - INTERVAL '1 week', now() - INTERVAL '1 week', 'SCHEDULE', NULL),
          ('auto03', 'DONE', now() - INTERVAL '5 days', now() - INTERVAL '5 days', 'SCHEDULE', '{"compressed_size": 64424509440}'::jsonb),
          ('auto04', 'DELETING', now() - INTERVAL '4 days', now() - INTERVAL '4 days', 'SCHEDULE', '{"compressed_size": 500}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
    And I enable backup billing for all clusters
    And I execute mdb-billing-bookkeeper to bill backups
    And I execute query in billingdb
    """
    SELECT bill_type, restart_count, seq_no, finished_at IS NOT NULL as sent FROM billing.metrics_queue
    """
    Then it returns from billingdb one row matches
    """
    bill_type: BACKUP
    sent: false
    restart_count: 0
    seq_no: 1
    """
    And I execute mdb-billing-bookkeeper to bill backups
    And I execute query in billingdb
    """
    SELECT bill_type, restart_count, seq_no, finished_at IS NOT NULL as sent FROM billing.metrics_queue
    """
    Then it returns from billingdb "2" rows matches
    """
    - bill_type: BACKUP
      restart_count: 0
      sent: false
      seq_no: 1
    - bill_type: BACKUP
      restart_count: 0
      sent: false
      seq_no: 2
    """
