@sqlserver
@restore-hints
@grpc_api
Feature: Restore SQLServer cluster from backup with hints

  Background:
    Given default headers
    And s3 response
    """
    {
        "Contents": [
            {
                "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19760101T010002Z_backup_stop_sentinel.json",
                "LastModified": 189307800,
                "Body": {
                    "StartLocalTime": "1976-01-01T01:00:02Z",
                    "StopLocalTime": "1976-01-01T01:20:02Z"
                }
            },
            {
                "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19730101T000002Z_backup_stop_sentinel.json",
                "LastModified": 94698000,
                "Body": {
                    "StartLocalTime": "1973-01-01T00:00:02Z"
                }
            }
        ]
    }
    """
    When we add default feature flag "MDB_SQLSERVER_CLUSTER"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "network_id": "network1",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": 10737418240
        }
      },
      "database_specs": [{
        "name": "testdb"
      }],
      "user_specs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "host_specs": [{
        "zone_id": "myt"
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
       "cluster_id": "cid1",
       "config_spec": {
         "resources": {
           "resource_preset_id": "s1.porto.2",
           "disk_size": "21474836480"
         }
       },
       "update_mask": {
         "paths": [
           "config_spec.resources.resource_preset_id",
           "config_spec.resources.disk_size"
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
    And all "cid1" revs committed before "1977-01-01T00:00:00+00:00"

  Scenario: Restore hints works with last backup
    When we "GetRestoreHints" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.console.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19760101T010002Z"
    }
    """
    Then we get gRPC response with body
    """
    {
       "environment": "PRESTABLE",
       "network_id": "network1",
       "version": "2016sp2ent",
       "time": "1976-01-01T01:21:02Z",
       "resources": {
         "resource_preset_id": "s1.porto.2",
         "disk_size": "21474836480"
       }
    }
    """

  Scenario: Restore resources depends on cluster disk_size at restore time
    When we "GetRestoreHints" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.console.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19730101T000002Z"
    }
    """
    Then we get gRPC response with body
    """
    {
       "environment": "PRESTABLE",
       "network_id": "network1",
       "version": "2016sp2ent",
       "time": "1973-01-01T01:01:00Z",
       "resources": {
         "resource_preset_id": "s1.porto.1",
         "disk_size": "10737418240"
       }
    }
    """

  #TODO: 
  #Scenario Outline: Restore hints for different SQLServer versions
