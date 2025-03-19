@greenplum
@grpc_api
Feature: Backup Greenplum cluster
  Background:
    Given default headers
    And we add default feature flag "MDB_GREENPLUM_CLUSTER"
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "environment": "PRESTABLE",
        "name": "test",
        "description": "test cluster",
        "labels": {
            "foo": "bar"
        },
        "network_id": "network1",
        "config": {
            "zone_id": "myt",
            "subnet_id": "",
            "assign_public_ip": false
        },
        "master_config": {
            "resources": {
                "resourcePresetId": "s1.compute.2",
                "diskTypeId": "network-ssd",
                "diskSize": 10737418240
            }
        },
        "segment_config": {
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "network-ssd",
                "diskSize": 10737418240
            }
        },
        "master_host_count": 2,
        "segment_in_host": 1,
        "segment_host_count": 4,
        "user_name": "usr1",
        "user_password": "Pa$$w0rd"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create Greenplum cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: Backup list works with backup service
    When I successfully execute query
    """
      SELECT * FROM code.set_backup_service_use(
        i_cid => 'cid1',
        i_val => true
    )
    """
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, t.method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('backup1', 'DONE', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371870}'::jsonb, 'FULL'::dbaas.backup_method),
		('backup2', 'DONE', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371871}'::jsonb, 'FULL'::dbaas.backup_method))
            AS t (bid, status, ts, scheduled_date, initiator, metadata, method),
            dbaas.clusters c JOIN dbaas.subclusters sc ON (c.cid = sc.cid AND sc.roles[1] = 'greenplum_cluster.master_subcluster') LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.BackupService" with data
    """
    {
        "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "2021-12-05T22:57:56.000001Z",
                "folder_id": "folder1",
                "id": "cid1:backup1",
                "source_cluster_id": "cid1",
                "started_at": "2021-12-05T22:57:56.000001Z",
                "size": "8371870"
            },
            {
                "created_at": "2021-12-05T22:57:56.000001Z",
                "folder_id": "folder1",
                "id": "cid1:backup2",
                "source_cluster_id": "cid1",
                "started_at": "2021-12-05T22:57:56.000001Z",
                "size": "8371871"
            }
        ]
    }
    """

  Scenario: Backup list works without backup service
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "wal-e/cid1/6/basebackups_005/1_backup_stop_sentinel.json",
                "LastModified": 1,
                "Body": {
                    "user_data": {
                        "backup_id": "backup1"
                    },
                    "start_time": "2021-12-05T22:57:30.478629Z",
                    "finish_time": "2021-12-05T22:57:56.754577Z",
                    "compressed_size": 8371870
                }
            },
            {
                "Key": "wal-e/cid1/6/basebackups_005/cronz45ngzingf82hxuw3dbtpq1_restore_point.json",
                "LastModified": 1,
                "Body": {
                    "name":"cronoibe0a6ie0rqnp0xk20380v",
                    "start_time":"2022-01-27T20:00:01.427207Z",
                    "finish_time":"2022-01-27T20:00:01.472855Z",
                    "hostname":"fqdn.db.yandex.net",
                    "gp_version":"6.19.5",
                    "system_identifier":null,
                    "lsn_by_segment": {
                        "-1":"3/8048300",
                        "0":"3/41B7118",
                        "1":"3/4288190"
                    }
                }
            },
            {
                "Key": "wal-e/cid1/6/basebackups_005/2_backup_stop_sentinel.json",
                "LastModified": 1,
                "Body": {
                    "user_data": {
                        "backup_id": "backup2"
                    },
                    "start_time": "2021-12-05T22:57:30.429Z",
                    "finish_time": "2021-12-05T22:57:56.754Z",
                    "compressed_size": 8371871
                }
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.BackupService" with data
    """
    {
        "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "2021-12-05T22:57:56.754577Z",
                "folder_id": "folder1",
                "id": "cid1:backup1",
                "source_cluster_id": "cid1",
                "started_at": "2021-12-05T22:57:30.478629Z",
                "size": "8371870"
            },
            {
                "created_at": "2021-12-05T22:57:56.754Z",
                "folder_id": "folder1",
                "id": "cid1:backup2",
                "source_cluster_id": "cid1",
                "started_at": "2021-12-05T22:57:30.429Z",
                "size": "8371871"
            }
        ]
    }
    """

  Scenario: Backup list with ignoring entries works with backup service
    When I successfully execute query
    """
      SELECT * FROM code.set_backup_service_use(
        i_cid => 'cid1',
        i_val => true
    )
    """
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, t.method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('backup1', 'DELETING', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371870}'::jsonb, 'FULL'::dbaas.backup_method),
		('backup2', 'DELETED', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371870}'::jsonb, 'FULL'::dbaas.backup_method),
		('backup3', 'DELETE-ERROR', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371870}'::jsonb, 'FULL'::dbaas.backup_method),
		('backup4', 'PLANNED', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371870}'::jsonb, 'FULL'::dbaas.backup_method),
		('backup5', 'DONE', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371870}'::jsonb, 'FULL'::dbaas.backup_method),
		('backup6', 'CREATE-ERROR', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371870}'::jsonb, 'FULL'::dbaas.backup_method),
		('backup7', 'CREATING', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371871}'::jsonb, 'FULL'::dbaas.backup_method))
            AS t (bid, status, ts, scheduled_date, initiator, metadata, method),
            dbaas.clusters c JOIN dbaas.subclusters sc ON (c.cid = sc.cid AND sc.roles[1] = 'greenplum_cluster.master_subcluster') LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.BackupService" with data
    """
    {
        "folder_id": "folder1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "2021-12-05T22:57:56.000001Z",
                "folder_id": "folder1",
                "id": "cid1:backup5",
                "source_cluster_id": "cid1",
                "started_at": "2021-12-05T22:57:56.000001Z",
                "size": "8371870"
            }
        ]
    }
    """

  Scenario: Backup list with page size works
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "wal-e/cid1/6/basebackups_005/1_backup_stop_sentinel.json",
                "LastModified": 1,
                "Body": {
                    "user_data": {
                        "backup_id": "backup1"
                    },
                    "start_time": "2021-12-05T22:57:30.478629Z",
                    "finish_time": "2021-12-05T22:57:56.754577Z",
                    "compressed_size": 8371870
                }
            },
            {
                "Key": "wal-e/cid1/6/basebackups_005/cronz45ngzingf82hxuw3dbtpq1_restore_point.json",
                "LastModified": 1,
                "Body": {
                    "name":"cronoibe0a6ie0rqnp0xk20380v",
                    "start_time":"2022-01-27T20:00:01.427207Z",
                    "finish_time":"2022-01-27T20:00:01.472855Z",
                    "hostname":"fqdn.db.yandex.net",
                    "gp_version":"6.19.5",
                    "system_identifier":null,
                    "lsn_by_segment": {
                        "-1":"3/8048300",
                        "0":"3/41B7118",
                        "1":"3/4288190"
                    }
                }
            },
            {
                "Key": "wal-e/cid1/6/basebackups_005/2_backup_stop_sentinel.json",
                "LastModified": 1,
                "Body": {
                    "user_data": {
                        "backup_id": "backup2"
                    },
                    "start_time": "2021-12-05T22:57:30.478629Z",
                    "finish_time": "2021-12-05T22:57:56.754577Z",
                    "compressed_size": 8371871
                }
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.BackupService" with data
    """
    {
        "folder_id": "folder1",
        "page_size": 1
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "2021-12-05T22:57:56.754577Z",
                "folder_id": "folder1",
                "id": "cid1:backup1",
                "source_cluster_id": "cid1",
                "started_at": "2021-12-05T22:57:30.478629Z",
                "size": "8371870"
            }
        ],
        "next_page_token": "eyJDbHVzdGVySUQiOiJjaWQxIiwiQmFja3VwSUQiOiJiYWNrdXAxIn0="
    }
    """

  Scenario: Backup list with page token works
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "wal-e/cid1/6/basebackups_005/1_backup_stop_sentinel.json",
                "LastModified": 1,
                "Body": {
                    "user_data": {
                        "backup_id": "backup1"
                    },
                    "start_time": "2021-12-05T22:57:30.478629Z",
                    "finish_time": "2021-12-05T22:57:56.754577Z",
                    "compressed_size": 8371870
                }
            },
            {
                "Key": "wal-e/cid1/6/basebackups_005/cronz45ngzingf82hxuw3dbtpq1_restore_point.json",
                "LastModified": 1,
                "Body": {
                    "name":"cronoibe0a6ie0rqnp0xk20380v",
                    "start_time":"2022-01-27T20:00:01.427207Z",
                    "finish_time":"2022-01-27T20:00:01.472855Z",
                    "hostname":"fqdn.db.yandex.net",
                    "gp_version":"6.19.5",
                    "system_identifier":null,
                    "lsn_by_segment": {
                        "-1":"3/8048300",
                        "0":"3/41B7118",
                        "1":"3/4288190"
                    }
                }
            },
            {
                "Key": "wal-e/cid1/6/basebackups_005/2_backup_stop_sentinel.json",
                "LastModified": 1,
                "Body": {
                    "user_data": {
                        "backup_id": "backup2"
                    },
                    "start_time": "2021-12-05T22:57:30.478629Z",
                    "finish_time": "2021-12-05T22:57:56.754577Z",
                    "compressed_size": 8371871
                }
            }
        ]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.BackupService" with data
    """
    {
        "folder_id": "folder1",
        "page_token": "eyJDbHVzdGVySUQiOiJjaWQxIiwiQmFja3VwSUQiOiJiYWNrdXAxIn0="
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "2021-12-05T22:57:56.754577Z",
                "folder_id": "folder1",
                "id": "cid1:backup2",
                "source_cluster_id": "cid1",
                "started_at": "2021-12-05T22:57:30.478629Z",
                "size": "8371871"
            }
        ]
    }
    """

  Scenario: Backup list by cid works without backup service
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "wal-e/cid1/6/basebackups_005/1_backup_stop_sentinel.json",
                "LastModified": 1,
                "Body": {
                    "user_data": {
                        "backup_id": "backup1"
                    },
                    "start_time": "2021-12-05T22:57:30.478629Z",
                    "finish_time": "2021-12-05T22:57:56.754577Z",
                    "compressed_size": 8371870
                }
            },
            {
                "Key": "wal-e/cid1/6/basebackups_005/cronz45ngzingf82hxuw3dbtpq1_restore_point.json",
                "LastModified": 1,
                "Body": {
                    "name":"cronoibe0a6ie0rqnp0xk20380v",
                    "start_time":"2022-01-27T20:00:01.427207Z",
                    "finish_time":"2022-01-27T20:00:01.472855Z",
                    "hostname":"fqdn.db.yandex.net",
                    "gp_version":"6.19.5",
                    "system_identifier":null,
                    "lsn_by_segment": {
                        "-1":"3/8048300",
                        "0":"3/41B7118",
                        "1":"3/4288190"
                    }
                }
            },
            {
                "Key": "wal-e/cid1/6/basebackups_005/2_backup_stop_sentinel.json",
                "LastModified": 1,
                "Body": {
                    "user_data": {
                        "backup_id": "backup2"
                    },
                    "start_time": "2021-12-05T22:57:30.478629Z",
                    "finish_time": "2021-12-05T22:57:56.754577Z",
                    "compressed_size": 8371871
                }
            }
        ]
    }
    """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "2021-12-05T22:57:56.754577Z",
                "folder_id": "folder1",
                "id": "cid1:backup1",
                "source_cluster_id": "cid1",
                "started_at": "2021-12-05T22:57:30.478629Z",
                "size": "8371870"
            },
            {
                "created_at": "2021-12-05T22:57:56.754577Z",
                "folder_id": "folder1",
                "id": "cid1:backup2",
                "source_cluster_id": "cid1",
                "started_at": "2021-12-05T22:57:30.478629Z",
                "size": "8371871"
            }
        ]
    }
    """

  Scenario: Backup list by cid works with backup service
    When I successfully execute query
    """
      SELECT * FROM code.set_backup_service_use(
        i_cid => 'cid1',
        i_val => true
    )
    """
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, t.method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('backup1', 'DONE', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371870}'::jsonb, 'FULL'::dbaas.backup_method),
		('backup2', 'DONE', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371871}'::jsonb, 'FULL'::dbaas.backup_method))
            AS t (bid, status, ts, scheduled_date, initiator, metadata, method),
            dbaas.clusters c JOIN dbaas.subclusters sc ON (c.cid = sc.cid AND sc.roles[1] = 'greenplum_cluster.master_subcluster') LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": [
            {
                "created_at": "2021-12-05T22:57:56.000001Z",
                "folder_id": "folder1",
                "id": "cid1:backup1",
                "source_cluster_id": "cid1",
                "started_at": "2021-12-05T22:57:56.000001Z",
                "size": "8371870"
            },
            {
                "created_at": "2021-12-05T22:57:56.000001Z",
                "folder_id": "folder1",
                "id": "cid1:backup2",
                "source_cluster_id": "cid1",
                "started_at": "2021-12-05T22:57:56.000001Z",
                "size": "8371871"
            }
        ]
    }
    """

  Scenario: Backup list on cluster with no backups works without backup service
    Given s3 response
    """
    {}
    """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": []
    }
    """

  Scenario: Backup list on cluster with no backups works with backup service
    When I successfully execute query
    """
      SELECT * FROM code.set_backup_service_use(
        i_cid => 'cid1',
        i_val => true
    )
    """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "backups": []
    }
    """

  Scenario: Backup get by id works without backup service
    Given s3 response
    """
    {
        "Contents": [
            {
                "Key": "wal-e/cid1/6/basebackups_005/1_backup_stop_sentinel.json",
                "LastModified": 1,
                "Body": {
                    "user_data": {
                        "backup_id": "backup1"
                    },
                    "start_time": "2021-12-05T22:57:30.478629Z",
                    "finish_time": "2021-12-05T22:57:56.754577Z",
                    "compressed_size": 8371870
                }
            },
            {
                "Key": "wal-e/cid1/6/basebackups_005/cronz45ngzingf82hxuw3dbtpq1_restore_point.json",
                "LastModified": 1,
                "Body": {
                    "name":"cronoibe0a6ie0rqnp0xk20380v",
                    "start_time":"2022-01-27T20:00:01.427207Z",
                    "finish_time":"2022-01-27T20:00:01.472855Z",
                    "hostname":"fqdn.db.yandex.net",
                    "gp_version":"6.19.5",
                    "system_identifier":null,
                    "lsn_by_segment": {
                        "-1":"3/8048300",
                        "0":"3/41B7118",
                        "1":"3/4288190"
                    }
                }
            },
            {
                "Key": "wal-e/cid1/6/basebackups_005/2_backup_stop_sentinel.json",
                "LastModified": 1,
                "Body": {
                    "user_data": {
                        "backup_id": "backup2"
                    },
                    "start_time": "2021-12-05T22:57:30.478629Z",
                    "finish_time": "2021-12-05T22:57:56.754577Z",
                    "compressed_size": 8371871
                }
            }
        ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:backup2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_at": "2021-12-05T22:57:56.754577Z",
        "folder_id": "folder1",
        "id": "cid1:backup2",
        "source_cluster_id": "cid1",
        "started_at": "2021-12-05T22:57:30.478629Z",
        "size": "8371871"
    }
    """

  Scenario: Backup get by id works with backup service
    When I successfully execute query
    """
      SELECT * FROM code.set_backup_service_use(
        i_cid => 'cid1',
        i_val => true
    )
    """
    And I successfully execute query
    """
    INSERT INTO dbaas.backups (backup_id, status, scheduled_date, initiator, method, created_at, delayed_until, finished_at, started_at, cid, subcid, shard_id, metadata)
        SELECT
          t.bid, t.status::dbaas.backup_status, t.scheduled_date, t.initiator::dbaas.backup_initiator, t.method, t.ts, t.ts, t.ts, t.ts, c.cid, sc.subcid, sh.shard_id, t.metadata
        FROM (VALUES
		('backup1', 'DONE', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371870}'::jsonb, 'FULL'::dbaas.backup_method),
		('backup2', 'DONE', '2021-12-05T22:57:56.000001+00:00'::timestamptz, DATE('1970-01-01T00:00:10.000001+00:00'), 'SCHEDULE', '{"compressed_size": 8371871}'::jsonb, 'FULL'::dbaas.backup_method))
            AS t (bid, status, ts, scheduled_date, initiator, metadata, method),
            dbaas.clusters c JOIN dbaas.subclusters sc ON (c.cid = sc.cid AND sc.roles[1] = 'greenplum_cluster.master_subcluster') LEFT JOIN dbaas.shards sh USING (subcid)
        WHERE c.cid = 'cid1';
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.BackupService" with data
    """
    {
        "backup_id": "cid1:backup2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_at": "2021-12-05T22:57:56.000001Z",
        "folder_id": "folder1",
        "id": "cid1:backup2",
        "source_cluster_id": "cid1",
        "started_at": "2021-12-05T22:57:56.000001Z",
        "size": "8371871"
    }
    """

