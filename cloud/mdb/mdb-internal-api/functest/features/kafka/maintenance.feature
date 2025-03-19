@kafka
@grpc_api
Feature: Create Compute Apache Kafka Cluster with maintenance window
  Background:
    Given default headers
    And we add default feature flag "MDB_KAFKA_CLUSTER"

  Scenario: Create cluster with maintenance window and then update maintenance window via update cluster
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
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      },
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
      "description": "Create Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
    Then we get gRPC response with body ignoring empty
    """
    {
      "created_by": "user",
      "description": "Modify Apache Kafka cluster",
      "done": true,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateClusterMetadata",
        "cluster_id": "cid1"
      },
      "response": {
		"@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.Cluster",
		"created_at": "**IGNORE**",
		"description": "test cluster",
		"environment": "PRESTABLE",
		"folder_id": "folder1",
		"health": "ALIVE",
		"id": "cid1",
		"maintenance_window": {
			"weekly_maintenance_window": {
				"day": "TUE",
				"hour": "2"
			}
		},
		"name": "test",
		"network_id": "network1",
		"status": "RUNNING"
	  }
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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

  Scenario: Create cluster without maintenance window and get cluster info should return any time maintenance window
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
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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

  Scenario: Create cluster with maintenance window and then update cluster with empty maintenance window should return any time after update
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
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      },
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
      "description": "Create Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
    Then we get gRPC response with body ignoring empty
    """
    {
      "created_by": "user",
      "description": "Modify Apache Kafka cluster",
      "done": true,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.UpdateClusterMetadata",
        "cluster_id": "cid1"
      },
      "response": {
		"@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.Cluster",
		"created_at": "**IGNORE**",
		"description": "test cluster",
		"environment": "PRESTABLE",
		"folder_id": "folder1",
		"health": "ALIVE",
		"id": "cid1",
		"maintenance_window": {
			"anytime": {}
		},
		"name": "test",
		"network_id": "network1",
		"status": "RUNNING"
	  }
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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

  Scenario: Plan and reschedule maintenance on Kafka cluster when no maintenance window is set
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
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
    # Plan maintenance task for cid1
    """
    WITH locked_cluster AS (
        SELECT rev AS actual_rev, next_rev, folder_id FROM code.lock_future_cluster('cid1')
    ), planned_task AS (
        SELECT code.plan_maintenance_task(
            i_task_id        => 'worker_task_maintenance_id1',
            i_cid            => 'cid1',
            i_config_id      => 'kafka_update_tls_certs',
            i_folder_id      => folder_id,
            i_operation_type => 'kafka_cluster_modify',
            i_task_type      => 'kafka_cluster_maintenance',
            i_task_args      => '{}',
            i_version        => 2,
            i_metadata       => '{}',
            i_user_id        => 'maintenance',
            i_rev            => actual_rev,
            i_target_rev     => next_rev,
            i_plan_ts        => '2000-01-08 00:00:00+00'::timestamp with time zone,
            i_create_ts      => '2000-01-01 00:00:00+00'::timestamp with time zone,
            i_info           => 'Updating kafka TLS certificates'
        ), actual_rev, next_rev
        FROM locked_cluster
    )
    SELECT code.complete_future_cluster_change('cid1', actual_rev, next_rev)
    FROM planned_task;
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
        "info": "Updating kafka TLS certificates"
      }
    }
    """
    And we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
       "clusterId": "cid1",
       "rescheduleType": "NEXT_AVAILABLE_WINDOW"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "The window need to be specified with next available window rescheduling type"
    And we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
      "description": "Reschedule maintenance in Kafka cluster",
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.RescheduleMaintenanceMetadata",
        "cluster_id": "cid1",
        "delayed_until": "2000-01-07T00:00:00Z"
      }
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
        "info": "Updating kafka TLS certificates"
      }
    }
    """


  Scenario: Plan and reschedule maintenance on Kafka cluster with maintenance window
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
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      },
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
      "description": "Create Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateClusterMetadata",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
    # Plan maintenance task for cid1
    """
    WITH locked_cluster AS (
        SELECT rev AS actual_rev, next_rev, folder_id FROM code.lock_future_cluster('cid1')
    ), planned_task AS (
        SELECT code.plan_maintenance_task(
            i_task_id        => 'worker_task_maintenance_id1',
            i_cid            => 'cid1',
            i_config_id      => 'kafka_update_tls_certs',
            i_folder_id      => folder_id,
            i_operation_type => 'kafka_cluster_modify',
            i_task_type      => 'kafka_cluster_maintenance',
            i_task_args      => '{}',
            i_version        => 2,
            i_metadata       => '{}',
            i_user_id        => 'maintenance',
            i_rev            => actual_rev,
            i_target_rev     => next_rev,
            i_plan_ts        => '2000-01-03 01:00:00+00'::timestamp with time zone,
            i_create_ts      => '2000-01-01 00:00:00+00'::timestamp with time zone,
            i_info           => 'Updating kafka TLS certificates'
        ), actual_rev, next_rev
        FROM locked_cluster
    )
    SELECT code.complete_future_cluster_change('cid1', actual_rev, next_rev)
    FROM planned_task;
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
        "info": "Updating kafka TLS certificates"
      }
    }
    """
    And we "RescheduleMaintenance" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
      "description": "Reschedule maintenance in Kafka cluster",
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.RescheduleMaintenanceMetadata",
        "cluster_id": "cid1",
        "delayed_until": "2000-01-10T01:00:00Z"
      }
    }
    """
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
        "info": "Updating kafka TLS certificates"
      }
    }
    """
