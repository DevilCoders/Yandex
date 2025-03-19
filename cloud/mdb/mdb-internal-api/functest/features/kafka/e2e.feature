@e2e
@grpc_api
Feature: Kafka E2E clusters

  Background:
    Given default headers with "rw-e2e-token" token
    And we add default feature flag "MDB_KAFKA_CLUSTER"

  Scenario: Create E2E cluster
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.kafka.v1.ClusterService" with data
    """
    {
      "folderId": "folder3",
      "environment": "PRESTABLE",
      "name": "dbaas_e2e_func_tests",
      "networkId": "",
      "configSpec": {
        "brokersCount": 1,
        "zoneId": ["myt", "sas"],
        "kafka": {
          "resources": {
            "resourcePresetId": "s2.porto.1",
            "diskTypeId": "local-ssd",
            "diskSize": 10737418240
          }
        },
        "zookeeper": {
          "resources": {
            "resourcePresetId": "s2.porto.1",
            "diskTypeId": "local-ssd",
            "diskSize": 10737418240
          }
        }
      }
    }
    """
    Then we get gRPC response OK
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And each path on body evaluates to
      | $.data.use_yasmagent               | false |
      | $.data.suppress_external_yasmagent | true  |
      | $.data.ship_logs                   | false |
      | $.data.mdb_metrics.enabled         | false |
      | $.data.billing.ship_logs           | false |
      | $.data.mdb_health.nonaggregatable  | true  |
