@sqlserver
@grpc_api
@search
Feature: Validate SQLServer search docs
  Background:
    Given default headers
    When we add default feature flag "MDB_SQLSERVER_CLUSTER"
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "testName",
      "description": "test cluster description",
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
      "done": false,
      "id": "worker_task_id1"
    }
    """
    When "worker_task_id1" acquired and finished by worker

  Scenario: Cluster creation deletion
    . name, description and labels modification doesn't work at PR time https://nda.ya.ru/t/m0TxZIlM3YAksV
    Then last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-sqlserver",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {},
            "hosts": ["myt-1.db.yandex.net"],
            "databases": ["testdb"],
            "users": ["test"]
        }
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-sqlserver",
        "deleted": "<TIMESTAMP>",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {}
    }
    """

  Scenario: User creation and deletion
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "test2",
        "password": "Test2Password"
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false,
      "id": "worker_task_id2"
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-sqlserver",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {},
            "hosts": ["myt-1.db.yandex.net"],
            "databases": ["testdb"],
            "users": ["test", "test2"]
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we "Delete" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "test"
    }
    """
    Then we get gRPC response with body
    """
    {
      "id": "worker_task_id3",
      "done": false
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-sqlserver",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {},
            "hosts": ["myt-1.db.yandex.net"],
            "databases": ["testdb"],
            "users": ["test2"]
        }
    }
    """

  Scenario: Database creation and deletion
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
      "done": false,
      "id": "worker_task_id2"
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-sqlserver",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {},
            "hosts": ["myt-1.db.yandex.net"],
            "databases": ["testdb", "testdb2"],
            "users": ["test"]
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
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
      "id": "worker_task_id3"
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-sqlserver",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {},
            "hosts": ["myt-1.db.yandex.net"],
            "databases": ["testdb2"],
            "users": ["test"]
        }
    }
    """

  @reindexer
  Scenario: Reindexer works with SQLServer clusters
    When we run mdb-search-reindexer
    Then last document in search_queue matches
    """
    {
        "service": "managed-sqlserver",
        "timestamp": "<TIMESTAMP>",
        "reindex_timestamp": "<TIMESTAMP>"
    }
    """