@greenplum
@grpc_api
Feature: Restore Greenplum cluster from backup

  Background:
    Given headers
        """
        {
            "X-YaCloud-SubjectToken": "rw-token",
            "Accept": "application/json",
            "Content-Type": "application/json",
            "Access-Id": "00000000-0000-0000-0000-000000000000",
            "Access-Secret": "dummy"
        }
        """
        When we add default feature flag "MDB_GREENPLUM_CLUSTER"
        When we "Create" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
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
        When "worker_task_id1" acquired and finished by worker
        And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"
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
        And I successfully execute query
        """
          SELECT * FROM code.set_backup_service_use(
            i_cid => 'cid1',
            i_val => true
        )
        """

  Scenario: Restoring from backup to original folder works
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "backup_id": "cid1:backup1",
        "folder_id": "folder1",
        "environment": "PRESTABLE",
        "name": "test_restored",
        "description": "restored test cluster",
        "labels": {
            "foo1": "bar1"
        },
        "network_id": "network1",
        "config": {
            "zone_id": "vla",
            "subnet_id": "",
            "assign_public_ip": true
        },
        "master_resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "network-ssd",
                "diskSize": 10737418240
        },
        "segment_resources": {
            "resourcePresetId": "s1.compute.2",
            "diskTypeId": "network-ssd",
            "diskSize": 10737418240
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Restore Greenplum cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.RestoreClusterMetadata",
            "backup_id": "cid1:backup1",
            "cluster_id": "cid2"
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "restore-from" containing map:
      |      cid                         |           cid1             |
      |    backup-id                     |          backup1           |
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "id": "cid2",
        "folder_id": "folder1",
        "environment": "PRESTABLE",
        "name": "test_restored",
        "description": "restored test cluster",
        "labels": {
            "foo1": "bar1"
        },
        "network_id": "network1",
        "config": {
            "access": {
                "data_lens": false,
                "web_sql": false,
                "data_transfer": false,
                "serverless": false
            },
            "version": "6.19",
            "zone_id": "vla",
            "subnet_id": "",
            "assign_public_ip": true,
            "segment_mirroring_enable": true,
            "segment_auto_rebalance": null,
            "backup_window_start": {
                "hours":22,
                "minutes":15,
                "seconds":30,
                "nanos":100
            }
        },
        "status": "CREATING",
        "master_host_count": "2",
        "segment_in_host": "1",
        "segment_host_count": "4",
        "user_name": "usr1",
        "master_config": {
            "resources": {
                "disk_size": "10737418240",
                "disk_type_id": "network-ssd",
                "resource_preset_id": "s1.compute.1"
            },
            "config": null
        },
        "segment_config": {
            "resources": {
                "disk_size": "10737418240",
                "disk_type_id": "network-ssd",
                "resource_preset_id": "s1.compute.2"
            },
            "config": null

        }
    }
    """

  Scenario: Restoring from backup to different folder works
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "backup_id": "cid1:backup1",
        "folder_id": "folder2",
        "environment": "PRESTABLE",
        "name": "test_restored",
        "description": "restored test cluster",
        "labels": {
            "foo1": "bar1"
        },
        "network_id": "network1",
        "config": {
            "zone_id": "vla",
            "subnet_id": "",
            "assign_public_ip": true
        },
        "master_resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "network-ssd",
                "diskSize": 10737418240
        },
        "segment_resources": {
            "resourcePresetId": "s1.compute.2",
            "diskTypeId": "network-ssd",
            "diskSize": 10737418240
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Restore Greenplum cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.RestoreClusterMetadata",
            "backup_id": "cid1:backup1",
            "cluster_id": "cid2"
        }
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "restore-from" containing map:
      |      cid                         |           cid1             |
      |    backup-id                     |          backup1           |
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "folder_id": "folder2"
    }
    """

  Scenario: Restoring from backup with less disk size fails
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "backup_id": "cid1:backup1",
        "folder_id": "folder1",
        "environment": "PRESTABLE",
        "name": "test_restored",
        "description": "restored test cluster",
        "labels": {
            "foo1": "bar1"
        },
        "network_id": "network1",
        "config": {
            "zone_id": "vla",
            "subnet_id": "",
            "assign_public_ip": true
        },
        "master_resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "network-ssd",
                "diskSize": 1073741824
        },
        "segment_resources": {
            "resourcePresetId": "s1.compute.2",
            "diskTypeId": "network-ssd",
            "diskSize": 1073741824
        }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "insufficient diskSize of master hosts, increase it to '10737418240'"


  Scenario: Restoring from backup with nonexistent backup fails
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "backup_id": "cid1:somerandombackupname",
        "folder_id": "folder1",
        "environment": "PRESTABLE",
        "name": "test_restored",
        "description": "restored test cluster",
        "labels": {
            "foo1": "bar1"
        },
        "network_id": "network1",
        "config": {
            "zone_id": "vla",
            "subnet_id": "",
            "assign_public_ip": true
        },
        "master_resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "network-ssd",
                "diskSize": 10737418240
        },
        "segment_resources": {
            "resourcePresetId": "s1.compute.2",
            "diskTypeId": "network-ssd",
            "diskSize": 10737418240
        }
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "backup "cid1:somerandombackupname" does not exist"
