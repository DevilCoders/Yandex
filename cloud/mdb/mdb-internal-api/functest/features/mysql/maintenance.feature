@maintenance
Feature: Management of MySQL maintenance window

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
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "network-ssd",
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
            "zoneId": "vla"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "network1",
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """
    And "worker_task_id1" acquired and finished by worker


Scenario: Check default maintenance window settings
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
        "id": "cid1",
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """

Scenario: Set maintenance window settings
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
        "id": "cid1",
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "maintenanceWindow": {
            "anytime": {}
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
        "id": "cid1",
        "maintenanceWindow": {
            "anytime": {}
        }
    }
    """

Scenario: Set maintenance window settings and protect it from reset
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
        "id": "cid1",
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "description": "Valid description, that does not clear maintenance window settings"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify MySQL cluster",
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "Valid description, that does not clear maintenance window settings",
        "folderId": "folder1",
        "id": "cid1",
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """


Scenario: Set invalid maintenance window settings and check error
    When we PATCH "/mdb/mysql/1.0/clusters/cid1" with data
    """
    {
        "maintenanceWindow": {
            "anytime": {},
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Update cluster maintenance window requires only one window type"
    }
    """


@events
Scenario: Plan maintenance and reschedule
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
        "id": "cid1",
        "labels": {},
        "plannedOperation": null,
        "maintenanceWindow": {
            "weeklyMaintenanceWindow": {
                "day": "MON",
                "hour": 1
            }
        }
    }
    """
    When we run query
    """
    WITH locked_cluster AS (
        SELECT rev AS actual_rev, next_rev, folder_id FROM code.lock_future_cluster('cid1')
    ), planned_task AS (
        SELECT code.plan_maintenance_task(
            i_max_delay      => '2000-01-22T00:00:00+00:00'::timestamp with time zone,
            i_task_id        => 'worker_task_maintenance_id1',
            i_cid            => 'cid1',
            i_config_id      => 'mysql_minor_upgrade',
            i_folder_id      => folder_id,
            i_operation_type => 'mysql_cluster_modify',
            i_task_type      => 'mysql_cluster_update',
            i_task_args      => '{}',
            i_version        => 2,
            i_metadata       => '{}',
            i_user_id        => 'maintenance',
            i_rev            => actual_rev,
            i_target_rev     => next_rev,
            i_plan_ts        => '2000-01-08 00:00:00+00'::timestamp with time zone,
            i_create_ts      => '2000-01-01 00:00:00+00'::timestamp with time zone,
            i_info           => 'Upgrade MySQL'
        ), actual_rev, next_rev
        FROM locked_cluster
    )
    SELECT code.complete_future_cluster_change('cid1', actual_rev, next_rev)
    FROM planned_task
    """
    When we POST "/mdb/mysql/1.0/clusters/cid1:rescheduleMaintenance" with data
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
        "description": "Reschedule maintenance in MySQL cluster",
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.RescheduleMaintenanceMetadata",
            "clusterId": "cid1",
            "delayedUntil": "2000-01-09T00:00:00Z"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.mysql.RescheduleMaintenance" event with
    """
    {
        "details": {
            "cluster_id": "cid1"
        },
        "request_parameters": {
            "cluster_id": "cid1",
            "reschedule_type": "SPECIFIC_TIME",
            "delayed_until": "2000-01-09T00:00:00Z"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
        "id": "cid1",
        "labels": {},
        "plannedOperation": {
            "delayedUntil": "2000-01-09T00:00:00+00:00",
            "info": "Upgrade MySQL",
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
    When we POST "/mdb/mysql/1.0/clusters/cid1:rescheduleMaintenance" with data
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
        "description": "Reschedule maintenance in MySQL cluster",
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.RescheduleMaintenanceMetadata",
            "clusterId": "cid1",
            "delayedUntil": "2000-01-10T00:00:00Z"
        }
    }
    """
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
        "id": "cid1",
        "labels": {},
        "plannedOperation": {
            "delayedUntil": "2000-01-10T00:00:00+00:00",
            "info": "Upgrade MySQL",
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
    When we POST "/mdb/mysql/1.0/clusters/cid1:rescheduleMaintenance" with data
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
    When we GET "/mdb/mysql/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
        "id": "cid1",
        "labels": {},
        "plannedOperation": {
            "delayedUntil": "2000-01-10T00:00:00+00:00",
            "info": "Upgrade MySQL",
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
