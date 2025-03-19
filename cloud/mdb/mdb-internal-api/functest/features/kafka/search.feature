@kafka
@grpc_api
@search
Feature: Validate Kafka search docs
  Background:
    Given default headers
    When we add default feature flag "MDB_KAFKA_CLUSTER"
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "testName",
      "description": "test cluster description",
      "networkId": "network1",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 32212254720
          }
        },
        "zookeeper": {
          "resources": {
            "resourcePresetId": "s2.compute.1",
            "diskTypeId": "network-ssd",
            "diskSize": 10737418240
          }
        }
      },
      "topic_specs": [
        {
          "name": "topic1",
          "partitions": 12,
          "replication_factor": 1
        }],
      "user_specs": [
        {
          "name": "producer",
          "password": "ProducerPassword"
        }]
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
    When "worker_task_id1" acquired and finished by worker

  Scenario: Cluster creation, modification and deletion
    Then last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-kafka",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {},
            "hosts": [
              "myt-1.df.cloud.yandex.net",
              "myt-2.df.cloud.yandex.net",
              "sas-1.df.cloud.yandex.net",
              "sas-2.df.cloud.yandex.net",
              "vla-1.df.cloud.yandex.net"
            ],
            "topics": ["topic1"],
            "users": ["producer"]
        }
    }
    """
    And we "Update" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1",
      "update_mask": {
        "paths": ["description", "labels"]
      },
      "description": "new description",
      "labels": {
        "key1": "val1",
        "key2": "val2"
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false,
      "id": "worker_task_id2"
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-kafka",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "new description",
            "labels": {
              "key1": "val1",
              "key2": "val2"
            },
            "hosts": [
              "myt-1.df.cloud.yandex.net",
              "myt-2.df.cloud.yandex.net",
              "sas-1.df.cloud.yandex.net",
              "sas-2.df.cloud.yandex.net",
              "vla-1.df.cloud.yandex.net"
            ],
            "topics": ["topic1"],
            "users": ["producer"]
        }
    }
    """
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "id": "worker_task_id3"
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-kafka",
        "deleted": "<TIMESTAMP>",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {}
    }
    """

  Scenario: User creation and deletion
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_spec": {
        "name": "consumer",
        "password": "ConsumerPassword"
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false,
      "id": "worker_task_id2"
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-kafka",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {},
            "hosts": [
              "myt-1.df.cloud.yandex.net",
              "myt-2.df.cloud.yandex.net",
              "sas-1.df.cloud.yandex.net",
              "sas-2.df.cloud.yandex.net",
              "vla-1.df.cloud.yandex.net"
            ],
            "topics": ["topic1"],
            "users": ["consumer", "producer"]
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    And we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.UserService" with data
    """
    {
      "cluster_id": "cid1",
      "user_name": "consumer"
    }
    """
    Then we get gRPC response with body
    """
    {
      "id": "worker_task_id3",
      "done": false
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-kafka",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {},
            "hosts": [
              "myt-1.df.cloud.yandex.net",
              "myt-2.df.cloud.yandex.net",
              "sas-1.df.cloud.yandex.net",
              "sas-2.df.cloud.yandex.net",
              "vla-1.df.cloud.yandex.net"
            ],
            "topics": ["topic1"],
            "users": ["producer"]
        }
    }
    """

  Scenario: Topic creation and deletion
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_spec": {
        "name": "topic2",
        "partitions": 3,
        "replication_factor": 1
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "done": false,
      "id": "worker_task_id2"
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-kafka",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "hosts": [
              "myt-1.df.cloud.yandex.net",
              "myt-2.df.cloud.yandex.net",
              "sas-1.df.cloud.yandex.net",
              "sas-2.df.cloud.yandex.net",
              "vla-1.df.cloud.yandex.net"
            ],
            "labels": {},
            "topics": ["topic1", "topic2"],
            "users": ["producer"]
        }
    }
    """
    When "worker_task_id2" acquired and finished by worker
    When we "Delete" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.TopicService" with data
    """
    {
      "cluster_id": "cid1",
      "topic_name": "topic1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "id": "worker_task_id3",
      "done": false
    }
    """
    And last document in search_queue is
    """
    {
        "resource_type": "cluster",
        "resource_id": "cid1",
        "resource_path": [{
            "resource_type": "resource-manager.folder",
            "resource_id": "folder1"
        }],
        "name": "testName",
        "service": "managed-kafka",
        "timestamp": "<TIMESTAMP>",
        "permission": "mdb.all.read",
        "cloud_id": "cloud1",
        "folder_id": "folder1",
        "attributes": {
            "name": "testName",
            "description": "test cluster description",
            "labels": {},
            "hosts": [
              "myt-1.df.cloud.yandex.net",
              "myt-2.df.cloud.yandex.net",
              "sas-1.df.cloud.yandex.net",
              "sas-2.df.cloud.yandex.net",
              "vla-1.df.cloud.yandex.net"
            ],
            "topics": ["topic2"],
            "users": ["producer"]
        }
    }
    """

  @reindexer
  Scenario: Reindexer works with Kafka clusters
    When we run mdb-search-reindexer
    Then last document in search_queue matches
    """
    {
        "service": "managed-kafka",
        "timestamp": "<TIMESTAMP>",
        "reindex_timestamp": "<TIMESTAMP>"
    }
    """