Feature: PostgreSQL backup scheduler plan delta backups
  Scenario: PostgreSQL scheduler schedule delta backup properly
    When I create "1" postgresql clusters
    And I enable backup service for all existed clusters via cli
    And I set delta_max_steps for "cid1" to 5
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id, metadata)
        SELECT
            t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, t.method::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
        ('auto010', 'DONE', 'FULL', now() - INTERVAL '1 day', now() - INTERVAL '2 day', 'SCHEDULE', '{"id":"auto010", "name": "auto010", "is_incremental": false, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ"}'::jsonb),
        ('auto011', 'DONE', 'INCREMENTAL', now(), now() - INTERVAL '1 day', 'SCHEDULE', '{"id":"auto011", "name": "auto011", "is_incremental": true, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "meta": {"date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ", "user_data": {"backup_id": "auto011"}, "is_permanent": false }, "root_path": "wal-e/cid1/1000", "increment_details": {"delta_count": 1}}'::jsonb))
            AS t (bid, status, method, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
	And I successfully execute query
	"""
	INSERT INTO dbaas.backups_dependencies VALUES('auto010','auto011')
	"""
    And I set "postgresql_01" cluster backup window at "1h" from now
    And I execute mdb-backup-scheduler for ctype "postgresql_cluster" to plan with config
    """
    schedule_config:
      past_interval: 2h
      future_interval: 2h
    """
    When I execute query
    """
    SELECT b.status as backup_status, b.method as method
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid)
    """
    Then it returns "3" rows matches
    """
      - backup_status: DONE
        method: FULL
      - backup_status: DONE
        method: INCREMENTAL
      - backup_status: PLANNED
        method: INCREMENTAL
    """
