@kafka
@logs
@grpc_api
Feature: Apache Kafka cluster logs
  Background:
    Given default headers
    And we add default feature flag "MDB_KAFKA_CLUSTER"
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "description": "test cluster",
      "labels": {
        "foo": "bar"
      },
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
      }
    }
    """
    Then we get gRPC response with body
    """
    {
      "created_by": "user",
      "description": "Create Apache Kafka cluster",
      "done": false,
      "id": "worker_task_id1",
      "metadata": {
        "@type": "type.googleapis.com/yandex.cloud.priv.mdb.kafka.v1.CreateClusterMetadata",
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

  Scenario: kafka logs can be viewed
    Given logsdb response
    """
    [
        [0, 0, "message 1"],
        [0, 1, "message 2"]
    ]
    """
    When we "ListLogs" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
        "cluster_id": "cid1",
        "column_filter": ["message"]
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
