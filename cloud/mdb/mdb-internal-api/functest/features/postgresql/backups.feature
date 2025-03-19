Feature: Backup PostgreSQL cluster

  Background:
    Given default headers
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
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
       "hostSpecs": [{
           "zoneId": "myt"
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker

  @events
  Scenario: Backup creation works
    When we POST "/mdb/postgresql/1.0/clusters/cid1:backup"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create a backup for PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.BackupClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.BackupCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1"
        }
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters/cid1:backup"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Conflicting operation worker_task_id2 detected"
    }
    """

  @backup_list
  Scenario: Backup list works
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, t.method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb, 'FULL'::dbaas.backup_method),
		('auto02', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb, 'INCREMENTAL'::dbaas.backup_method))
            AS t (bid, status, ts, scheduled_date, initiator, metadata, method),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    When we GET "/mdb/postgresql/1.0/backups?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
	"backups": [
			{
				"createdAt":       "1970-01-01T00:00:10.000001+00:00",
				"folderId":        "folder1",
				"id":               "cid1:auto01",
				"size":             11,
				"method":          "BASE",
				"sourceClusterId": "cid1",
				"startedAt":       "1970-01-01T00:00:10.000001+00:00",
				"type":            "AUTOMATED"
			},
			{
				"createdAt":       "1970-01-01T00:00:10.000001+00:00",
				"folderId":        "folder1",
				"id":               "cid1:auto02",
				"size":             11,
				"sourceClusterId": "cid1",
				"startedAt":       "1970-01-01T00:00:10.000001+00:00",
				"method":          "INCREMENTAL",
				"type":            "AUTOMATED"
			}
        ]
    }
    """

  @backup_list
  Scenario: Backup list with page size works
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb),
		('auto02', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    When we GET "/mdb/postgresql/1.0/backups?folderId=folder1&pageSize=1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
			{
				"createdAt":       "1970-01-01T00:00:10.000001+00:00",
				"folderId":        "folder1",
				"id":               "cid1:auto01",
				"size":             11,
				"sourceClusterId": "cid1",
				"method":          "BASE",
				"startedAt":       "1970-01-01T00:00:10.000001+00:00",
				"type":            "AUTOMATED"
			}
        ],
        "nextPageToken": "Y2lkMTphdXRvMDE="
    }
    """

  @backup_list
  Scenario: Backup list with page token works
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb),
		('auto02', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    When we GET "/mdb/postgresql/1.0/backups?folderId=folder1&pageToken=Y2lkMTphdXRvMDE="
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
			{
				"createdAt":       "1970-01-01T00:00:10.000001+00:00",
				"folderId":        "folder1",
				"id":               "cid1:auto02",
				"size":             11,
				"sourceClusterId": "cid1",
				"startedAt":       "1970-01-01T00:00:10.000001+00:00",
				"method":          "BASE",
				"type":            "AUTOMATED"
			}
        ]
    }
    """

  @backup_list
  Scenario: Backup list by cid works
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb),
		('auto02', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
			{
				"createdAt":       "1970-01-01T00:00:10.000001+00:00",
				"folderId":        "folder1",
				"id":               "cid1:auto01",
				"size":             11,
				"sourceClusterId": "cid1",
				"startedAt":       "1970-01-01T00:00:10.000001+00:00",
				"method":          "BASE",
				"type":            "AUTOMATED"
			},
			{
				"createdAt":       "1970-01-01T00:00:10.000001+00:00",
				"folderId":        "folder1",
				"id":               "cid1:auto02",
				"size":             11,
				"sourceClusterId": "cid1",
				"startedAt":       "1970-01-01T00:00:10.000001+00:00",
				"method":          "BASE",
				"type":            "AUTOMATED"
			}
        ]
    }
    """

  @backup_list
  Scenario: Backup list on cluster with no backups works
    Given s3 response
    """
    {}
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": []
    }
    """

  @backup_list
  Scenario: Backup get by id works
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb),
		('auto02', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    When we GET "/mdb/postgresql/1.0/backups/cid1:auto02"
    Then we get response with status 200 and body contains
    """
	{
		"createdAt":       "1970-01-01T00:00:10.000001+00:00",
		"folderId":        "folder1",
		"id":               "cid1:auto02",
		"size":             11,
		"sourceClusterId": "cid1",
		"startedAt":       "1970-01-01T00:00:10.000001+00:00",
		"method":          "BASE",
		"type":            "AUTOMATED"
	}
    """

  @delete
  Scenario: After cluster delete its backups are shown
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb),
		('auto02', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "Delete PostgreSQL cluster",
        "id": "worker_task_id2"
    }
    """
    When we GET "/mdb/postgresql/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
			{
				"createdAt":       "1970-01-01T00:00:10.000001+00:00",
				"folderId":        "folder1",
				"id":               "cid1:auto01",
				"size":             11,
				"sourceClusterId": "cid1",
				"startedAt":       "1970-01-01T00:00:10.000001+00:00",
				"method":          "BASE",
				"type":            "AUTOMATED"
			},
			{
				"createdAt":       "1970-01-01T00:00:10.000001+00:00",
				"folderId":        "folder1",
				"id":               "cid1:auto02",
				"size":             11,
				"sourceClusterId": "cid1",
				"startedAt":       "1970-01-01T00:00:10.000001+00:00",
				"method":          "BASE",
				"type":            "AUTOMATED"
			}
        ]
    }
    """
    When we GET "/mdb/postgresql/1.0/backups/cid1:auto01"
    Then we get response with status 200 and body contains
    """
	{
		"createdAt":       "1970-01-01T00:00:10.000001+00:00",
		"folderId":        "folder1",
		"id":               "cid1:auto01",
		"size":             11,
		"method":          "BASE",
		"sourceClusterId": "cid1",
		"startedAt":       "1970-01-01T00:00:10.000001+00:00",
		"type":            "AUTOMATED"
	}
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid1/backups"
    Then we get response with status 403
    # backups for purged cluster not available
    When "worker_task_id2" acquired and finished by worker
    And "worker_task_id3" acquired and finished by worker
    And "worker_task_id4" acquired and finished by worker
    And we GET "/mdb/postgresql/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": []
    }
    """
    When we GET "/mdb/postgresql/1.0/backups/cid1:base_1"
    Then we get response with status 403
   
   @backup_api    
   Scenario: managed backup list works
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "wal-e/cid1/basebackups_005/backup_1_backup_stop_sentinel.json",
                "LastModified": 31,
                "Body": {
                    "start_time": "1970-01-01T00:00:00.000002Z",
                    "finish_time": "1970-01-01T00:00:01.000002Z",
                    "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
                    "pg_version": 1000,
                    "name": "base0",
                    "method": "BASE",
                    "compressed_size": 1,
                    "is_permanent": true,
                    "user_data": {
                        "backup_id": "man01"
                    }
                 }
             }
        ]
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test2",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
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
       "hostSpecs": [{
           "zoneId": "myt"
       }, {
           "zoneId": "iva"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster"
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we add managed backup
    """
    {
        "backup_id": "backup_1",
        "status": "DONE",
        "method": "FULL",
        "initiator": "SCHEDULE",
        "cid": "cid2",
        "subcid": "subcid2",
        "ts": "1970-01-01T00:00:41+00:00",
        "start_ts": "1970-01-01T00:00:42.000000+00:00",
        "stop_ts": "1970-01-01T00:00:43.000000+00:00",
        "meta": {
            "start_time": "1970-01-01T00:00:00.000002Z",
            "finish_time": "1970-01-01T00:00:01.000002Z",
            "pg_version": 1000,
            "compressed_size": 1,
            "name": "base0",
            "is_permanent": true,
            "date_fmt": "%Y-%m-%dT%H:%M:%S.%fZ",
            "user_data": {
            }
        }
    }
    """
    When we enable BackupService for cluster "cid2"
    When we GET "/mdb/postgresql/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "folderId": "folder1",
                "id": "cid2:backup_1",
                "size": 1,
                "sourceClusterId": "cid2",
                "type": "AUTOMATED",
                "method": "BASE",
                "startedAt": "1970-01-01T00:00:42+00:00",
                "createdAt": "1970-01-01T00:00:43+00:00"
            }
        ]
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid2/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "folderId": "folder1",
                "id": "cid2:backup_1",
                "size": 1,
                "sourceClusterId": "cid2",
                "type": "AUTOMATED",
                "method": "BASE",
                "startedAt": "1970-01-01T00:00:42+00:00",
                "createdAt": "1970-01-01T00:00:43+00:00"
            }
        ]
    }
    """
    When we GET "/mdb/postgresql/1.0/backups/cid2:backup_1"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder1",
        "id": "cid2:backup_1",
        "size": 1,
        "sourceClusterId": "cid2",
        "type": "AUTOMATED",
        "method": "BASE",
        "startedAt": "1970-01-01T00:00:42+00:00",
        "createdAt": "1970-01-01T00:00:43+00:00"
    }
    """
  @backup_api    
  Scenario: Backup creation with BackupService works
    When we enable BackupService for cluster "cid1"
    And we POST "/mdb/postgresql/1.0/clusters/cid1:backup"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create a backup for PostgreSQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.BackupClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.BackupCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1"
        }
    }
    """
    And last managed backup for cluster "cid1" has status "PLANNED"
    When we POST "/mdb/postgresql/1.0/clusters/cid1:backup"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Conflicting operation worker_task_id2 detected"
    }
    """
