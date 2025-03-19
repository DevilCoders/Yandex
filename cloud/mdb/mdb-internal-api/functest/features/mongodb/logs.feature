Feature: MongoDB logs

  Background:
    Given default headers
    And "create" data
    """
    {
        "name": "test",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
                    }
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
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder1" with "create" data
    Then we get response with status 200 and body contains
    """
    {
        "createdBy": "user",
        "description": "Create MongoDB cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "yandex.cloud.mdb.mongodb.v1.CreateClusterMetadata",
            "clusterId": "cid1"
        }
    }
    """
    When "worker_task_id1" acquired and finished by worker

  Scenario: mongod logs can be viewed
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.mongodb.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
        "service_type": "MONGOD"
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

  Scenario: mongod logs with page size limit can be viewed
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"],
        [0, 2, "should not see this"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.mongodb.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
        "service_type": "MONGOD",
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

  Scenario Outline: Request for logs with invalid parameters fails (gRPC)
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.mongodb.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        <filter>
        <page token>
        "service_type": "<service type>"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "<message>"
    Examples:
      | service type | filter                          | page token          | message                                                                                               |
      | MONGOD       | "column_filter": ["nocolumn"],  |                     | invalid column "nocolumn", valid columns: ["component", "context", "hostname", "message", "severity"] |
      | MONGOD       |                                 | "page_token": "A",  | invalid page token                                                                                    |

  Scenario Outline: <service> logs can be viewed via gRPC
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.mongodb.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
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
    Examples:
        | service  |
        | MONGOD   |
        | MONGOS   |
        | MONGOCFG |
        | AUDIT    |
