@grpc_api
Feature: Test feature flags
  Background:
    Given default headers

  Scenario: Worker task has feature flags
    When we add default feature flag "MDB_KAFKA_CLUSTER"
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
      }
    }
    """
    Then we get gRPC response OK
    And "worker_task_id1" acquired and finished by worker
    Then in worker_queue exists "worker_task_id1" id with args "feature_flags" containing:
          |MDB_KAFKA_CLUSTER|
