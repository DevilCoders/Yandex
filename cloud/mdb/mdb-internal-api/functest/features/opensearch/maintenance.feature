@opensearch
@grpc_api
Feature: Maintenance Window for OpenSearch

  Background:
    Given default headers

  Scenario: Create a cluster without a maintenance window
    Given health response
    """
    {
      "clusters": [
          {
              "cid": "cid1",
              "status": "Alive"
          }
      ]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }],
        "description": "test description",
        "networkId": "network1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create OpenSearch cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.CreateClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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

  Scenario: Create a cluster with a maintenance window, and then update it
    Given health response
    """
    {
      "clusters": [
          {
              "cid": "cid1",
              "status": "Alive"
          }
      ]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                }
            },
            "access": null
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }],
        "description": "test description",
        "networkId": "network1",
        "maintenance_window": {
            "weekly_maintenance_window": {
                "day": "MON",
                "hour": "1"
            }
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create OpenSearch cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.CreateClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
        "description": "Modify OpenSearch cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.UpdateClusterMetadata",
            "cluster_id": "cid1"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.Cluster",
            "config": {
                "version": "7.14",
                "opensearch": {
                    "data_node": {
                        "resources": {
                            "disk_size":          "10737418240",
                            "disk_type_id":       "network-ssd",
                            "resource_preset_id": "s1.compute.1"
                        }
                    },
                    "master_node": null,
                    "plugins": []
                },
                "access": null
            },
            "deletion_protection": false,
            "created_at": "**IGNORE**",
            "description": "test description",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "health": "ALIVE",
            "id": "cid1",
            "labels": {},
            "maintenance_window": {
                "weekly_maintenance_window": {
                    "day": "TUE",
                    "hour": "2"
                }
            },
            "monitoring": "**IGNORE**",
            "name": "test",
            "network_id": "network1",
            "planned_operation": null,
            "security_group_ids": [],
            "service_account_id": "",
            "status": "RUNNING"
        }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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

  Scenario: Create a cluster with a maintenance window, and then delete it
    Given health response
    """
    {
      "clusters": [
          {
              "cid": "cid1",
              "status": "Alive"
          }
      ]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }],
        "description": "test description",
        "networkId": "network1",
        "maintenance_window": {
            "weekly_maintenance_window": {
                "day": "MON",
                "hour": "1"
            }
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create OpenSearch cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.CreateClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "update_mask": {
            "paths": [
                "maintenance_window"
            ]
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Modify OpenSearch cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.UpdateClusterMetadata",
            "cluster_id": "cid1"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.Cluster",
            "config": {
                "version": "7.14",
                "opensearch": {
                    "data_node": {
                        "resources": {
                            "disk_size":          "10737418240",
                            "disk_type_id":       "network-ssd",
                            "resource_preset_id": "s1.compute.1"
                        }
                    },
                    "master_node": null,
                    "plugins": []
                },
                "access": null
            },
            "deletion_protection": false,
            "created_at": "**IGNORE**",
            "description": "test description",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "health": "ALIVE",
            "id": "cid1",
            "labels": {},
            "maintenance_window": {
                "anytime": {}
            },
            "monitoring": "**IGNORE**",
            "name": "test",
            "network_id": "network1",
            "planned_operation": null,
            "security_group_ids": [],
            "service_account_id": "",
            "status": "RUNNING"
        }
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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

  Scenario: Schedule and reschedule a maintenance task on a cluster without specified maintenance window
    Given health response
    """
    {
        "clusters": [
            {
                "cid": "cid1",
                "status": "Alive"
            }
        ]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }],
        "description": "test description",
        "networkId": "network1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create OpenSearch cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.CreateClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
    And we run query
    """
    WITH locked_cluster AS (
        SELECT rev AS actual_rev, next_rev, folder_id FROM code.lock_future_cluster('cid1')
    ), planned_task AS (
        SELECT code.plan_maintenance_task(
            i_task_id        => 'worker_task_maintenance_id1',
            i_cid            => 'cid1',
            i_config_id      => 'opensearch_test_config',
            i_folder_id      => folder_id,
            i_operation_type => 'opensearch_cluster_modify',
            i_task_type      => 'opensearch_cluster_modify',
            i_task_args      => '{}',
            i_version        => 2,
            i_metadata       => '{}',
            i_user_id        => 'maintenance',
            i_rev            => actual_rev,
            i_target_rev     => next_rev,
            i_plan_ts        => '2000-01-08 00:00:00+00'::timestamp with time zone,
            i_create_ts      => '2000-01-01 00:00:00+00'::timestamp with time zone,
            i_info           => 'OpenSearch test maintenance'
        ), actual_rev, next_rev
        FROM locked_cluster
    )
    SELECT code.complete_future_cluster_change('cid1', actual_rev, next_rev)
    FROM planned_task;
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
        },
        "planned_operation": {
            "latest_maintenance_time": "2000-01-22T00:00:00Z",
            "delayed_until": "2000-01-08T00:00:00Z",
            "next_maintenance_window_time": null,
            "info": "OpenSearch test maintenance"
        }
    }
    """
    And we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "clusterId": "cid1",
        "rescheduleType": "NEXT_AVAILABLE_WINDOW"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "The window need to be specified with next available window rescheduling type"
    And we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "clusterId": "cid1",
        "rescheduleType": "SPECIFIC_TIME",
        "delayedUntil": "2000-01-07T00:00:00Z"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Reschedule maintenance in OpenSearch cluster",
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.RescheduleMaintenanceMetadata",
            "cluster_id": "cid1",
            "delayed_until": "2000-01-07T00:00:00Z"
        }
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
        },
        "planned_operation": {
            "latest_maintenance_time": "2000-01-22T00:00:00Z",
            "delayed_until": "2000-01-07T00:00:00Z",
            "next_maintenance_window_time": null,
            "info": "OpenSearch test maintenance"
        }
    }
    """

  Scenario: Schedule and reschedule a maintenance task on a cluster with specified maintenance window
    Given health response
    """
    {
      "clusters": [
          {
              "cid": "cid1",
              "status": "Alive"
          }
      ]
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }],
        "description": "test description",
        "networkId": "network1",
        "maintenance_window": {
            "weekly_maintenance_window": {
                "day": "MON",
                "hour": "2"
            }
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create OpenSearch cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.CreateClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
                "hour": "2"
            }
        }
    }
    """
    And we run query
    """
    WITH locked_cluster AS (
        SELECT rev AS actual_rev, next_rev, folder_id FROM code.lock_future_cluster('cid1')
    ), planned_task AS (
        SELECT code.plan_maintenance_task(
            i_task_id        => 'worker_task_maintenance_id1',
            i_cid            => 'cid1',
            i_config_id      => 'opensearch_test_config',
            i_folder_id      => folder_id,
            i_operation_type => 'opensearch_cluster_modify',
            i_task_type      => 'opensearch_cluster_modify',
            i_task_args      => '{}',
            i_version        => 2,
            i_metadata       => '{}',
            i_user_id        => 'maintenance',
            i_rev            => actual_rev,
            i_target_rev     => next_rev,
            i_plan_ts        => '2000-01-03 01:00:00+00'::timestamp with time zone,
            i_create_ts      => '2000-01-01 00:00:00+00'::timestamp with time zone,
            i_info           => 'OpenSearch test maintenance'
        ), actual_rev, next_rev
        FROM locked_cluster
    )
    SELECT code.complete_future_cluster_change('cid1', actual_rev, next_rev)
    FROM planned_task;
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
                "hour": "2"
            }
        },
        "planned_operation": {
            "latest_maintenance_time": "2000-01-22T00:00:00Z",
            "delayed_until": "2000-01-03T01:00:00Z",
            "next_maintenance_window_time": "2000-01-10T01:00:00Z",
            "info": "OpenSearch test maintenance"
        }
    }
    """
    And we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
        "description": "Reschedule maintenance in OpenSearch cluster",
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.RescheduleMaintenanceMetadata",
            "cluster_id": "cid1",
            "delayed_until": "2000-01-10T01:00:00Z"
        }
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
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
                "hour": "2"
            }
        },
        "planned_operation": {
            "latest_maintenance_time": "2000-01-22T00:00:00Z",
            "delayed_until": "2000-01-10T01:00:00Z",
            "next_maintenance_window_time": "2000-01-17T01:00:00Z",
            "info": "OpenSearch test maintenance"
        }
    }
    """
