Feature: MySQL logs

  Background:
    Given default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mysqlConfig_5_7": {
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
            "password": "test_password"
        }],
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
       "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    When we POST "/mdb/mysql/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create MySQL cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.mysql.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker

  Scenario Outline: <service> logs can be viewed (gRPC)
    Given named logsdb response
    """
    [
        ["log_seconds", "log_milliseconds", "message", "argument", "schema", "query", "record", "raw"],
        [0, 0, "message 1", "argument 1", "schema 1", "query 1", "record 1", "raw 1"],
        [0, 1, "message 2", "argument 2", "schema 2", "query 2", "record 2", "raw 2"],
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.mysql.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["<column1>", "<column2>"],
        "service_type": "<service>"
    }
    """
    Then we get gRPC response with body
    """
    {
        "logs": [
            {
                "message": {
                    "<column1>": "<column1> 1",
                    "<column2>": "<column2> 1"
                },
                "timestamp": "1970-01-01T00:00:00Z"
            },
            {
                "message": {
                    "<column1>": "<column1> 2",
                    "<column2>": "<column2> 2"
                },
                "timestamp": "1970-01-01T00:00:00.001Z"
            }
        ]
    }
    """
  Examples:
      | service          | column1  | column2 |
      | MYSQL_ERROR      | message  | raw     |
      | MYSQL_GENERAL    | argument | raw     |
      | MYSQL_SLOW_QUERY | raw      | schema  |
      | MYSQL_AUDIT      | raw      | record  |
      | MYSQL_SLOW_QUERY | query    | raw     |


  Scenario Outline: <service> logs with page size limit can be viewed (gRPC)
    Given named logsdb response
    """
    [
        ["log_seconds", "log_milliseconds", "message", "argument", "schema", "query", "record"],
        [0, 0, "message 1", "argument 1", "schema 1", "query 1", "record 1"],
        [0, 1, "message 2", "argument 2", "schema 2", "query 2", "record 2"],
        [0, 2, "should not see this", "should not see this", "should not see this", "should not see this", "should not see this"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.mysql.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["<column>"],
        "service_type": "<service>",
        "page_size": 2
    }
    """
    Then we get gRPC response with body
    """
    {
        "logs": [
            {
                "message": {
                    "<column>": "<column> 1"
                },
                "timestamp": "1970-01-01T00:00:00Z"
            },
            {
                "message": {
                    "<column>": "<column> 2"
                },
                "timestamp": "1970-01-01T00:00:00.001Z"
            }
        ]
    }
    """
    Examples:
      | service          | column   |
      | MYSQL_ERROR      | message  |
      | MYSQL_GENERAL    | argument |
      | MYSQL_SLOW_QUERY | schema   |
      | MYSQL_SLOW_QUERY | query    |
      | MYSQL_AUDIT      | record   |

  Scenario Outline: Request for logs with invalid parameters fails (gRPC)
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.mysql.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        <filter>
        <page token>
        "service_type": "MYSQL_ERROR"
    }
    """
    Then we get gRPC response error with code <error code> and message "<message>"
    Examples:
      | filter                          | page token          | error code       | message                                                                                                           |
      | "column_filter": ["nocolumn"],  |                     | INVALID_ARGUMENT | invalid column "nocolumn", valid columns: ["hostname", "id", "message", "raw", "status"] |
      |                                 | "page_token": "A",  | INVALID_ARGUMENT | invalid page token                                                                                                |
