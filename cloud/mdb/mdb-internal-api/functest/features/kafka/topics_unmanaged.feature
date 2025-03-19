@kafka
@grpc_api
Feature: Unmanaged topics in Kafka cluster
  Background:
    Given default headers
    When we add default feature flag "MDB_KAFKA_CLUSTER"

  Scenario: Create cluster with admin user without unmanaged flag fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"]
      },
      "user_specs": [
        {
          "name": "admin",
          "password": "Password",
          "permissions": [{
              "topic_name": "*",
              "role": "ACCESS_ROLE_ADMIN"
          }]
        }
      ]
    }
    """
    Then we get gRPC response error with code INVALID_ARGUMENT and message "admin role can be set only on clusters with unmanaged topics. user with admin role "admin""


  Scenario: Create topic in cluster with unmanaged flag fails
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"],
        "unmanaged_topics": true
      }
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker
    And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "description": "test cluster",
      "environment": "PRESTABLE",
      "folder_id": "folder1",
      "id": "cid1",
      "name": "test",
      "network_id": "network1",
      "config": {
        "access": {
                "web_sql": false,
                "serverless": false,
                "data_transfer": false
        },
        "assign_public_ip": false,
        "brokers_count": "1",
        "version": "3.0",
        "zone_id": ["myt", "sas"],
        "unmanaged_topics": true,
        "sync_topics": false,
        "schema_registry": false,
        "kafka": {
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "32212254720"
          }
        },
        "zookeeper": {
          "resources": {
            "resource_preset_id": "s2.compute.1",
            "disk_type_id": "network-ssd",
            "disk_size": "10737418240"
          }
        }
      }
    }
    """
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "topic1",
        "partitions": 3,
        "replication_factor": 1
      }
    }
    """
    Then we get gRPC response error with code UNIMPLEMENTED and message "add topic in unmanaged mode is unimplemented"

Scenario: List topics in cluster with unmanaged flag works
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"],
        "unmanaged_topics": true
      }
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker
    And we "List" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
        "topics": []
    }
    """
