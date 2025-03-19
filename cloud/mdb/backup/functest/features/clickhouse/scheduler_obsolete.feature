Feature: Clickhouse backup scheduler delete works

  Scenario: Delete backups works properly
    When I create "1" clickhouse clusters
    And I enable backup service for all existed clusters via cli
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
        FROM (VALUES
          ('man01', 'DONE', now() - INTERVAL '3 weeks', NULL, 'USER'),
          ('auto01', 'DONE', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE'),
          ('auto02', 'CREATE-ERROR', now() - INTERVAL '1 week', now() - INTERVAL '1 week', 'SCHEDULE'),
          ('auto03', 'DONE', now() - INTERVAL '5 days', now() - INTERVAL '5 days', 'SCHEDULE'))
            AS t (bid, status, ts, scheduled_date, initiator),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.name = 'clickhouse_01';
    """
    And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to obsolete
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, c.name cluster_name, sc.name as subcluster_name, sh.name as shard_name
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "4" rows matches
    """
      - backup_id: man01
        backup_status: DONE
      - backup_id: auto01
        backup_status: OBSOLETE
      - backup_id: auto02
        backup_status: CREATE-ERROR
      - backup_id: auto03
        backup_status: DONE
    """

    Scenario: Delete backups works sequentially per cluster
      When I create "2" clickhouse clusters
      And I enable backup service for all existed clusters via cli
      And I successfully execute query
      """
      INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
          SELECT
            t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
          FROM (VALUES
            ('auto01', 'DONE', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE'),
            ('auto02', 'DONE', now() - INTERVAL '2 weeks', now() - INTERVAL '2 weeks', 'SCHEDULE'))
              AS t (bid, status, ts, scheduled_date, initiator),
              dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
          WHERE c.name = 'clickhouse_01';
      """
      And I successfully execute query
      """
      INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
          SELECT
            t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
          FROM (VALUES
            ('auto03', 'DONE', now() - INTERVAL '2 weeks', now() - INTERVAL '2 weeks', 'SCHEDULE'))
              AS t (bid, status, ts, scheduled_date, initiator),
              dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
          WHERE c.name = 'clickhouse_02';
      """
      And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to obsolete
      And I execute query
      """
      SELECT b.backup_id, b.status as backup_status, c.name cluster_name, sc.name as subcluster_name, sh.name as shard_name
      FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
      """
      Then it returns "3" rows matches
      """
        - backup_id: auto01
          backup_status: OBSOLETE
        - backup_id: auto02
          backup_status: DONE
        - backup_id: auto03
          backup_status: OBSOLETE
      """
      And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to obsolete
      And I execute query
      """
      SELECT b.backup_id, b.status as backup_status, c.name cluster_name, sc.name as subcluster_name, sh.name as shard_name
      FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
      """
      Then it returns "3" rows matches
      """
        - backup_id: auto01
          backup_status: OBSOLETE
        - backup_id: auto02
          backup_status: DONE
        - backup_id: auto03
          backup_status: OBSOLETE
      """

    Scenario: Delete CREATE-ERROR backups works
      When I create "1" clickhouse clusters
      And I enable backup service for all existed clusters via cli
      And I successfully execute query
      """
      INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
          SELECT
            t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
          FROM (VALUES
            ('auto01', 'CREATE-ERROR', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE'))
              AS t (bid, status, ts, scheduled_date, initiator),
              dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
          WHERE c.name = 'clickhouse_01';
      """
      And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to obsolete
      And I execute query
      """
      SELECT b.backup_id, b.status as backup_status, c.name cluster_name, sc.name as subcluster_name, sh.name as shard_name
      FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
      """
      Then it returns "1" rows matches
      """
        - backup_id: auto01
          backup_status: OBSOLETE
      """

    Scenario: CREATE-ERROR backups are not marked for deletion in case there are Obsolete/Deleting backups for cluster
      When I create "2" clickhouse clusters
      And I enable backup service for all existed clusters via cli
      And I successfully execute query
      """
      INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
          SELECT
            t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
          FROM (VALUES
            ('auto01', 'DELETING', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE'),
            ('auto02', 'CREATE-ERROR', now() - INTERVAL '3 weeks', now() - INTERVAL '4 weeks', 'SCHEDULE')
            )
              AS t (bid, status, ts, scheduled_date, initiator),
              dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
          WHERE c.name = 'clickhouse_01';
      """
      And I successfully execute query
      """
      INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
          SELECT
            t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
          FROM (VALUES
            ('auto03', 'OBSOLETE', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE'),
            ('auto04', 'CREATE-ERROR', now() - INTERVAL '3 weeks', now() - INTERVAL '4 weeks', 'SCHEDULE')
            )
              AS t (bid, status, ts, scheduled_date, initiator),
              dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
          WHERE c.name = 'clickhouse_02';
      """
      And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to obsolete
      And I execute query
      """
      SELECT b.backup_id, b.status as backup_status, c.name cluster_name, sc.name as subcluster_name, sh.name as shard_name
      FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
      """
      Then it returns "4" rows matches
      """
        - backup_id: auto01
          backup_status: DELETING
        - backup_id: auto02
          backup_status: CREATE-ERROR
        - backup_id: auto03
          backup_status: OBSOLETE
        - backup_id: auto04
          backup_status: CREATE-ERROR
      """

    Scenario: Sequential deletion of CREATE-ERROR backups works
      When I create "1" clickhouse clusters
      And I enable backup service for all existed clusters via cli
      And I successfully execute query
      """
      INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
          SELECT
            t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
          FROM (VALUES
            ('auto01', 'CREATE-ERROR', now() - INTERVAL '3 weeks', now() - INTERVAL '2 weeks', 'SCHEDULE'),
            ('auto02', 'CREATE-ERROR', now() - INTERVAL '3 weeks', now() - INTERVAL '3 weeks', 'SCHEDULE')
            )
              AS t (bid, status, ts, scheduled_date, initiator),
              dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
          WHERE c.name = 'clickhouse_01';
      """
      And I execute mdb-backup-scheduler for ctype "clickhouse_cluster" to obsolete
      And I execute query
      """
      SELECT b.backup_id, b.status as backup_status, c.name cluster_name, sc.name as subcluster_name, sh.name as shard_name
      FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
      """
      Then it returns "2" rows matches
      """
        - backup_id: auto01
          backup_status: OBSOLETE
        - backup_id: auto02
          backup_status: CREATE-ERROR
      """