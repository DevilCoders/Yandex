@e2e
Feature: ClickHouse E2E clusters

  Background:
    Given default headers with "rw-e2e-token" token

  Scenario: Create E2E cluster
    When we "POST" via REST at "/mdb/clickhouse/1.0/clusters?folderId=folder3" with data
    """
    {
        "name": "dbaas_e2e_func_tests",
        "environment": "PRESTABLE",
        "configSpec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resourcePresetId": "s1.porto.1",
                    "diskTypeId": "local-ssd",
                    "diskSize": 10737418240
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
            "type": "CLICKHOUSE",
            "zoneId": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zoneId": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "man"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zoneId": "iva"
        }],
        "description": "test cluster",
        "networkId": "IN-PORTO-NO-NETWORK-API"
    }
    """
    Then we get response with status 200
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.clickhouse.v1.ClusterService" with data
    """
    {
        "folder_id": "folder3",
        "name": "dbaas_e2e_func_tests",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "21.3",
            "clickhouse": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            },
            "zookeeper": {
                "resources": {
                    "resource_preset_id": "s2.porto.1",
                    "disk_type_id": "local-ssd",
                    "disk_size": 10737418240
                }
            }
        },
        "database_specs": [{
            "name": "testdb"
        }],
        "user_specs": [{
            "name": "test",
            "password": "test_password"
        }],
        "host_specs": [{
            "type": "CLICKHOUSE",
            "zone_id": "myt"
        }, {
            "type": "CLICKHOUSE",
            "zone_id": "sas"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "man"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "vla"
        }, {
            "type": "ZOOKEEPER",
            "zone_id": "iva"
        }],
        "description": "test cluster",
        "network_id": "IN-PORTO-NO-NETWORK-API"
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
