Feature: PostgreSQL backup scheduler delete works

  Scenario: Delete backups works properly
    When I create "1" postgresql clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
        FROM (VALUES
          ('man01', 'DONE', now() - INTERVAL '3 weeks', NULL, 'USER'),
          ('auto01', 'DONE', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE'),
          ('auto02', 'CREATE-ERROR', now() - INTERVAL '1 week', now() - INTERVAL '1 week', 'SCHEDULE'),
          ('auto03', 'DONE', now() - INTERVAL '5 days', now() - INTERVAL '5 days', 'SCHEDULE'),
          ('auto04', 'DELETING', now() - INTERVAL '4 days', now() - INTERVAL '4 days', 'SCHEDULE'))
            AS t (bid, status, ts, scheduled_date, initiator),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
    And I execute mdb-backup-scheduler for ctype "postgresql_cluster" to obsolete
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, c.name cluster_name, sc.name as subcluster_name
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid)
    """
    Then it returns "5" rows matches
    """
      - backup_id: man01
        backup_status: DONE
      - backup_id: auto01
        backup_status: OBSOLETE
      - backup_id: auto02
        backup_status: CREATE-ERROR
      - backup_id: auto03
        backup_status: DONE
      - backup_id: auto04
        backup_status: DELETING
    """

  Scenario: Delete delta backups respects retention period properly
    When I create "1" postgresql clusters
    And I set delta_max_steps for "cid1" to 5
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id, metadata)
        SELECT
            t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, t.method::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
        ('auto01', 'DONE', 'FULL', now() - INTERVAL '5 weeks', now() - INTERVAL '5 weeks', 'SCHEDULE', '{"id":"auto01", "name": "auto01", "is_incremental": false, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ"}'::jsonb),
        ('auto010', 'DONE', 'FULL', now() - INTERVAL '2 days', now() - INTERVAL '2 days', 'SCHEDULE', '{"id":"auto010", "name": "auto010", "is_incremental": false, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ"}'::jsonb),
        ('auto011', 'DONE', 'INCREMENTAL', now() - INTERVAL '1 day', now() - INTERVAL '1 day', 'SCHEDULE', '{"id":"auto011", "name": "auto011", "is_incremental": true, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ"}'::jsonb),
        ('auto020', 'DONE', 'FULL', now() - INTERVAL '5 weeks', now() - INTERVAL '5 weeks', 'SCHEDULE', '{"id":"auto020", "name": "auto020", "is_incremental": false, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ"}'::jsonb),
        ('auto021', 'DONE', 'INCREMENTAL', now() - INTERVAL '2 day', now() - INTERVAL '2 day', 'SCHEDULE', '{"id":"auto021", "name": "auto021", "is_incremental": true, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ"}'::jsonb),
        ('auto022', 'DONE', 'INCREMENTAL', now() - INTERVAL '1 day', now() - INTERVAL '1 day', 'SCHEDULE', '{"id":"auto022", "name": "auto022", "is_incremental": true, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ"}'::jsonb),
        ('auto030', 'DONE', 'FULL', now() - INTERVAL '5 weeks', now() - INTERVAL '5 weeks', 'SCHEDULE', '{"id":"auto030", "name": "auto030", "is_incremental": false, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ"}'::jsonb),
        ('auto031', 'DONE', 'INCREMENTAL', now() - INTERVAL '4 weeks', now() - INTERVAL '4 weeks', 'SCHEDULE', '{"id":"auto031", "name": "auto031", "is_incremental": true, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ"}'::jsonb),
        ('auto032', 'DONE', 'INCREMENTAL', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE', '{"id":"auto032", "name": "auto032", "is_incremental": true, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ"}'::jsonb))
            AS t (bid, status, method, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
	And I successfully execute query
	"""
	INSERT INTO dbaas.backups_dependencies VALUES('auto010','auto011'), ('auto020', 'auto021'), ('auto030', 'auto031')
	"""
    And I execute mdb-backup-scheduler for ctype "postgresql_cluster" to obsolete
    And I execute query
    """
    SELECT b.backup_id as backup_id, b.status as backup_status, b.method as method
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid)
    """
    Then it returns "9" rows matches
    """
      - backup_id: auto01
        backup_status: OBSOLETE
        method: FULL
      - backup_id: auto010
        backup_status: DONE
        method: FULL
      - backup_id: auto011
        backup_status: DONE
        method: INCREMENTAL
      - backup_id: auto020
        backup_status: DONE
        method: FULL
      - backup_id: auto021
        backup_status: DONE
        method: INCREMENTAL
      - backup_id: auto022
        backup_status: DONE
        method: INCREMENTAL
      - backup_id: auto030
        backup_status: OBSOLETE
        method: FULL
      - backup_id: auto032
        backup_status: OBSOLETE
        method: INCREMENTAL
      - backup_id: auto031
        backup_status: OBSOLETE
        method: INCREMENTAL
    """

  Scenario: PostgreSQL backup scheduler do not delete backups that has creating backups depenting on it
    When I create "1" postgresql clusters
    And I enable backup service for all existed clusters via cli
    And I set delta_max_steps for "cid1" to 5
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id, metadata)
        SELECT
            t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, t.method::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
        ('auto010', 'DONE', 'FULL', now() - INTERVAL '120 day', now() - INTERVAL '121 day', 'SCHEDULE', '{"id":"auto010", "name": "auto010", "is_incremental": false, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ"}'::jsonb),
        ('auto011', 'CREATING', 'INCREMENTAL', now(), now(), 'SCHEDULE', '{"id":"auto011", "name": "auto011", "is_incremental": true, "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "meta": {"date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ", "user_data": {"backup_id": "auto011"}, "is_permanent": false }, "root_path": "wal-e/cid1/1000", "increment_details": {"delta_count": 1}}'::jsonb))
            AS t (bid, status, method, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
	And I successfully execute query
	"""
	INSERT INTO dbaas.backups_dependencies VALUES('auto010','auto011')
	"""
    And I set "postgresql_01" cluster backup window at "1h" from now
    And I execute mdb-backup-scheduler for ctype "postgresql_cluster" to obsolete
    When I execute query
    """
    SELECT b.status as backup_status, b.method as method
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid)
    """
    Then it returns "2" rows matches
    """
      - backup_status: DONE
        method: FULL
      - backup_status: CREATING
        method: INCREMENTAL
    """
