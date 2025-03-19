@elasticsearch
@grpc_api
Feature: Elasticsearch logs

  Background:
    Given default headers
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "user_specs": [{
            "name": "user1",
            "password": "fencesmakegoodneighbours"
        }],
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "elasticsearch_spec": {
                "data_node": {
                    "elasticsearch_config_7": {
                        "fielddata_cache_size": "100mb",
                        "max_clause_count": 200
                    },
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }, {
            "zone_id": "sas",
            "type": "DATA_NODE"
        }, {
            "zone_id": "man",
            "type": "DATA_NODE"
        }, {
            "zone_id": "myt",
            "type": "MASTER_NODE"
        }, {
            "zone_id": "sas",
            "type": "MASTER_NODE"
        }, {
            "zone_id": "man",
            "type": "MASTER_NODE"
        }],
        "description": "test cluster",
        "labels": {
            "foo": "bar"
        },
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create ElasticSearch cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.elasticsearch.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: elasticsearch logs can be viewed
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"],
        "service_type": "ELASTICSEARCH"
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

  Scenario: elasticsearch logs with page size limit can be viewed
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"],
        [0, 2, "should not see this"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        "service_type": "ELASTICSEARCH",
        "cluster_id": "cid1",
        "column_filter": ["message"],
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
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
    """
    {
        <filter>
        <page token>
        "service_type": "ELASTICSEARCH",
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "<message>"
    Examples:
      | filter                          | page token          | message                                                                          |
      | "column_filter": ["nocolumn"],  |                     | invalid column "nocolumn", valid columns: ["component", "hostname", "level", "message", "stacktrace"] |
      |                                 | "page_token": "A",  | invalid page token                                                               |


  Scenario Outline: <service> logs can be viewed via gRPC
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.elasticsearch.v1.ClusterService" with data
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
        | service                  |
        | SERVICE_TYPE_UNSPECIFIED |
        | ELASTICSEARCH            |
        | KIBANA                   |
