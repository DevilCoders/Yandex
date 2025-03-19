Feature: Backup MongoDB Cluster

  Background:
    Given default headers
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_2": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
                    }
                }
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
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T03:00:30.000000+03:00'::timestamptz, DATE('1970-01-01T03:00:30.000000+03:00'), 'SCHEDULE', '{ "before_ts": {"TS": 30, "Inc": 1}, "after_ts": {"TS": 31, "Inc": 6 }, "shard_names": ["shard1"], "permanent": false,"data_size": 30, "name": "stream1", "root_path": "mongodb-backup/cid1/shard1"}'::jsonb),
		('auto02', 'DONE', '1970-01-01T00:00:35+00:00'::timestamptz, DATE('1970-01-02T00:00:35+00:00'), 'SCHEDULE', '{ "before_ts": {"TS": 35, "Inc": 10 }, "after_ts": { "TS": 36, "Inc": 60 }, "shard_names": ["shard1"],"data_size": 35, "name": "stream2", "root_path": "mongodb-backup/cid1/shard1", "Permanent": true}'::jsonb),
		('man03', 'DONE', '1970-01-01T00:00:41+00:00'::timestamptz, NULL::DATE, 'USER', '{"before_ts":{"TS": 40,"Inc": 1},"after_ts": {"TS": 41,"Inc": 1}, "shard_names": ["shard2"], "permanent": true, "data_size": 40, "name": "stream3", "root_path": "mongodb-backup/cid1/shard2"}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """

  @backup_list
  Scenario: Backup list works
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test50",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_5_0": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
                    }
                }
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
    When we add managed backup
    """
    {
        "backup_id": "backup_1",
        "status": "DONE",
        "method": "FULL",
        "initiator": "SCHEDULE",
        "cid": "cid2",
        "subcid": "subcid2",
        "shard_id": "shard_id2",
        "ts": "1970-01-01T00:00:41+00:00",
        "meta": {
            "name": "stream_19700101T000041Z",
            "before_ts": {"TS": 40, "Inc": 1},
            "after_ts": {"TS": 41, "Inc": 1},
            "data_size": 41,
            "shard_names": ["shard2"],
            "root_path": "mongodb-backup/cid2/shard2"
      }
    }
    """
    And we add managed backup
    """
    {
        "backup_id": "backup_2",
        "status": "DONE",
        "method": "FULL",
        "initiator": "USER",
        "cid": "cid2",
        "subcid": "subcid2",
        "shard_id": "shard_id2",
        "ts": "1970-01-01T00:00:51+00:00",
        "meta": {
            "name": "stream_19700101T000051Z",
            "before_ts": {"TS": 50, "Inc": 1},
            "after_ts": {"TS": 51, "Inc": 1},
            "data_size": 51,
            "shard_names": ["shard2"],
            "root_path": "mongodb-backup/cid2/shard2"
        }
    }
    """
    When we enable BackupService for cluster "cid2"
    And we GET "/mdb/mongodb/1.0/backups?folderId=folder1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:51+00:00",
                "folderId": "folder1",
                "id": "cid2:backup_2",
                "size": 51,
                "sourceClusterId": "cid2",
                "sourceShardNames": ["shard2"],
                "startedAt": "1970-01-01T00:00:50+00:00",
                "type": "MANUAL"
            },
            {
                "createdAt": "1970-01-01T00:00:41+00:00",
                "folderId": "folder1",
                "id": "cid2:backup_1",
                "size": 41,
                "sourceClusterId": "cid2",
                "sourceShardNames": ["shard2"],
                "startedAt": "1970-01-01T00:00:40+00:00",
                "type": "AUTOMATED"
            },
            {
                "createdAt": "1970-01-01T00:00:41+00:00",
                "folderId": "folder1",
                "size": 40,
                "id": "cid1:man03",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard2"],
                "startedAt": "1970-01-01T00:00:40+00:00",
                "type": "MANUAL"
            },
            {
                "createdAt": "1970-01-01T00:00:36+00:00",
                "folderId": "folder1",
                "id": "cid1:auto02",
                "size": 35,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:35+00:00",
                "type": "AUTOMATED"
            },
            {
                "createdAt": "1970-01-01T00:00:31+00:00",
                "folderId": "folder1",
                "id": "cid1:auto01",
                "size": 30,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:30+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """

  @paging
  @backup_list
  Scenario: Backup list with page size works
    When we GET "/mdb/mongodb/1.0/backups?folderId=folder1&pageSize=1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:41+00:00",
                "folderId": "folder1",
                "id": "cid1:man03",
                "size": 40,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard2"],
                "startedAt": "1970-01-01T00:00:40+00:00",
                "type": "MANUAL"
            }
        ],
        "nextPageToken": "Y2lkMTptYW4wMw=="
    }
    """

  @paging
  @backup_list
  Scenario: Backup list with page token works
    When we GET "/mdb/mongodb/1.0/backups?folderId=folder1&pageToken=Y2lkMTptYW4wMw==&pageSize=1"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:36+00:00",
                "folderId": "folder1",
                "id": "cid1:auto02",
                "size": 35,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:35+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """

  @backup_list
  Scenario: Backup list by cid works
    When we GET "/mdb/mongodb/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:41+00:00",
                "folderId": "folder1",
                "size": 40,
                "id": "cid1:man03",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard2"],
                "startedAt": "1970-01-01T00:00:40+00:00",
                "type": "MANUAL"
            },
            {
                "createdAt": "1970-01-01T00:00:36+00:00",
                "folderId": "folder1",
                "id": "cid1:auto02",
                "size": 35,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:35+00:00",
                "type": "AUTOMATED"
            },
            {
                "createdAt": "1970-01-01T00:00:31+00:00",
                "folderId": "folder1",
                "id": "cid1:auto01",
                "size": 30,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:30+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """

  @backup_list
  Scenario: Backup list on cluster with no backups works
    When I successfully execute query
	"""
	DELETE FROM dbaas.backups WHERE cid = 'cid1'
	"""
	When we GET "/mdb/mongodb/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": []
    }
    """
    When we enable BackupService for cluster "cid1"
    When we GET "/mdb/mongodb/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": []
    }
    """

  Scenario: Backup get by id works
    When we GET "/mdb/mongodb/1.0/backups/cid1:man03"
    Then we get response with status 200 and body contains
    """
    {
        "createdAt": "1970-01-01T00:00:41+00:00",
        "folderId": "folder1",
        "size": 40,
        "id": "cid1:man03",
        "sourceClusterId": "cid1",
        "sourceShardNames": ["shard2"],
        "startedAt": "1970-01-01T00:00:40+00:00",
        "type": "MANUAL"
    }
    """
  @backup_list
  Scenario: Backup list by cid with Managed Backups works
    When I successfully execute query
	"""
	DELETE FROM dbaas.backups WHERE cid = 'cid1'
	"""
    When we add managed backup
    """
    {
        "backup_id": "backup_1",
        "status": "DONE",
        "method": "FULL",
        "initiator": "SCHEDULE",
        "cid": "cid1",
        "subcid": "subcid1",
        "shard_id": "shard_id1",
        "ts": "1970-01-01T00:00:41+00:00",
        "meta": {
            "name": "stream_19700101T000041Z",
            "before_ts": {"TS": 40, "Inc": 1},
            "after_ts": {"TS": 41, "Inc": 1},
            "data_size": 41,
            "shard_names": ["shard1"],
            "root_path": "mongodb-backup/cid1/shard1"
        }
    }
    """
    And we add managed backup
    """
    {
        "backup_id": "backup_2",
        "status": "DONE",
        "method": "FULL",
        "initiator": "USER",
        "cid": "cid1",
        "subcid": "subcid1",
        "shard_id": "shard_id1",
        "ts": "1970-01-01T00:00:51+00:00",
        "meta": {
            "name": "stream_19700101T000051Z",
            "before_ts": {"TS": 50, "Inc": 1},
            "after_ts": {"TS": 51, "Inc": 1},
            "data_size": 51,
            "shard_names": ["shard1"],
            "root_path": "mongodb-backup/cid1/shard1"
        }
    }
    """
    And we GET "/mdb/mongodb/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:51+00:00",
                "folderId": "folder1",
                "id": "cid1:backup_2",
                "size": 51,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:50+00:00",
                "type": "MANUAL"
            },
            {
                "createdAt": "1970-01-01T00:00:41+00:00",
                "folderId": "folder1",
                "id": "cid1:backup_1",
                "size": 41,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:40+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """


  Scenario: Backup list of sharded cluster by cid with Managed Backups works
    When I successfully execute query
	"""
	DELETE FROM dbaas.backups WHERE cid = 'cid1'
	"""
    And we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOINFRA"
            },
            {
                "zoneId": "sas",
                "type": "MONGOINFRA"
            },
            {
                "zoneId": "vla",
                "type": "MONGOINFRA"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "zoneId": "sas",
                "type": "MONGOD"
            }, {
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id3" acquired and finished by worker
    And we enable BackupService for cluster "cid1"
    When we add managed backup
    """
    {
        "backup_id": "backup_1",
        "status": "DONE",
        "method": "FULL",
        "initiator": "SCHEDULE",
        "cid": "cid1",
        "subcid": "subcid1",
        "shard_id": "shard_id1",
        "ts": "1970-01-01T00:00:41+00:00",
        "meta": {
            "name": "stream_19700101T000041Z",
            "before_ts": {"TS": 40, "Inc": 1},
            "after_ts": {"TS": 41, "Inc": 1},
            "data_size": 41,
            "shard_names": ["shard1"],
            "root_path": "mongodb-backup/cid1/shard1"
        }
    }
    """
    And we add managed backup
    """
    {
        "backup_id": "backup_2",
        "status": "DONE",
        "method": "FULL",
        "initiator": "USER",
        "cid": "cid1",
        "subcid": "subcid1",
        "shard_id": "shard_id3",
        "ts": "1970-01-01T00:00:51+00:00",
        "meta": {
            "name": "stream_19700101T000051Z",
            "before_ts": {"TS": 50, "Inc": 1},
            "after_ts": {"TS": 51, "Inc": 1},
            "data_size": 51,
            "shard_names": ["shard3"],
            "root_path": "mongodb-backup/cid1/shard3"
        }
    }
    """
    And we add managed backup
    """
    {
        "backup_id": "backup_3",
        "status": "DONE",
        "method": "FULL",
        "initiator": "USER",
        "cid": "cid1",
        "subcid": "subcid2",
        "ts": "1970-01-01T00:00:60+00:00",
        "meta": {
            "name": "stream_19700101T000061Z",
            "before_ts": {"TS": 60, "Inc": 1},
            "after_ts": {"TS": 60, "Inc": 1},
            "data_size": 60,
            "shard_names": ["mongoinfra_subcluster"],
            "root_path": "mongodb-backup/cid1/subcid2"
        }
    }
    """
    And we GET "/mdb/mongodb/1.0/clusters/cid1/backups"
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:51+00:00",
                "folderId": "folder1",
                "id": "cid1:backup_2",
                "size": 51,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard3"],
                "startedAt": "1970-01-01T00:00:50+00:00",
                "type": "MANUAL"
            },
            {
                "createdAt": "1970-01-01T00:00:41+00:00",
                "folderId": "folder1",
                "id": "cid1:backup_1",
                "size": 41,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard2"],
                "startedAt": "1970-01-01T00:00:40+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """


  Scenario: Backup get by Managed id works
    When I successfully execute query
	"""
	DELETE FROM dbaas.backups WHERE cid = 'cid1'
	"""
    And we add managed backup
    """
    {
        "backup_id": "backup_1",
        "status": "DONE",
        "method": "FULL",
        "initiator": "SCHEDULE",
        "cid": "cid1",
        "subcid": "subcid1",
        "shard_id": "shard_id1",
        "ts": "1970-01-01T00:00:41+00:00",
        "meta": {
            "name": "stream_19700101T000041Z",
            "id:": "backup_1",
            "before_ts": {"TS": 40, "Inc": 1},
            "after_ts": {"TS": 41, "Inc": 1},
            "data_size": 41,
            "shard_names": ["shard1"],
            "root_path": "mongodb-backup/cid1/shard1"
        }
    }
    """
    And we GET "/mdb/mongodb/1.0/backups/cid1:backup_1"
    Then we get response with status 200 and body contains
    """
      {
          "createdAt": "1970-01-01T00:00:41+00:00",
          "folderId": "folder1",
          "id": "cid1:backup_1",
          "size": 41,
          "sourceClusterId": "cid1",
          "sourceShardNames": ["shard1"],
          "startedAt": "1970-01-01T00:00:40+00:00",
          "type": "AUTOMATED"
      }
    """

  @delete
  Scenario: After cluster delete its backups are shown
    When we DELETE "/mdb/mongodb/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "Delete MongoDB cluster",
        "id": "worker_task_id2"
    }
    """
    When we GET "/mdb/mongodb/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-01T00:00:41+00:00",
                "folderId": "folder1",
                "size": 40,
                "id": "cid1:man03",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard2"],
                "startedAt": "1970-01-01T00:00:40+00:00",
                "type": "MANUAL"
            },
            {
                "createdAt": "1970-01-01T00:00:36+00:00",
                "folderId": "folder1",
                "id": "cid1:auto02",
                "size": 35,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:35+00:00",
                "type": "AUTOMATED"
            },
            {
                "createdAt": "1970-01-01T00:00:31+00:00",
                "folderId": "folder1",
                "id": "cid1:auto01",
                "size": 30,
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:30+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """
    When we GET "/mdb/mongodb/1.0/backups/cid1:auto01"
    Then we get response with status 200 and body contains
    """
	{
		"createdAt": "1970-01-01T00:00:31+00:00",
		"folderId": "folder1",
		"id": "cid1:auto01",
		"size": 30,
		"sourceClusterId": "cid1",
		"sourceShardNames": ["shard1"],
		"startedAt": "1970-01-01T00:00:30+00:00",
		"type": "AUTOMATED"
	}
    """
    When we GET "/mdb/mongodb/1.0/clusters/cid1/backups"
    Then we get response with status 403
    # backups for purged cluster not available
    When "worker_task_id2" acquired and finished by worker
    And "worker_task_id3" acquired and finished by worker
    And "worker_task_id4" acquired and finished by worker
    And we GET "/mdb/mongodb/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": []
    }
    """
    When we GET "/mdb/mongodb/1.0/backups/cid1:1"
    Then we get response with status 403


  @events
  Scenario: Backup creation works
    When we POST "/mdb/mongodb/1.0/clusters/cid1:backup"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create a backup for MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.BackupClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.BackupCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    When we POST "/mdb/mongodb/1.0/clusters/cid1:backup"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Conflicting operation worker_task_id2 detected"
    }
    """

  Scenario: Managed Backup listing with bad metadata works
    When I successfully execute query
    """
    DELETE FROM dbaas.backups WHERE cid = 'cid1'
    """
    When we enable BackupService for cluster "cid1"
    And we add managed backup
    """
    {
        "backup_id": "backup_1",
        "status": "DONE",
        "method": "FULL",
        "initiator": "SCHEDULE",
        "cid": "cid1",
        "subcid": "subcid1",
        "shard_id": "shard_id1",
        "ts": "1970-01-01T00:00:30+00:00",
        "meta": {
            "name": "stream_19700101T000030Z",
            "before_ts": {"TS": 0, "Inc": 0},
            "after_ts": {"TS": 0, "Inc": 0},
            "data_size": 35,
            "shard_names": ["shard1"],
            "root_path": "mongodb-backup/cid1/shard1",
            "raw_meta": {
                "StartLocalTime": "1970-01-01T03:00:30.000000+03:00",
                "FinishLocalTime": "1970-01-01T03:00:35.000000+03:00",
                "MongoMeta": {
                    "Before": {
                        "LastMajTS": {
                            "TS": 0,
                            "Inc": 0
                        },
                        "LastTS": {
                            "TS": 0,
                            "Inc": 0
                        }
                    },
                    "After": {
                        "LastMajTS": {
                            "TS": 0,
                            "Inc": 0
                        },
                        "LastTS": {
                            "TS": 0,
                            "Inc": 0
                        }
                    }
                }
            }
        }
    }
    """
    And we add managed backup
    """
    {
        "backup_id": "backup_2",
        "status": "DONE",
        "method": "FULL",
        "initiator": "USER",
        "cid": "cid1",
        "subcid": "subcid1",
        "shard_id": "shard_id1",
        "ts": "1970-01-01T00:00:41+00:00",
        "meta": {
            "name": "stream_19700101T000041Z",
            "before_ts": {"TS": 0, "Inc": 0},
            "after_ts": {"TS": 0, "Inc": 0},
            "data_size": 41,
            "shard_names": ["shard1"],
            "root_path": "mongodb-backup/cid1/shard1",
            "raw_meta": {
                "StartLocalTime": "1970-01-01T03:00:40.000000+03:00",
                "FinishLocalTime": "1970-01-01T03:00:42.000000+03:00",
                "MongoMeta": {
                    "Before": {
                        "LastMajTS": {
                            "TS": 0,
                            "Inc": 0
                        },
                        "LastTS": {
                            "TS": 40,
                            "Inc": 1
                        }
                    },
                    "After": {
                        "LastMajTS": {
                            "TS": 0,
                            "Inc": 0
                        },
                        "LastTS": {
                            "TS": 41,
                            "Inc": 1
                        }
                    }
                }
            }
        }
    }
    """
    And we add managed backup
    """
    {
        "backup_id": "backup_3",
        "status": "DONE",
        "method": "FULL",
        "initiator": "SCHEDULE",
        "cid": "cid1",
        "subcid": "subcid1",
        "shard_id": "shard_id1",
        "ts": "1970-01-02T00:00:51+00:00",
        "meta": {
            "name": "stream_19700102T000051Z",
            "before_ts": {"TS": 86450, "Inc": 1},
            "after_ts": {"TS": 86451, "Inc": 1},
            "data_size": 51,
            "shard_names": ["shard2"],
            "root_path": "mongodb-backup/cid1/shard1"
        }
    }
    """
    And we GET "/mdb/mongodb/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-02T00:00:51+00:00",
                "folderId": "folder1",
                "size": 51,
                "id": "cid1:backup_3",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard2"],
                "startedAt": "1970-01-02T00:00:50+00:00",
                "type": "AUTOMATED"
            },
            {
                "createdAt": "1970-01-01T00:00:41+00:00",
                "folderId": "folder1",
                "size": 41,
                "id": "cid1:backup_2",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:40+00:00",
                "type": "MANUAL"
            },
            {
                "createdAt": "1970-01-01T00:00:35+00:00",
                "folderId": "folder1",
                "size": 35,
                "id": "cid1:backup_1",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard1"],
                "startedAt": "1970-01-01T00:00:30+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """


  @events
  Scenario: Backup creation with BackupService works
    When we enable BackupService for cluster "cid1"
    And we POST "/mdb/mongodb/1.0/clusters/cid1:backup"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create a backup for MongoDB cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.BackupClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mongodb.BackupCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        }
    }
    """
    And last managed backup for cluster "cid1" has status "PLANNED"
    When we POST "/mdb/mongodb/1.0/clusters/cid1:backup"
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Conflicting operation worker_task_id2 detected"
    }
    """

  @sharded_backup
  Scenario: Backup of sharded cluster creation with BackupService works
    When I successfully execute query
	"""
	DELETE FROM dbaas.backups WHERE cid = 'cid1'
	"""
    And we POST "/mdb/mongodb/1.0/clusters/cid1:enableSharding" with data
    """
    {
        "mongos": {
            "resources": {
                "resourcePresetId": "s1.porto.3",
                "diskSize": 21474836480,
                "diskTypeId": "local-ssd"
            }
        },
        "mongocfg": {
            "resources": {
                "resourcePresetId": "s1.porto.2",
                "diskSize": 32212254720,
                "diskTypeId": "local-ssd"
            }
        },
        "hostSpecs": [
            {
                "zoneId": "man",
                "type": "MONGOS"
            },
            {
                "zoneId": "sas",
                "type": "MONGOS"
            },
            {
                "zoneId": "man",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "sas",
                "type": "MONGOCFG"
            },
            {
                "zoneId": "vla",
                "type": "MONGOCFG"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id2" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1/shards" with data
    """
    {
        "shardName": "shard2",
        "hostSpecs": [
            {
                "zoneId": "sas",
                "type": "MONGOD"
            }, {
                "zoneId": "vla"
            }
        ]
    }
    """
    Then we get response with status 200
    When "worker_task_id3" acquired and finished by worker
    And we POST "/mdb/mongodb/1.0/clusters/cid1:backup"
    Then we get response with status 200
    And in backups there are "3" jobs


  @events @grpc_api
  Scenario: Managed Backup deleteing works
    When I successfully execute query
	"""
	DELETE FROM dbaas.backups WHERE cid = 'cid1'
	"""
    When we enable BackupService for cluster "cid1"
    And we add managed backup
    """
    {
        "backup_id": "backup_1",
        "status": "DONE",
        "method": "FULL",
        "initiator": "SCHEDULE",
        "cid": "cid1",
        "subcid": "subcid1",
        "shard_id": "shard_id1",
        "ts": "1970-01-01T00:00:41+00:00",
        "meta": {
            "name": "stream_19700101T000041Z",
            "before_ts": {"TS": 40, "Inc": 1},
            "after_ts": {"TS": 41, "Inc": 1},
            "data_size": 41,
            "shard_names": ["shard2"],
            "root_path": "mongodb-backup/cid1/shard1"
      }
    }
    """
    And we add managed backup
    """
    {
        "backup_id": "backup_2",
        "status": "DONE",
        "method": "FULL",
        "initiator": "SCHEDULE",
        "cid": "cid1",
        "subcid": "subcid1",
        "shard_id": "shard_id1",
        "ts": "1970-01-02T00:00:51+00:00",
        "meta": {
            "name": "stream_19700102T000051Z",
            "before_ts": {"TS": 86450, "Inc": 1},
            "after_ts": {"TS": 86451, "Inc": 1},
            "data_size": 51,
            "shard_names": ["shard2"],
            "root_path": "mongodb-backup/cid1/shard1"
      }
    }
    """
    And we GET "/mdb/mongodb/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-02T00:00:51+00:00",
                "folderId": "folder1",
                "size": 51,
                "id": "cid1:backup_2",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard2"],
                "startedAt": "1970-01-02T00:00:50+00:00",
                "type": "AUTOMATED"
            },
            {
                "createdAt": "1970-01-01T00:00:41+00:00",
                "folderId": "folder1",
                "size": 41,
                "id": "cid1:backup_1",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard2"],
                "startedAt": "1970-01-01T00:00:40+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.mongodb.v1.BackupService" with data
    """
    {
        "backup_id": "backup_1"
    }
    """
    Then we get gRPC response with body
        """
    {
        "created_by": "user",
        "description": "Delete given backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.mongodb.v1.DeleteBackupMetadata",
            "backup_id": "backup_1"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    Then managed backup with id "backup_1" has status "OBSOLETE"
    When we GET "/mdb/mongodb/1.0/backups?folderId=folder1&pageSize=5"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
            {
                "createdAt": "1970-01-02T00:00:51+00:00",
                "folderId": "folder1",
                "size": 51,
                "id": "cid1:backup_2",
                "sourceClusterId": "cid1",
                "sourceShardNames": ["shard2"],
                "startedAt": "1970-01-02T00:00:50+00:00",
                "type": "AUTOMATED"
            }
        ]
    }
    """
