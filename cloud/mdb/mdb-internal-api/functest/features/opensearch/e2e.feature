@e2e
@grpc_api
Feature: OpenSearch E2E clusters

  Background:
    Given default headers with "rw-e2e-token" token

  Scenario: Create E2E cluster
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.opensearch.v1.ClusterService" with data
    """
    {
        "folder_id": "folder3",
        "name": "dbaas_e2e_func_tests",
        "environment": "PRESTABLE",
        "config_spec": {
            "version": "7.14",
            "admin_password": "admin_password",
            "opensearch_spec": {
                "data_node": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                },
                "master_node": {
                    "resources": {
                        "resource_preset_id": "s1.porto.1",
                        "disk_type_id": "local-ssd",
                        "disk_size": 10737418240
                    }
                }
            }
        },
        "host_specs": [{
            "zone_id": "myt",
            "type": "DATA_NODE"
        }, {
            "zone_id": "sas",
            "type": "DATA_NODE"
        }, {
            "zone_id": "man",
            "type": "DATA_NODE"
        }, {
            "zone_id": "myt",
            "type": "MASTER_NODE"
        }, {
            "zone_id": "sas",
            "type": "MASTER_NODE"
        }, {
            "zone_id": "man",
            "type": "MASTER_NODE"
        }],
        "description": "test cluster",
        "labels": {
            "foo": "bar"
        },
        "networkId": "IN-PORTO-NO-NETWORK-API"
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
