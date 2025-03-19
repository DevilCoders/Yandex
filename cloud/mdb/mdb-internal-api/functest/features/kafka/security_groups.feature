@kafka
@grpc_api
@security_groups
Feature: Security groups in Kafka cluster
  Background:
    Given default headers
    When we add default feature flag "MDB_KAFKA_CLUSTER"

  Scenario: Create cluster with SG
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "testName",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1", "sg_id2"],
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 32212254720
          }
        }
      }
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
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "testName",
      "networkId": "network1",
      "securityGroupIds": ["non_existed_sg_id"],
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 32212254720
          }
        }
      }
    }
    """
    Then we get gRPC response error with code FAILED_PRECONDITION and message "security group "non_existed_sg_id" not found"

  Scenario: Create cluster with security groups duplicates
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "testName",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1", "sg_id1", "sg_id1", "sg_id2"],
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 32212254720
          }
        }
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateClusterMetadata",
        "cluster_id": "cid1"
      }
    }
    """
    And in worker_queue exists "worker_task_id1" id with args "security_group_ids" set to "[sg_id1, sg_id2]"

  Scenario: Modify security groups
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "testName",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1"],
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 32212254720
          }
        }
      }
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "testName",
      "networkId": "network1",
      "securityGroupIds": ["sg_id1", "sg_id2"],
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 32212254720
          }
        }
      }
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
    When we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
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
