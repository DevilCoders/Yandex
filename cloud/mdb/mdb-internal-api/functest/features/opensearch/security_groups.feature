@opensearch
@grpc_api
@security_groups
Feature: Security groups in OpenSearch cluster
  Background:
    Given default headers

  Scenario: Create cluster with SG
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
      "folder_id": "folder1",
      "name": "testName",
      "environment": "PRESTABLE",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1", "sg_id2"],
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
          }
        }
      },
      "host_specs": [{
          "zone_id": "myt",
          "type": "DATA_NODE"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false,
      "id": "worker_task_id1"
    }
    """
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" set to "[sg_id1, sg_id2]"
    When "worker_task_id1" acquired and finished by worker
    And worker set "sg_id1,sg_id2" security groups on "cid1"
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "security_group_ids": ["sg_id1", "sg_id2"]
    }
    """


  Scenario: Create cluster with non existed security group
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
      "folder_id": "folder1",
      "name": "testName",
      "environment": "PRESTABLE",
      "networkId": "network1",
      "securityGroupIds": ["non_existed_sg_id"],
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
          }
        }
      },
      "host_specs": [{
          "zone_id": "myt",
          "type": "DATA_NODE"
      }]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "security group "non_existed_sg_id" not found"

  Scenario: Create cluster with security groups duplicates
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
      "folder_id": "folder1",
      "name": "testName",
      "environment": "PRESTABLE",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1", "sg_id1", "sg_id1", "sg_id2"],
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
          }
        }
      },
      "host_specs": [{
          "zone_id": "myt",
          "type": "DATA_NODE"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false,
      "id": "worker_task_id1"
    }
    """
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" set to "[sg_id1, sg_id2]"

  Scenario: Modify security groups
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
      "folder_id": "folder1",
      "name": "testName",
      "environment": "PRESTABLE",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1"],
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
          }
        }
      },
      "host_specs": [{
          "zone_id": "myt",
          "type": "DATA_NODE"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false,
      "id": "worker_task_id1"
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And worker set "sg_id1" security groups on "cid1"
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["security_group_ids"]
      },
      "security_group_ids": ["sg_id2"]
    }
    """
    Then we get gRPC response with body
    """
    {
      "id": "worker_task_id2"
    }
    """
    And in worker_queue exists "worker_task_id2" id with args "security_group_ids" set to "[sg_id2]"

  Scenario: Modify security groups to same value
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
      "folder_id": "folder1",
      "name": "testName",
      "environment": "PRESTABLE",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1", "sg_id2"],
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
          }
        }
      },
      "host_specs": [{
          "zone_id": "myt",
          "type": "DATA_NODE"
      }]
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false,
      "id": "worker_task_id1"
    }
    """
    When "worker_task_id1" acquired and finished by worker
    And worker set "sg_id1,sg_id2" security groups on "cid1"
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["security_group_ids"]
      },
      "security_group_ids": ["sg_id2", "sg_id1"]
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "no changes detected"
