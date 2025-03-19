@greenplum
@grpc_api
Feature: Create Compute Greenplum Cluster with maintenance window
    Background:
        Given default headers
        When we add default feature flag "MDB_GREENPLUM_CLUSTER"

    Scenario: Create Greenplum cluster with maintenance window and then update maintenance window via update cluster
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
            "maintenance_window": {
                "weekly_maintenance_window": {
                    "day": "MON",
                    "hour": "1"
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
        And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "folder_id": "folder1",
            "id": "cid1",
            "maintenance_window": {
                "weekly_maintenance_window": {
                    "day": "MON",
                    "hour": "1"
                }
            }
        }
        """
        When we "Update" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1",
            "update_mask": {
                "paths": [
                    "maintenance_window"
                ]
            },
            "maintenance_window": {
                "weekly_maintenance_window": {
                    "day": "TUE",
                    "hour": "2"
                }
            }
        }
        """
        Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Modify the Greenplum cluster",
            "done": true,
            "id": "worker_task_id2",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.UpdateClusterMetadata",
                "cluster_id": "cid1"
            }
        }
        """
        And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "folder_id": "folder1",
            "id": "cid1",
            "maintenance_window": {
                "weekly_maintenance_window": {
                    "day": "TUE",
                    "hour": "2"
                }
            }
        }
        """

    Scenario: Create Greenplum cluster without maintenance window and get cluster info should return any time maintenance window
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
        And "worker_task_id1" acquired and finished by worker
        And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "folder_id": "folder1",
            "id": "cid1",
            "maintenance_window": {
                "anytime": {}
            }
        }
        """

    Scenario: Plan and reschedule maintenance on Greenplum cluster
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
            "maintenance_window": {
                "weekly_maintenance_window": {
                    "day": "MON",
                    "hour": "1"
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
        And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "folder_id": "folder1",
            "id": "cid1",
            "maintenance_window": {
                "weekly_maintenance_window": {
                    "day": "MON",
                    "hour": "1"
                }
            }
        }
        """
        And we run query
        # Plan maintenance task for cid1
        """
        WITH locked_cluster AS (
            SELECT rev AS actual_rev, next_rev, folder_id FROM code.lock_future_cluster('cid1')
        ), planned_task AS (
            SELECT code.plan_maintenance_task(
                i_task_id        => 'worker_task_maintenance_id1',
                i_cid            => 'cid1',
                i_config_id      => 'greenplum_update_tls_certs',
                i_folder_id      => folder_id,
                i_operation_type => 'greenplum_cluster_modify',
                i_task_type      => 'greenplum_cluster_maintenance',
                i_task_args      => '{}',
                i_version        => 2,
                i_metadata       => '{}',
                i_user_id        => 'maintenance',
                i_rev            => actual_rev,
                i_target_rev     => next_rev,
                i_plan_ts        => '2000-01-08 00:00:00+00'::timestamp with time zone,
                i_create_ts      => '2000-01-01 00:00:00+00'::timestamp with time zone,
                i_info           => 'Updating tls certificates'
            ), actual_rev, next_rev
            FROM locked_cluster
        )
        SELECT code.complete_future_cluster_change('cid1', actual_rev, next_rev)
        FROM planned_task;
        """
        When we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
           "clusterId": "cid1",
           "rescheduleType": "SPECIFIC_TIME",
           "delayedUntil": "2000-01-09T00:00:00Z"
        }
        """
        Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Reschedule maintenance in Greenplum cluster",
            "id": "worker_task_id2",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.RescheduleMaintenanceMetadata",
                "cluster_id": "cid1",
                "delayed_until": "2000-01-09T00:00:00Z"
            }
        }
        """
        And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "folder_id": "folder1",
            "id": "cid1",
            "maintenance_window": {
                "weekly_maintenance_window": {
                    "day": "MON",
                    "hour": "1"
                }
            },
            "planned_operation": {
                "latest_maintenance_time": "2000-01-22T00:00:00Z",
                "delayed_until": "2000-01-09T00:00:00Z",
                "next_maintenance_window_time": "2000-01-10T00:00:00Z",
                "info": "Updating tls certificates"
            }
        }
        """
        And we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
           "clusterId": "cid1",
           "rescheduleType": "NEXT_AVAILABLE_WINDOW"
        }
        """
        Then we get gRPC response with body
        """
        {
            "created_by": "user",
            "description": "Reschedule maintenance in Greenplum cluster",
            "id": "worker_task_id3",
            "metadata": {
                "@type": "type.googleapis.com/yandex.cloud.priv.mdb.greenplum.v1.RescheduleMaintenanceMetadata",
                "cluster_id": "cid1",
                "delayed_until": "2000-01-10T00:00:00Z"
            }
        }
        """
        And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "folder_id": "folder1",
            "id": "cid1",
            "maintenance_window": {
                "weekly_maintenance_window": {
                    "day": "MON",
                    "hour": "1"
                }
            },
            "planned_operation": {
                "latest_maintenance_time": "2000-01-22T00:00:00Z",
                "delayed_until": "2000-01-10T00:00:00Z",
                "next_maintenance_window_time": "2000-01-17T00:00:00Z",
                "info": "Updating tls certificates"
            }
        }
        """
        When we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
           "clusterId": "cid1",
           "rescheduleType": "SPECIFIC_TIME",
           "delayedUntil": "2000-01-25T00:00:00Z"
        }
        """
        Then we get gRPC response error with code INVALID_ARGUMENT and message "Maintenance operation time cannot exceed 2000-01-22T00:00:00Z"
        And we "Get" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
        """
        {
            "cluster_id": "cid1"
        }
        """
        Then we get gRPC response with body
        """
        {
            "folder_id": "folder1",
            "id": "cid1",
            "maintenance_window": {
                "weekly_maintenance_window": {
                    "day": "MON",
                    "hour": "1"
                }
            },
            "planned_operation": {
                "latest_maintenance_time": "2000-01-22T00:00:00Z",
                "delayed_until": "2000-01-10T00:00:00Z",
                "next_maintenance_window_time": "2000-01-17T00:00:00Z",
                "info": "Updating tls certificates"
            }
        }
        """
