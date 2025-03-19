Feature: PostgreSQL backup scheduler purge works

  Scenario: Purge backups works properly
    When I create "1" postgresql clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
        FROM (VALUES
          ('man01', 'DELETED', now() - INTERVAL '3 weeks', NULL, 'USER'),
          ('auto01', 'DELETED', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE'),
          ('auto02', 'CREATE-ERROR', now() - INTERVAL '3 weeks', now() - INTERVAL '1 weeks', 'SCHEDULE'),
          ('auto03', 'DONE', now() - INTERVAL '5 days', now() - INTERVAL '5 days', 'SCHEDULE'),
          ('auto04', 'DELETING', now() - INTERVAL '4 days', now() - INTERVAL '4 days', 'SCHEDULE'))
            AS t (bid, status, ts, scheduled_date, initiator),
      	  	dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
    And I execute mdb-backup-scheduler for ctype "postgresql_cluster" to purge
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, c.name cluster_name, sc.name as subcluster_name
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid)
    """
    Then it returns "3" rows matches
    """
      - backup_id: auto02
        backup_status: CREATE-ERROR
      - backup_id: auto03
        backup_status: DONE
      - backup_id: auto04
        backup_status: DELETING
    """
