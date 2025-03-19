Feature: Management of ClickHouse user quotas

  Background:
    Given default headers
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }],
        "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: Adding user with quotas works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "settings": {
                "readonly": 2
            },
            "quotas" : [
            {
                "intervalDuration" : 3600000,
                "queries" : 100,
                "errors" : 1000,
                "resultRows" : 1000000,
                "readRows" : 100000000,
                "executionTime" : 100000
            }, {
                "intervalDuration" : 7200000,
                "queries" : 0,
                "errors" : 0,
                "resultRows" : 1000000,
                "readRows" : 100000000,
                "executionTime" : 0
            }]
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test2",
            "password": "password",
            "settings": {
                "readonly": 2
            },
            "quotas" : [
            {
                "interval_duration" : 3600000,
                "queries" : 100,
                "errors" : 1000,
                "result_rows" : 1000000,
                "read_rows" : 100000000,
                "execution_time" : 100000
            }, {
                "interval_duration" : 7200000,
                "queries" : 0,
                "errors" : 0,
                "result_rows" : 1000000,
                "read_rows" : 100000000,
                "execution_time" : 0
            }]
        }
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateUserMetadata",
            "cluster_id": "cid1",
            "user_name": "test2"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "settings": {
            "readonly": 2
        },
        "quotas" : [
          {
            "intervalDuration": 3600000,
            "queries": 100,
            "errors": 1000,
            "resultRows": 1000000,
            "readRows": 100000000,
            "executionTime": 100000
          },
          {
            "intervalDuration" : 7200000,
            "queries" : 0,
            "errors" : 0,
            "resultRows" : 1000000,
            "readRows" : 100000000,
            "executionTime" : 0
          }
        ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test2",
        "permissions": [{
            "database_name": "testdb",
            "data_filters": []
        }],
        "quotas" : [
        {
            "interval_duration": "3600000",
            "queries": "100",
            "errors": "1000",
            "result_rows": "1000000",
            "read_rows": "100000000",
            "execution_time": "100000"
        },
        {
            "interval_duration": "7200000",
            "queries": "0",
            "errors": "0",
            "result_rows": "1000000",
            "read_rows": "100000000",
            "execution_time": "0"
        }
        ]
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse.users" contains
    """
    {
        "test": {
            "databases": {
                "testdb": {}
            },
            "password": {
                "data": "test_password",
                "encryption_version": 0
            },
            "hash": {
                "data": "10a6e6cc8311a3e2bcc09bf6c199adecd5dd59408c343e926b129c4914f3cb01",
                "encryption_version": 0
            },
            "settings": {},
            "quotas" : []
        },
        "test2": {
            "databases": {
                "testdb": {}
            },
            "password": {
                "data": "password",
                "encryption_version": 0
            },
            "hash": {
                "data": "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8",
                "encryption_version": 0
            },
            "settings": {
                "readonly": 2
            },
            "quotas" : [
              {
                "interval_duration" : 3600,
                "queries" : 100,
                "errors" : 1000,
                "result_rows" : 1000000,
                "read_rows" : 100000000,
                "execution_time" : 100
              },
              {
                "interval_duration" : 7200,
                "queries" : 0,
                "errors" : 0,
                "result_rows" : 1000000,
                "read_rows" : 100000000,
                "execution_time" : 0
              }
            ]
        }
    }
    """

  Scenario: Modifying user quotas works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test2",
            "password": "password"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2" with data
    """
    {
       "quotas" : [
            {
                "intervalDuration" : 3600000,
                "queries" : 100,
                "errors" : 1000,
                "resultRows" : 1000000,
                "readRows" : 100000000,
                "executionTime" : 100000
            }]
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2",
        "quotas": [{
            "interval_duration" : 3600000,
            "queries" : 100,
            "errors" : 1000,
            "result_rows" : 1000000,
            "read_rows" : 100000000,
            "execution_time" : 100000
        }],
        "update_mask": {
            "paths": [
                "quotas"
            ]
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "quotas" : [
          {
            "intervalDuration" : 3600000,
            "queries" : 100,
            "errors" : 1000,
            "resultRows" : 1000000,
            "readRows" : 100000000,
            "executionTime" : 100000
          }
        ]
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test2",
        "permissions": [{
            "database_name": "testdb",
            "data_filters": []
        }],
        "quotas" : [{
            "interval_duration": "3600000",
            "queries": "100",
            "errors": "1000",
            "result_rows": "1000000",
            "read_rows": "100000000",
            "execution_time": "100000"
        }]
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse.users" contains
    """
    {
        "test": {
            "databases": {
                "testdb": {}
            },
            "password": {
                "data": "test_password",
                "encryption_version": 0
            },
            "hash": {
                "data": "10a6e6cc8311a3e2bcc09bf6c199adecd5dd59408c343e926b129c4914f3cb01",
                "encryption_version": 0
            },
            "settings": {},
            "quotas": []
        },
        "test2": {
            "databases": {
                "testdb": {}
            },
            "password": {
                "data": "password",
                "encryption_version": 0
            },
            "hash": {
                "data": "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8",
                "encryption_version": 0
            },
            "settings": {},
            "quotas" : [
              {
                "interval_duration" : 3600,
                "queries" : 100,
                "errors" : 1000,
                "result_rows" : 1000000,
                "read_rows" : 100000000,
                "execution_time" : 100
              }
            ]
        }
    }
    """

  Scenario: Removing user quotas works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "quotas" : [
            {
                "intervalDuration" : 3600000,
                "queries" : 100,
                "errors" : 1000,
                "resultRows" : 1000000,
                "readRows" : 100000000,
                "executionTime" : 100000
            }]
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test2",
            "password": "password",
            "quotas" : [
            {
                "interval_duration" : 3600000,
                "queries" : 100,
                "errors" : 1000,
                "result_rows" : 1000000,
                "read_rows" : 100000000,
                "execution_time" : 100000
            }]
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2" with data
    """
    {
       "quotas" : []
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Modify user in ClickHouse cluster",
        "done": false,
        "id": "worker_task_id3",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.UpdateUserMetadata",
            "clusterId": "cid1",
            "userName": "test2"
        }
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2",
        "quotas": [],
        "update_mask": {
            "paths": [
                "quotas"
            ]
        }
    }
    """
    When "worker_task_id3" acquired and finished by worker
    And we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "quotas" : []
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "test2",
        "permissions": [{
            "database_name": "testdb",
            "data_filters": []
        }],
        "quotas" : []
    }
    """
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And body at path "$.data.clickhouse.users" contains
    """
    {
        "test": {
            "databases": {
                "testdb": {}
            },
            "password": {
                "data": "test_password",
                "encryption_version": 0
            },
            "hash": {
                "data": "10a6e6cc8311a3e2bcc09bf6c199adecd5dd59408c343e926b129c4914f3cb01",
                "encryption_version": 0
            },
            "settings": {},
            "quotas" : []
        },
        "test2": {
            "databases": {
                "testdb": {}
            },
            "password": {
                "data": "password",
                "encryption_version": 0
            },
            "hash": {
                "data": "5e884898da28047151d0e56f8dc6292773603d0d6aabbdd62a11ef721d1542d8",
                "encryption_version": 0
            },
            "settings": {},
            "quotas" : []
        }
    }
    """

  Scenario: Adding user with quotas with same interval fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "quotas" : [
            {
                "intervalDuration" : 3600000,
                "queries" : 100,
                "errors" : 1000,
                "resultRows" : 1000000,
                "readRows" : 100000000,
                "executionTime" : 100000
            }, {
                "intervalDuration" : 3600000,
                "queries" : 0,
                "errors" : 0,
                "resultRows" : 1000000,
                "readRows" : 100000000,
                "executionTime" : 0
            }]
        }
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Multiple occurrence of the same IntervalDuration is not allowed"
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test2",
            "password": "password",
            "quotas" : [
            {
                "interval_duration" : 3600000,
                "queries" : 100,
                "errors" : 1000,
                "result_rows" : 1000000,
                "read_rows" : 100000000,
                "execution_time" : 100000
            }, {
                "interval_duration" : 3600000,
                "queries" : 0,
                "errors" : 0,
                "result_rows" : 1000000,
                "read_rows" : 100000000,
                "execution_time" : 0
            }]
        }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "multiple occurrence of the same IntervalDuration is not allowed"

  Scenario: Modifying user quotas to create quotas with same interval fails
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_spec": {
            "name": "test2",
            "password": "password"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "PATCH" via REST at "/mdb/clickhouse/1.0/clusters/cid1/users/test2" with data
    """
    {
       "quotas" : [
            {
                "intervalDuration" : 3600000,
                "queries" : 100
            }, {
                "intervalDuration" : 3600000,
                "queries" : 0
            }]
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Multiple occurrence of the same IntervalDuration is not allowed"
    }
    """
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.UserService" with data
    """
    {
        "cluster_id": "cid1",
        "user_name": "test2",
        "quotas": [{
            "interval_duration" : 3600000,
            "queries" : 100
        }, {
            "interval_duration" : 3600000,
            "queries" : 0
        }],
        "update_mask": {
            "paths": [
                "quotas"
            ]
        }
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "multiple occurrence of the same IntervalDuration is not allowed"

  Scenario: Creating a cluster with a user whose quota intervals are duplicated fails
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
    """
    {
        "name": "test2",
        "environment": "PRESTABLE",
        "configSpec": {
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
                }
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password",
            "quotas" : [{
                    "intervalDuration" : 3600000,
                    "queries" : 100
                }, {
                    "intervalDuration" : 3600000,
                    "queries" : 0
            }]
        }],
        "hostSpecs": [{
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }],
        "description": "test cluster"
    }
    """
    Then we get response with status 422 and body contains
    """
    {
        "code": 3,
        "message": "Multiple occurrence of the same IntervalDuration is not allowed"
    }
    """

  @grpc_api
  Scenario: Go internal api wont mess user quotas
    When we POST "/mdb/clickhouse/1.0/clusters/cid1/users" with data
    """
    {
        "userSpec": {
            "name": "test2",
            "password": "password",
            "settings": {
                "readonly": 2
            },
            "quotas" : [
            {
                "intervalDuration" : 3600000,
                "queries" : 100,
                "errors" : 1000,
                "resultRows" : 1000000,
                "readRows" : 100000000,
                "executionTime" : 100000
            }, {
                "intervalDuration" : 7200000,
                "queries" : 0,
                "errors" : 0,
                "resultRows" : 1000000,
                "readRows" : 100000000,
                "executionTime" : 0
            }]
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    And we "CreateShardGroup" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "shard_group_name": "test_group",
        "description": "Test shard group",
        "shard_names": ["shard1"]
    }
    """
    When we GET "/mdb/clickhouse/1.0/clusters/cid1/users/test2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "test2",
        "permissions": [{
            "databaseName": "testdb"
        }],
        "settings": {
            "readonly": 2
        },
        "quotas" : [
          {
            "intervalDuration" : 3600000,
            "queries" : 100,
            "errors" : 1000,
            "resultRows" : 1000000,
            "readRows" : 100000000,
            "executionTime" : 100000
          },
          {
            "intervalDuration" : 7200000,
            "queries" : 0,
            "errors" : 0,
            "resultRows" : 1000000,
            "readRows" : 100000000,
            "executionTime" : 0
          }
        ]
    }
    """
