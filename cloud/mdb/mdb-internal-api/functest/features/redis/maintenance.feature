@maintenance
Feature: Management of Redis maintenance window

Background:
    Given feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_5"]
    """
    And default headers
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "passw0rd"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
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
        "description": "test cluster",
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
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

Scenario: Set maintenance window settings
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
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
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
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
        "description": "Modify Redis cluster",
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
        "id": "cid1",
        "plannedOperation": null,
        "maintenanceWindow": {
            "anytime": {}
        }
    }
    """

Scenario: Set maintenance window settings and protect it from reset
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
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
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
    """
    {
        "description": "Valid description, that does not clear maintenance window settings"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify Redis cluster",
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.UpdateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "Valid description, that does not clear maintenance window settings",
        "folderId": "folder1",
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


Scenario: Set invalid maintenance window settings and check error
    When we PATCH "/mdb/redis/1.0/clusters/cid1" with data
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


Scenario: Plan maintenance and reschedule
    When we GET "/mdb/redis/1.0/clusters/cid1"
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
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
            i_config_id      => 'redis_minor_upgrade',
            i_folder_id      => folder_id,
            i_operation_type => 'redis_cluster_modify',
            i_task_type      => 'redis_cluster_update',
            i_task_args      => '{}',
            i_version        => 2,
            i_metadata       => '{}',
            i_user_id        => 'maintenance',
            i_rev            => actual_rev,
            i_target_rev     => next_rev,
            i_plan_ts        => '2000-01-08 00:00:00+00'::timestamp with time zone,
            i_create_ts      => '2000-01-01 00:00:00+00'::timestamp with time zone,
            i_info           => 'Upgrade Redis'
        ), actual_rev, next_rev
        FROM locked_cluster
    )
    SELECT code.complete_future_cluster_change('cid1', actual_rev, next_rev)
    FROM planned_task
    """
    When we POST "/mdb/redis/1.0/clusters/cid1:rescheduleMaintenance" with data
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
        "description": "Reschedule maintenance in Redis cluster",
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.RescheduleMaintenanceMetadata",
            "clusterId": "cid1",
            "delayedUntil": "2000-01-09T00:00:00Z"
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
        "id": "cid1",
        "labels": {},
        "plannedOperation": {
            "delayedUntil": "2000-01-09T00:00:00+00:00",
            "info": "Upgrade Redis",
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
    When we POST "/mdb/redis/1.0/clusters/cid1:rescheduleMaintenance" with data
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
        "description": "Reschedule maintenance in Redis cluster",
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.RescheduleMaintenanceMetadata",
            "clusterId": "cid1",
            "delayedUntil": "2000-01-10T00:00:00Z"
        }
    }
    """
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
        "id": "cid1",
        "labels": {},
        "plannedOperation": {
            "delayedUntil": "2000-01-10T00:00:00+00:00",
            "info": "Upgrade Redis",
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
    When we POST "/mdb/redis/1.0/clusters/cid1:rescheduleMaintenance" with data
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
    When we GET "/mdb/redis/1.0/clusters/cid1"
    Then we get response with status 200 and body contains
    """
    {
        "description": "test cluster",
        "folderId": "folder1",
        "id": "cid1",
        "labels": {},
        "plannedOperation": {
            "delayedUntil": "2000-01-10T00:00:00+00:00",
            "info": "Upgrade Redis",
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
