@kafka
@grpc_api
Feature: Operation for Managed Apache Kafka
  Background:
    Given default headers
    And we add default feature flag "MDB_KAFKA_CLUSTER"

  Scenario: Check Kafka-specific operation service
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
      "topic_specs": [
        {
          "name": "topic1",
          "partitions": 12,
          "replication_factor": 1
        },
        {
          "name": "topic2",
          "partitions": 12,
          "replication_factor": 1
        }
      ]
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
   And we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.OperationService" with data
   """
   {
     "operation_id": "worker_task_id1"
   }
   """
   Then we get gRPC response OK

