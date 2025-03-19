Feature: Clickhouse backup worker creation works

  @worker_create
  Scenario: Clickhouse backup creation works properly
    When I create "1" clickhouse clusters
    And I successfully execute query
    """
	INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, cid, subcid, shard_id)
		SELECT
		  t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id
		FROM (VALUES
		  ('man01', 'PLANNED', now(), NULL, 'USER'),
		  ('auto01', 'PLANNED', now() + INTERVAL '1 week', now() + INTERVAL '1 week', 'SCHEDULE'))
			AS t (bid, status, ts, scheduled_date, initiator),
			dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
		WHERE c.name = 'clickhouse_01';
    """
    And I execute mdb-backup-worker with config
    """
    {planned: {enabled: true, queue_producer: {max_tasks: 1}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid)
    """
    Then it returns "2" rows matches
    """
      - backup_id: man01
        backup_status: CREATING
        shipment_id: '1'
      - backup_id: auto01
        backup_status: PLANNED
        shipment_id: null
    """
    Given all deploy shipments get status is "DONE"
    Given s3 responses sequence
    """
    {
      "Responses": [
        {
            "ListObjects": {
              "CommonPrefixes": [
                  {
                      "Prefix": "ch-backup/cid1/shard1/man01"
                  }
              ]
            }
        },
        {
            "GetObject": {
              "Body": {
                  "meta": {
                      "date_fmt": "%Y-%m-%d %H:%M:%S",
                      "start_time": "1970-01-01 00:00:10",
                      "end_time": "1970-01-01 00:00:11",
                      "labels": {
                          "shard_name": "shard1"
                      },
                      "bytes": 1024,
                      "state": "created"
                  }
              }
            }
          }
      ]
    }
    """
    When I execute mdb-backup-worker with config
    """
    {creating: {enabled: true, queue_producer: {max_tasks: 1}, fraction_delayer: {min_delay: 1s, fractions: {1: 1s}}}}
    """
    And I execute query
    """
    SELECT b.backup_id, b.status as backup_status, b.shipment_id as shipment_id
    FROM dbaas.backups b JOIN dbaas.clusters c USING (cid) JOIN dbaas.subclusters sc USING (subcid) LEFT JOIN dbaas.shards sh USING (shard_id)
    """
    Then it returns "2" rows matches
    """
      - backup_id: man01
        backup_status: DONE
        shipment_id: '1'
      - backup_id: auto01
        backup_status: PLANNED
        shipment_id: null
    """

  Scenario: Planned clickhouse backup delayed due to deploy fails
    Given all deploy shipments creation fails
    When I create "1" clickhouse clusters
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
		WHERE c.name = 'clickhouse_01';
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
  
  @worker_resume_after_delay
  Scenario: Creating clickhouse backup delayed and resumed due to deploy status
    Given all deploy shipments get status is "INPROGRESS"
    When I create "1" clickhouse clusters
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
        WHERE c.name = 'clickhouse_01';
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
    Given s3 responses sequence
	  """
      {
        "Responses": [
          {
              "ListObjects": {
                "CommonPrefixes": [
                    {
                        "Prefix": "ch-backup/cid1/shard1/auto01"
                    }
                ]
              }
          },
          {
              "GetObject": {
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "bytes": 1024,
                        "state": "created"
                    }
                }
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
    Given s3 responses sequence
	  """
      {
        "Responses": [
          {
              "ListObjects": {
                "CommonPrefixes": [
                    {
                        "Prefix": "ch-backup/cid1/shard1/auto01"
                    }
                ]
              }
          },
          {
              "GetObject": {
                "Body": {
                    "meta": {
                        "date_fmt": "%Y-%m-%d %H:%M:%S",
                        "start_time": "1970-01-01 00:00:10",
                        "end_time": "1970-01-01 00:00:11",
                        "labels": {
                            "shard_name": "shard1"
                        },
                        "bytes": 1024,
                        "state": "created"
                    }
                }
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
