@opensearch
@grpc_api
Feature: OpenSearch extensions operations
  Background:
    Given default headers
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.compute.1",
                        "disk_type_id": "network-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "extension_specs": [{
            "name": "initial",
            "uri": "https://storage.yandexcloud.net",
            "disabled": true
        }],
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }],
        "description": "test cluster",
        "networkId": "network1"
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

  Scenario: Get extension works
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ExtensionService" with data
    """
    {
        "cluster_id": "cid1",
        "extension_id": "extension_id1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "id": "extension_id1",
        "name": "initial",
        "version": "1",
        "active": false
    }
    """

  Scenario: List extensions works
    When we "List" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ExtensionService" with data
    """
    {
        "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "extensions": [{
            "cluster_id": "cid1",
            "id": "extension_id1",
            "name": "initial",
            "version": "1",
            "active": false
        }]
    }
    """

  Scenario: Create dicts extension
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ExtensionService" with data
    """
    {
        "cluster_id": "cid1",
        "name": "dicts",
        "uri": "https://storage.yandexcloud.net"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Create extension in OpenSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.CreateExtensionMetadata",
            "cluster_id": "cid1",
            "extension_id": "extension_id2"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
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
        "description": "Create extension in OpenSearch cluster",
        "done": true,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.CreateExtensionMetadata",
            "cluster_id": "cid1",
            "extension_id": "extension_id2"
        },
        "response": {
            "@type":      "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.Extension",
            "active":     true,
            "cluster_id": "cid1",
            "id":         "extension_id2",
            "name":       "dicts",
            "version":    "1"
        }
    }
    """

  Scenario: Update extension
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ExtensionService" with data
    """
    {
        "cluster_id": "cid1",
        "extension_id": "extension_id1",
        "active" : true
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Update extension in OpenSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.UpdateExtensionMetadata",
            "cluster_id": "cid1",
            "extension_id": "extension_id1"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ExtensionService" with data
    """
    {
        "cluster_id": "cid1",
        "extension_id": "extension_id1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "cluster_id": "cid1",
        "id": "extension_id1",
        "name": "initial",
        "version": "1",
        "active": true
    }
    """

  Scenario: Delete extension
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ExtensionService" with data
    """
    {
        "cluster_id": "cid1",
        "extension_id": "extension_id1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "created_by": "user",
        "description": "Delete extension in OpenSearch cluster",
        "done": false,
        "id": "worker_task_id2",
        "metadata": {
            "@type": "type.googleapis.com/yandex.cloud.priv.mdb.opensearch.v1.DeleteExtensionMetadata",
            "cluster_id": "cid1",
            "extension_id": "extension_id1"
        },
        "response": {
            "@type": "type.googleapis.com/google.rpc.Status",
            "code": 0,
            "details": [],
            "message": "OK"
        }
    }
    """
    And "worker_task_id2" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ExtensionService" with data
    """
    {
        "cluster_id": "cid1",
        "extension_id": "extension_id1"
    }
    """
    Then we get gRPC response error with code NOT_FOUND and message "extension with id "extension_id1" is not found"
