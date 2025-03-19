@maintenance
Feature: Management of ClickHouse maintenance window

Background:
    Given default headers

Scenario: Create cluster with default maintenance window settings
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
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
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }]
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id1" acquired and finished by worker
    And we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "cid1",
        "plannedOperation": null,
        "maintenanceWindow": {
            "anytime": {}
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "id": "cid1",
        "planned_operation": null,
        "maintenance_window": {
            "anytime": {}
        }
    }
    """

Scenario: Create cluster with custom maintenance window settings
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
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
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }]
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "maintenance_window": {
            "weekly_maintenance_window": {
                "day": "MON",
                "hour": 1
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id1" acquired and finished by worker
    And we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "cid1",
        "plannedOperation": null,
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "id": "cid1",
        "planned_operation": null,
        "maintenance_window": {
            "weekly_maintenance_window": {
                "day": "MON",
                "hour": "1"
            }
        }
    }
    """

Scenario: Update maintenance window settings
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
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
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }]
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id1" acquired and finished by worker
    And we PATCH "/mdb/clickhouse/1.0/clusters/cid1" with data
    """
    {
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify ClickHouse cluster",
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
#    TODO: uncomment when https://st.yandex-team.ru/MDB-7127 get implemented
#    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
#    """
#    {
#        "cluster_id": "cid1",
#        "maintenance_window": {
#            "weekly_maintenance_window": {
#                "day": "MON",
#                "hour": 1
#            }
#        },
#        "update_mask": {
#            "paths": [
#                "maintenance_window"
#            ]
#        }
#    }
#    """
#    Then we get gRPC response with body
#    """
#    {
#        "created_by": "user",
#        "description": "Modify ClickHouse cluster",
#        "done": true,
#        "id": "worker_task_id2",
#        "metadata": {
#            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.UpdateClusterMetadata",
#            "cluster_id": "cid1"
#        }
#    }
#    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "cid1",
        "plannedOperation": null,
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "id": "cid1",
        "planned_operation": null,
        "maintenance_window": {
            "weekly_maintenance_window": {
                "day": "MON",
                "hour": "1"
            }
        }
    }
    """

Scenario: Reschedule maintenance
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
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
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }]
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "maintenance_window": {
            "weekly_maintenance_window": {
                "day": "MON",
                "hour": 1
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id1" acquired and finished by worker
    And we run query
    """
    WITH locked_cluster AS (
        SELECT rev AS actual_rev, next_rev, folder_id FROM code.lock_future_cluster('cid1')
    ), planned_task AS (
        SELECT code.plan_maintenance_task(
            i_max_delay      => '2000-01-22T00:00:00+00:00'::timestamp with time zone,
            i_task_id        => 'worker_task_maintenance_id1',
            i_cid            => 'cid1',
            i_config_id      => 'clickhouse_minor_upgrade',
            i_folder_id      => folder_id,
            i_operation_type => 'clickhouse_cluster_modify',
            i_task_type      => 'clickhouse_cluster_update',
            i_task_args      => '{}',
            i_version        => 2,
            i_metadata       => '{}',
            i_user_id        => 'maintenance',
            i_rev            => actual_rev,
            i_target_rev     => next_rev,
            i_plan_ts        => '2000-01-08 00:00:00+00'::timestamp with time zone,
            i_create_ts      => '2000-01-01 00:00:00+00'::timestamp with time zone,
            i_info           => 'Upgrade Clickhouse'
        ), actual_rev, next_rev
        FROM locked_cluster
    )
    SELECT code.complete_future_cluster_change('cid1', actual_rev, next_rev)
    FROM planned_task
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:rescheduleMaintenance" with data
    """
    {
       "clusterId": "cid1",
       "rescheduleType": "SPECIFIC_TIME",
       "delayedUntil": "2000-01-09T00:00:00Z"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Reschedule maintenance in ClickHouse cluster",
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.RescheduleMaintenanceMetadata",
            "clusterId": "cid1",
            "delayedUntil": "2000-01-09T00:00:00Z"
        }
    }
    """
    And we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
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
        "description": "Reschedule maintenance in ClickHouse cluster",
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.RescheduleMaintenanceMetadata",
            "cluster_id": "cid1",
            "delayed_until": "2000-01-09T00:00:00Z"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "cid1",
        "plannedOperation": {
            "delayedUntil": "2000-01-09T00:00:00+00:00",
            "info": "Upgrade Clickhouse",
            "latestMaintenanceTime": "2000-01-22T00:00:00+00:00",
            "nextMaintenanceWindowTime": "2000-01-10T00:00:00+00:00"
        },
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "id": "cid1",
        "planned_operation": {
            "latest_maintenance_time": "2000-01-22T00:00:00Z",
            "delayed_until": "2000-01-09T00:00:00Z",
            "next_maintenance_window_time": "2000-01-10T00:00:00Z",
            "info": "Upgrade Clickhouse"
        },
        "maintenance_window": {
            "weekly_maintenance_window": {
                "day": "MON",
                "hour": "1"
            }
        }
    }
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:rescheduleMaintenance" with data
    """
    {
       "clusterId": "cid1",
       "rescheduleType": "NEXT_AVAILABLE_WINDOW"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Reschedule maintenance in ClickHouse cluster",
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.RescheduleMaintenanceMetadata",
            "clusterId": "cid1",
            "delayedUntil": "2000-01-10T00:00:00Z"
        }
    }
    """
    And we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
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
        "description": "Reschedule maintenance in ClickHouse cluster",
        "id": "worker_task_id3",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.RescheduleMaintenanceMetadata",
            "cluster_id": "cid1",
            "delayed_until": "2000-01-10T00:00:00Z"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "id": "cid1",
        "plannedOperation": {
            "delayedUntil": "2000-01-10T00:00:00+00:00",
            "info": "Upgrade Clickhouse",
            "latestMaintenanceTime": "2000-01-22T00:00:00+00:00",
            "nextMaintenanceWindowTime": "2000-01-17T00:00:00+00:00"
        },
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "id": "cid1",
        "planned_operation": {
            "latest_maintenance_time": "2000-01-22T00:00:00Z",
            "delayed_until": "2000-01-10T00:00:00Z",
            "next_maintenance_window_time": "2000-01-17T00:00:00Z",
            "info": "Upgrade Clickhouse"
        },
        "maintenance_window": {
            "weekly_maintenance_window": {
                "day": "MON",
                "hour": "1"
            }
        }
    }
    """

Scenario: Attempt to reschedule maintenance outside allowed time window fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
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
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }]
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id1" acquired and finished by worker
    And we run query
    """
    WITH locked_cluster AS (
        SELECT rev AS actual_rev, next_rev, folder_id FROM code.lock_future_cluster('cid1')
    ), planned_task AS (
        SELECT code.plan_maintenance_task(
            i_max_delay      => '2000-01-22T00:00:00+00:00'::timestamp with time zone,
            i_task_id        => 'worker_task_maintenance_id1',
            i_cid            => 'cid1',
            i_config_id      => 'clickhouse_minor_upgrade',
            i_folder_id      => folder_id,
            i_operation_type => 'clickhouse_cluster_modify',
            i_task_type      => 'clickhouse_cluster_update',
            i_task_args      => '{}',
            i_version        => 2,
            i_metadata       => '{}',
            i_user_id        => 'maintenance',
            i_rev            => actual_rev,
            i_target_rev     => next_rev,
            i_plan_ts        => '2000-01-08 00:00:00+00'::timestamp with time zone,
            i_create_ts      => '2000-01-01 00:00:00+00'::timestamp with time zone,
            i_info           => 'Upgrade Clickhouse'
        ), actual_rev, next_rev
        FROM locked_cluster
    )
    SELECT code.complete_future_cluster_change('cid1', actual_rev, next_rev)
    FROM planned_task
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:rescheduleMaintenance" with data
    """
    {
       "clusterId": "cid1",
       "rescheduleType": "SPECIFIC_TIME",
       "delayedUntil": "2000-01-25T00:00:00Z"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Maintenance operation time cannot exceed 2000-01-22 00:00:00+00:00"
    }
    """
    And we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "clusterId": "cid1",
        "rescheduleType": "SPECIFIC_TIME",
        "delayedUntil": "2000-01-25T00:00:00Z"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "Maintenance operation time cannot exceed 2000-01-22T00:00:00Z"

