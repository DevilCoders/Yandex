@e2e
Feature: MongoDB E2E clusters

  Background:
    Given default headers with "rw-e2e-token" token

  Scenario: Create E2E cluster
    When we POST "/mdb/mongodb/1.0/clusters?folderId=folder3" with data
    """
    {
        "name": "dbaas_e2e_func_tests",
        "environment": "PRESTABLE",
        "configSpec": {
            "mongodbSpec_4_4": {
                "mongod": {
                    "resources": {
                        "resourcePresetId": "s1.porto.1",
                        "diskTypeId": "local-ssd",
                        "diskSize": 10737418240
                    }
                }
            }
        },
        "databaseSpecs": [{
            "name": "testdb"
        }],
        "userSpecs": [{
            "name": "test",
            "password": "test_password"
        }],
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
