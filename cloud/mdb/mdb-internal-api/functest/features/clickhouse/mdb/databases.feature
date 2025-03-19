Feature: Management of ClickHouse databases

  Background:
    Given default headers
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder1" with data
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
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s1.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "iva"
        }],
        "description": "test cluster"
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: Database get works
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "testdb"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
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

  Scenario: Nonexistent database get fails
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases/testdb2"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Database 'testdb2' does not exist"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_name": "testdb2"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "database "testdb2" not found"

  Scenario: Database list works
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": [{
            "clusterId": "cid1",
            "name": "testdb"
        }]
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "databases": [{
            "cluster_id": "cid1",
            "name": "testdb"
        }],
        "next_page_token": ""
    }
    """

  Scenario: Database list with pagination works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb2"
        }
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_spec": {
            "name": "testdb2"
        }
    }
    """
    Then we get gRPC response OK
    And "worker_task_id2" acquired and finished by worker
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb3"
        }
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_spec": {
            "name": "testdb3"
        }
    }
    """
    Then we get gRPC response OK
    And "worker_task_id3" acquired and finished by worker
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "page_size": 2
    }
    """
    Then we get gRPC response with body
    """
    {
        "databases": [{
            "cluster_id": "cid1",
            "name": "testdb"
        },{
            "cluster_id": "cid1",
            "name": "testdb2"
        }],
        "next_page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "page_token": "eyJPZmZzZXQiOjJ9"
    }
    """
    Then we get gRPC response with body
    """
    {
        "databases": [{
            "cluster_id": "cid1",
            "name": "testdb3"
        }],
        "next_page_token": ""
    }
    """

  Scenario Outline: Adding database <name> with invalid params fails (REST)
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases" with data
    """
    {
       "databaseSpec": {
           "name": "<name>"
       }
    }
    """
    Then we get response with status <status> and body contains
    """
    {
        "code": <error code>,
        "message": "<message>"
    }
    """
    Examples:
      | name    | status | error code | message                                                                                              |
      | testdb  | 409    | 6          | Database 'testdb' already exists                                                                     |
      | b@dN@me | 422    | 3          | The request is invalid.\ndatabaseSpec.name: Database name 'b@dN@me' does not conform to naming rules |
      | default | 422    | 3          | Database name 'default' is not allowed                                                               |
      | system  | 422    | 3          | Database name 'system' is not allowed                                                                |

  Scenario Outline: Adding database <name> with invalid params fails (gRPC)
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_spec": {
            "name": "<name>"
        }
    }
    """
    Then we get gRPC response error with code <error code> and message "<message>"
    Examples:
      | name    | error code       | message                                     |
      | testdb  | ALREADY_EXISTS   | database "testdb" already exists            |
      | b@dN@me | INVALID_ARGUMENT | database name "b@dN@me" has invalid symbols |
      | default | INVALID_ARGUMENT | invalid database name "default"             |
      | system  | INVALID_ARGUMENT | invalid database name "system"              |

  @events
  Scenario: Adding database works
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases" with data
    """
    {
        "databaseSpec": {
            "name": "testdb2"
        }
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Add database to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb2"
        }
    }
    """
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
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
        "description": "Add database to ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateDatabaseMetadata",
            "cluster_id": "cid1",
            "database_name": "testdb2"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.CreateDatabase" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "database_name": "testdb2"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_by": "user",
        "description": "Add database to ClickHouse cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.CreateDatabaseMetadata",
            "cluster_id": "cid1",
            "database_name": "testdb2"
        },
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.clickhouse.v1.Database",
            "cluster_id": "cid1",
            "name": "testdb2"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases/testdb2"
    Then we get response with status 200 and body contains
    """
    {
        "clusterId": "cid1",
        "name": "testdb2"
    }
    """
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_name": "testdb2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "name": "testdb2"
    }
    """

  @events
  Scenario: Deleting database works
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases/testdb"
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Delete database from ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.DeleteDatabaseMetadata",
            "clusterId": "cid1",
            "databaseName": "testdb"
        }
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_name": "testdb"
    }
    """
    Then we get response with status 200 and body contains
    """
    {
        "created_by": "user",
        "description": "Delete database from ClickHouse cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.DeleteDatabaseMetadata",
            "cluster_id": "cid1",
            "database_name": "testdb"
        }
    }
    """
    And for "worker_task_id2" exists "yandex.cloud.events.mdb.clickhouse.DeleteDatabase" event with
    """
    {
        "details": {
            "cluster_id": "cid1",
            "database_name": "testdb"
        }
    }
    """
    When we "GET" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases"
    Then we get response with status 200 and body contains
    """
    {
        "databases": []
    }
    """
    When we "List" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "databases": []
    }
    """

  Scenario: Deleting nonexistent database fails
    When we "DELETE" via REST at "/mdb/clickhouse/1.0/clusters/cid1/databases/nodb"
    Then we get response with status 404 and body contains
    """
    {
        "code": 5,
        "message": "Database 'nodb' does not exist"
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.DatabaseService" with data
    """
    {
        "cluster_id": "cid1",
        "database_name": "nodb"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "database "nodb" does not exist"
