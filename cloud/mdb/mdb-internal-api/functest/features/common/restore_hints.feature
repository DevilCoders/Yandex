@restore-hints
Feature: Common restore hints logic

  Background:
    Given default headers

  Scenario: Non existed backup
    Given s3 response
    """
    {
        "Contents": []
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
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
    Then we get response with status 200
    When we GET "/mdb/postgresql/1.0/console/restore-hints/cid1:base_0"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Backup 'cid1:base_0' does not exist"
    }
    """

  Scenario: Non existed cluster
    When we GET "/mdb/postgresql/1.0/console/restore-hints/cid2:base_42"
    Then we get response with status 403 and body contains
    """
    {
        "code": 7,
        "message": "You do not have permission to access the requested object or object does not exist"
    }
    """

  Scenario: Cluster in compute
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {},
           "resources": {
               "resourcePresetId": "s1.compute.1",
               "diskTypeId": "network-ssd",
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
           "zoneId": "vla"
       }, {
           "zoneId": "sas"
       }],
       "description": "test cluster",
       "networkId": "network1"
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
		('auto01', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb),
		('auto02', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    And we GET "/mdb/postgresql/1.0/console/restore-hints/cid1:auto01"
    Then we get response with status 200 and body equals
    """
    {
        "environment": "PRESTABLE",
        "networkId": "network1",
        "resources": {
            "diskSize": 10737418240,
            "resourcePresetId": "s1.compute.1"
        },
        "time": "1970-01-01T00:01:10.000001+00:00",
        "version": "14"
    }
    """

  Scenario: Decommissioning resourcePreset
    Given feature flags
    """
    ["MDB_ALLOW_DECOMMISSIONED_FLAVOR_USE"]
    """
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "resources": {
               "resourcePresetId": "s1.porto.legacy",
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
    Then we get response with status 200
    When all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
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
    Given feature flags
    """
    []
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
