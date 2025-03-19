Feature: Restore PostgreSQL cluster from backup

  Background:
    Given default headers
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "postgresqlConfig_14": {
               "sharedPreloadLibraries": ["SHARED_PRELOAD_LIBRARIES_PG_HINT_PLAN"]
           },
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test",
           "extensions": [{"name": "pg_hint_plan"}]
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password",
           "grants": ["test_role"]
       }, {
           "name": "test_role",
           "password": "test_role_password",
           "login": false,
           "grants": ["mdb_admin", "mdb_replication"]
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
    Then we get response with status 200 and body contains
    """
    {
        "id": "worker_task_id1"
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
	And I successfully execute query
	"""
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, 'FULL'::dbaas.backup_method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('auto01', 'DONE', '1970-01-01T00:00:01.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:01.000001+00:00'), 'SCHEDULE', '{"name": "auto01","start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb),
		('auto02', 'DONE', '1970-01-01T00:00:01.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:01.000001+00:00'), 'SCHEDULE', '{"name": "auto02", "start_time":"1970-01-01T00:00:10.000001+00:00", "finish_time":"1970-01-01T00:00:11.000002+00:00", "date_fmt":"%Y-%m-%dT%H:%M:%S.%fZ", "compressed_size": 11}'::jsonb))
            AS t (bid, status, ts, scheduled_date, initiator, metadata),
            dbaas.clusters c JOIN dbaas.subclusters sc USING (cid) LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """

  @events
  Scenario: Restoring from backup to original folder works
    When we POST "/mdb/postgresql/1.0/clusters:restore" with data
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
        "time": "1970-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new PostgreSQL cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "restore-from" containing map:
      |      cid                         |           cid1            |
      |    backup-id                     |          auto01           |
      |      time                        | 1970-01-01T00:00:02+00:00 |
      | time-inclusive                   |           false           |
      | restore-latest                   |           false           |
      | use_backup_service_at_latest_rev |           true            |
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.RestoreCluster" event with
    """
    {
        "details": {
            "backup_id": "cid1:auto01",
            "cluster_id": "cid2"
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
            "time": "1970-01-01T00:00:02+00:00"
        }
    }
    """

  Scenario: Restoring from backup to different folder works
    When we POST "/mdb/postgresql/1.0/clusters:restore?folderId=folder2" with data
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
        "time": "1970-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new PostgreSQL cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder2"
    }
    """

  Scenario: Restoring from backup with large user connections works
    When we PATCH "/mdb/postgresql/1.0/clusters/cid1" with data
    """
    {
        "configSpec": {
            "resources": {
                "resourcePresetId": "s1.porto.2"
            }
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we PATCH "/mdb/postgresql/1.0/clusters/cid1/users/test" with data
    """
    {
        "connLimit": 350
    }
    """
    And we POST "/mdb/postgresql/1.0/clusters:restore?folderId=folder1" with data
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
        "time": "1970-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new PostgreSQL cluster from the backup",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    And in worker_queue exists "worker_task_id3" id with args "restore-from" containing map:
      |      cid                         |           cid1            |
      |    backup-id                     |          auto01           |
      |      time                        | 1970-01-01T00:00:02+00:00 |
      | time-inclusive                   |           false           |
      | restore-latest                   |           false           |
      | use_backup_service_at_latest_rev |           true            |
    When we GET "/mdb/postgresql/1.0/clusters/cid2/users"
    Then we get response with status 200 and body contains
    """
    {
        "users": [{
                "clusterId": "cid2",
                "connLimit": 50,
                "name": "test",
                "permissions": [
                    {
                        "databaseName": "testdb"
                    }
                ],
                "settings": {},
                "login": true,
                "grants": ["test_role"]
            }, {
                "clusterId": "cid2",
                "connLimit": 50,
                "name": "test_role",
                "permissions": [
                    {
                        "databaseName": "testdb"
                    }
                ],
                "settings": {},
                "login": false,
                "grants": ["mdb_admin", "mdb_replication"]
            }

        ]
    }
    """

  Scenario: Restoring from backup with wrongly overrided shared_preload_libraries fails
    When we POST "/mdb/postgresql/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "postgresqlConfig_14": {
                "sharedPreloadLibraries": ["SHARED_PRELOAD_LIBRARIES_AUTO_EXPLAIN"]
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskTypeId": "local-ssd",
                "diskSize": 10737418240
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
        "time": "1970-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The specified extension 'pg_hint_plan' is not present in shared_preload_libraries."
    }
    """

  Scenario: Restoring from backup with less disk size fails
    When we POST "/mdb/postgresql/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 4194304,
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
        "time": "1970-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Insufficient diskSize, increase it to '10737418240'"
    }
    """

  Scenario: Restoring from backup with nonexistent backup fails
    When we POST "/mdb/postgresql/1.0/clusters:restore?folderId=folder1" with data
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
        "backupId": "cid1:auto0no",
        "time": "1970-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Backup 'cid1:auto0no' does not exist"
    }
    """

  Scenario: Restoring to future PITR fails
    When we POST "/mdb/postgresql/1.0/clusters:restore?folderId=folder1" with data
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
        "time": "9999-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "It is not possible to restore to future point in time"
    }
    """

  Scenario: Restoring from backup with incorrect time fails
    When we POST "/mdb/postgresql/1.0/clusters:restore?folderId=folder1" with data
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
        "time": "1970-01-01T00:00:01+00:00"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Unable to restore to '1970-01-01 00:00:01+00:00' using this backup, cause it finished at '1970-01-01 00:00:01.000001+00:00' (use older backup or increase 'time')"
    }
    """

  Scenario: Restoring from backup with incorrect disk unit fails
    When we POST "/mdb/postgresql/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 10737418245,
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
        "time": "1970-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Disk size must be a multiple of 4194304 bytes"
    }
    """

  @delete
  Scenario: Restore from backup belongs to deleted cluster
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "Delete PostgreSQL cluster",
        "id": "worker_task_id2"
    }
    """
    When we GET "/mdb/postgresql/1.0/backups/cid1:auto01"
    Then we get response with status 200 and body contains
    """
    {
        "createdAt": "1970-01-01T00:00:01.000001+00:00",
        "folderId": "folder1",
        "id": "cid1:auto01",
        "sourceClusterId": "cid1",
        "startedAt": "1970-01-01T00:00:01.000001+00:00"
    }
    """
    When we POST "/mdb/postgresql/1.0/clusters:restore?folderId=folder1" with data
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
        "time": "1970-01-01T00:00:01.000001+00:00"
    }
    """
    # task_id == worker_task_id4, cause delete = 2, purge = 3
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new PostgreSQL cluster from the backup",
        "done": false,
        "id": "worker_task_id5",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    And in worker_queue exists "worker_task_id5" id with args "restore-from" containing map:
      |      cid                         |               cid1               |
      |    backup-id                     |              auto01              |
      |      time                        | 1970-01-01T00:00:01.000001+00:00 |
      | time-inclusive                   |               false              |
      | restore-latest                   |               false              |
      | use_backup_service_at_latest_rev |               true               |

  Scenario: Restore from backup belongs to deleted cluster to latest restore point
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "Delete PostgreSQL cluster",
        "id": "worker_task_id2"
    }
    """
    When we GET "/mdb/postgresql/1.0/backups/cid1:auto01"
    Then we get response with status 200 and body contains
    """
    {
        "createdAt": "1970-01-01T00:00:01.000001+00:00",
        "folderId": "folder1",
        "id": "cid1:auto01",
        "sourceClusterId": "cid1",
        "startedAt": "1970-01-01T00:00:01.000001+00:00"
    }
    """
    Then I save NOW as "restore_timestamp"
    When we POST "/mdb/postgresql/1.0/clusters:restore?folderId=folder1" with data
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
        "time": "{{ (.restore_timestamp.Add 9000000000).Format "2006-01-02T15:04:05Z07:00" }}"
    }
    """
    # task_id == worker_task_id4, cause delete = 2, purge = 3
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new PostgreSQL cluster from the backup",
        "done": false,
        "id": "worker_task_id5",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    And in worker_queue exists "worker_task_id5" id with args "restore-from" containing map:
      |      cid                         |           cid1                                                                 |
      |    backup-id                     |          auto01                                                                |
      |      time                        | {{ (.restore_timestamp.Add 9000000000).Format "2006-01-02T15:04:05-07:00" }} |
      | time-inclusive                   |          false                                                                 |
      | restore-latest                   |           true                                                                 |
      | use_backup_service_at_latest_rev |           true                                                                 |

  @delete
  Scenario: Restoring from backup belogns to deleted cluster with less disk size fails
    When we DELETE "/mdb/postgresql/1.0/clusters/cid1"
    Then we get response with status 200
    When we POST "/mdb/postgresql/1.0/clusters:restore?folderId=folder1" with data
    """
    {
        "name": "test_restored",
        "environment": "PRESTABLE",
        "configSpec": {
            "resources": {
                "diskSize": 4194304,
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
        "time": "1970-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 9,
        "message": "Insufficient diskSize, increase it to '10737418240'"
    }
    """

  Scenario: Restoring IDM cluster from backup works
    When we run query
    """
    UPDATE dbaas.pillar
       SET value = jsonb_insert(value, '{data,sox_audit}', 'true')
     WHERE cid = 'cid1'
    """
    And we POST "/mdb/postgresql/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "idm_user",
            "password": "password",
            "connLimit": 10,
            "grants": ["reader", "writer"]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "description": "Create user in PostgreSQL cluster",
        "id": "worker_task_id2"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And all "cid1" revs committed before "1980-01-01T01:00:00+00:00"
    And we POST "/mdb/postgresql/1.0/clusters:restore" with data
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
        "time": "1980-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new PostgreSQL cluster from the backup",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    And in worker_queue exists "worker_task_id3" id with args "restore-from" containing map:
      |      cid                         |           cid1            |
      |    backup-id                     |          auto01           |
      |      time                        | 1980-01-01T00:00:02+00:00 |
      | time-inclusive                   |           false           |
      | restore-latest                   |           false           |
      | use_backup_service_at_latest_rev |           true            |
    When we GET "/mdb/postgresql/1.0/clusters/cid2/users/idm_user"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid2",
        "name": "idm_user",
        "grants": []
    }
    """

  @timetravel
  Scenario: Restoring cluster on time before database delete
    . restored cluster should have that deleted database
    When we POST "/mdb/postgresql/1.0/clusters/cid1/databases" with data
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
    And we DELETE "/mdb/postgresql/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200
    When we POST "/mdb/postgresql/1.0/clusters:restore" with data
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
            "@type": "yandex.cloud.mdb.postgresql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    When we GET "/mdb/postgresql/1.0/clusters/cid2/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": [{
            "clusterId": "cid2",
            "extensions": [{"name": "pg_hint_plan"}],
            "lcCollate": "C",
            "lcCtype": "C",
            "name": "testdb",
            "owner": "test"
        }]
    }
    """

  @security_groups
  Scenario: Restoring from backup to original folder with security group works
    When we POST "/mdb/postgresql/1.0/clusters:restore" with data
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
        "securityGroupIds": ["sg_id1", "sg_id3"],
        "backupId": "cid1:auto01",
        "time": "1970-01-01T00:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new PostgreSQL cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.RestoreClusterMetadata",
            "backupId": "cid1:auto01",
            "clusterId": "cid2"
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" containing:
      |sg_id1|
      |sg_id3|
    And in worker_queue exists "worker_task_id2" id with args "restore-from" containing map:
      |      cid                          |           cid1            |
      |    backup-id                      |          auto01           |
      |      time                         | 1970-01-01T00:00:02+00:00 |
      | time-inclusive                    |           false           |
      | restore-latest                    |           false           |
      |  use_backup_service_at_latest_rev |           true            |
    When "worker_task_id2" acquired and finished by worker
    And worker set "sg_id1,sg_id3" security groups on "cid2"
    When we GET "/mdb/postgresql/1.0/clusters/cid2"
    Then we get response with status 200 and body contains
    """
    {
        "folderId": "folder1",
        "securityGroupIds": ["sg_id1", "sg_id3"]
    }
    """
  Scenario: Restoring from backup after enable backup service works
    When we add managed backup
    """
    {
        "backup_id": "bid1",
        "status": "DONE",
        "method": "FULL",
        "initiator": "SCHEDULE",
        "cid": "cid1",
        "subcid": "subcid1",
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
    When we enable BackupService for cluster "cid1"
    And we POST "/mdb/postgresql/1.0/clusters:restore?folderId=folder1" with data
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
        "backupId": "cid1:bid1",
        "time": "1970-01-01T01:00:02+00:00"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create new PostgreSQL cluster from the backup",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.postgresql.v1.RestoreClusterMetadata",
            "backupId": "cid1:bid1",
            "clusterId": "cid2"
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "restore-from" containing map:
      |      cid                         |           cid1            |
      |    backup-id                     |           base0           |
      |      time                        | 1970-01-01T01:00:02+00:00 |
      | time-inclusive                   |           false           |
      | restore-latest                   |           false           |
      | use_backup_service_at_latest_rev |           true            |
