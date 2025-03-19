@restore-hints
Feature: Restore PostgreSQL cluster from backup with hints

  Background:
    Given default headers
    And feature flags
    """
    ["MDB_PG_ALLOW_DEPRECATED_VERSIONS", "MDB_POSTGRESQL_10_1C", "MDB_POSTGRESQL_11", "MDB_POSTGRESQL_11_1C", "MDB_POSTGRESQL_12", "MDB_POSTGRESQL_12_1C", "MDB_POSTGRESQL_13", "MDB_POSTGRESQL_13_1C", "MDB_POSTGRESQL_14", "MDB_POSTGRESQL_14_1C"]
    """

  Scenario Outline: Restore hints for different Postgre versions
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "<version>",
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{ "zoneId": "myt" }]
    }
    """
    When all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
	And I successfully execute query
	"""
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:01.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:01.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    And we GET "/mdb/postgresql/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt":       "1970-01-01T00:00:01.000001+00:00",
                "folderId":        "folder1",
                "id":              "cid1:auto01",
	 			"sourceClusterId": "cid1",
				"size":             11,
				"method":          "BASE",
				"startedAt":       "1970-01-01T00:00:01.000001+00:00",
				"type":            "AUTOMATED"
            }
        ]
    }
    """
    When we GET "/mdb/postgresql/1.0/console/restore-hints/cid1:auto01"
    Then we get response with status 200 and body equals
    """
    {
        "environment": "PRESTABLE",
        "networkId": "",
        "resources": {
            "diskSize": 10737418240,
            "resourcePresetId": "s1.porto.1"
        },
        "time": "1970-01-01T00:01:01.000001+00:00",
        "version": "<version>"
    }
    """

    Examples:
    | version | backup_path |
    | 10      | 1000        |
    | 11      | 1100        |
    | 12      | 1200        |
    | 13      | 1300        |
    | 14      | 1400        |
    | 10-1c   | 1000        |
    | 11-1c   | 1100        |
    | 12-1c   | 1200        |
    | 13-1c   | 1300        |
    | 14-1c   | 1400        |


  Scenario: Restore disk_size depends on cluster disk_size at restore time
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "12",
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{ "zoneId": "myt" }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
    When "worker_task_id1" acquired and finished by worker
	And I successfully execute query
	"""
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:01.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:01.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 21474836480
            }
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/console/restore-hints/cid1:auto01"
    Then we get response with status 200 and body contains
    """
    {
        "resources": {
            "diskSize": 10737418240,
            "resourcePresetId": "s1.porto.1"
        }
    }
    """
