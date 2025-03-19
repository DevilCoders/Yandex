Feature: Backup scheduler delete works

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

  Scenario: Delete backups works properly with retain period days setting
    When I create "1" mongodb clusters
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
        WHERE c.name = 'mongodb_01';
    """
	And I change retain period for "cid1" to 33 days
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to obsolete
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
        backup_status: DONE
      - backup_id: auto02
        backup_status: CREATE-ERROR
      - backup_id: auto03
        backup_status: DONE
      - backup_id: auto04
        backup_status: DELETING
    """


  Scenario: Backups for deleted clusers are not obsoleted
    When I create "1" postgresql clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
        FROM (VALUES
          ('man01', 'DONE', now() - INTERVAL '3 weeks', NULL, 'USER'),
          ('auto01', 'DONE', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE'),
          ('auto02', 'CREATE-ERROR', now() - INTERVAL '5 week', now() - INTERVAL '1 week', 'SCHEDULE'),
          ('auto03', 'DONE', now() - INTERVAL '6 week', now() - INTERVAL '5 days', 'SCHEDULE'))
            AS t (bid, status, ts, scheduled_date, initiator),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
    And I successfully execute query
	"""
	UPDATE dbaas.clusters set STATUS='METADATA-DELETED' where cid = 'cid1';
	"""
    And I execute mdb-backup-scheduler for ctype "postgresql_cluster" to obsolete
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, c.name cluster_name, sc.name as subcluster_name
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid)
    """
    Then it returns "4" rows matches
    """
      - backup_id: man01
        backup_status: DONE
      - backup_id: auto01
        backup_status: DONE
      - backup_id: auto02
        backup_status: CREATE-ERROR
      - backup_id: auto03
        backup_status: DONE
    """

  Scenario: Backups for stopped clusters are not obsoleted
    When I create "1" postgresql clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
        FROM (VALUES
          ('man01', 'DONE', now() - INTERVAL '3 weeks', NULL, 'USER'),
          ('auto01', 'DONE', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE'),
          ('auto02', 'CREATE-ERROR', now() - INTERVAL '5 week', now() - INTERVAL '1 week', 'SCHEDULE'),
          ('auto03', 'DONE', now() - INTERVAL '6 week', now() - INTERVAL '5 days', 'SCHEDULE'))
            AS t (bid, status, ts, scheduled_date, initiator),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
    And I successfully execute query
	"""
	UPDATE dbaas.clusters set STATUS='STOPPED' where cid = 'cid1';
	"""
    And I execute mdb-backup-scheduler for ctype "postgresql_cluster" to obsolete
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, c.name cluster_name, sc.name as subcluster_name
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid)
    """
    Then it returns "4" rows matches
    """
      - backup_id: man01
        backup_status: DONE
      - backup_id: auto01
        backup_status: DONE
      - backup_id: auto02
        backup_status: CREATE-ERROR
      - backup_id: auto03
        backup_status: DONE
    """

  Scenario: Backups in create-error status are obsoleted after some time
    When I create "1" postgresql clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
        FROM (VALUES
          ('man01', 'DONE', now() - INTERVAL '2 days', NULL, 'USER'),
          ('auto01', 'DONE', now() - INTERVAL '1 day', now() - INTERVAL '3 weeks', 'SCHEDULE'),
          ('auto02', 'CREATE-ERROR', now() - INTERVAL '5 week', now() - INTERVAL '1 week', 'SCHEDULE'),
          ('auto03', 'CREATE-ERROR', now() - INTERVAL '1 week', now() - INTERVAL '5 days', 'SCHEDULE'))
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
    Then it returns "4" rows matches
    """
      - backup_id: man01
        backup_status: DONE
      - backup_id: auto01
        backup_status: DONE
      - backup_id: auto02
        backup_status: DELETED
      - backup_id: auto03
        backup_status: CREATE-ERROR
    """

  Scenario: Only requested cluster types are handled
    When I create "1" postgresql clusters
    And I create "1" mongodb clusters
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
        FROM (VALUES
          ('auto01', 'DONE', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE'),
          ('auto02', 'DONE', now() - INTERVAL '5 weeks', now() - INTERVAL '5 weeks', 'SCHEDULE'))
            AS t (bid, status, ts, scheduled_date, initiator),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'postgresql_01';
    """
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
        FROM (VALUES
          ('auto11', 'DONE', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE'),
          ('auto12', 'DONE', now() - INTERVAL '5 weeks', now() - INTERVAL '5 weeks', 'SCHEDULE'))
            AS t (bid, status, ts, scheduled_date, initiator),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'mongodb_02';
    """
    And I execute mdb-backup-scheduler for ctype "postgresql_cluster" to obsolete
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, c.name cluster_name, sc.name as subcluster_name
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid)
    """
    Then it returns "4" rows matches
    """
      - backup_id: auto01
        backup_status: OBSOLETE
      - backup_id: auto02
        backup_status: OBSOLETE
      - backup_id: auto11
        backup_status: DONE
      - backup_id: auto12
        backup_status: DONE
    """
    And I execute mdb-backup-scheduler for ctype "mongodb_cluster" to obsolete
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, c.name cluster_name, sc.name as subcluster_name
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid)
    """
    Then it returns "4" rows matches
    """
      - backup_id: auto01
        backup_status: OBSOLETE
      - backup_id: auto02
        backup_status: OBSOLETE
      - backup_id: auto11
        backup_status: OBSOLETE
      - backup_id: auto12
        backup_status: OBSOLETE
    """