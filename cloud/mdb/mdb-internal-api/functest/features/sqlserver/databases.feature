@sqlserver
@grpc_api
Feature: Create, delete and modify SQL Server databases
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
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      },
      {
        "name": "testdb0"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!",
        "permissions" : [
            {
                "database_name": "testdb0",
                "roles": [
                    "DB_DENYDATAWRITER",
                    "DB_DENYDATAREADER"
                ]
            },
            {
                "database_name": "testdb",
                "roles": [
                    "DB_DENYDATAWRITER",
                    "DB_DENYDATAREADER"
                ]
            }
        ]
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

  Scenario: Database get works
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_name": "testdb"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "testdb"
    }
    """
  Scenario: Database list works
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
                "name": "testdb0"
            }
        ]
    }
    """

  Scenario: Create existing database fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_spec": {
            "name": "testdb0"
        }
    }
    """
  Then we get gRPC response error with code ALREADY_EXISTS and message "database "testdb0" already exists"

  Scenario: Create invalid database fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_spec": {
            "name": "master"
        }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "invalid database name "master""

  Scenario: Database create works
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
                "name": "testdb0"
            },
            {
                "cluster_id": "cid1",
                "name": "testdb2"
            }
        ]
    }
    """

  Scenario: Database delete works
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_name": "testdb0"
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
        "database_name": "testdb0"
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
                    "DB_DENYDATAWRITER",
                    "DB_DENYDATAREADER"
              ]
            }
        ],
        "server_roles": []
    }
    """

  Scenario: delete incorrect database fails
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_name": "nosuchdb"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "database "nosuchdb" not found"

  Scenario: Delete invalid database fails
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_name": "master"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "invalid database name "master""
