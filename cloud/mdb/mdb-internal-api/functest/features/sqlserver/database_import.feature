@sqlserver
@grpc_api
Feature: Import single SQLServer database

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
      "service_account_id": "sa1",
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

  Scenario: Import single database works
    When we "ImportBackup" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "newdb",
      "s3_bucket": "userbucketname",
      "s3_path": "/userpath",
      "files": ["newdb1.bak"]      
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Import database from external storage to Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.ImportDatabaseBackupMetadata",
        "cluster_id": "cid1",
        "database_name": "newdb",
        "s3_bucket": "userbucketname",
        "s3_path": "/userpath"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """

  Scenario: Import single database with path default works
    When we "ImportBackup" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "newdb",
      "s3_bucket": "userbucketname",
      "files": ["newdb1.bak"]      
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Import database from external storage to Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.ImportDatabaseBackupMetadata",
        "cluster_id": "cid1",
        "database_name": "newdb",
        "s3_bucket": "userbucketname",
        "s3_path": "/"
      },
      "response": {
        "@type": "type.googleapis.com/google.rpc.Status",
        "code": 0,
        "details": [],
        "message": "OK"
      }
    }
    """

  Scenario: Import existing database fails
    When we "ImportBackup" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "testdb",
      "s3_bucket": "userbucketname",
      "s3_path": "/userpath",
      "files": ["testdb1.bak"]      
    }
    """
    Then we get gRPC response error with code ALREADY_EXISTS and message "database "testdb" already exists"

  Scenario: Import database without service account fails
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
       "cluster_id": "cid1",
       "service_account_id": "",
       "update_mask": {
         "paths": [
           "service_account_id"
         ]
       }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "ImportBackup" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "newdb",
      "s3_bucket": "userbucketname",
      "s3_path": "/userpath",
      "files": ["newdb1.bak"]      
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "service account should be set"

  Scenario Outline: Import with invalid bucket name fails
    When we "ImportBackup" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "newdb",
      "s3_bucket": "<bucket>",
      "s3_path": "/userpath",
      "files": ["newdb1.bak"]      
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "s3-bucket name "<bucket>" has invalid symbols"
    Examples:
    | bucket           |
    | with spaces      |
    | with'specials    |
    | withрусскаябуква |

  Scenario Outline: Import with invalid path fails
    When we "ImportBackup" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "newdb",
      "s3_bucket": "userbucketname",
      "s3_path": "<path>",
      "files": ["newdb1.bak"]      
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "s3-path "<path>" <error>"
    Examples:
    | path                  | error |
    | foobar                | should be absolute  |
    | /../../../backpath    | should be canonical |
    | /with spaces          | has invalid symbols |
    | /with'specials        | has invalid symbols |
    | /не/латиница          | has invalid symbols |

  Scenario Outline: Import with invalid files fails
    When we "ImportBackup" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
      "cluster_id": "cid1",
      "database_name": "newdb",
      "s3_bucket": "userbucketname",
      "s3_path": "/userpath",
      "files": ["<file>"]      
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "file name "<file>" has invalid symbols"
    Examples:
    | file             |
    | with spaces      |
    | with'specials    |
    | withрусскаябуква |
