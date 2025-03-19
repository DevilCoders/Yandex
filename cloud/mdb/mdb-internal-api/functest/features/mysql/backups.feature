Feature: Backup MySQL cluster

  Background:
    Given default headers
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
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
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, t.method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb, 'FULL'::dbaas.backup_method),
		('auto02', 'DONE', '1970-01-01T00:00:11.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:11.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:11.000001+00:00", "finish_time":"1970-01-01T00:00:12.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 112}'::jsonb, 'FULL'::dbaas.backup_method))
            AS t (bid, status, ts, scheduled_date, initiator, metadata, method),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """

  @events
  Scenario: Backup creation works
    When we POST "/mdb/mysql/1.0/clusters/cid1:backup"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create a backup for MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.BackupClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.BackupCluster" event with
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
    When we POST "/mdb/mysql/1.0/clusters/cid1:backup"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Conflicting operation worker_task_id2 detected"
    }
    """

  @backup_list
  Scenario: Backup list works
    When we GET "/mdb/mysql/1.0/backups?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:11.000001+00:00",
                "folderId": "folder1",
                "id": "cid1:auto02",
                "size": 112,
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:11.000001+00:00",
                "type": "AUTOMATED"
            },
            {
                "createdAt": "1970-01-01T00:00:10.000001+00:00",
                "folderId": "folder1",
                "id": "cid1:auto01",
                "size": 11,
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:10.000001+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """


  Scenario: Backup list with page size works
    When we GET "/mdb/mysql/1.0/backups?folderId=folder1&pageSize=1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:11.000001+00:00",
                "folderId": "folder1",
                "id": "cid1:auto02",
                "size": 112,
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:11.000001+00:00",
                "type": "AUTOMATED"
            }
        ],
        "nextPageToken": "Y2lkMTphdXRvMDI="
    }
    """

  Scenario: Backup list with page token works
    When we GET "/mdb/mysql/1.0/backups?folderId=folder1&pageToken=Y2lkMTphdXRvMDI="
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:10.000001+00:00",
                "folderId": "folder1",
                "id": "cid1:auto01",
                "size": 11,
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:10.000001+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """

  Scenario: Backup list by cid works
    When we GET "/mdb/mysql/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:11.000001+00:00",
                "folderId": "folder1",
                "id": "cid1:auto02",
                "size": 112,
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:11.000001+00:00",
                "type": "AUTOMATED"
            },
            {
                "createdAt": "1970-01-01T00:00:10.000001+00:00",
                "folderId": "folder1",
                "id": "cid1:auto01",
                "size": 11,
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:10.000001+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """

  Scenario: Backup list on cluster with no backups works
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_5_7": {
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
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
    When we GET "/mdb/mysql/1.0/clusters/cid2/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": []
    }
    """

  Scenario: Backup get by id works
    When we GET "/mdb/mysql/1.0/backups/cid1:auto01"
    Then we get response with status 200 and body contains
    """
	{
		"createdAt": "1970-01-01T00:00:10.000001+00:00",
		"folderId": "folder1",
		"id": "cid1:auto01",
		"size": 11,
		"sourceClusterId": "cid1",
		"startedAt": "1970-01-01T00:00:10.000001+00:00",
		"type": "AUTOMATED"
	}
	"""


  @delete
  Scenario: After cluster delete its backups are shown
    When we DELETE "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "Delete MySQL cluster",
        "id": "worker_task_id2"
    }
    """
    When we GET "/mdb/mysql/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:11.000001+00:00",
                "folderId": "folder1",
                "id": "cid1:auto02",
                "size": 112,
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:11.000001+00:00",
                "type": "AUTOMATED"
            },
            {
                "createdAt": "1970-01-01T00:00:10.000001+00:00",
                "folderId": "folder1",
                "id": "cid1:auto01",
                "size": 11,
                "sourceClusterId": "cid1",
                "startedAt": "1970-01-01T00:00:10.000001+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """
    When we GET "/mdb/mysql/1.0/backups/cid1:auto02"
    Then we get response with status 200 and body contains
    """
	{
		"createdAt": "1970-01-01T00:00:11.000001+00:00",
		"folderId": "folder1",
		"id": "cid1:auto02",
		"size": 112,
		"sourceClusterId": "cid1",
		"startedAt": "1970-01-01T00:00:11.000001+00:00",
		"type": "AUTOMATED"
	}
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1/backups"
    Then we get response with status 403
    # backups for purged cluster not available
    When "worker_task_id2" acquired and finished by worker
    And "worker_task_id3" acquired and finished by worker
    And "worker_task_id4" acquired and finished by worker
    And we GET "/mdb/mysql/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": []
    }
    """
    When we GET "/mdb/mysql/1.0/backups/cid1:auto02"
    Then we get response with status 403

  @backup_api
  Scenario: Backup creation with BackupService works
    When we enable BackupService for cluster "cid1"
    And we POST "/mdb/mysql/1.0/clusters/cid1:backup"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create a backup for MySQL cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.BackupClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.BackupCluster" event with
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
    When we POST "/mdb/mysql/1.0/clusters/cid1:backup"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Conflicting operation worker_task_id2 detected"
    }
    """
