Feature: ClickHouse logs

  Background:
    Given default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
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
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    When we POST "/mdb/clickhouse/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create ClickHouse cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.clickhouse.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker

  Scenario: clickhouse logs can be viewed
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
        "service_type": "CLICKHOUSE"
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
        ]
    }
    """

  Scenario: clickhouse logs with page size limit can be viewed
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"],
        [0, 2, "should not see this"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
        "service_type": "CLICKHOUSE",
        "page_size": 2
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

  Scenario: clickhouse logs can be viewed with pagination
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"],
        [0, 2, "message 3"],
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
        "service_type": "CLICKHOUSE",
        "page_size": 1
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
            }
        ],
        "next_page_token": "1"
    }
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data and last page token
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
        "service_type": "CLICKHOUSE",
        "page_size": 1,
        "page_token": "**IGNORE**"
    }
    """
    Then we get gRPC response with body
    """
    {
        "logs": [
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

  Scenario Outline: Request for logs with invalid parameters fails (gRPC)
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        <filter>
        <page token>
        "service_type": "CLICKHOUSE"
    }
    """
    Then we get gRPC response error with code <error code> and message "<message>"
    Examples:
      | filter                          | page token          | error code       | message                                                                                                          |
      | "column_filter": ["nocolumn"],  |                     | INVALID_ARGUMENT | invalid column "nocolumn", valid columns: ["component", "hostname", "message", "query_id", "severity", "thread"] |
      |                                 | "page_token": "A",  | INVALID_ARGUMENT | invalid page token                                                                                               |
