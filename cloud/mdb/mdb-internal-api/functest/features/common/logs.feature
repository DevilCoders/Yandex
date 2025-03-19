@logs
Feature: Common logs logic

  Background: Use PostgreSQL cluster as example
    Given default headers
    When we POST "/mdb/postgresql/1.0/clusters?folderId=folder1" with data
    """
    {
       "name": "test",
       "environment": "PRESTABLE",
       "configSpec": {
           "version": "14",
           "resources": {
               "resourcePresetId": "s1.porto.1",
               "diskTypeId": "local-ssd",
               "diskSize": 10737418240
           }
       },
       "databaseSpecs": [{
           "name": "testdb",
           "owner": "test"
       }],
       "userSpecs": [{
           "name": "test",
           "password": "test_password"
       }],
       "hostSpecs": [{
           "zoneId": "myt"
       }]
    }
    """

  Scenario: Request logs always_next_page_token parameter
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"],
        [0, 2, "message 3"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.postgresql.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
        "service_type": "POSTGRESQL",
        "page_size": 2,
        "always_next_page_token": true
    }
    """
    Then we get gRPC response with body
    """
    {
        "logs": [
            {
                "message": {
                    "message": "message 1"
                },
                "timestamp": "1970-01-01T00:00:00Z"
            },
            {
                "message": {
                    "message": "message 2"
                },
                "timestamp": "1970-01-01T00:00:00.001Z"
            }
        ],
        "next_page_token": "2"
    }
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.postgresql.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
        "service_type": "POSTGRESQL",
        "page_token": "2",
        "page_size": 2,
        "always_next_page_token": true
    }
    """
    Then we get gRPC response with body
    """
    {
        "logs": [
            {
                "message": {
                    "message": "message 3"
                },
                "timestamp": "1970-01-01T00:00:00.002Z"
            }
        ],
        "next_page_token": "3"
    }
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.postgresql.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
        "service_type": "POSTGRESQL",
        "page_token": "3",
        "page_size": 2,
        "always_next_page_token": true
    }
    """
    Then we get gRPC response with body
    """
    {
        "logs": [],
        "next_page_token": "3"
    }
    """

  @errors
  Scenario: Request logs from not existed cluster
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.postgresql.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid42",
        "service_type": "POSTGRESQL"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "cluster id "cid42" not found"

  @grpc_api @filter
  Scenario: Request logs with filter
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"],
        [0, 2, "message 3"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.postgresql.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "service_type": "POSTGRESQL",
        "column_filter": ["message"],
        "filter": "message.hostname='myt-1.db.yandex.net' AND message.error_severity = 'FATAL'"
    }
    """
    Then we get gRPC response OK

  @grpc_api @filter @errors
  Scenario Outline: Request logs with invalid filter
   When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.postgresql.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "service_type": "POSTGRESQL",
        "column_filter": ["message"],
        "filter": "<filter>"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "<message>"

  Examples:
    | filter                           | message                                                                                                |
    | message.notExistedColumn='foo'   | invalid filter field "message.notExistedColumn", valid: ["message.error_severity", "message.hostname"] |
    | error_severity = 'WARN'          | invalid filter field "error_severity", valid: ["message.error_severity", "message.hostname"]           |
    | message.message='42'             | invalid filter field "message.message", valid: ["message.error_severity", "message.hostname"]          |
    | message.error_severity >= 'WARN' | unsupported filter operator >= for field "message.error_severity"                                      |
    | message.error_severity = 42      | unsupported filter "message.error_severity" field type. Should be a string or a list of strings        |
    | message.error_severity += 1      | filter syntax error at or near column 24: invalid token '+'                                            |

  @grpc_api @errors
  Scenario: Request logs for invalid cluster type
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "service_type": "CLICKHOUSE"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "cluster "cid1" does not exist"
