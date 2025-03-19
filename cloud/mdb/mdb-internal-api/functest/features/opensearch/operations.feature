@opensearch
@grpc_api
Feature: Operation for Managed OpenSearch
  Background:
    Given default headers
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
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
        "description": "Create OpenSearch cluster",
        "done": false,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
    And "worker_task_id1" acquired and finished by worker

  Scenario: Get operation returns cluster data
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id1"
    }
    """
    Then we get gRPC response with body ignoring empty
    """
    {
        "created_at": "**IGNORE**",
        "created_by": "user",
        "description": "Create OpenSearch cluster",
        "done": true,
        "id": "worker_task_id1",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.CreateClusterMetadata",
            "cluster_id": "cid1"
        },
        "modified_at": "**IGNORE**",
        "response": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.Cluster",
            "config": {
                "opensearch": {
                    "data_node": {
                        "resources": {
                            "disk_size": "10737418240",
                            "disk_type_id": "local-ssd",
                            "resource_preset_id": "s1.porto.1"
                        }
                    },
                    "master_node": {
                        "resources": {
                            "disk_size": "10737418240",
                            "disk_type_id": "local-ssd",
                            "resource_preset_id": "s1.porto.1"
                        }
                    }
                },
                "version": "7.14"
            },
            "created_at": "**IGNORE**",
            "description": "test cluster",
            "environment": "PRESTABLE",
            "folder_id": "folder1",
            "id": "cid1",
            "labels": {
                "foo": "bar"
            },
            "maintenance_window": { "anytime": {} },
            "monitoring": [
                {
                    "description": "YaSM (Golovan) charts",
                    "link": "https://yasm/cid=cid1",
                    "name": "YASM"
                },
                {
                    "description": "Solomon charts",
                    "link": "https://solomon/cid=cid1\u0026fExtID=folder1",
                    "name": "Solomon"
                },
                {
                    "description": "Console charts",
                    "link": "https://console/cid=cid1\u0026fExtID=folder1",
                    "name": "Console"
                }
            ],
            "name": "test",
            "status": "RUNNING"
        }
    }
    """


  Scenario: Update cluster name
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "update_mask": {
            "paths": ["name"]
        },
        "name" : "new cluster name"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Update OpenSearch cluster metadata",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.UpdateClusterMetadata",
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
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.OperationService" with data
    """
    {
        "operation_id": "worker_task_id2"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Update OpenSearch cluster metadata",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.UpdateClusterMetadata",
            "cluster_id": "cid1"
        }
    }
    """
