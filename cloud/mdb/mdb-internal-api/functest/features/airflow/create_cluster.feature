@airflow
@grpc_api
@wip
Feature: Create Managed Airflow Cluster
  Background:
    Given default headers
    And we add default feature flag "MDB_AIRFLOW_CLUSTER"

  Scenario: Cluster creation API works
    When we "Create" via gRPC at "yandex.cloud.priv.airflow.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "name": "test",
      "description": "Cluster description",
      "labels": {
        "foo": "bar"
      },
      "config": {
        "versionId": "1",
        "airflow": {
          "config": {}
        },
        "webserver": {
          "count": 1,
          "resources": {"vcpuCount": 1, "memoryBytes": "100500"}
        },
        "scheduler": {
          "count": 1,
          "resources": {"vcpuCount": 1, "memoryBytes": "100500"}
        },
        "worker": {
          "minCount": 1,
          "maxCount": 2,
          "resources": {"vcpuCount": 1, "memoryBytes": "100500"}
        }
      },
      "network": {
          "subnetIds": ["subnet1"],
          "securityGroupIds": []
      },
      "codeSync": {
          "s3Bucket": "s3-bucket-name"
      },
      "deletionProtection": false
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Managed Airflow cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.airflow.v1.CreateClusterMetadata",
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
    And "worker_task_id1" acquired and finished by worker
    When we "Get" via gRPC at "yandex.cloud.priv.airflow.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "id": "cid1",
      "folder_id": "folder1",
      "name": "test",
      "description": "Cluster description",
      "labels": {
        "foo": "bar"
      },
      "monitoring": "**IGNORE**",
      "config": {
        "version_id": "1",
        "airflow": {
          "config": {}
        },
        "webserver": {
          "count": "1",
          "resources": {
            "vcpu_count": "1",
            "memory_bytes": "100500"
          }
        },
        "scheduler": {
          "count": "1",
          "resources": {
            "vcpu_count": "1",
            "memory_bytes": "100500"
          }
        },
        "triggerer": null,
        "worker": {
          "min_count": "1",
          "max_count": "2",
          "cooldown_period": "0s",
          "polling_interval": "0s",
          "resources": {
            "vcpu_count": "1",
            "memory_bytes": "100500"
          }
        }
      },
      "network": {
          "subnet_ids": ["subnet1"],
          "security_group_ids": []
      },
      "code_sync": {
          "s3_bucket": "s3-bucket-name"
      },
      "status": "RUNNING",
      "deletion_protection": false
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.airflow.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Delete Managed Airflow cluster",
      "done": false,
      "id": "worker_task_id2",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.airflow.v1.DeleteClusterMetadata",
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
