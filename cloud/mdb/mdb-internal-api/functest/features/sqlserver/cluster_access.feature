@sqlserver
@grpc_api
Feature: Test Access settings creation and update

  Background:
    Given default headers
    When we add default feature flag "MDB_SQLSERVER_CLUSTER"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "folderId": "folder1",
        "environment": "PRESTABLE",
        "name": "test",
        "description": "test cluster",
        "networkId": "network1",
        "config_spec": {
          "access": {
            "data_lens": true,
            "data_transfer": true
          },
          "version": "2016sp2ent",
          "backup_window_start": {
             "hours": 11,
             "minutes": 11,
             "seconds": 9
          },
          "sqlserver_config_2016sp2ent": {
            "maxDegreeOfParallelism": 30,
            "auditLevel": 3
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
          "password": "test_password1!"
        }],
        "hostSpecs": [{
          "zoneId": "myt"
        }]
      }
      """
    Then we get gRPC response with body
      """
      {
        "created_by": "user",
        "description": "Create Microsoft SQLServer cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
          "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateClusterMetadata",
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

  Scenario: Cluster get works
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1"
      }
      """
    Then we get gRPC response with body
      """
      {
        "config": {
          "access": {
            "data_lens": true,
            "data_transfer": true,
            "web_sql": false,
            "serverless": false
          },
          "backup_window_start": {
                "hours": 11,
                "minutes": 11,
                "seconds": 9,
                "nanos": 0
          },
          "secondary_connections": "SECONDARY_CONNECTIONS_OFF",
          "version": "2016sp2ent",
          "sqlserver_config_2016sp2ent": {
            "default_config": {
              "audit_level":                    "0",
              "cost_threshold_for_parallelism": "5",
              "fill_factor_percent":            "0",
              "max_degree_of_parallelism":      "0",
              "optimize_for_ad_hoc_workloads":  false
            },
            "effective_config": {
              "audit_level":                    "3",
              "cost_threshold_for_parallelism": "5",
              "fill_factor_percent":            "0",
              "max_degree_of_parallelism":      "30",
              "optimize_for_ad_hoc_workloads":  false
            },
            "user_config": {
              "cost_threshold_for_parallelism": null,
              "fill_factor_percent":            null,
              "optimize_for_ad_hoc_workloads":  null,
              "max_degree_of_parallelism": "30",
              "audit_level": "3"
            }
          },
          "resources": {
            "resource_preset_id": "s1.porto.1",
            "disk_type_id": "local-ssd",
            "disk_size": "10737418240"
          }
        },
        "description": "test cluster",
        "environment": "PRESTABLE",
        "folder_id": "folder1",
        "id": "cid1",
        "name": "test",
        "network_id": "network1"
      }
      """

  Scenario: Access settings update
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "config_spec": {
          "access": {
            "data_lens": false,
            "data_transfer": false,
            "web_sql": false,
            "serverless": false
          }
        },
        "update_mask": {
          "paths": [
            "config_spec.access.data_lens",
            "config_spec.access.data_transfer"
          ]
        }
      }
      """
    Then we get gRPC response with body
      """
      {
         "created_by": "user",
         "description": "Modify Microsoft SQLServer cluster",
         "done": false,
         "id": "worker_task_id2",
         "metadata": {
           "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.UpdateClusterMetadata",
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
    And "worker_task_id2" acquired and finished by worker
    Then we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1"
      }
      """
    Then we get gRPC response OK

    And gRPC response body at path "$.config.access" contains
    """
    {
    }
    """