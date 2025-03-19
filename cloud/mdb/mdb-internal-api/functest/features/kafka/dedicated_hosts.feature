@kafka
@grpc_api
Feature: Kafka Cluster on dedicated hosts
  Background:
    Given default headers
    And we add default feature flag "MDB_KAFKA_CLUSTER"

  Scenario: Create Kafka cluster on dedicated hosts
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder1",
      "environment": "PRESTABLE",
      "name": "test",
      "networkId": "network1",
      "hostGroupIds": ["hg1", "hg2", "hg3"],
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"],
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
    Then we get gRPC response OK
    When we "Get" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "cluster_id": "cid1"
    }
    """
    Then we get gRPC response with body
    """
    {
      "host_group_ids": ["hg1", "hg2", "hg3"]
    }
    """
