@e2e
@grpc_api
Feature: Greenplum E2E clusters

  Background:
    Given default headers with "rw-e2e-token" token
    When we add default feature flag "MDB_GREENPLUM_CLUSTER"

  Scenario: Create E2E cluster
    When we "Create" via gRPC at "yandex.cloud.priv.mdb.greenplum.v1.ClusterService" with data
    """
    {
        "folder_id": "folder3",
        "environment": "PRESTABLE",
        "name": "dbaas_e2e_func_tests",
        "network_id": "network3",
        "config": {
            "zone_id": "myt",
            "subnet_id": "",
            "assign_public_ip": false
        },
        "master_config": {
            "resources": {
                "resourcePresetId": "s1.compute.2",
                "diskTypeId": "network-ssd",
                "diskSize": 10737418240
            }
        },
        "segment_config": {
            "resources": {
                "resourcePresetId": "s1.compute.1",
                "diskTypeId": "network-ssd",
                "diskSize": 10737418240
            }
        },
        "master_host_count": 2,
        "segment_in_host": 1,
        "segment_host_count": 4,
        "user_name": "usr1",
        "user_password": "Pa$$w0rd"
    }
    """
    Then we get gRPC response OK
    When we GET "/api/v1.0/config/myt-1.df.cloud.yandex.net"
    Then we get response with status 200
    And each path on body evaluates to
      | $.data.ship_logs                   | false |
      | $.data.billing.ship_logs           | false |
      | $.data.mdb_health.nonaggregatable  | true  |
