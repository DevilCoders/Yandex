Feature: Restore MySQL cluster from backup

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
        }]
    }
    """
    And "worker_task_id1" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, t.method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb, 'FULL'::dbaas.backup_method),
		('auto02', 'DONE', '1970-01-01T00:00:10.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb, 'FULL'::dbaas.backup_method))
            AS t (bid, status, ts, scheduled_date, initiator, metadata, method),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """

  @events
  Scenario: Restoring from backup to original folder works
    And we POST "/mdb/mysql/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto01",
        "time": "1970-01-02T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new MySQL cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.RestoreCluster" event with
    """
    {
        "details": {
            "cluster_id": "cid2",
            "backup_id": "cid1:auto01"
        },
        "request_parameters": {
            "name": "test_restored",
            "environment": "PRESTABLE",
            "config_spec": {
                "resources": {
                    "disk_size": 10737418240,
                    "disk_type_id": "local-ssd",
                    "resource_preset_id": "s1.porto.1"
                }
            },
            "host_specs": [{
                "zone_id": "myt"
            }, {
                "zone_id": "iva"
            }, {
                "zone_id": "sas"
            }],
            "backup_id": "cid1:auto01",
            "time": "1970-01-02T00:00:02+00:00"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder1"
    }
    """

  Scenario: Restoring from backup to different folder works
    When we POST "/mdb/mysql/1.0/clusters:restore?folderId=folder2" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto01",
        "time": "1970-01-02T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new MySQL cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder2"
    }
    """

  Scenario: Restoring from backup with less disk size fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 21474836480
            }
        }
    }
    """
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we POST "/mdb/mysql/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto01",
        "time": "1970-01-02T00:00:02+00:00"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Insufficient diskSize, increase it to '21474836480'"
    }
    """

  Scenario: Restoring from backup with nonexistent backup fails
    When we POST "/mdb/mysql/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:base_7777",
        "time": "1970-01-02T00:00:02+00:00"
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Backup 'cid1:base_7777' does not exist"
    }
    """

  Scenario: Restoring to future PITR fails
    When we POST "/mdb/mysql/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto01",
        "time": "9999-01-02T00:00:02+00:00"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "It is not possible to restore to future point in time"
    }
    """

  @delete
  Scenario: Restoring from backup belogns to deleted cluster
    When we DELETE "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200
    When all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we POST "/mdb/mysql/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto01",
        "time": "1970-01-02T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new MySQL cluster from the backup"
    }
    """

  @delete
  Scenario: Restoring from backup belogns to deleted cluster with less disk size fails
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "diskSize": 21474836480
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "description": "Modify MySQL cluster",
        "id": "worker_task_id2"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we DELETE "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200
    When all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
    And we POST "/mdb/mysql/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "backupId": "cid1:auto01",
        "time": "1970-01-02T00:00:02+00:00"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Insufficient diskSize, increase it to '21474836480'"
    }
    """

  @timetravel
  Scenario: Restoring cluster on time before database delete
    . restored cluster should have that deleted database
    When we POST "/mdb/mysql/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "other_database",
            "owner": "test"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id2"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we DELETE "/mdb/mysql/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200
    When we POST "/mdb/mysql/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "hostSpecs": [{ "zoneId": "iva" }],
        "backupId": "cid1:auto01",
        "time": "1980-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid2/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": [{
            "clusterId": "cid2",
            "name": "testdb"
        }]
    }
    """

  Scenario: Restoring from backup to original folder with security group works
    And we POST "/mdb/mysql/1.0/clusters:restore" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418240,
                "diskTypeId": "local-ssd",
                "resourcePresetId": "s1.porto.1"
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "securityGroupIds": ["sg_id1", "sg_id4"],
        "backupId": "cid1:auto01",
        "time": "1970-01-02T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new MySQL cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" containing:
      |sg_id1|
      |sg_id4|
    When "worker_task_id2" acquired and finished by worker
    And worker set "sg_id1,sg_id4" security groups on "cid2"
    When we GET "/mdb/mysql/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder1",
        "securityGroupIds": ["sg_id1", "sg_id4"]
    }
    """

