@e2e
@grpc_api
Feature: SQLServer E2E clusters

  Background:
    Given default headers with "rw-e2e-token" token
    When we add default feature flag "MDB_SQLSERVER_CLUSTER"

  Scenario: Create E2E cluster
    And we "Create" via gRPC at "yandex.cloud.priv.mdb.sqlserver.v1.ClusterService" with data
    """
    {
      "folderId": "folder3",
      "environment": "PRESTABLE",
      "name": "dbaas_e2e_func_tests",
      "networkId": "network3",
      "config_spec": {
        "version": "2016sp2ent",
        "sqlserver_config_2016sp2ent": {
          "maxDegreeOfParallelism": 30,
          "auditLevel": 3
        },
        "resources": {
          "resourcePresetId": "s1.porto.1",
          "diskTypeId": "local-ssd",
          "diskSize": 10737418240
        }
      },
      "databaseSpecs": [{
        "name": "testdb"
      }],
      "userSpecs": [{
        "name": "test",
        "password": "test_password1!"
      }],
      "hostSpecs": [{
        "zoneId": "myt"
      }]
    }
    """
    Then we get gRPC response OK
    When we GET "/api/v1.0/config/myt-1.db.yandex.net"
    Then we get response with status 200
    And each path on body evaluates to
      | $.data.ship_logs                   | false |
      | $.data.billing.ship_logs           | false |
      | $.data.mdb_health.nonaggregatable  | true  |
