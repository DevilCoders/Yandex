@restore-hints
Feature: Restore MongoDB cluster from backup with hints

  Background:
    Given default headers

  Scenario Outline: Restore hints for different MongoDB versions
    Given feature flags
    """
    ["MDB_MONGODB_ALLOW_DEPRECATED_VERSIONS"]
    """
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_<spec_version_suffix>": {
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
        "hostSpecs": [{ "zoneId": "myt" }]
    }
    """
    Then we get response with status 200
    When all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T03:00:30.000000+03:00'::timestamptz, DATE('1970-01-01T03:00:30.000000+03:00'), 'SCHEDULE', '{ "before_ts": {"TS": 30, "Inc": 1}, "after_ts": {"TS": 31, "Inc": 6 }, "shard_names": ["shard1"], "permanent": false,"data_size": 30, "name": "stream1", "root_path": "mongodb-backup/cid1/shard1"}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    And we GET "/mdb/mongodb/1.0/clusters/cid1/backups"
    Then we get response with status 200 and body contains
    """
    {
        "backups": [
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
    When we GET "/mdb/mongodb/1.0/console/restore-hints/cid1:auto01"
    Then we get response with status 200 and body equals
    """
    {
        "environment": "PRESTABLE",
        "networkId": "",
        "resources": {
            "diskSize": 10737418240,
            "resourcePresetId": "s1.porto.1"
        },
        "version": "<version>",
        "time": "1970-01-01T00:00:32+00:00"
    }
    """

    Examples:
    | version | spec_version_suffix |
    | 4.0     | 4_0                 |
    | 4.2     | 4_2                 |


  Scenario: Restore disk_size depends on cluster disk_size at restore time
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
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
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    When I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T03:00:30.000000+03:00'::timestamptz, DATE('1970-01-01T03:00:30.000000+03:00'), 'SCHEDULE', '{ "before_ts": {"TS": 30, "Inc": 1}, "after_ts": {"TS": 31, "Inc": 6 }, "shard_names": ["shard1"], "permanent": false,"data_size": 30, "name": "stream1", "root_path": "mongodb-backup/cid1/shard1"}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    When we PATCH "/mdb/mongodb/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 21474836480
            }
        }
    }
    """
    When we GET "/mdb/mongodb/1.0/console/restore-hints/cid1:auto01"
    Then we get response with status 200 and body contains
    """
    {
        "resources": {
            "diskSize": 10737418240,
            "resourcePresetId": "s1.porto.1"
        }
    }
    """

