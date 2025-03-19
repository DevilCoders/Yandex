@sqlserver
@grpc_api
Feature: Backup SQLServer cluster

  Background:
    Given default headers
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

  Scenario: Backup creation works
    When we "Backup" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create a backup for Microsoft SQLServer cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.sqlserver.v1.BackupClusterMetadata",
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
    When we "Backup" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "conflicting operation "worker_task_id2" detected"

  Scenario: Backup list for cluster works
    Given s3 response
      """
      {
          "Contents": [
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T010002Z_backup_stop_sentinel.json",
                  "LastModified": 3610,
                  "Body": {
                      "Databases": ["db1", "db2"],
                      "StartLocalTime": "1970-01-01T01:00:02Z",
                      "StopLocalTime": "1970-01-01T01:00:08Z"
                  }
              },
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T000002Z_backup_stop_sentinel.json",
                  "LastModified": 10,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T00:00:02Z"
                  }
              }
          ]
      }
      """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1"
      }
      """
    Then we get gRPC response with body
      """
      {
        "backups": [
          {
              "id": "cid1:base_19700101T010002Z",
              "source_cluster_id": "cid1",
              "databases": ["db1","db2"],
              "folder_id": "folder1",
              "created_at": "1970-01-01T01:00:08Z",
              "started_at": "1970-01-01T01:00:02Z"
          },
          {
              "id": "cid1:base_19700101T000002Z",
              "source_cluster_id": "cid1",
              "databases": ["db1"],
              "folder_id": "folder1",
              "created_at": "1970-01-01T00:00:10Z",
              "started_at": "1970-01-01T00:00:02Z"
          }
        ],
        "next_page_token": ""
      }
      """

  Scenario: Backup list for cluster with no backups works
    Given s3 response
      """
      { }
      """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1"
      }
      """
    Then we get gRPC response with body
      """
      {
        "backups": [],
        "next_page_token": ""
      }
      """

  Scenario: Backup list for cluster with page size works
    Given s3 response
      """
      {
          "Contents": [
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T010002Z_backup_stop_sentinel.json",
                  "LastModified": 3610,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T01:00:02Z",
                      "StopLocalTime": "1970-01-01T01:00:08Z"
                  }
              },
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T000002Z_backup_stop_sentinel.json",
                  "LastModified": 10,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T00:00:02Z"
                  }
              }
          ]
      }
      """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "page_size": 1
      }
      """
    Then we get gRPC response with body
      """
      {
        "backups": [
          {
              "id": "cid1:base_19700101T010002Z",
              "source_cluster_id": "cid1",
              "databases": ["db1"],
              "folder_id": "folder1",
              "created_at": "1970-01-01T01:00:08Z",
              "started_at": "1970-01-01T01:00:02Z"
          }
        ],
        "next_page_token": "eyJDbHVzdGVySUQiOiJjaWQxIiwiQmFja3VwSUQiOiJiYXNlXzE5NzAwMTAxVDAxMDAwMloifQ=="
      }
      """

  Scenario: Backup list for cluster with page token works
    Given s3 response
      """
      {
          "Contents": [
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T010002Z_backup_stop_sentinel.json",
                  "LastModified": 3610,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T01:00:02Z",
                      "StopLocalTime": "1970-01-01T01:00:08Z"
                  }
              },
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T000002Z_backup_stop_sentinel.json",
                  "LastModified": 10,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T00:00:02Z"
                  }
              }
          ]
      }
      """
    When we "ListBackups" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
      """
      {
        "cluster_id": "cid1",
        "page_size": 1,
        "page_token": "eyJDbHVzdGVySUQiOiJjaWQxIiwiQmFja3VwSUQiOiJiYXNlXzE5NzAwMTAxVDAxMDAwMloifQ=="
      }
      """
    Then we get gRPC response with body
      """
      {
        "backups": [
          {
              "id": "cid1:base_19700101T000002Z",
              "source_cluster_id": "cid1",
              "databases": ["db1"],
              "folder_id": "folder1",
              "created_at": "1970-01-01T00:00:10Z",
              "started_at": "1970-01-01T00:00:02Z"
          }
        ],
        "next_page_token": ""
      }
      """

  Scenario: Backup list for folder works
    Given s3 response
      """
      {
          "Contents": [
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T010002Z_backup_stop_sentinel.json",
                  "LastModified": 3610,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T01:00:02Z",
                      "StopLocalTime": "1970-01-01T01:00:08Z"
                  }
              },
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T000002Z_backup_stop_sentinel.json",
                  "LastModified": 10,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T00:00:02Z"
                  }
              }
          ]
      }
      """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.BackupService" with data
      """
      {
        "folder_id": "folder1"
      }
      """
    Then we get gRPC response with body
      """
      {
        "backups": [
          {
              "id": "cid1:base_19700101T010002Z",
              "source_cluster_id": "cid1",
              "databases": ["db1"],
              "folder_id": "folder1",
              "created_at": "1970-01-01T01:00:08Z",
              "started_at": "1970-01-01T01:00:02Z"
          },
          {
              "id": "cid1:base_19700101T000002Z",
              "source_cluster_id": "cid1",
              "databases": ["db1"],
              "folder_id": "folder1",
              "created_at": "1970-01-01T00:00:10Z",
              "started_at": "1970-01-01T00:00:02Z"
          }
        ],
        "next_page_token": ""
      }
      """

  Scenario: Backup list for folder with no backups works
    Given s3 response
      """
      { }
      """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.BackupService" with data
      """
      {
        "folder_id": "folder1"
      }
      """
    Then we get gRPC response with body
      """
      {
        "backups": [],
        "next_page_token": ""
      }
      """

  Scenario: Backup list for folder with page size works
    Given s3 response
      """
      {
          "Contents": [
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T010002Z_backup_stop_sentinel.json",
                  "LastModified": 3610,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T01:00:02Z",
                      "StopLocalTime": "1970-01-01T01:00:08Z"
                  }
              },
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T000002Z_backup_stop_sentinel.json",
                  "LastModified": 10,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T00:00:02Z"
                  }
              }
          ]
      }
      """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.BackupService" with data
      """
      {
        "folder_id": "folder1",
        "page_size": 1
      }
      """
    Then we get gRPC response with body
      """
      {
        "backups": [
          {
              "id": "cid1:base_19700101T010002Z",
              "source_cluster_id": "cid1",
              "databases": ["db1"],
              "folder_id": "folder1",
              "created_at": "1970-01-01T01:00:08Z",
              "started_at": "1970-01-01T01:00:02Z"
          }
        ],
        "next_page_token": "eyJDbHVzdGVySUQiOiJjaWQxIiwiQmFja3VwSUQiOiJiYXNlXzE5NzAwMTAxVDAxMDAwMloifQ=="
      }
      """

  Scenario: Backup list for folder with page token works
    Given s3 response
      """
      {
          "Contents": [
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T010002Z_backup_stop_sentinel.json",
                  "LastModified": 3610,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T01:00:02Z",
                      "StopLocalTime": "1970-01-01T01:00:08Z"
                  }
              },
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T000002Z_backup_stop_sentinel.json",
                  "LastModified": 10,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T00:00:02Z"
                  }
              }
          ]
      }
      """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.BackupService" with data
      """
      {
        "folder_id": "folder1",
        "page_size": 1,
        "page_token": "eyJDbHVzdGVySUQiOiJjaWQxIiwiQmFja3VwSUQiOiJiYXNlXzE5NzAwMTAxVDAxMDAwMloifQ=="
      }
      """
    Then we get gRPC response with body
      """
      {
        "backups": [
          {
              "id": "cid1:base_19700101T000002Z",
              "source_cluster_id": "cid1",
              "databases": ["db1"],
              "folder_id": "folder1",
              "created_at": "1970-01-01T00:00:10Z",
              "started_at": "1970-01-01T00:00:02Z"
          }
        ],
        "next_page_token": ""
      }
      """

  Scenario: Backup get by id works
    Given s3 response
      """
      {
          "Contents": [
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T010002Z_backup_stop_sentinel.json",
                  "LastModified": 3610,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T01:00:02Z",
                      "StopLocalTime": "1970-01-01T01:00:09Z"
                  }
              },
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T000002Z_backup_stop_sentinel.json",
                  "LastModified": 10,
                  "Body": {
                      "Databases": ["db1", "db2"],
                      "StartLocalTime": "1970-01-01T00:00:02Z"
                  }
              }
          ]
      }
      """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.BackupService" with data
      """
      {
        "backup_id": "cid1:base_19700101T000002Z"
      }
      """
    Then we get gRPC response with body
      """
      {
        "id": "cid1:base_19700101T000002Z",
        "source_cluster_id": "cid1",
        "folder_id": "folder1",
        "databases": ["db1", "db2"],
        "created_at": "1970-01-01T00:00:10Z",
        "started_at": "1970-01-01T00:00:02Z"
      }
      """

  @delete
  Scenario: After cluster delete its backups are shown
    Given s3 response
      """
      {
          "Contents": [
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T010002Z_backup_stop_sentinel.json",
                  "LastModified": 3610,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T01:00:02Z",
                      "StopLocalTime": "1970-01-01T01:00:08Z"
                  }
              },
              {
                  "Key": "wal-g/cid1/2016sp2ent/basebackups_005/base_19700101T000002Z_backup_stop_sentinel.json",
                  "LastModified": 10,
                  "Body": {
                      "Databases": ["db1"],
                      "StartLocalTime": "1970-01-01T00:00:02Z"
                  }
              }
          ]
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
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.BackupService" with data
      """
      {
        "folder_id": "folder1"
      }
      """
    Then we get gRPC response with body
      """
      {
        "backups": [
          {
              "id": "cid1:base_19700101T010002Z",
              "source_cluster_id": "cid1",
              "databases": ["db1"],
              "folder_id": "folder1",
              "created_at": "1970-01-01T01:00:08Z",
              "started_at": "1970-01-01T01:00:02Z"
          },
          {
              "id": "cid1:base_19700101T000002Z",
              "source_cluster_id": "cid1",
              "databases": ["db1"],
              "folder_id": "folder1",
              "created_at": "1970-01-01T00:00:10Z",
              "started_at": "1970-01-01T00:00:02Z"
          }
        ],
        "next_page_token": ""
      }
      """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.BackupService" with data
      """
      {
        "backup_id": "cid1:base_19700101T000002Z"
      }
      """
    Then we get gRPC response with body
      """
      {
        "id": "cid1:base_19700101T000002Z",
        "source_cluster_id": "cid1",
        "folder_id": "folder1",
        "created_at": "1970-01-01T00:00:10Z",
        "started_at": "1970-01-01T00:00:02Z"
      }
      """
    When "worker_task_id2" acquired and finished by worker
    And "worker_task_id3" acquired and finished by worker
    And "worker_task_id4" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.BackupService" with data
      """
      {
        "folder_id": "folder1"
      }
      """
    Then we get gRPC response with body
      """
      {
        "backups": [],
        "next_page_token": ""
      }
      """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.BackupService" with data
      """
      {
        "backup_id": "cid1:base_19700101T000002Z"
      }
      """
    Then we get gRPC response error with code NOT_FOUND and message "cluster id "cid1" not found"
