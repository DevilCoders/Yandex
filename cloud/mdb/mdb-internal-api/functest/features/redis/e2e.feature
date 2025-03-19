@e2e
Feature: Redis E2E clusters

  Background:
    Given feature flags
    """
    ["MDB_REDIS_62"]
    """
    And default headers with "rw-e2e-token" token

  Scenario: Create E2E cluster
    When we POST "/mdb/redis/1.0/clusters?folderId=folder3" with data
    """
    {
        "name": "dbaas_e2e_func_tests",
        "environment": "PRESTABLE",
        "configSpec": {
            "redisConfig_6_2": {
                "password": "p@ssw#$rd!?"
            },
            "resources": {
                "resourcePresetId": "s1.porto.1",
                "diskSize": 17179869184
            }
        },
        "hostSpecs": [{
            "zoneId": "myt"
        }, {
            "zoneId": "iva"
        }, {
            "zoneId": "sas"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 200
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And each path on body evaluates to
      | $.data.use_yasmagent               | false |
      | $.data.suppress_external_yasmagent | true  |
      | $.data.ship_logs                   | false |
      | $.data.mdb_metrics.enabled         | false |
      | $.data.billing.ship_logs           | false |
      | $.data.mdb_health.nonaggregatable  | true  |