Scenario: Attempt to reschedule maintenance to the next window for cluster with anytime maintenance configuration fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
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
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }]
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }]
    }
    """
    Then we get gRPC response OK
    When "worker_task_id1" acquired and finished by worker
    And we run query
    """
    WITH locked_cluster AS (
        SELECT rev AS actual_rev, next_rev, folder_id FROM code.lock_future_cluster('cid1')
    ), planned_task AS (
        SELECT code.plan_maintenance_task(
            i_max_delay      => '2000-01-22T00:00:00+00:00'::timestamp with time zone,
            i_task_id        => 'worker_task_maintenance_id1',
            i_cid            => 'cid1',
            i_config_id      => 'clickhouse_minor_upgrade',
            i_folder_id      => folder_id,
            i_operation_type => 'clickhouse_cluster_modify',
            i_task_type      => 'clickhouse_cluster_update',
            i_task_args      => '{}',
            i_version        => 2,
            i_metadata       => '{}',
            i_user_id        => 'maintenance',
            i_rev            => actual_rev,
            i_target_rev     => next_rev,
            i_plan_ts        => '2000-01-08 00:00:00+00'::timestamp with time zone,
            i_create_ts      => '2000-01-01 00:00:00+00'::timestamp with time zone,
            i_info           => 'Upgrade Clickhouse'
        ), actual_rev, next_rev
        FROM locked_cluster
    )
    SELECT code.complete_future_cluster_change('cid1', actual_rev, next_rev)
    FROM planned_task
    """
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1:rescheduleMaintenance" with data
    """
    {
       "clusterId": "cid1",
       "rescheduleType": "NEXT_AVAILABLE_WINDOW"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "The window need to be specified with next available window rescheduling type"
    }
    """
    And we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "clusterId": "cid1",
        "rescheduleType": "NEXT_AVAILABLE_WINDOW"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "The window need to be specified with next available window rescheduling type"
