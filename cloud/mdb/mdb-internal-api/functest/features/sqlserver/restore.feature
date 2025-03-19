@sqlserver
@grpc_api
Feature: Restore SQLServer cluster from backup

  Background:
    Given default headers
    And s3 response
    """
    {
        "Contents": [
            {
                "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T010002Z_backup_stop_sentinel.json",
                "LastModified": 5400,
                "Body": {
                    "StartLocalTime": "1970-01-01T01:00:02Z",
                    "StopLocalTime": "1970-01-01T01:29:02Z"
                }
            },
            {
                "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T000002Z_backup_stop_sentinel.json",
                "LastModified": 3600,
                "Body": {
                    "StartLocalTime": "1970-01-01T00:00:02Z",
                    "StopLocalTime": "1970-01-01T01:01:02Z"
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
      "sqlcollation": "Cyrillic_General_CI_AI",
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
        "password": "test_password1!",
        "permissions": [
            {
              "database_name": "testdb",
              "roles": [
                    "DB_DDLADMIN",
                    "DB_DATAREADER",
                    "DB_DATAWRITER"
              ]
            }
          ]
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
    And all "cid1" revs committed before "1970-01-01T00:00:00+00:00"

  @events
  Scenario: Restoring from backup to original folder works
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z",
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
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
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create new Microsoft SQLServer cluster from backup",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.RestoreClusterMetadata",
        "cluster_id": "cid2",
        "backup_id": "cid1:base_19700101T010002Z"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    # TODO: events
    #And for "worker_task_id2" exists "yandex.cloud.events.mdb.postgresql.RestoreCluster" event with
    #"""
    #{
    #    "details": {
    #        "backup_id": "cid1:base_0",
    #        "cluster_id": "cid2"
    #    }
    #}
    #"""
Scenario: Restoring from backup to original folder inherits collation
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z",
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test_inheritcollation",
      "description": "test cluster 3 inheritcollation",
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
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create new Microsoft SQLServer cluster from backup",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.RestoreClusterMetadata",
        "cluster_id": "cid2",
        "backup_id": "cid1:base_19700101T010002Z"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
    "sqlcollation": "Cyrillic_General_CI_AI"
    }
    """


  Scenario: Restoring from backup to different folder works
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z",
      "folder_id": "folder2",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
      "network_id": "network1",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resource_preset_id": "s1.compute.1",
          "disk_type_id": "network-ssd",
          "disk_size": 10737418240
        }
      },
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create new Microsoft SQLServer cluster from backup",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.RestoreClusterMetadata",
        "cluster_id": "cid2",
        "backup_id": "cid1:base_19700101T010002Z"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
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
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z",
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
      "network_id": "network1",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": 5368709120
        }
      },
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "insufficient diskSize, increase it to '10737418240'"

  Scenario: Restoring from backup with incorrect backup id fails
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_0",
      "time": "1970-01-01T03:00:00Z",
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
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
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "backup "cid1:base_0" does not exist"

  Scenario: Restoring from backup with incorrect time fails
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T01:15:00Z",
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
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
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "unable to restore to "1970-01-01T01:15:00Z" using this backup, cause it finished at "1970-01-01T01:29:02Z" (use older backup or increase "time")"

  Scenario: Restoring from backup with incorrect time before cluster creation fails
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1959-01-01T00:00:00Z",
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
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
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "cluster "cid1" didn't exist at "1959-01-01T00:00:00Z""

  Scenario: Restoring from backup with incorrect disk unit fails
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z",
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
      "network_id": "network1",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": 10737418245
        }
      },
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "invalid disk size, must be multiple of 4194304 bytes"

  @delete
  Scenario: Restore from backup belongs to deleted cluster
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.DeleteClusterMetadata",
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
    Then "worker_task_id2" acquired and finished by worker
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z",
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
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
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create new Microsoft SQLServer cluster from backup",
      "done": false,
      "id": "worker_task_id5",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.RestoreClusterMetadata",
        "cluster_id": "cid2",
        "backup_id": "cid1:base_19700101T010002Z"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """

  @delete
  Scenario: Restoring from backup belogns to deleted cluster with less disk size fails
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.DeleteClusterMetadata",
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
    Then "worker_task_id2" acquired and finished by worker
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z",
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
      "network_id": "network1",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": 5368709120
        }
      },
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "insufficient diskSize, increase it to '10737418240'"

  @timetravel
  Scenario: Restoring cluster on time before database delete
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_spec": {
            "name": "testdb2"
        }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Add database to Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateDatabaseMetadata",
        "cluster_id": "cid1",
        "database_name": "testdb2"
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
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.DeleteClusterMetadata",
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
    And "worker_task_id3" acquired and finished by worker
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z",
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test2",
      "description": "test cluster 2",
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
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create new Microsoft SQLServer cluster from backup",
      "done": false,
      "id": "worker_task_id6",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.RestoreClusterMetadata",
        "cluster_id": "cid2",
        "backup_id": "cid1:base_19700101T010002Z"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """
    And "worker_task_id6" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid2",
      "page_size": 10
    }
    """
    Then we get gRPC response with body
    """
    {
      "databases" : [
        {
          "cluster_id": "cid2",
          "name": "testdb"
        }
      ]
    }
    """

Scenario: Restoring from backup to original folder with different edition fails
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z",
      "folder_id": "folder1",
      "environment": "PRESTABLE",
      "name": "test_inheritversion",
      "description": "test cluster 3 inheritversion",
      "network_id": "network1",
      "config_spec": {
        "version": "2016sp2std",
        "sqlserver_config_2016sp2std": {
        },
        "resources": {
          "resource_preset_id": "s1.porto.1",
          "disk_type_id": "local-ssd",
          "disk_size": 10737418240
        }
      },
      "host_specs": [{
        "zone_id": "myt"
      }]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "SQL Server versions need to match"
