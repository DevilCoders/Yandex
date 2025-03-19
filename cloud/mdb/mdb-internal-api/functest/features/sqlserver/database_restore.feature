@sqlserver
@grpc_api
Feature: Restore single SQLServer database from backup

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
                    "Databases": [
                        "testdb"
                    ],
                    "StartLocalTime": "1970-01-01T01:00:02Z",
                    "StopLocalTime": "1970-01-01T01:29:02Z"
                }
            },
            {
                "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T000002Z_backup_stop_sentinel.json",
                "LastModified": 3600,
                "Body": {
                    "Databases": [
                        "testdb"
                    ],
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
      },
      {
        "name": "testtwo",
        "password": "TestPassword",
        "permissions": []
      }
      ],
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

@singledbrestore
Scenario: Restoring database on time before database delete
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_name": "testdb"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete database from Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.DeleteDatabaseMetadata",
        "cluster_id": "cid1",
        "database_name": "testdb"
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
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "testdb",
      "from_database": "testdb",
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z"      
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Restore database to Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.RestoreDatabaseMetadata",
        "cluster_id": "cid1",
        "database_name": "testdb",
        "from_database": "testdb",
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
    And "worker_task_id3" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "page_size": 10
    }
    """
    Then we get gRPC response with body
    """
    {
      "databases" : [
        {
          "cluster_id": "cid1",
          "name": "testdb"
        }
      ]
    }
    """
@singledbsidebysiderestore
Scenario: Restoring database copy side by side
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "testdb2",
      "from_database": "testdb",
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z"      
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Restore database to Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.RestoreDatabaseMetadata",
        "cluster_id": "cid1",
        "database_name": "testdb2",
        "from_database": "testdb",
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
    And "worker_task_id2" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "page_size": 10
    }
    """
    Then we get gRPC response with body
    """
    {
      "databases" : [
        {
          "cluster_id": "cid1",
          "name": "testdb"
        },
        {
          "cluster_id": "cid1",
          "name": "testdb2"
        }
      ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test"
    }
    """
    Then we get gRPC response with body
    """
    {
      "name": "test",
      "cluster_id": "cid1",
      "permissions": [
        {
          "database_name": "testdb",
          "roles": [
                "DB_DDLADMIN",
                "DB_DATAREADER",
                "DB_DATAWRITER"
          ]
        },
        {
          "database_name": "testdb2",
          "roles": [
                "DB_DDLADMIN",
                "DB_DATAREADER",
                "DB_DATAWRITER"
          ]
        }
      ],
      "server_roles": []
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "testtwo"
    }
    """
    Then we get gRPC response with body
    """
    {
      "name": "testtwo",
      "cluster_id": "cid1",
      "permissions": [],
      "server_roles": []
    }
    """
@singledboverwritefails
  Scenario: Restoring database without deleting it first fails
  When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "testdb",
      "from_database": "testdb",
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z"      
    }
    """
    Then we get gRPC response error with code ALREADY_EXISTS and message "database "testdb" already exists"

@singledbrestorewithinvalidname
  Scenario: Restoring a database with invalid name fails
  When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "?????",
      "from_database": "testdb",
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z"      
    }
    """
  Then we get gRPC response error with code INVALID_ARGUMENT and message "database name "?????" has invalid symbols"

@singledbrestoreinvalidpitr
  Scenario: Restoring a database that is not present in the backup at a given time fails
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
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 10
    }
    """
    Then we get gRPC response with body
    """
    {
        "databases" : [
            {
                "cluster_id": "cid1",
                "name": "testdb"
            },
            {
                "cluster_id": "cid1",
                "name": "testdb2"
            }
        ]
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_name": "testdb2"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete database from Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.DeleteDatabaseMetadata",
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
    And "worker_task_id3" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 10
    }
    """
    Then we get gRPC response with body
    """
    {
        "databases" : [
            {
                "cluster_id": "cid1",
                "name": "testdb"
            }
        ]
    }
    """
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "testdb2",
      "from_database": "testdb2",
      "backup_id": "cid1:base_19700101T010002Z",
      "time": "1970-01-01T03:00:00Z"      
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "database testdb2 is not found in the backup chosen"

@singledbrestorewithinvalidbackupid
  Scenario: Restoring a database from incorrect backup fails
    When we "Restore" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "testdb2",
      "from_database": "testdb",
      "backup_id": "cid1:base_0",
      "time": "1970-01-01T03:00:00Z"      
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "backup "cid1:base_0" does not exist"
