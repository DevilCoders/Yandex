@opensearch
@grpc_api
Feature: OpenSearch version upgrade validations
  Background:
    Given default headers
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder1",
        "name": "test",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.12",
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

  Scenario: Version downgrade is not allowed
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "update_mask": {
            "paths": ["config_spec.version"]
        },
        "config_spec": {
            "version": "7.11"
        }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "version downgrade is not allowed"
