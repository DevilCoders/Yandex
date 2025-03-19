Feature: Redis logs

  Background:
    Given feature flags
    """
    ["MDB_REDIS_ALLOW_DEPRECATED_5"]
    """
    And default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_5_0": {
                "password": "p@ssw#$rd!?"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 17179869184
            }
        },
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
    When we POST "/mdb/redis/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create Redis cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.redis.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker

  @grpc
  Scenario: Redis logs can be viewed
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
        "service_type": "REDIS"
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

  @grpc
  Scenario: Redis logs with page size limit can be viewed
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"],
        [0, 2, "should not see this"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
        "service_type": "REDIS",
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

  @grpc
  Scenario Outline: Request for logs with invalid parameters fails (gRPC)
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.redis.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        <filter>
        <page token>
        "service_type": "REDIS"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "<message>"
    Examples:
      | filter                          | page token          | message                                                                          |
      | "column_filter": ["nocolumn"],  |                     | invalid column "nocolumn", valid columns: ["hostname", "message", "pid", "role"] |
      |                                 | "page_token": "A",  | invalid page token                                                               |
