@sqlserver
@grpc_api
Feature: Operation for Managed Microsoft SQLServer
  Background:
    Given default headers
    And we add default feature flag "MDB_KAFKA_CLUSTER"

  Scenario: Check SQLServer-specific operation service
    When we add default feature flag "MDB_SQLSERVER_CLUSTER"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "labels": {
        "foo": "bar"
      },
      "networkId": "network1",
      "config_spec": {
        "version": "2016sp2ent",
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.OperationService" with data
    """
    {
      "operation_id": "worker_task_id1"
    }
    """
    Then we get gRPC response OK
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.OperationService" with data
    """
    {
      "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Add database to Microsoft SQLServer cluster",
      "done": true,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateDatabaseMetadata",
        "cluster_id": "cid1",
        "database_name": "testdb2"
      },
      "response": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.Database",
        "cluster_id": "cid1",
        "name": "testdb2"
      }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "testtwo",
        "password": "TestPassword",
        "permissions": [
          {
            "database_name": "testdb",
            "roles": [
                "DB_DDLADMIN",
                "DB_DATAWRITER",
                "DB_DENYDATAWRITER"
            ]
          }
        ]
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create user in Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateUserMetadata",
        "cluster_id": "cid1",
        "user_name": "testtwo"
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.OperationService" with data
    """
    {
      "operation_id": "worker_task_id3"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create user in Microsoft SQLServer cluster",
      "done": true,
      "id": "worker_task_id3",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.CreateUserMetadata",
        "cluster_id": "cid1",
        "user_name": "testtwo"
      },
      "response": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.User",
        "name": "testtwo",
        "cluster_id": "cid1",
        "permissions": [
          {
            "database_name": "testdb",
            "roles": [
                  "DB_DDLADMIN",
                  "DB_DATAWRITER",
                  "DB_DENYDATAWRITER"
            ]
          }
        ],
        "server_roles": []
      }
    }
    """
